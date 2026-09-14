#pragma once
#include "saex/engine/file_manager_ready.hpp"
namespace saex::engine {
struct CdStreamTablesSpec {
    std::uint32_t target_rva{}, handles_rva{}, names_rva{}, disk_iat_rva{};
};
struct CdStreamTablesCode {
    std::array<std::byte,7> caller{};
    std::array<std::byte,74> body{}; // Includes the unexecuted disk API CALL.
};
inline constexpr std::size_t cd_stream_table_bytes=2192;
using CdStreamTableWindow=std::array<std::byte,cd_stream_table_bytes>;
[[nodiscard]] bool cd_stream_tables_code(const CdStreamTablesSpec&,const FileManagerEntrySpec&,CdStreamTablesCode&) noexcept;
[[nodiscard]] bool cd_stream_tables_memory(const CdStreamTableWindow&,const CdStreamTableWindow&,bool initialized) noexcept;
[[nodiscard]] bool cd_stream_tables_frame(std::uint32_t stage,const EventDispatchRegisters& caller,
    const EventDispatchRegisters& current,std::uint32_t handles) noexcept;
struct CdStreamTablesObservation {
    std::uint32_t stage{},stop_address{};
    CdStreamTableWindow before{},after{};
    bool armed{},continued{},shape_valid{},frame_valid{},memory_read{},tables_valid{},arguments_valid{},
        caller_preserved{},manager_preserved{},seh_preserved{},lock_preserved{},localisation_preserved{},
        last_error_preserved{},verified{};
};
}
