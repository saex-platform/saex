#include "saex/engine/bootstrap_lifecycle.hpp"
#include <algorithm>
#include <cstring>

namespace saex::engine {
bool bootstrap_export_prefix(const BootstrapExportSpec& spec, std::uint32_t actual_base,
    std::array<std::byte,20>& output) noexcept {
    if (spec.highlow_mask & ~0x1ffffU || (spec.highlow_mask && (!spec.preferred_base || spec.preferred_base % 65536))) return false;
    auto bytes = spec.prefix;
    std::uint32_t covered{};
    for (unsigned offset = 0; offset <= 16; ++offset) {
        if (!(spec.highlow_mask & (1U << offset))) continue;
        if (covered & (15U << offset)) return false;
        covered |= 15U << offset;
        std::uint32_t value{};
        for (unsigned i = 0; i < 4; ++i) value |= std::to_integer<std::uint32_t>(bytes[offset+i]) << (8*i);
        value += actual_base - spec.preferred_base; // PE32 HIGHLOW arithmetic is modulo 2^32.
        for (unsigned i = 0; i < 4; ++i) bytes[offset+i] = static_cast<std::byte>((value >> (8*i)) & 255U);
    }
    output = bytes;
    return true;
}
bool valid_bootstrap_lifecycle_spec(const BootstrapLifecycleSpec& spec) noexcept {
    if (spec.profile_id.empty() || spec.profile_id.size() >= 96 || spec.profile_source_digest.size() != 64 ||
        spec.profile_id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") != std::string_view::npos ||
        spec.profile_source_digest.find_first_not_of("0123456789abcdef") != std::string_view::npos) return false;
    for (std::size_t i = 0; i < spec.exports.size(); ++i) {
        const auto rva = spec.exports[i].rva;
        std::array<std::byte,20> checked{};
        if (!bootstrap_export_prefix(spec.exports[i], spec.exports[i].preferred_base, checked)) return false;
        if (!rva || rva > 16U * 1024 * 1024 - 20) return false;
        for (std::size_t j = 0; j < i; ++j)
            if (rva < spec.exports[j].rva + 20 && spec.exports[j].rva < rva + 20) return false;
    }
    return true;
}
bool valid_bootstrap_status(const BootstrapLifecycleSpec& spec, std::uint32_t index,
    const SaexBootstrapStatus& status, const SaexBootstrapStatus* initialized) noexcept {
    if (index >= bootstrap_call_sequence.size() || status.abi_major != SAEX_BOOTSTRAP_ABI_MAJOR ||
        status.struct_bytes != sizeof(status) || status.can_attach || status.bindings_loaded || status.reserved ||
        std::string_view(status.profile_source_digest, 64) != spec.profile_source_digest) return false;
    SaexBootstrapStatus expected{};
    expected.abi_major = SAEX_BOOTSTRAP_ABI_MAJOR; expected.struct_bytes = sizeof(expected);
    std::copy(spec.profile_source_digest.begin(), spec.profile_source_digest.end(), expected.profile_source_digest);
    if (index == 1) {
        expected.observation_attempts = 1;
        if (status.state == SAEX_BOOTSTRAP_OBSERVED_UNVERIFIED && status.reason == SAEX_BOOTSTRAP_RUNTIME_UNVERIFIED) {
            expected.state = status.state; expected.reason = status.reason;
            std::copy(spec.profile_id.begin(), spec.profile_id.end(), expected.observed_profile_id);
        } else if (status.state == SAEX_BOOTSTRAP_REJECTED && status.reason >= SAEX_BOOTSTRAP_HOST_PATH &&
            status.reason <= SAEX_BOOTSTRAP_INTERNAL_ERROR && status.reason != SAEX_BOOTSTRAP_RUNTIME_UNVERIFIED) {
            expected.state = status.state; expected.reason = status.reason;
        } else return false;
    } else if (index > 1) {
        if (!initialized) return false;
        expected = *initialized;
        if (index >= 4) expected.state = SAEX_BOOTSTRAP_STOPPED;
    }
    return std::memcmp(&expected, &status, sizeof(expected)) == 0;
}
}
