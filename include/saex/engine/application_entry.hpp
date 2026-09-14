#pragma once
#include "saex/engine/crt_startup.hpp"

namespace saex::engine {
struct ApplicationEntrySpec {
    std::span<const std::byte> initializer_body;
    std::array<std::byte,16> initializer_return{}, second_return{};
    std::uint32_t second_call_rva{}, reentry_rva{}, once_rva{};
    std::array<std::byte,31> reentry_normalized{}; // Absolute operands at 5/19/27 are zero in the recipe.
};
struct ApplicationEntryObservation {
    std::uint32_t stage{}, stop_address{}, initializer_stack{}, initializer_result{}, second_stack{}, second_argument{};
    std::uint32_t show_command{}, command_line_bytes{}, call_stack{};
    std::array<std::uint32_t,4> arguments{};
    bool armed{}, continued{}, initializer_returned{}, initializer_abi_valid{}, second_call_reached{}, second_returned{};
    bool second_abi_valid{}, once_valid{}, arguments_valid{}, entry_reached{}, entry_stack_valid{}, verified{};
};
[[nodiscard]] bool valid_application_entry_spec(const ApplicationEntrySpec&,const CrtStartupSpec&) noexcept;
[[nodiscard]] bool application_reentry_prefix(const ApplicationEntrySpec&,std::uint32_t base,
    std::uint32_t startup_slot,std::array<std::byte,31>& output) noexcept;
}
