#pragma once
#include "saex/engine/cwd_lock.hpp"
namespace saex::engine {
struct CwdAcquireSpec {
    std::uint32_t iat_rva{};
    BootstrapExportSpec function{}; // Pinned ntdll RtlEnterCriticalSection entry.
    std::array<std::byte,16> return_prefix{};
};
[[nodiscard]] bool cwd_acquire_code(const CwdAcquireSpec&,const CwdLockSpec&,std::array<std::byte,11>&) noexcept;
[[nodiscard]] bool valid_cwd_acquire_spec(const CwdAcquireSpec&,const CwdLockSpec&) noexcept;
[[nodiscard]] bool cwd_acquire_object(std::uint32_t address,const std::array<std::uint32_t,6>& before,
    const std::array<std::uint32_t,6>& now,std::uint32_t thread_id,bool acquired) noexcept;
[[nodiscard]] bool cwd_acquire_frame(std::uint32_t stage,const EventDispatchRegisters& saved,const EventDispatchRegisters& current,
    std::uint32_t restored_ebp) noexcept;
struct CwdAcquireObservation {
    std::uint32_t stage{},stop_address{},object_address{},function_address{},thread_id{},object_memory_type{},object_protection{};
    bool armed{},continued{},branch_reached{},call_reached{},function_entered{},function_returned{},selector_returned{},
        shape_valid{},frame_valid{},object_read{},object_valid{},slot_preserved{},stack_preserved{},seh_preserved{},prior_record_preserved{},
        caller_preserved{},buffer_preserved{},localisation_preserved{},acquired{},verified{};
    std::array<std::uint32_t,6> object_before{},object_after{};
    std::array<std::uint32_t,21> stack_before{},stack_after{};
};
}
