#pragma once
#include "saex/engine/cd_stream_disk.hpp"
namespace saex::engine {
struct CdStreamAllocationSpec {
    std::uint32_t malloc_rva{},nh_rva{},heap_rva{},scope_rva{},new_mode_rva{},threshold_rva{},heap_handle_rva{},heap_mode_rva{},iat_rva{};
    std::uint32_t scope_cleanup_rva{};
    BootstrapExportSpec function{};
};
struct CdStreamAllocationCode {
    std::array<std::byte,35> aligned{};
    std::array<std::byte,18> malloc{};
    std::array<std::byte,21> nh{};
    std::array<std::byte,32> heap_entry{};
    std::array<std::byte,41> heap_tail{};
};
struct CdStreamAllocationLayout { std::uint32_t raw{},aligned{},metadata{},bytes{}; };
[[nodiscard]] bool cd_stream_allocation_code(const CdStreamAllocationSpec&,const CdStreamDiskSpec&,
    const FileManagerEntrySpec&,const CwdSehSpec&,const FileManagerReadySpec&,CdStreamAllocationCode&) noexcept;
[[nodiscard]] bool cd_stream_allocation_request(std::uint32_t alignment,const std::array<std::uint32_t,4>& globals) noexcept;
[[nodiscard]] bool cd_stream_allocation_layout(std::uint32_t raw,std::uint32_t alignment,CdStreamAllocationLayout&) noexcept;
[[nodiscard]] bool cd_stream_allocation_frame(std::uint32_t stage,const EventDispatchRegisters& caller,
    const EventDispatchRegisters& current,std::uint32_t alignment,const CdStreamAllocationLayout&,
    const EventDispatchRegisters& heap_return) noexcept;
struct CdStreamAllocationObservation {
    std::uint32_t stage{},stop_address{},function_address{},last_error{},metadata_before{},metadata_after{},shape_failure{};
    std::array<std::uint32_t,4> globals{}; // new-handler mode, SBH threshold, heap handle, heap mode
    CdStreamAllocationLayout block{};
    std::array<std::byte,32> block_before_hash{},block_after_hash{};
    bool armed{},continued{},shape_valid{},request_valid{},frame_valid{},arguments_valid{},globals_preserved{},
        branch_verified{},api_returned{},allocation_succeeded{},block_valid{},metadata_valid{},content_preserved{},
        parent_preserved{},seh_preserved{},prior_record_preserved{},verified{};
};
}
