#pragma once
#include "saex/engine/cwd_query.hpp"
namespace saex::engine {
struct CwdCopySpec {
    std::uint32_t copier_rva{};
    std::array<std::byte,16> return_prefix{};
};
struct CwdCopyCode {
    std::array<std::byte,19> checks{};
    std::array<std::byte,5> capacity{};
    std::array<std::byte,13> call{};
    std::array<std::byte,7> entry{};
    std::array<std::byte,131> body{};
};
[[nodiscard]] bool cwd_copy_code(const CwdCopySpec&,const CwdQuerySpec&,const CwdLockSpec&,CwdCopyCode&) noexcept;
[[nodiscard]] bool cwd_copy_frame(std::uint32_t stage,const EventDispatchRegisters& helper,
    const EventDispatchRegisters& api_return,const EventDispatchRegisters& current,
    std::uint32_t source,std::uint32_t destination,std::uint32_t length) noexcept;
[[nodiscard]] bool cwd_copy_buffer(const std::array<std::byte,136>& before,const std::array<std::byte,136>& after,
    const std::array<std::byte,260>& source,std::uint32_t length,bool copied) noexcept;
struct CwdCopyObservation {
    std::uint32_t stage{},stop_address{},function_address{},source_address{},destination_address{},copied_bytes{};
    bool armed{},continued{},pointer_branch_reached{},capacity_branch_reached{},call_reached{},function_entered{},function_returned{},
        shape_valid{},frame_valid{},arguments_valid{},stack_preserved{},source_preserved{},destination_valid{},
        seh_preserved{},prior_record_preserved{},caller_preserved{},lock_preserved{},cookie_preserved{},guard_preserved{},
        localisation_preserved{},last_error_preserved{},copied{},verified{};
    std::array<std::byte,136> destination_before{},destination_after{};
};
}
