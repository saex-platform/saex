#pragma once
#include "saex/engine/cwd_copy.hpp"
namespace saex::engine {
struct CwdReturnSpec {
    std::uint32_t checker_rva{};
    std::array<std::byte,16> return_prefix{};
};
struct CwdReturnCode {
    std::array<std::byte,7> cleanup{};
    std::array<std::byte,14> epilogue{};
    std::array<std::byte,9> checker{};
};
[[nodiscard]] bool cwd_return_code(const CwdReturnSpec&,const CwdCopySpec&,const CwdQuerySpec&,const CwdLockSpec&,CwdReturnCode&) noexcept;
[[nodiscard]] bool cwd_return_frame(std::uint32_t stage,const EventDispatchRegisters& helper,
    const EventDispatchRegisters& copy_return,const EventDispatchRegisters& current,
    std::uint32_t cookie,std::uint32_t destination) noexcept;
struct CwdReturnObservation {
    std::uint32_t stage{},stop_address{},checker_address{},return_address{};
    bool armed{},continued{},arguments_cleaned{},cookie_call_reached{},checker_entered{},cookie_matched{},
        checker_returned{},helper_returned{},shape_valid{},frame_valid{},stack_preserved{},
        source_snapshot_preserved{},source_retired{},destination_preserved{},seh_preserved{},prior_record_preserved{},
        caller_preserved{},lock_preserved{},cookie_preserved{},guard_preserved{},localisation_preserved{},last_error_preserved{},verified{};
};
}
