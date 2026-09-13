#include "saex/engine/profile_gate.hpp"
#include <algorithm>

namespace saex::engine {
namespace {
bool valid_anchor(const ImageAnchor& a, const PeLayout& l) noexcept {
    if (l.section_count == 0 || l.section_count > max_sections || a.length == 0 || a.length > a.bytes.size() || a.section_index >= l.section_count) return false;
    const auto& s = l.sections[a.section_index];
    return (s.characteristics & 0x20000000U) && a.rva >= s.rva &&
        static_cast<std::uint64_t>(a.rva - s.rva) + a.length <= s.raw_size;
}
}
ProfileResult match_file_observation(std::span<const std::byte> file, const Sha256& hash,
    const PeLayout& l, const ObservedProfile& p) noexcept {
    if (file.size() != p.file_bytes || hash != p.sha256) return ProfileResult::unknown_fingerprint;
    if (l != p.layout) return ProfileResult::layout_mismatch;
    if (p.anchors.empty() || p.anchors.size() > 32) return ProfileResult::invalid_anchor;
    for (const auto& a : p.anchors) {
        const auto offset = raw_offset(l, a.rva, a.length);
        if (!valid_anchor(a, l) || !offset || *offset > file.size() || a.length > file.size() - *offset)
            return ProfileResult::invalid_anchor;
        if (!std::equal(a.bytes.begin(), a.bytes.begin() + a.length, file.begin() + *offset)) return ProfileResult::anchor_mismatch;
    }
    return ProfileResult::matched_observation;
}

ProfileResult check_mapped_observation(const ImageReader& image, std::span<const std::byte> file,
    const PeLayout& l, const ObservedProfile& p) noexcept {
    if (l != p.layout || l.headers_size > file.size()) return ProfileResult::layout_mismatch;
    // Compare checked raw headers, including section table, in bounded chunks.
    std::array<std::byte, 1024> buffer{};
    for (std::uint32_t at = 0; at < l.headers_size;) {
        const auto count = std::min<std::size_t>(buffer.size(), l.headers_size - at);
        auto chunk = std::span{buffer}.first(count);
        if (!image.copy(at, chunk)) return ProfileResult::unreadable_image;
        if (!std::equal(chunk.begin(), chunk.end(), file.begin() + at)) return ProfileResult::layout_mismatch;
        at += static_cast<std::uint32_t>(count);
    }
    if (p.anchors.empty() || p.anchors.size() > 32) return ProfileResult::invalid_anchor;
    for (const auto& a : p.anchors) {
        if (!valid_anchor(a, l)) return ProfileResult::invalid_anchor;
        auto chunk = std::span{buffer}.first(a.length);
        if (!image.copy(a.rva, chunk)) return ProfileResult::unreadable_image;
        if (!std::equal(chunk.begin(), chunk.end(), a.bytes.begin())) return ProfileResult::anchor_mismatch;
    }
    return ProfileResult::matched_observation;
}

std::string_view profile_result_name(ProfileResult result) noexcept {
    switch (result) {
    case ProfileResult::matched_observation: return "observed_profile_runtime_unverified";
    case ProfileResult::unknown_fingerprint: return "unknown_fingerprint";
    case ProfileResult::layout_mismatch: return "profile_layout_mismatch";
    case ProfileResult::invalid_anchor: return "invalid_anchor";
    case ProfileResult::anchor_mismatch: return "anchor_mismatch";
    case ProfileResult::unreadable_image: return "unreadable_image";
    }
    return "invalid_result";
}
}
