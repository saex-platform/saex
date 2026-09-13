#pragma once
#include <compare>
#include <cstdint>
#include <limits>
#include <optional>

namespace saex::contracts {
template <typename Tag>
struct StrongId final {
    std::uint64_t value{};
    explicit constexpr StrongId(std::uint64_t input = 0) noexcept : value(input) {}
    constexpr auto operator<=>(const StrongId&) const noexcept = default;
};

template <typename Tag>
[[nodiscard]] constexpr std::optional<StrongId<Tag>> next(StrongId<Tag> current) noexcept {
    if (current.value == std::numeric_limits<std::uint64_t>::max()) return std::nullopt;
    return StrongId<Tag>{current.value + 1};
}
} // namespace saex::contracts
