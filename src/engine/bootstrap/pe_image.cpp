#include "saex/engine/pe_image.hpp"
#include <algorithm>

namespace saex::engine {
namespace {
std::uint16_t u16(std::span<const std::byte> b, std::size_t p) noexcept {
    return static_cast<std::uint16_t>(std::to_integer<unsigned>(b[p]) | (std::to_integer<unsigned>(b[p + 1]) << 8U));
}
std::uint32_t u32(std::span<const std::byte> b, std::size_t p) noexcept {
    return static_cast<std::uint32_t>(u16(b, p)) | (static_cast<std::uint32_t>(u16(b, p + 2)) << 16U);
}
bool fits(std::uint64_t offset, std::uint64_t length, std::uint64_t limit) noexcept {
    return offset <= limit && length <= limit - offset;
}
bool power2(std::uint32_t n) noexcept { return n != 0 && (n & (n - 1)) == 0; }
bool overlaps(std::uint32_t a, std::uint32_t an, std::uint32_t b, std::uint32_t bn) noexcept {
    return an && bn && static_cast<std::uint64_t>(a) < static_cast<std::uint64_t>(b) + bn &&
        static_cast<std::uint64_t>(b) < static_cast<std::uint64_t>(a) + an;
}
}

PeResult parse_pe32(std::span<const std::byte> b) noexcept {
    const auto fail = [](PeError error) { return PeResult{error, {}}; };
    if (b.size() < 64 || b.size() > max_image_bytes) return fail(PeError::size);
    if (u16(b, 0) != 0x5a4d) return fail(PeError::dos_signature);
    const auto pe = static_cast<std::size_t>(u32(b, 60));
    if (pe < 64 || !fits(pe, 24, b.size())) return fail(PeError::header_bounds);
    if (u32(b, pe) != 0x4550) return fail(PeError::pe_signature);
    if (u16(b, pe + 4) != 0x14c) return fail(PeError::architecture);
    PeLayout l{};
    l.section_count = u16(b, pe + 6);
    if (l.section_count == 0 || l.section_count > max_sections) return fail(PeError::section_count);
    l.timestamp = u32(b, pe + 8);
    l.characteristics = u16(b, pe + 22);
    if (!(l.characteristics & 2) || (l.characteristics & 0x2000)) return fail(PeError::executable_kind);
    const auto optional_size = u16(b, pe + 20);
    const auto o = pe + 24;
    if (optional_size < 96 || !fits(o, optional_size, b.size())) return fail(PeError::optional_header);
    if (u16(b, o) != 0x10b) return fail(PeError::architecture);
    const auto directories = u32(b, o + 92);
    if (directories > 16 || 96U + directories * 8U > optional_size) return fail(PeError::optional_header);
    if (directories > 14 && (u32(b, o + 96 + 14 * 8) || u32(b, o + 100 + 14 * 8)))
        return fail(PeError::executable_kind);
    const auto table = o + optional_size;
    const auto table_size = static_cast<std::size_t>(l.section_count) * 40;
    if (!fits(table, table_size, b.size())) return fail(PeError::header_bounds);
    l.entry_rva = u32(b, o + 16);
    l.image_base = u32(b, o + 28);
    l.section_alignment = u32(b, o + 32);
    l.file_alignment = u32(b, o + 36);
    l.image_size = u32(b, o + 56);
    l.headers_size = u32(b, o + 60);
    l.dll_characteristics = u16(b, o + 70);
    if (!power2(l.file_alignment) || l.file_alignment < 512 || l.file_alignment > 65536 ||
        !power2(l.section_alignment) || l.section_alignment < l.file_alignment ||
        !l.image_size || l.image_size > max_image_bytes || l.image_size % l.section_alignment ||
        !l.headers_size || l.headers_size % l.file_alignment || l.headers_size > b.size() ||
        l.headers_size > l.image_size || table + table_size > l.headers_size ||
        !l.image_base || l.image_base % 65536 || static_cast<std::uint64_t>(l.image_base) + l.image_size > 0x100000000ULL)
        return fail(PeError::image_layout);
    bool executable_entry = false;
    for (std::size_t i = 0; i < l.section_count; ++i) {
        auto& s = l.sections[i];
        const auto p = table + i * 40;
        for (std::size_t n = 0; n < s.name.size(); ++n) s.name[n] = static_cast<char>(std::to_integer<unsigned char>(b[p + n]));
        s.virtual_size = u32(b, p + 8); s.rva = u32(b, p + 12);
        s.raw_size = u32(b, p + 16); s.raw_offset = u32(b, p + 20);
        s.characteristics = u32(b, p + 36);
        const auto extent = std::max(s.virtual_size, s.raw_size);
        if (!extent || s.rva < l.headers_size || s.rva % l.section_alignment ||
            !fits(s.rva, extent, l.image_size) || !fits(s.raw_offset, s.raw_size, b.size()) ||
            (s.raw_size && (s.raw_offset < l.headers_size || s.raw_offset % l.file_alignment || s.raw_size % l.file_alignment)))
            return fail(PeError::section_bounds);
        for (std::size_t j = 0; j < i; ++j) {
            const auto& t = l.sections[j];
            if (overlaps(s.rva, extent, t.rva, std::max(t.virtual_size, t.raw_size)) ||
                overlaps(s.raw_offset, s.raw_size, t.raw_offset, t.raw_size)) return fail(PeError::section_overlap);
        }
        if (l.entry_rva >= s.rva && l.entry_rva - s.rva < s.raw_size && (s.characteristics & 0x20000000U))
            executable_entry = true;
    }
    if (!executable_entry) return fail(PeError::entry_point);
    return {PeError::none, l};
}

std::optional<std::size_t> raw_offset(const PeLayout& l, std::uint32_t rva, std::size_t length) noexcept {
    if (!length || l.section_count > max_sections) return std::nullopt;
    std::optional<std::size_t> result;
    for (std::size_t i = 0; i < l.section_count; ++i) {
        const auto& s = l.sections[i];
        if (rva >= s.rva && fits(rva - s.rva, length, s.raw_size)) {
            if (result) return std::nullopt;
            result = static_cast<std::size_t>(s.raw_offset) + (rva - s.rva);
        }
    }
    return result;
}

std::string_view pe_error_name(PeError error) noexcept {
    switch (error) {
    case PeError::none: return "none";
    case PeError::size: return "engine_file_size";
    case PeError::dos_signature: return "dos_signature";
    case PeError::header_bounds: return "header_bounds";
    case PeError::pe_signature: return "pe_signature";
    case PeError::architecture: return "unsupported_engine_architecture";
    case PeError::executable_kind: return "unsupported_executable_kind";
    case PeError::section_count: return "section_count";
    case PeError::optional_header: return "optional_header";
    case PeError::image_layout: return "image_layout";
    case PeError::section_bounds: return "section_bounds";
    case PeError::section_overlap: return "section_overlap";
    case PeError::entry_point: return "entry_point";
    }
    return "invalid_error";
}
}
