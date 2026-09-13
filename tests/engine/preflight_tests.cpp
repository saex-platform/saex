#include "saex/engine/profile_gate.hpp"
#include "saex/engine/observed_profile.generated.hpp"
#ifdef _WIN32
#include "saex/engine/windows_image_reader.hpp"
#endif
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace saex::engine;
namespace {
void require(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
void put16(std::vector<std::byte>& b, std::size_t p, std::uint16_t v) {
    b[p] = static_cast<std::byte>(v & 255); b[p + 1] = static_cast<std::byte>(v >> 8);
}
void put32(std::vector<std::byte>& b, std::size_t p, std::uint32_t v) {
    put16(b, p, static_cast<std::uint16_t>(v)); put16(b, p + 2, static_cast<std::uint16_t>(v >> 16));
}
constexpr std::size_t pe = 64, opt = pe + 24, table = opt + 224;
std::vector<std::byte> fixture() {
    std::vector<std::byte> b(1536);
    put16(b, 0, 0x5a4d); put32(b, 60, pe); put32(b, pe, 0x4550);
    put16(b, pe + 4, 0x14c); put16(b, pe + 6, 2); put16(b, pe + 20, 224); put16(b, pe + 22, 0x102);
    put16(b, opt, 0x10b); put32(b, opt + 16, 4096); put32(b, opt + 28, 0x400000);
    put32(b, opt + 32, 4096); put32(b, opt + 36, 512); put32(b, opt + 56, 12288);
    put32(b, opt + 60, 512); put32(b, opt + 92, 16);
    for (std::size_t i = 0; i < 2; ++i) {
        const auto p = table + i * 40;
        // Duplicate section names are valid: identity is the index and RVA.
        b[p] = std::byte{'.'}; b[p + 1] = std::byte{'x'};
        put32(b, p + 8, 1024); put32(b, p + 12, 4096U * static_cast<std::uint32_t>(i + 1));
        put32(b, p + 16, 512); put32(b, p + 20, 512U * static_cast<std::uint32_t>(i + 1));
        put32(b, p + 36, 0x60000020);
    }
    b[512] = std::byte{0xe8}; b[513] = std::byte{0x42};
    return b;
}
class TestImage final : public ImageReader {
public:
    std::vector<std::byte> bytes;
    bool readable = true;
    bool copy(std::uint32_t rva, std::span<std::byte> out) const noexcept override {
        if (!readable || rva > bytes.size() || out.size() > bytes.size() - rva) return false;
        std::copy_n(bytes.begin() + rva, out.size(), out.begin()); return true;
    }
};
}
int main() {
    try {
        auto file = fixture();
        const auto parsed = parse_pe32(file);
        require(parsed.error == PeError::none, "valid duplicate-name PE rejected");
        require(raw_offset(parsed.layout, 4096, 2) == std::size_t{512}, "RVA differs from disk offset");
        require(!raw_offset(parsed.layout, 4608, 1), "zero-fill tail accepted as file bytes");
        require(!raw_offset(parsed.layout, 4096, 0), "zero length accepted");
        require(!raw_offset(parsed.layout, 0xffffffffU, 100), "overflow RVA accepted");
        unsigned malformed{};
        const auto reject = [&](std::size_t offset, std::uint32_t value, PeError expected) {
            auto copy = file; put32(copy, offset, value);
            require(parse_pe32(copy).error == expected, "wrong malformed PE classification"); ++malformed;
        };
        reject(0, 0, PeError::dos_signature);
        reject(60, 0xfffffff0U, PeError::header_bounds);
        reject(pe, 0, PeError::pe_signature);
        reject(pe + 4, 0x00028664, PeError::architecture);
        reject(pe + 4, 0x0061014c, PeError::section_count);
        reject(pe + 20, 0x210200e0, PeError::executable_kind);
        reject(opt + 92, 17, PeError::optional_header);
        reject(opt + 96 + 14 * 8, 4096, PeError::executable_kind);
        reject(opt + 36, 513, PeError::image_layout);
        reject(opt + 60, 1, PeError::image_layout);
        reject(opt + 28, 0xffff0000, PeError::none); // still fits 32-bit address space
        reject(opt + 56, 0x20000, PeError::none);
        reject(table + 20, 0xfffffe00, PeError::section_bounds);
        reject(table + 12, 0xfffff000, PeError::section_bounds);
        reject(table + 40 + 12, 4096, PeError::section_overlap);
        reject(table + 40 + 20, 512, PeError::section_overlap);
        reject(opt + 16, 4800, PeError::entry_point);
        reject(table + 36, 0x40000040, PeError::entry_point);
        auto overflow = file; put32(overflow, opt + 28, 0xffff0000); put32(overflow, opt + 56, 0x20000);
        require(parse_pe32(overflow).error == PeError::image_layout, "image-base overflow accepted");
        for (std::size_t n = 0; n < file.size(); ++n)
            require(parse_pe32(std::span{file}.first(n)).error != PeError::none, "truncated file accepted");

        Sha256 hash{}; hash[0] = std::byte{7}; // caller-supplied digest seam; CLI computes CNG SHA256
        std::array anchors{ImageAnchor{"fixture-call", 4096, 0, 2, {std::byte{0xe8}, std::byte{0x42}}}};
        const ObservedProfile profile{"test.observed", file.size(), hash, parsed.layout, anchors};
        require(match_file_observation(file, hash, parsed.layout, profile) == ProfileResult::matched_observation, "file profile");
        require(match_file_observation(file, {}, parsed.layout, profile) == ProfileResult::unknown_fingerprint, "hash mismatch");
        auto layout = parsed.layout; ++layout.timestamp;
        require(match_file_observation(file, hash, layout, profile) == ProfileResult::layout_mismatch, "layout mismatch");
        auto changed = file; changed[512] = std::byte{0};
        require(match_file_observation(changed, hash, parsed.layout, profile) == ProfileResult::anchor_mismatch, "anchor mismatch");
        anchors[0].section_index = 96;
        require(match_file_observation(file, hash, parsed.layout, profile) == ProfileResult::invalid_anchor, "anchor index");
        anchors[0].section_index = 0; anchors[0].length = 17;
        require(match_file_observation(file, hash, parsed.layout, profile) == ProfileResult::invalid_anchor, "anchor length");
        anchors[0].length = 2;
        TestImage mapped; mapped.bytes.resize(parsed.layout.image_size);
        std::copy_n(file.begin(), 512, mapped.bytes.begin());
        std::copy_n(file.begin() + 512, 512, mapped.bytes.begin() + 4096);
        require(check_mapped_observation(mapped, file, parsed.layout, profile) == ProfileResult::matched_observation, "mapped match");
        mapped.bytes[4096] = std::byte{0};
        require(check_mapped_observation(mapped, file, parsed.layout, profile) == ProfileResult::anchor_mismatch, "mapped anchor");
        mapped.bytes[0] = std::byte{0};
        require(check_mapped_observation(mapped, file, parsed.layout, profile) == ProfileResult::layout_mismatch, "mapped headers");
        mapped.readable = false;
        require(check_mapped_observation(mapped, file, parsed.layout, profile) == ProfileResult::unreadable_image, "unreadable mapping");
        mapped.readable = true; mapped.bytes.resize(4096);
        std::copy_n(file.begin(), 512, mapped.bytes.begin());
        require(check_mapped_observation(mapped, file, parsed.layout, profile) == ProfileResult::unreadable_image, "unreadable anchor after valid headers");
#ifdef _WIN32
        std::array<std::byte, 8> out{};
        require(!WindowsImageReader(nullptr, 4096).copy(0, out), "null mapping accepted");
        require(!WindowsImageReader(file.data(), file.size()).copy(0, out), "heap accepted as MEM_IMAGE");
        require(!WindowsImageReader(reinterpret_cast<void*>(1), 4096).copy(0, out), "invalid address accepted");
        require(!WindowsImageReader(file.data(), file.size()).copy(0xfffffff0U, out), "reader overflow accepted");
#endif
        require(observed_profile.anchors.size() == 4, "generated profile changed without test review");
        std::cout << "PASS engine preflight: " << malformed << " PE mutations, " << file.size()
            << " truncations, file/mapped profile and reader rejection scenarios\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
