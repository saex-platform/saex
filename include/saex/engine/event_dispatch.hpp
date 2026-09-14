#pragma once
#include "saex/engine/instance_startup.hpp"
namespace saex::engine {
inline constexpr std::array<std::byte,7> event_dispatch_branch{std::byte{0x85},std::byte{0xc0},std::byte{0x75},std::byte{0x0f},std::byte{0x53},std::byte{0x6a},std::byte{0x18}};
inline constexpr std::array<std::byte,12> event_dispatch_prologue{std::byte{0x56},std::byte{0x8b},std::byte{0x74},std::byte{0x24},std::byte{0x0c},std::byte{0x57},std::byte{0x8b},std::byte{0x7c},std::byte{0x24},std::byte{0x0c},std::byte{0x56},std::byte{0x57}};
struct EventDispatchSpec {
    FrameTargetSpec dispatcher{}, application{};
    std::array<std::byte,21> application_prefix{}; // Read-only; never authorizes its body.
};
[[nodiscard]] bool valid_event_dispatch_spec(const EventDispatchSpec&,const InstanceStartupSpec&) noexcept;
struct EventDispatchRegisters {
    std::uint32_t esp{},eax{},ebx{},ecx{},edx{},esi{},edi{},ebp{},flags{};
};
// Stage 1: outer CALL; 2: dispatcher entry; 3: before application CALL.
[[nodiscard]] bool event_dispatch_frame(const EventDispatchSpec&,std::uint32_t stage,
    const EventDispatchRegisters& caller,const EventDispatchRegisters& current,std::span<const std::uint32_t> stack) noexcept;
struct EventDispatchObservation {
    std::uint32_t stage{},stop_address{},caller_stack{};
    bool armed{},continued{},call_reached{},entry_reached{},application_call_reached{},shape_valid{},frame_valid{},stack_preserved{},verified{};
    std::array<std::byte,21> application_sample{};
};
}
