#include "saex/engine/bootstrap_session.hpp"
#include <cstring>
#include <iostream>
#include <latch>
#include <stdexcept>
#include <thread>

using namespace saex::engine;
namespace {
void require(bool condition, const char* why) { if (!condition) throw std::runtime_error(why); }
struct Probe final : BootstrapObservation {
    unsigned calls{};
    std::uint32_t reason = SAEX_BOOTSTRAP_RUNTIME_UNVERIFIED;
    std::uint32_t inspect() noexcept override { ++calls; return reason; }
};
struct BlockingProbe final : BootstrapObservation {
    std::latch entered{1}, release{1};
    std::uint32_t inspect() noexcept override { entered.count_down(); release.wait(); return SAEX_BOOTSTRAP_RUNTIME_UNVERIFIED; }
};
}
int main() {
    try {
        BootstrapSession session;
        Probe probe;
        SaexBootstrapStatus out{};
        const auto call = [&](BootstrapOperation operation) { return session.invoke(operation, probe, 1, &out, sizeof(out)); };
        require(call(BootstrapOperation::query) == SAEX_BOOTSTRAP_OK && out.state == SAEX_BOOTSTRAP_DISCOVERED && probe.calls == 0, "query triggered observation");
        out.reserved = 42;
        const auto before = out;
        require(session.invoke(BootstrapOperation::initialize, probe, 2, &out, sizeof(out)) == SAEX_BOOTSTRAP_ABI_MISMATCH, "ABI mismatch");
        require(session.invoke(BootstrapOperation::initialize, probe, 1, nullptr, sizeof(out)) == SAEX_BOOTSTRAP_INVALID_ARGUMENT, "null output");
        for (std::uint32_t n : {0U, 191U, 193U, 0xffffffffU})
            require(session.invoke(BootstrapOperation::initialize, probe, 1, &out, n) == SAEX_BOOTSTRAP_INVALID_ARGUMENT, "output size");
        require(std::memcmp(&before, &out, sizeof(out)) == 0 && probe.calls == 0, "rejection changed state/output");
        require(call(BootstrapOperation::initialize) == SAEX_BOOTSTRAP_OK && out.state == SAEX_BOOTSTRAP_OBSERVED_UNVERIFIED &&
            out.observation_attempts == 1 && out.can_attach == 0 && out.bindings_loaded == 0 && out.observed_profile_id[0], "observation became permission");
        require(call(BootstrapOperation::initialize) == SAEX_BOOTSTRAP_OK && probe.calls == 1, "duplicate initialize repeated IO");
        require(call(BootstrapOperation::stop) == SAEX_BOOTSTRAP_OK && out.state == SAEX_BOOTSTRAP_STOPPED, "stop");
        require(call(BootstrapOperation::stop) == SAEX_BOOTSTRAP_OK && out.state == SAEX_BOOTSTRAP_STOPPED, "idempotent stop");
        require(call(BootstrapOperation::initialize) == SAEX_BOOTSTRAP_OK && out.state == SAEX_BOOTSTRAP_STOPPED && probe.calls == 1, "restart after stop");
        BootstrapSession early;
        require(early.invoke(BootstrapOperation::stop, probe, 1, &out, sizeof(out)) == SAEX_BOOTSTRAP_OK && out.observation_attempts == 0, "stop before initialize");
        require(early.invoke(BootstrapOperation::initialize, probe, 1, &out, sizeof(out)) == SAEX_BOOTSTRAP_OK && out.state == SAEX_BOOTSTRAP_STOPPED, "early stopped session restarted");
        for (const auto reason : {0U, 1U, 2U, 3U, 4U, 6U, 999U}) {
            BootstrapSession rejected;
            Probe failure; failure.reason = reason;
            require(rejected.invoke(BootstrapOperation::initialize, failure, 1, &out, sizeof(out)) == SAEX_BOOTSTRAP_OK &&
                out.state == SAEX_BOOTSTRAP_REJECTED && !out.can_attach && !out.bindings_loaded && !out.observed_profile_id[0], "failed observation allowed");
            require(rejected.invoke(BootstrapOperation::initialize, failure, 1, &out, sizeof(out)) == SAEX_BOOTSTRAP_OK && failure.calls == 1, "failure retry storm");
        }
        BootstrapSession concurrent;
        BlockingProbe blocked;
        SaexBootstrapStatus worker_out{};
        std::uint32_t worker_result = 99;
        std::thread worker([&] { worker_result = concurrent.invoke(BootstrapOperation::initialize, blocked, 1, &worker_out, sizeof(worker_out)); });
        blocked.entered.wait();
        const auto saved = out;
        const auto query = concurrent.invoke(BootstrapOperation::query, probe, 1, &out, sizeof(out));
        const auto stop = concurrent.invoke(BootstrapOperation::stop, probe, 1, &out, sizeof(out));
        const auto initialize = concurrent.invoke(BootstrapOperation::initialize, probe, 1, &out, sizeof(out));
        blocked.release.count_down(); worker.join();
        require(query == SAEX_BOOTSTRAP_BUSY && stop == SAEX_BOOTSTRAP_BUSY && initialize == SAEX_BOOTSTRAP_BUSY &&
            std::memcmp(&saved, &out, sizeof(out)) == 0, "concurrent call blocked or overwrote output");
        require(worker_result == SAEX_BOOTSTRAP_OK && worker_out.state == SAEX_BOOTSTRAP_OBSERVED_UNVERIFIED, "in-flight initialize failed");
        require(concurrent.invoke(BootstrapOperation::stop, probe, 1, &out, sizeof(out)) == SAEX_BOOTSTRAP_OK, "stop after busy");
        std::cout << "PASS bootstrap session: ABI/output, no lazy observation, terminal states, reason rejection, concurrent BUSY and stop\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
