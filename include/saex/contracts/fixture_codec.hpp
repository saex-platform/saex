#pragma once
#include <saex/contracts/foundation.generated.hpp>
#include <array>
#include <cstddef>
#include <span>

namespace saex::contracts {
inline constexpr std::size_t fixture_header_bytes = 12;
inline constexpr std::size_t fixture_frame_bytes = fixture_header_bytes + fixture_payload_bytes;
enum class DecodeError { none, truncated, bad_magic, unsupported_version, reserved_bits, invalid_length, invalid_entity };
struct DecodeResult final { DecodeError error; EntityRef entity; };
[[nodiscard]] DecodeError validate_fixture_header(std::span<const std::byte> header) noexcept;
[[nodiscard]] DecodeResult decode_fixture(std::span<const std::byte> frame) noexcept;
[[nodiscard]] std::array<std::byte, fixture_frame_bytes> encode_fixture(EntityRef entity);
} // namespace saex::contracts
