#pragma once
#include "saex/engine/bootstrap_lifecycle.hpp"
#include "saex/engine/frame_target.hpp"

namespace saex::engine {
inline constexpr std::array<std::uint32_t,2> startup_image_protections{0x40,0x80};
[[nodiscard]] constexpr bool valid_startup_image_protection(std::uint32_t protection) noexcept {
    return protection==startup_image_protections[0] || protection==startup_image_protections[1];
}
// Trusted startup-tail recipe; no address input from launcher/server/resources.
struct StartupReturnSpec {
    std::uint32_t protect_call_rva{}, protect_slot_rva{}, startup_slot_rva{}, forward_rva{};
    std::array<std::byte,16> protect_return_prefix{};
    BootstrapExportSpec protect_function{}, startup_function{}; // Pinned kernel32 exports.
    FrameTargetSpec frame{};
};
struct StartupReturnObservation {
    std::array<FrameTargetSample,2> frame_samples{}; // ASI return, natural startup return.
    std::uint32_t stage{}, stop_address{}, protect_target{}, startup_target{}, protect_stack{};
    std::uint32_t old_protect_address{}, old_protect{}, first_page_protect{}, region_count{}, return_code{}, startup_info_bytes{};
    std::uint32_t last_region_rva{}, last_region_protect{}, last_region_state{}, last_region_type{};
    std::uint32_t copy_on_write_regions{}, read_write_regions{};
    bool armed{}, continued{}, protect_call_reached{}, arguments_valid{}, protect_continued{}, protect_return_reached{};
    bool protections_verified{}, startup_return_reached{}, stack_valid{}, registers_valid{}, verified{};
};
[[nodiscard]] bool valid_startup_return_spec(const StartupReturnSpec& spec) noexcept;
}
