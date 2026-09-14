#pragma once
#include "saex/engine/frame_target.hpp"
#include <span>

namespace saex::engine {
struct CrtInitializerTable {
    std::uint32_t rva{};
    std::span<const std::uint32_t> targets; // Zero or reviewed EXE RVA; never raw external pointers.
};
struct CrtStartupSpec {
    FrameTargetSpec io, initialize, application;
    std::array<std::byte,16> io_return_prefix{};
    std::uint32_t io_return_stack_offset{};
    std::span<const CrtInitializerTable> tables;
};
struct CrtStartupObservation {
    std::uint32_t stage{}, stop_address{}, io_return_stack{}, return_code{}, checked_slots{}, nonzero_slots{};
    std::uint32_t failed_slot_rva{}, actual_target{}, samples_verified{};
    bool armed{}, continued{}, io_return_reached{}, stack_valid{}, registers_valid{}, initializer_call_reached{}, verified{};
};
[[nodiscard]] bool valid_crt_startup_spec(const CrtStartupSpec& spec) noexcept;
}
