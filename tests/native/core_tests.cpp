#include <saex/core/lease_authority.hpp>
#include <saex/core/bounded_inbox.hpp>
#include <saex/contracts/fixture_codec.hpp>
#include <atomic>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <thread>
#include <type_traits>

using namespace saex::contracts;
using namespace saex::core;
static_assert(!std::is_convertible_v<WorldEpoch, OwnerEpoch>);
static_assert(!std::is_convertible_v<std::uint64_t, WorldId>);

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
struct FakeClock final : ControlClock {
    ControlStamp stamp{ClockDomainId{1}, 0};
    ControlStamp now() const noexcept override { return stamp; }
};
const EntityRef entity{WorldId{1}, WorldEpoch{2}, EntityId{3}, Generation{4}};
LeaseGrant grant(LeaseAuthority& gate) {
    const auto result = gate.issue(SessionId{5}, CatalogRevision{6}, StateRevision{7}, 3, true);
    require(result.has_value(), "expected grant");
    return *result;
}
OwnerProposal proposal(const LeaseGrant& value, std::uint64_t sequence = 1) {
    return {value.entity, value.session, value.owner, value.catalog, value.state, Sequence{sequence}, 1};
}
}

int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests{
        {"checked identities never wrap", [] {
            require(!next(Generation{std::numeric_limits<std::uint64_t>::max()}), "wrapped generation");
            require(next(OwnerEpoch{0})->value == 1, "first epoch");
            require(!EntityRef{}.valid(), "zero entity accepted");
        }},
        {"AC-71 simulation freeze cannot extend lease", [] {
            FakeClock clock; LeaseAuthority gate(clock, entity); const auto lease = grant(gate);
            const SimulationTick frozen{42};
            clock.stamp.microseconds = 1'999'999;
            require(gate.accept(proposal(lease)) == LeaseResult::accepted, "early expiry");
            clock.stamp.microseconds = 2'000'000;
            require(gate.accept(proposal(lease, 2)) == LeaseResult::expired, "deadline not exclusive");
            require(frozen.value == 42, "simulation changed");
            require(gate.renew(lease.session, lease.owner, Sequence{1}) == LeaseResult::no_grant, "resurrected lease");
        }},
        {"late first proposal expires at dequeue", [] {
            FakeClock clock; LeaseAuthority gate(clock, entity); auto lease = grant(gate);
            clock.stamp.microseconds = 3'000'000;
            require(gate.accept(proposal(lease)) == LeaseResult::expired, "queued stale command accepted");
        }},
        {"renewal sequence does not replay", [] {
            FakeClock clock; LeaseAuthority gate(clock, entity); const auto lease = grant(gate);
            clock.stamp.microseconds = 500'000;
            require(gate.renew(lease.session, lease.owner, Sequence{1}) == LeaseResult::accepted, "renew failed");
            clock.stamp.microseconds = 1'000'000;
            require(gate.renew(lease.session, lease.owner, Sequence{1}) == LeaseResult::stale_sequence, "replay renewed");
            clock.stamp.microseconds = 2'500'000;
            require(gate.accept(proposal(lease)) == LeaseResult::expired, "duplicate extended deadline");
        }},
        {"lease identity revision and permissions", [] {
            FakeClock clock; LeaseAuthority gate(clock, entity); const auto lease = grant(gate);
            auto input = proposal(lease); input.entity.generation = Generation{99};
            require(gate.accept(input) == LeaseResult::stale_entity, "stale generation");
            input = proposal(lease); input.session = SessionId{99};
            require(gate.accept(input) == LeaseResult::stale_owner, "foreign session");
            input = proposal(lease); input.catalog = CatalogRevision{99};
            require(gate.accept(input) == LeaseResult::stale_revision, "wrong catalog");
            input = proposal(lease); input.field_mask = 4;
            require(gate.accept(input) == LeaseResult::field_denied, "unauthorized field");
            require(gate.accept(proposal(lease)) == LeaseResult::accepted, "reject consumed sequence");
            require(gate.accept(proposal(lease)) == LeaseResult::stale_sequence, "motion replay");
        }},
        {"readiness required and old owner revoked", [] {
            FakeClock clock; LeaseAuthority gate(clock, entity);
            require(!gate.issue(SessionId{5}, CatalogRevision{6}, StateRevision{7}, 3, false), "unready grant");
            const auto old = grant(gate); const auto newer = grant(gate);
            require(newer.owner.value > old.owner.value, "owner epoch reused");
            require(gate.accept(proposal(old)) == LeaseResult::stale_owner, "old owner accepted");
        }},
        {"AC-72 clock regression requires recovery", [] {
            FakeClock clock; clock.stamp.microseconds = 100;
            LeaseAuthority gate(clock, entity); const auto old = grant(gate);
            clock.stamp.microseconds = 99;
            require(gate.accept(proposal(old)) == LeaseResult::clock_discontinuity, "clock rollback accepted");
            require(!gate.issue(SessionId{5}, CatalogRevision{6}, StateRevision{7}, 3, true), "fence bypassed");
            require(gate.recover_after_readiness(), "recovery failed");
            const auto newer = grant(gate);
            require(newer.owner.value > old.owner.value, "recovery reused owner");
        }},
        {"AC-72 clock domain and explicit resume fence", [] {
            FakeClock clock; LeaseAuthority gate(clock, entity); const auto old = grant(gate);
            clock.stamp.domain = ClockDomainId{2};
            require(gate.accept(proposal(old)) == LeaseResult::clock_discontinuity, "domain mismatch");
            require(gate.recover_after_readiness(), "new domain recovery");
            const auto fresh = grant(gate); gate.suspend_or_resume();
            require(gate.accept(proposal(fresh)) == LeaseResult::recovery_required, "resume stale grant");
        }},
        {"deadline addition overflow fences", [] {
            FakeClock clock; clock.stamp.microseconds = std::numeric_limits<std::uint64_t>::max() - 10;
            LeaseAuthority gate(clock, entity);
            require(!gate.issue(SessionId{5}, CatalogRevision{6}, StateRevision{7}, 3, true), "overflow grant");
        }},
        {"real steady provider is monotonic", [] {
            SteadyControlClock clock(ClockDomainId{9}); const auto first = clock.now();
            const auto second = clock.now();
            require(first.domain == second.domain && second.microseconds >= first.microseconds, "non-monotonic");
        }},
        {"inbox limits and close preserve admitted work", [] {
            std::array<std::byte, 4> data{};
            BoundedInbox queue(2, BoundedInbox::command_charge + data.size());
            require(queue.push(ResourceEpoch{}, data) == Admission::invalid_epoch, "invalid resource");
            require(queue.push(ResourceEpoch{1}, data) == Admission::admitted, "admit");
            require(queue.push(ResourceEpoch{1}, data) == Admission::byte_limit, "byte cap");
            queue.close(); require(queue.push(ResourceEpoch{1}, data) == Admission::closed, "closed admission");
            require(queue.pop().has_value() && !queue.pop(), "drain lost work");
            require(queue.usage().charged_bytes == 0, "bytes leaked");
        }},
        {"inbox concurrent producers stay bounded", [] {
            BoundedInbox queue(128, 128 * (BoundedInbox::command_charge + 8));
            std::atomic<int> accepted{}; std::vector<std::jthread> producers;
            for (int index = 0; index < 8; ++index) producers.emplace_back([&] {
                std::array<std::byte, 8> data{};
                for (int attempt = 0; attempt < 100; ++attempt)
                    if (queue.push(ResourceEpoch{1}, data) == Admission::admitted) ++accepted;
            });
            producers.clear();
            require(accepted == 128 && queue.usage().items == 128, "concurrent cap violated");
            int drained{}; while (queue.pop()) ++drained;
            require(drained == 128 && queue.usage().charged_bytes == 0, "drain/accounting mismatch");
        }},
        {"fixture explicit little endian and full width", [] {
            const EntityRef wide{WorldId{0x0102030405060708ULL}, WorldEpoch{2}, EntityId{3}, Generation{~0ULL}};
            const auto frame = encode_fixture(wide);
            require(frame[12] == std::byte{8} && frame[19] == std::byte{1}, "endianness");
            const auto result = decode_fixture(frame);
            require(result.error == DecodeError::none && result.entity == wide, "roundtrip");
        }},
        {"fixture rejects every truncated prefix", [] {
            const auto frame = encode_fixture(entity);
            for (std::size_t size = 0; size < frame.size(); ++size)
                require(decode_fixture(std::span{frame}.first(size)).error != DecodeError::none, "prefix accepted");
        }},
        {"fixture malformed headers and zero identity", [] {
            auto frame = encode_fixture(entity); frame[8] = std::byte{255};
            require(decode_fixture(frame).error == DecodeError::invalid_length, "oversize accepted");
            frame = encode_fixture(entity); frame[4] = std::byte{2};
            require(decode_fixture(frame).error == DecodeError::unsupported_version, "version accepted");
            frame = encode_fixture(entity); frame[6] = std::byte{1};
            require(decode_fixture(frame).error == DecodeError::reserved_bits, "reserved accepted");
            frame = encode_fixture(entity); std::fill(frame.begin() + 12, frame.begin() + 20, std::byte{0});
            require(decode_fixture(frame).error == DecodeError::invalid_entity, "zero identity accepted");
        }},
        {"bounded decoder random corpus", [] {
            std::mt19937 random(20260912); std::array<std::byte, 80> input{};
            for (int iteration = 0; iteration < 10'000; ++iteration) {
                for (auto& value : input) value = static_cast<std::byte>(random() & 0xffU);
                const auto size = static_cast<std::size_t>(random() % input.size());
                const auto result = decode_fixture(std::span{input}.first(size));
                require(result.error != DecodeError::none, "random frame unexpectedly valid");
            }
        }}
    };
    std::size_t failures{};
    for (const auto& [name, test] : tests) {
        try { test(); std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& error) { ++failures; std::cerr << "FAIL " << name << ": " << error.what() << '\n'; }
    }
    std::cout << tests.size() - failures << '/' << tests.size() << " native tests passed\n";
    return failures == 0 ? 0 : 1;
}
