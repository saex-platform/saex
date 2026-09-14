#pragma once
#include "saex/engine/cd_stream_allocation.hpp"
namespace saex::engine {
inline constexpr std::uint32_t cd_stream_channel_count=5,cd_stream_channel_bytes=240;
struct CdStreamChannelsSpec {
    std::uint32_t error_iat_rva{},local_iat_rva{},pointer_rva{},filename_rva{},open_rva{},error_thunk_slot_rva{},local_thunk_slot_rva{};
    BootstrapExportSpec error_thunk{},error_implementation{},local_thunk{},local_implementation{};
};
[[nodiscard]] bool cd_stream_channels_code(const CdStreamChannelsSpec&,const CdStreamTablesSpec&,
    const FileManagerEntrySpec&,std::array<std::byte,64>&) noexcept;
[[nodiscard]] bool cd_stream_channels_frame(std::uint32_t stage,const EventDispatchRegisters& caller,
    const EventDispatchRegisters& current,const EventDispatchRegisters& error_return,const EventDispatchRegisters& local_return) noexcept;
[[nodiscard]] bool cd_stream_channels_range(std::uint32_t pointer,const CdStreamAllocationLayout&) noexcept;
[[nodiscard]] bool cd_stream_channels_memory(std::span<const std::byte>) noexcept;
[[nodiscard]] bool cd_stream_channels_tables(const CdStreamTableWindow& before,const CdStreamTableWindow& after,bool prepared) noexcept;
struct CdStreamChannelsObservation {
    std::uint32_t stage{},stop_address{},shape_failure{},pointer{},error_before{},error_after_reset{},error_after_local{};
    std::array<std::uint32_t,4> functions{};
    std::array<std::byte,12> pointer_before{},pointer_after{};
    std::array<std::byte,240> channels{};
    bool armed{},continued{},shape_valid{},arguments_valid{},frame_valid{},error_reset{},local_returned{},
        allocation_succeeded{},block_valid{},zero_initialized{},pointer_published{},tables_valid{},parent_preserved{},
        allocation_preserved{},seh_preserved{},verified{};
};
}
