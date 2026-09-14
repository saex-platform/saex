#pragma once
#include "saex/engine/cwd_acquire.hpp"
namespace saex::engine {
struct CwdQuerySpec {
    std::uint32_t helper_rva{},cookie_rva{},iat_rva{},thunk_slot_rva{};
    BootstrapExportSpec function{},implementation{};
    std::array<std::byte,16> return_prefix{};
    std::string_view expected_directory{}; // Local launch input; never a server-provided address/policy.
};
[[nodiscard]] bool cwd_query_code(const CwdQuerySpec&,const CwdLockSpec&,
    std::array<std::byte,18>& wrapper,std::array<std::byte,28>& helper,std::array<std::byte,18>& query) noexcept;
[[nodiscard]] bool valid_cwd_query_spec(const CwdQuerySpec&,const CwdLockSpec&) noexcept;
[[nodiscard]] bool cwd_query_directory(std::string_view) noexcept;
[[nodiscard]] std::string_view cwd_query_result(std::uint32_t,const std::array<std::byte,260>&,std::string_view) noexcept;
[[nodiscard]] bool cwd_query_frame(std::uint32_t,const EventDispatchRegisters&,const EventDispatchRegisters&,
    std::uint32_t cookie,std::uint32_t helper_return) noexcept;
[[nodiscard]] bool cwd_query_stack(std::uint32_t,const std::array<std::uint32_t,21>&,
    const std::array<std::uint32_t,21>&,std::uint32_t buffer,std::uint32_t helper_return) noexcept;
struct CwdQueryObservation {
    std::uint32_t stage{},stop_address{},function_address{},implementation_address{},buffer_address{},
        returned_length{},last_error_before{},last_error_after{};
    bool armed{},continued{},wrapper_call_reached{},helper_entered{},branch_reached{},query_path_reached{},
        call_reached{},function_entered{},function_returned{},shape_valid{},frame_valid{},stack_valid{},
        seh_preserved{},prior_record_preserved{},caller_preserved{},root_preserved{},localisation_preserved{},
        lock_preserved{},cookie_preserved{},guard_preserved{},buffer_read{},path_verified{},verified{};
    std::array<std::byte,260> directory{};
};
}
