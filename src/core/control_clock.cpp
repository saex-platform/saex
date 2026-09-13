#include <saex/core/control_clock.hpp>
#include <stdexcept>

namespace saex::core {
SteadyControlClock::SteadyControlClock(contracts::ClockDomainId domain)
    : domain_(domain), origin_(std::chrono::steady_clock::now()) {
    if (domain_.value == 0) throw std::invalid_argument("ClockDomainId must be nonzero");
}

ControlStamp SteadyControlClock::now() const noexcept {
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - origin_).count();
    return {domain_, static_cast<std::uint64_t>(elapsed)};
}
} // namespace saex::core
