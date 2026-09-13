#pragma once
#include "saex/engine/pe_image.hpp"

namespace saex::engine {
using Sha256 = std::array<std::byte, 32>;
struct ImageAnchor {
    std::string_view id;
    std::uint32_t rva{};
    std::uint16_t section_index{}, length{};
    std::array<std::byte, 16> bytes{};
};
struct ObservedProfile {
    std::string_view id;
    std::size_t file_bytes{};
    Sha256 sha256{};
    PeLayout layout{};
    std::span<const ImageAnchor> anchors;
};
enum class ProfileResult { matched_observation, unknown_fingerprint, layout_mismatch, invalid_anchor, anchor_mismatch, unreadable_image };
[[nodiscard]] ProfileResult match_file_observation(std::span<const std::byte> file, const Sha256& computed_hash,
    const PeLayout& layout, const ObservedProfile& profile) noexcept;

// Read only owned, bounded image bytes. The production OS reader checks every region.
class ImageReader {
public:
    virtual ~ImageReader() = default;
    virtual bool copy(std::uint32_t rva, std::span<std::byte> output) const noexcept = 0;
};
[[nodiscard]] ProfileResult check_mapped_observation(const ImageReader& image, std::span<const std::byte> checked_file,
    const PeLayout& checked_layout, const ObservedProfile& profile) noexcept;
[[nodiscard]] std::string_view profile_result_name(ProfileResult result) noexcept;
// Deliberately no activate/attach/export-address API. Observations are not executable profiles.
}
