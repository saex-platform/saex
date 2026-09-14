#pragma once
#include "saex/engine/event_dispatch.hpp"
namespace saex::engine {
// Only event 24 is executable under this contract. Other table entries are identities.
struct ApplicationRoutingSpec {
    FrameTargetSpec initializer{};
    std::uint32_t detour_rva{}, index_rva{}, table_rva{};
    std::array<std::byte,39> indices{};
    std::array<std::uint32_t,11> target_rvas{};
};
struct ApplicationRoutingCode {
    std::array<std::byte,21> entry{};
    std::array<std::byte,12> detour{};
    std::array<std::byte,7> indirect{};
    std::array<std::uint32_t,11> targets{};
};
// Reconstructs exact x86 operands at the admitted image base; no child memory writes.
[[nodiscard]] bool application_routing_code(const ApplicationRoutingSpec&,const EventDispatchSpec&,ApplicationRoutingCode&) noexcept;
[[nodiscard]] bool valid_application_routing_spec(const ApplicationRoutingSpec&,const EventDispatchSpec&) noexcept;
// 1: handler entry; 2: detour entry; 3: indirect JMP; 4: before initializer CALL.
[[nodiscard]] bool application_routing_frame(const ApplicationRoutingSpec&,const EventDispatchSpec&,std::uint32_t stage,
    const EventDispatchRegisters& caller,const EventDispatchRegisters& current,std::span<const std::uint32_t> stack) noexcept;
struct ApplicationRoutingObservation {
    std::uint32_t stage{},stop_address{},selected_index{},selected_target{};
    bool armed{},continued{},entry_reached{},detour_reached{},indirect_reached{},initializer_call_reached{},
        shape_valid{},frame_valid{},stack_preserved{},verified{};
};
}
