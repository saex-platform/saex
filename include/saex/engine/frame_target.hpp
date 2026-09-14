#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace saex::engine {
// Read-only candidate recipe. This is not a callable binding or hook permission.
struct FrameTargetSpec {
    std::uint32_t image_base{}, image_size{}, call_rva{}, target_rva{};
    std::array<std::byte, 5> call{};
    std::array<std::byte, 16> target_prefix{};
};
struct FrameTargetSample {
    std::array<std::byte, 5> call{};
    std::array<std::byte, 16> target_prefix{};
    std::uint32_t thread_id{}, event_index{};
    bool attempted{}, call_read{}, target_read{}, match{};
};
struct FrameTargetObservation {
    // CREATE_PROCESS, verified ASI return, verified terminal bootstrap Stop.
    std::array<FrameTargetSample, 3> samples{};
    bool verified{};
};
[[nodiscard]] bool valid_frame_target_spec(const FrameTargetSpec& spec) noexcept;
[[nodiscard]] bool matches_frame_target(const FrameTargetSpec& spec, const FrameTargetSample& sample) noexcept;
}
