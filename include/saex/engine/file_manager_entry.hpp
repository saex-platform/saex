#pragma once
#include "saex/engine/game_prelude.hpp"
namespace saex::engine {
struct FileManagerEntrySpec {
    FrameTargetSpec manager{};
    std::uint32_t cwd_rva{},buffer_rva{},suffix_rva{};
    std::array<std::byte,16> return_prefix{};
};
// Complete CFileMgr body is sampled; only its first 11 bytes may execute.
[[nodiscard]] bool file_manager_body(const FileManagerEntrySpec&,std::array<std::byte,51>&) noexcept;
[[nodiscard]] bool valid_file_manager_entry_spec(const FileManagerEntrySpec&,const GamePreludeSpec&,const ApplicationRoutingSpec&) noexcept;
// saved is the verified prelude terminal. 1 manager entry; 2 before cwd CALL.
[[nodiscard]] bool file_manager_entry_frame(const FileManagerEntrySpec&,const GamePreludeSpec&,const ApplicationRoutingSpec&,
    std::uint32_t stage,const EventDispatchRegisters& saved,const EventDispatchRegisters& current,std::span<const std::uint32_t> stack) noexcept;
struct FileManagerEntryObservation {
    std::uint32_t stage{},stop_address{},buffer_address{},cwd_address{};
    bool armed{},continued{},entry_reached{},cwd_call_reached{},shape_valid{},frame_valid{},stack_preserved{},
        buffer_read{},buffer_unchanged{},localisation_preserved{},verified{};
    std::array<std::byte,136> buffer_before{},buffer_after{}; // 4 + 128 + 4, never decoded as a path.
};
}
