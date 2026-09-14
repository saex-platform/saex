#pragma once
#include "saex/engine/platform_suppression.hpp"
namespace saex::engine {
struct InstanceStartupSpec {
    FrameTargetSpec caller{};
    std::array<std::byte,91> normalized_body{};
    std::array<std::uint32_t,9> body_address_rvas{};
    std::uint32_t name_rva{}, create_thunk_rva{}, create_thunk_slot_rva{};
    std::string_view name;
    BootstrapExportSpec create_function{}; // Six-byte kernel32 FF25 thunk -> kernelbase
};
inline constexpr std::array<std::uint32_t,9> instance_address_offsets{1,14,20,33,39,47,58,69,78};
[[nodiscard]] bool valid_instance_startup_spec(const InstanceStartupSpec&,const PlatformStartupSpec&,
    const CrtStartupSpec&,const PlatformSuppressionSpec&) noexcept;
[[nodiscard]] bool instance_body(const InstanceStartupSpec&,std::uint32_t image_base,std::array<std::byte,91>&) noexcept;
struct InstanceStartupObservation {
    std::uint32_t stage{}, stop_address{}, caller_stack{}, create_stack{}, event_handle{}, last_error{};
    bool armed{}, continued{}, suppression_boundary_validated{}, create_call_reached{}, create_returned{},
        arguments_valid{}, event_identity_valid{}, getter_returned{}, existing_detected{},
        caller_returned{}, stack_preserved{}, verified{};
};
}
