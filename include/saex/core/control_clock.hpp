#pragma once
#include <saex/contracts/foundation.generated.hpp>
#include <chrono>

namespace saex::core {
struct ControlStamp final {
    contracts::ClockDomainId domain;
    std::uint64_t microseconds{};
};

class ControlClock {
public:
    virtual ~ControlClock() = default;
    [[nodiscard]] virtual ControlStamp now() const noexcept = 0;
};

// Domain identity is owned by the trusted process bootstrap, never by a peer.
class SteadyControlClock final : public ControlClock {
public:
    explicit SteadyControlClock(contracts::ClockDomainId domain);
    [[nodiscard]] ControlStamp now() const noexcept override;
private:
    contracts::ClockDomainId domain_;
    std::chrono::steady_clock::time_point origin_;
};
} // namespace saex::core
