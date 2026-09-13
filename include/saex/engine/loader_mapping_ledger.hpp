#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace saex::engine {
inline constexpr std::uint32_t max_loader_mappings = 64;
struct LoaderMapping {
    std::uintptr_t base{};
    std::uint32_t id{}, load_event{}, unload_event{};
};
struct MappingTransition {
    std::uint32_t mapping_id{};
    std::string_view error;
};
// Local observation lifetime only: IDs are never reused, even for the same base.
// Call admit only AFTER each load's file identity/hash has been independently checked.
// This portable ledger grants no file, process, executable or native-call permission.
class LoaderMappingLedger final {
public:
    explicit LoaderMappingLedger(std::uint32_t limit = max_loader_mappings) noexcept : limit_(limit) {}
    [[nodiscard]] MappingTransition admit(std::uintptr_t base, std::uint32_t event) noexcept;
    [[nodiscard]] MappingTransition retire(std::uintptr_t base, std::uint32_t event) noexcept;
    [[nodiscard]] bool active(std::uint32_t mapping_id, std::uintptr_t base) const noexcept;
    [[nodiscard]] std::span<const LoaderMapping> history() const noexcept { return {history_.data(), count_}; }
    [[nodiscard]] std::uint32_t active_count() const noexcept { return active_; }
    [[nodiscard]] std::uint32_t unload_count() const noexcept { return count_ - active_; }
private:
    [[nodiscard]] std::string_view validate(std::uintptr_t base, std::uint32_t event) const noexcept;
    std::array<LoaderMapping, max_loader_mappings> history_{};
    std::uint32_t limit_{}, count_{}, active_{}, last_event_{};
};
}
