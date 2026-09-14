#pragma once
#include "saex/engine/file_manager_entry.hpp"
namespace saex::engine {
struct CwdSehSpec {
    FrameTargetSpec wrapper{},prologue{};
    std::uint32_t scope_rva{},handler_rva{},cleanup_rva{};
    std::array<std::byte,16> stop_prefix{};
};
[[nodiscard]] bool cwd_seh_code(const CwdSehSpec&,std::array<std::byte,16>& wrapper,std::array<std::byte,59>& prologue,std::array<std::byte,12>& scope) noexcept;
[[nodiscard]] bool valid_cwd_seh_spec(const CwdSehSpec&,const FileManagerEntrySpec&) noexcept;
// NT_TIB words: exception head, stack base, stack limit, subsystem, fiber, arbitrary, self.
[[nodiscard]] bool valid_cwd_seh_origin(const EventDispatchRegisters&,const std::array<std::uint32_t,7>& tib) noexcept;
[[nodiscard]] bool cwd_seh_frame(const CwdSehSpec&,std::uint32_t stage,const EventDispatchRegisters& saved,const EventDispatchRegisters& current) noexcept;
// 80-byte window starts at saved.esp - 60: guards, native writes, 20-byte caller.
[[nodiscard]] bool cwd_seh_memory(const CwdSehSpec&,std::uint32_t stage,const EventDispatchRegisters& saved,
    const std::array<std::uint32_t,7>& before_tib,const std::array<std::uint32_t,7>& now_tib,
    const std::array<std::uint32_t,20>& before_stack,const std::array<std::uint32_t,20>& now_stack) noexcept;
struct CwdSehObservation {
    std::uint32_t stage{},stop_address{},record_address{};
    bool armed{},continued{},wrapper_entry_reached{},prologue_entry_reached{},prologue_returned{},shape_valid{},frame_valid{},
        memory_read{},memory_valid{},prior_record_preserved{},caller_preserved{},buffer_preserved{},localisation_preserved{},verified{};
    std::array<std::uint32_t,7> tib_before{},tib_after{};
    std::array<std::uint32_t,20> stack_before{},stack_after{};
};
}
