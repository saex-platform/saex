#pragma once
#include "saex/engine/application_routing.hpp"
namespace saex::engine {
struct GamePreludeSpec {
    FrameTargetSpec empty{},localisation{};
    std::uint32_t flags_rva{}; // Three consecutive bytes; four leading and nine trailing guard bytes.
    std::array<std::byte,16> stop_prefix{};
};
[[nodiscard]] bool game_prelude_body(const GamePreludeSpec&,std::array<std::byte,20>&) noexcept;
[[nodiscard]] bool valid_game_prelude_spec(const GamePreludeSpec&,const ApplicationRoutingSpec&) noexcept;
// 1 initializer entry, 2 empty helper entry, 3 localisation CALL, 4 helper entry, 5 next CALL.
[[nodiscard]] bool game_prelude_frame(const GamePreludeSpec&,const ApplicationRoutingSpec&,std::uint32_t stage,
    const EventDispatchRegisters& saved,const EventDispatchRegisters& current,std::span<const std::uint32_t> stack) noexcept;
[[nodiscard]] bool game_prelude_flags(std::uint32_t stage,const std::array<std::byte,16>& before,
    const std::array<std::byte,16>& current) noexcept;
struct GamePreludeObservation {
    std::uint32_t stage{},stop_address{},flags_address{};
    bool armed{},continued{},initializer_entry_reached{},empty_entry_reached{},empty_returned{},localisation_entry_reached{},
        localisation_returned{},shape_valid{},frame_valid{},stack_preserved{},flags_read{},flags_valid{},verified{};
    std::array<std::byte,16> flags_before{},flags_after{};
};
}
