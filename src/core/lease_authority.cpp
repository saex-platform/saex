#include <saex/core/lease_authority.hpp>
#include <limits>
#include <stdexcept>

namespace saex::core {
LeaseAuthority::LeaseAuthority(ControlClock& clock, contracts::EntityRef entity)
    : clock_(clock), entity_(entity), last_stamp_(clock.now()) {
    if (!entity_.valid() || last_stamp_.domain.value == 0)
        throw std::invalid_argument("Invalid entity or clock domain");
}

std::optional<ControlStamp> LeaseAuthority::sample() noexcept {
    const auto current = clock_.now();
    if (fenced_) return std::nullopt;
    if (current.domain != last_stamp_.domain || current.microseconds < last_stamp_.microseconds) {
        suspend_or_resume();
        return std::nullopt;
    }
    last_stamp_ = current;
    return current;
}

std::optional<LeaseGrant> LeaseAuthority::issue(contracts::SessionId session,
    contracts::CatalogRevision catalog, contracts::StateRevision state,
    std::uint64_t field_mask, bool participant_applied) {
    const auto current = sample();
    if (!current || !participant_applied || session.value == 0 || catalog.value == 0 ||
        state.value == 0 || field_mask == 0) return std::nullopt;
    const auto owner = contracts::next(last_owner_);
    if (!owner || current->microseconds > std::numeric_limits<std::uint64_t>::max() - lease_microseconds) {
        suspend_or_resume();
        return std::nullopt;
    }
    last_owner_ = *owner;
    last_motion_ = contracts::Sequence{};
    last_renewal_ = contracts::Sequence{};
    grant_ = LeaseGrant{entity_, session, *owner, catalog, state, field_mask,
        {current->domain, current->microseconds + lease_microseconds}};
    return grant_;
}

LeaseResult LeaseAuthority::accept(const OwnerProposal& proposal) {
    if (fenced_) return LeaseResult::recovery_required;
    const auto current = sample();
    if (!current) return LeaseResult::clock_discontinuity;
    if (!grant_) return LeaseResult::no_grant;
    if (current->microseconds >= grant_->deadline.microseconds) {
        revoke();
        return LeaseResult::expired;
    }
    if (proposal.entity != entity_) return LeaseResult::stale_entity;
    if (proposal.session != grant_->session || proposal.owner != grant_->owner) return LeaseResult::stale_owner;
    if (proposal.catalog != grant_->catalog || proposal.state != grant_->state) return LeaseResult::stale_revision;
    if (proposal.sequence.value <= last_motion_.value) return LeaseResult::stale_sequence;
    if (proposal.field_mask == 0 || (proposal.field_mask & ~grant_->field_mask) != 0) return LeaseResult::field_denied;
    last_motion_ = proposal.sequence;
    return LeaseResult::accepted;
}

LeaseResult LeaseAuthority::renew(contracts::SessionId session, contracts::OwnerEpoch owner,
    contracts::Sequence renewal) {
    if (fenced_) return LeaseResult::recovery_required;
    const auto current = sample();
    if (!current) return LeaseResult::clock_discontinuity;
    if (!grant_) return LeaseResult::no_grant;
    if (current->microseconds >= grant_->deadline.microseconds) {
        revoke();
        return LeaseResult::expired;
    }
    if (session != grant_->session || owner != grant_->owner) return LeaseResult::stale_owner;
    if (renewal.value <= last_renewal_.value) return LeaseResult::stale_sequence;
    if (current->microseconds > std::numeric_limits<std::uint64_t>::max() - lease_microseconds) {
        suspend_or_resume();
        return LeaseResult::recovery_required;
    }
    last_renewal_ = renewal;
    grant_->deadline.microseconds = current->microseconds + lease_microseconds;
    return LeaseResult::accepted;
}

void LeaseAuthority::revoke() noexcept { grant_.reset(); }
void LeaseAuthority::suspend_or_resume() noexcept { fenced_ = true; revoke(); }
bool LeaseAuthority::recover_after_readiness() noexcept {
    const auto current = clock_.now();
    if (current.domain.value == 0) return false;
    last_stamp_ = current;
    grant_.reset();
    fenced_ = false;
    return true;
}
} // namespace saex::core
