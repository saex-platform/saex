#include "saex/engine/bootstrap_session.hpp"
#include "saex/engine/observed_profile.generated.hpp"
#include <algorithm>

namespace saex::engine {
std::uint32_t BootstrapSession::invoke(BootstrapOperation operation, BootstrapObservation& observation,
    std::uint32_t abi, SaexBootstrapStatus* output, std::uint32_t bytes) noexcept {
    if (!output || bytes != sizeof(SaexBootstrapStatus)) return SAEX_BOOTSTRAP_INVALID_ARGUMENT;
    if (abi != SAEX_BOOTSTRAP_ABI_MAJOR) return SAEX_BOOTSTRAP_ABI_MISMATCH;
    if (busy_.test_and_set(std::memory_order_acquire)) return SAEX_BOOTSTRAP_BUSY;
    if (operation == BootstrapOperation::initialize && status_.state == SAEX_BOOTSTRAP_DISCOVERED) {
        status_.observation_attempts = 1;
        const auto reason = observation.inspect();
        status_.reason = reason >= SAEX_BOOTSTRAP_HOST_PATH && reason <= SAEX_BOOTSTRAP_INTERNAL_ERROR
            ? reason : SAEX_BOOTSTRAP_INTERNAL_ERROR;
        status_.state = status_.reason == SAEX_BOOTSTRAP_RUNTIME_UNVERIFIED
            ? SAEX_BOOTSTRAP_OBSERVED_UNVERIFIED : SAEX_BOOTSTRAP_REJECTED;
        if (status_.state == SAEX_BOOTSTRAP_OBSERVED_UNVERIFIED) {
            static_assert(observed_profile.id.size() < sizeof(status_.observed_profile_id));
            std::copy(observed_profile.id.begin(), observed_profile.id.end(), status_.observed_profile_id);
        }
    } else if (operation == BootstrapOperation::stop) {
        status_.state = SAEX_BOOTSTRAP_STOPPED;
    }
    static_assert(observed_profile_source_digest.size() == sizeof(status_.profile_source_digest));
    std::copy(observed_profile_source_digest.begin(), observed_profile_source_digest.end(), status_.profile_source_digest);
    *output = status_;
    busy_.clear(std::memory_order_release);
    return SAEX_BOOTSTRAP_OK;
}
}
