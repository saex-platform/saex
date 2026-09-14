#include "saex/engine/frame_target.hpp"

namespace saex::engine {
bool valid_frame_target_spec(const FrameTargetSpec& s) noexcept {
    if (!s.image_base || s.image_base % 65536 || s.image_size < 4096 || s.image_size > 256U*1024*1024 ||
        s.image_base > UINT32_MAX - s.image_size || s.call_rva < 4096 || s.target_rva < 4096 ||
        s.call_rva > s.image_size - s.call.size() || s.target_rva > s.image_size - s.target_prefix.size() ||
        (s.call_rva < s.target_rva + s.target_prefix.size() && s.target_rva < s.call_rva + s.call.size()) ||
        s.call[0] != std::byte{0xe8}) return false;
    std::uint32_t raw{};
    for (unsigned i = 0; i < 4; ++i) raw |= std::to_integer<std::uint32_t>(s.call[i + 1]) << (i * 8);
    // Signed rel32 without host-endian/implementation-defined signed conversion or wrap acceptance.
    const auto delta = raw <= INT32_MAX ? static_cast<std::int64_t>(raw) : static_cast<std::int64_t>(raw) - 0x100000000LL;
    return static_cast<std::int64_t>(s.call_rva) + 5 + delta == s.target_rva;
}
bool matches_frame_target(const FrameTargetSpec& spec, const FrameTargetSample& sample) noexcept {
    return valid_frame_target_spec(spec) && sample.attempted && sample.call_read && sample.target_read &&
        sample.call == spec.call && sample.target_prefix == spec.target_prefix;
}
}
