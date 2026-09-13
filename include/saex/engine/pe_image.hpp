#pragma once
#include <compare>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace saex::engine {
inline constexpr std::size_t max_image_bytes = 256U * 1024U * 1024U;
inline constexpr std::size_t max_sections = 96;

struct PeSection {
    std::array<char, 8> name{};
    std::uint32_t virtual_size{}, rva{}, raw_size{}, raw_offset{}, characteristics{};
    auto operator<=>(const PeSection&) const = default;
};

struct PeLayout {
    std::uint32_t timestamp{}, image_base{}, entry_rva{}, image_size{}, headers_size{};
    std::uint32_t section_alignment{}, file_alignment{};
    std::uint16_t characteristics{}, dll_characteristics{}, section_count{};
    std::array<PeSection, max_sections> sections{};
    auto operator<=>(const PeLayout&) const = default;
};

enum class PeError { none, size, dos_signature, header_bounds, pe_signature, architecture,
    executable_kind, section_count, optional_header, image_layout, section_bounds, section_overlap, entry_point };
struct PeResult { PeError error{PeError::none}; PeLayout layout{}; };

// Disk layout only. This does not execute the image or validate imports/instructions.
[[nodiscard]] PeResult parse_pe32(std::span<const std::byte> file) noexcept;
[[nodiscard]] std::optional<std::size_t> raw_offset(const PeLayout& layout, std::uint32_t rva, std::size_t length) noexcept;
[[nodiscard]] std::string_view pe_error_name(PeError error) noexcept;
}
