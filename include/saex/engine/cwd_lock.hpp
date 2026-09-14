#pragma once
#include "saex/engine/cwd_seh.hpp"
namespace saex::engine {
struct CwdLockSpec {
    FrameTargetSpec selector{};
    std::uint32_t table_rva{};
    std::array<std::byte,16> stop_prefix{};
};
[[nodiscard]] bool cwd_lock_code(const CwdLockSpec&,std::array<std::byte,17>&) noexcept;
[[nodiscard]] bool valid_cwd_lock_spec(const CwdLockSpec&,const CwdSehSpec&) noexcept;
// saved is the completed SEH prologue context. The slot value remains opaque.
[[nodiscard]] bool valid_cwd_lock_origin(const EventDispatchRegisters&,const std::array<std::uint32_t,7>& tib) noexcept;
[[nodiscard]] bool cwd_lock_frame(const CwdLockSpec&,std::uint32_t stage,const EventDispatchRegisters& saved,
    const EventDispatchRegisters& current,std::uint32_t slot_value) noexcept;
// 100 bytes at saved.esp - 32, including the live SEH record and caller arguments.
[[nodiscard]] bool cwd_lock_memory(const CwdLockSpec&,std::uint32_t stage,const EventDispatchRegisters& saved,
    const std::array<std::uint32_t,25>& before,const std::array<std::uint32_t,25>& now) noexcept;
struct CwdLockObservation {
    std::uint32_t stage{},stop_address{},slot_address{},slot_value{};
    bool armed{},continued{},entry_reached{},comparison_reached{},shape_valid{},frame_valid{},slot_read{},slot_preserved{},
        memory_valid{},seh_preserved{},prior_record_preserved{},caller_preserved{},buffer_preserved{},localisation_preserved{},verified{};
    std::array<std::byte,16> slot_before{},slot_after{};
    std::array<std::uint32_t,25> stack_before{},stack_after{};
};
}
