#pragma once
#include "saex/engine/bootstrap_api.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace saex::engine {
struct BootstrapExportSpec {
    std::uint32_t rva{};
    std::array<std::byte, 20> prefix{}; // Audited executable bytes at preferred base.
    std::uint32_t preferred_base{};
    std::uint32_t highlow_mask{}; // Starting byte offsets 0..16; disjoint four-byte fixups.
};
struct BootstrapLifecycleSpec {
    std::array<BootstrapExportSpec, 3> exports{}; // Initialize, Query, Stop.
    std::string_view profile_id, profile_source_digest;
};
inline constexpr std::array<std::uint32_t, 8> bootstrap_call_sequence{1, 0, 1, 0, 2, 1, 0, 2};
struct BootstrapCallObservation {
    std::uint32_t export_index{}, target{}, return_code{}, stack_after{};
    SaexBootstrapStatus status{};
    bool armed{}, continued{}, returned{}, stack_valid{}, guards_valid{}, status_valid{};
};
struct BootstrapLifecycleObservation {
    std::array<BootstrapCallObservation, 8> calls{};
    std::uint32_t calls_armed{}, calls_returned{}, frame_address{}, output_address{};
    bool stack_write_attempted{}, stack_written{}, verified{}, observed_unverified{};
};
// An offline experiment recipe only: no game address or caller-selected function.
bool valid_bootstrap_lifecycle_spec(const BootstrapLifecycleSpec& spec) noexcept;
bool bootstrap_export_prefix(const BootstrapExportSpec& spec, std::uint32_t actual_base,
    std::array<std::byte,20>& output) noexcept;
bool valid_bootstrap_status(const BootstrapLifecycleSpec& spec, std::uint32_t call_index,
    const SaexBootstrapStatus& status, const SaexBootstrapStatus* initialized) noexcept;
}
