#pragma once
#include "saex/engine/cwd_return.hpp"
namespace saex::engine {
struct FileManagerReadySpec {
    std::uint32_t unlock_rva{}, iat_rva{}, epilogue_rva{};
    BootstrapExportSpec function{};
};
struct FileManagerReadyCode {
    std::array<std::byte,24> wrapper{};
    std::array<std::byte,9> cleanup{};
    std::array<std::byte,21> unlock{};
    std::array<std::byte,17> epilogue{};
};
[[nodiscard]] bool file_manager_ready_code(const FileManagerReadySpec&,const CwdReturnSpec&,
    const CwdLockSpec&,const CwdSehSpec&,const FileManagerEntrySpec&,FileManagerReadyCode&) noexcept;
[[nodiscard]] bool file_manager_ready_frame(std::uint32_t stage,const EventDispatchRegisters& helper,
    const EventDispatchRegisters& caller,const EventDispatchRegisters& current,std::uint32_t destination,
    std::uint32_t length) noexcept;
[[nodiscard]] bool file_manager_ready_buffer(const std::array<std::byte,136>& copied,
    const std::array<std::byte,136>& current,std::uint32_t length,bool suffix_written) noexcept;
struct FileManagerReadyObservation {
    std::uint32_t stage{},stop_address{},function_address{},return_address{};
    std::array<std::byte,136> directory_after{};
    bool armed{},continued{},shape_valid{},frame_valid{},arguments_valid{},lock_released{},
        seh_removed{},prior_record_preserved{},caller_preserved{},buffer_valid{},suffix_written{},
        wrapper_returned{},manager_returned{},localisation_preserved{},last_error_preserved{},verified{};
};
}
