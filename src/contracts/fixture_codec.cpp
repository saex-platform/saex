#include <saex/contracts/fixture_codec.hpp>
#include <stdexcept>

namespace saex::contracts {
namespace {
std::uint64_t read_le(std::span<const std::byte> bytes) noexcept {
    std::uint64_t value{};
    for (std::size_t index = 0; index < bytes.size(); ++index)
        value |= std::uint64_t{std::to_integer<unsigned char>(bytes[index])} << (8 * index);
    return value;
}
void write_le(std::span<std::byte> bytes, std::uint64_t value) noexcept {
    for (std::size_t index = 0; index < bytes.size(); ++index)
        bytes[index] = static_cast<std::byte>((value >> (8 * index)) & 0xff);
}
}
DecodeError validate_fixture_header(std::span<const std::byte> header) noexcept {
    if (header.size() != fixture_header_bytes) return DecodeError::truncated;
    if (read_le(header.first(4)) != fixture_magic) return DecodeError::bad_magic;
    if (read_le(header.subspan(4, 2)) != fixture_version) return DecodeError::unsupported_version;
    if (read_le(header.subspan(6, 2)) != 0) return DecodeError::reserved_bits;
    if (read_le(header.subspan(8, 4)) != fixture_payload_bytes) return DecodeError::invalid_length;
    return DecodeError::none;
}
DecodeResult decode_fixture(std::span<const std::byte> frame) noexcept {
    if (frame.size() < fixture_header_bytes) return {DecodeError::truncated, {}};
    const auto header = validate_fixture_header(frame.first(fixture_header_bytes));
    if (header != DecodeError::none) return {header, {}};
    if (frame.size() != fixture_frame_bytes) return {DecodeError::invalid_length, {}};
    const EntityRef entity{WorldId{read_le(frame.subspan(12, 8))}, WorldEpoch{read_le(frame.subspan(20, 8))},
        EntityId{read_le(frame.subspan(28, 8))}, Generation{read_le(frame.subspan(36, 8))}};
    return {entity.valid() ? DecodeError::none : DecodeError::invalid_entity, entity};
}
std::array<std::byte, fixture_frame_bytes> encode_fixture(EntityRef entity) {
    if (!entity.valid()) throw std::invalid_argument("Invalid entity reference");
    std::array<std::byte, fixture_frame_bytes> frame{};
    auto bytes = std::span{frame};
    write_le(bytes.first(4), fixture_magic);
    write_le(bytes.subspan(4, 2), fixture_version);
    write_le(bytes.subspan(8, 4), fixture_payload_bytes);
    write_le(bytes.subspan(12, 8), entity.worldId.value);
    write_le(bytes.subspan(20, 8), entity.worldEpoch.value);
    write_le(bytes.subspan(28, 8), entity.entityId.value);
    write_le(bytes.subspan(36, 8), entity.generation.value);
    return frame;
}
} // namespace saex::contracts
