#pragma once
#include "saex/engine/application_entry.hpp"
#include "saex/engine/bootstrap_lifecycle.hpp"
namespace saex::engine {
// Reviewed application prologue only; never permits the system API itself.
struct PlatformStartupSpec {
    std::span<const std::byte> prologue;
    std::uint32_t call_rva{}, iat_rva{}, stack_bytes{152};
    std::array<std::byte,16> return_prefix{};
    BootstrapExportSpec function{}; // Exact pinned user32 export, with full relocation comparison.
    std::string_view module{"user32.dll"}; // Trusted recipe; fixture-only callers use their pinned canary DLL.
};
struct PlatformStartupObservation {
    std::uint32_t stop_address{}, function_address{}, entry_stack{};
    std::array<std::uint32_t,4> arguments{};
    bool armed{}, continued{}, call_reached{}, shape_valid{}, stack_valid{}, registers_valid{}, arguments_valid{}, verified{};
};
inline constexpr std::array<std::uint32_t,4> platform_startup_arguments{0x2001,0,0,2};
[[nodiscard]] bool valid_platform_startup_spec(const PlatformStartupSpec&,const CrtStartupSpec&) noexcept;
}
