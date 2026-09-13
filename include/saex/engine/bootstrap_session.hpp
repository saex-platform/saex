#pragma once
#include "saex/engine/bootstrap_api.h"
#include <atomic>
#include <cstdint>

namespace saex::engine {
enum class BootstrapOperation { initialize, query, stop };
class BootstrapObservation {
public:
    virtual ~BootstrapObservation() = default;
    virtual std::uint32_t inspect() noexcept = 0;
};

class BootstrapSession {
public:
    constexpr BootstrapSession() noexcept = default;
    // BUSY never waits or writes the caller's output. Terminal states cannot restart.
    std::uint32_t invoke(BootstrapOperation operation, BootstrapObservation& observation,
        std::uint32_t abi, SaexBootstrapStatus* output, std::uint32_t bytes) noexcept;
private:
    std::atomic_flag busy_ = ATOMIC_FLAG_INIT;
    SaexBootstrapStatus status_{SAEX_BOOTSTRAP_ABI_MAJOR, sizeof(SaexBootstrapStatus),
        SAEX_BOOTSTRAP_DISCOVERED, SAEX_BOOTSTRAP_NONE, 0, 0, 0, 0, {}, {}};
};
}
