#pragma once
#include <saex/core/control_clock.hpp>
#include <optional>

namespace saex::core {
enum class LeaseResult {
    accepted, no_grant, expired, stale_entity, stale_owner, stale_revision,
    stale_sequence, field_denied, clock_discontinuity, recovery_required
};

struct LeaseGrant final {
    contracts::EntityRef entity;
    contracts::SessionId session;
    contracts::OwnerEpoch owner;
    contracts::CatalogRevision catalog;
    contracts::StateRevision state;
    std::uint64_t field_mask{};
    ControlStamp deadline;
};

struct OwnerProposal final {
    contracts::EntityRef entity;
    contracts::SessionId session;
    contracts::OwnerEpoch owner;
    contracts::CatalogRevision catalog;
    contracts::StateRevision state;
    contracts::Sequence sequence;
    std::uint64_t field_mask{};
};

// Single authoritative thread. It is a lease gate, not a movement/anti-cheat solver.
class LeaseAuthority final {
public:
    static constexpr std::uint64_t lease_microseconds = 2'000'000;
    explicit LeaseAuthority(ControlClock& clock, contracts::EntityRef entity);
    [[nodiscard]] std::optional<LeaseGrant> issue(contracts::SessionId session,
        contracts::CatalogRevision catalog, contracts::StateRevision state,
        std::uint64_t field_mask, bool participant_applied);
    [[nodiscard]] LeaseResult accept(const OwnerProposal& proposal);
    [[nodiscard]] LeaseResult renew(contracts::SessionId session,
        contracts::OwnerEpoch owner, contracts::Sequence renewal);
    void revoke() noexcept;
    void suspend_or_resume() noexcept;
    // Explicit trusted recovery barrier; never exposed as a client command.
    [[nodiscard]] bool recover_after_readiness() noexcept;
private:
    [[nodiscard]] std::optional<ControlStamp> sample() noexcept;
    ControlClock& clock_;
    contracts::EntityRef entity_;
    ControlStamp last_stamp_;
    contracts::OwnerEpoch last_owner_{};
    contracts::Sequence last_motion_{};
    contracts::Sequence last_renewal_{};
    bool fenced_{};
    std::optional<LeaseGrant> grant_;
};
} // namespace saex::core
