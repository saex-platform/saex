#pragma once
#include "saex/engine/cd_stream_tables.hpp"
namespace saex::engine {
struct CdStreamDiskSpec {
    std::uint32_t flags_rva{}, allocator_rva{}, thunk_rva{}, thunk_slot_rva{};
    BootstrapExportSpec implementation{};
};
struct CdStreamDiskGeometry {
    std::uint32_t sectors_per_cluster{}, bytes_per_sector{}, free_clusters{}, total_clusters{};
};
struct CdStreamDiskCode { std::array<std::byte,69> body{}; };
enum class CdStreamDiskQueryResult { failed, geometry_rejected, accepted };
[[nodiscard]] CdStreamDiskQueryResult cd_stream_disk_query_result(std::uint32_t api_result,
    const std::array<std::uint32_t,4>& native_locals,CdStreamDiskGeometry& output) noexcept;
[[nodiscard]] bool cd_stream_disk_code(const CdStreamDiskSpec&,const CdStreamTablesSpec&,
    const FileManagerEntrySpec&,CdStreamDiskCode&) noexcept;
[[nodiscard]] bool cd_stream_disk_geometry(const CdStreamDiskGeometry&) noexcept;
[[nodiscard]] std::uint32_t cd_stream_disk_flags(std::uint32_t sector) noexcept;
[[nodiscard]] bool cd_stream_disk_frame(std::uint32_t stage,const EventDispatchRegisters& caller,
    const EventDispatchRegisters& current,std::uint32_t handles,const CdStreamDiskGeometry&,
    const EventDispatchRegisters* api_return=nullptr) noexcept;
[[nodiscard]] bool cd_stream_disk_memory(const std::array<std::uint32_t,5>& before,
    const std::array<std::uint32_t,5>& after,std::uint32_t flags,bool prepared) noexcept;
struct CdStreamDiskObservation {
    std::uint32_t stage{},stop_address{},function_address{},implementation_address{},api_result{},last_error{};
    CdStreamDiskGeometry geometry{};
    std::array<std::uint32_t,5> globals_before{},globals_after{};
    bool armed{},continued{},shape_valid{},frame_valid{},arguments_valid{},api_returned{},query_succeeded{},
        geometry_valid{},globals_valid{},tables_preserved{},caller_preserved{},manager_preserved{},
        seh_preserved{},lock_preserved{},localisation_preserved{},last_error_read{},verified{};
};
}
