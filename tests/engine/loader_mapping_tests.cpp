#include "saex/engine/loader_mapping_ledger.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace saex::engine;
namespace {
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
std::uint32_t admitted(LoaderMappingLedger& ledger, std::uintptr_t base, std::uint32_t event) {
    const auto result = ledger.admit(base, event);
    require(result.error.empty() && result.mapping_id, "load transition failed");
    return result.mapping_id;
}
void retired(LoaderMappingLedger& ledger, std::uintptr_t base, std::uint32_t event, std::uint32_t id) {
    const auto result = ledger.retire(base, event);
    require(result.error.empty() && result.mapping_id == id, "unload retired wrong generation");
}
void error(MappingTransition result, std::string_view expected) {
    require(result.mapping_id == 0 && result.error == expected, "incorrect rejection");
}
void normal_lifetime() {
    LoaderMappingLedger ledger;
    const auto id = admitted(ledger, 0x10000, 2);
    require(id == 1 && ledger.active_count() == 1 && !ledger.unload_count() && ledger.active(id, 0x10000), "initial active mapping");
    retired(ledger, 0x10000, 8, id);
    const auto record = ledger.history().front();
    require(!ledger.active(id, 0x10000) && !ledger.active_count() && ledger.unload_count() == 1, "unload left active mapping");
    require(record.id == id && record.base == 0x10000 && record.load_event == 2 && record.unload_event == 8, "lost immutable load history");
}
void reuse_never_revives_old_identity() {
    LoaderMappingLedger ledger;
    const auto old = admitted(ledger, 0x10000, 2);
    const auto peer = admitted(ledger, 0x20000, 3);
    retired(ledger, 0x10000, 5, old);
    const auto replacement = admitted(ledger, 0x10000, 6);
    require(replacement != old && replacement != peer && !ledger.active(old, 0x10000), "base reuse revived stale identity");
    require(ledger.active(replacement, 0x10000) && ledger.active(peer, 0x20000), "live mapping lost");
    retired(ledger, 0x10000, 9, replacement);
    require(ledger.history()[0].unload_event == 5 && ledger.history()[2].unload_event == 9, "replacement rewrote old history");
}
void duplicate_base_rejection_is_atomic() {
    LoaderMappingLedger ledger;
    const auto id = admitted(ledger, 0x10000, 2);
    error(ledger.admit(0x10000, 8), "loader_mapping_base_active");
    require(ledger.history().size() == 1 && ledger.active_count() == 1, "duplicate load changed counters");
    retired(ledger, 0x10000, 4, id); // Rejection must not consume event 8.
}
void unknown_and_duplicate_unload_are_atomic() {
    LoaderMappingLedger ledger;
    error(ledger.retire(0x20000, 12), "loader_mapping_not_active");
    const auto id = admitted(ledger, 0x10000, 2);
    error(ledger.retire(0x20000, 13), "loader_mapping_not_active");
    require(ledger.active(id, 0x10000) && !ledger.unload_count(), "unknown unload changed valid mapping");
    retired(ledger, 0x10000, 3, id);
    error(ledger.retire(0x10000, 14), "loader_mapping_not_active");
    require(ledger.unload_count() == 1 && ledger.history()[0].unload_event == 3, "duplicate unload changed history");
    require(admitted(ledger, 0x10000, 4) != id, "rejected event consumed ordering");
}
void ordering_rejection() {
    LoaderMappingLedger ledger;
    error(ledger.admit(0x10000, 0), "loader_mapping_event_order");
    const auto id = admitted(ledger, 0x10000, 9);
    for (const auto event : {0U, 8U, 9U}) {
        error(ledger.admit(0x20000, event), "loader_mapping_event_order");
        error(ledger.retire(0x10000, event), "loader_mapping_event_order");
    }
    require(ledger.history().size() == 1 && ledger.active(id, 0x10000), "out-of-order event mutated state");
    retired(ledger, 0x10000, 10, id);
}
void zero_base_and_stale_queries() {
    LoaderMappingLedger ledger;
    error(ledger.admit(0, 1), "loader_mapping_invalid_base");
    error(ledger.retire(0, 1), "loader_mapping_invalid_base");
    const auto id = admitted(ledger, 0x10000, 1);
    require(!ledger.active(0, 0x10000) && !ledger.active(id, 0) && !ledger.active(id, 0x20000) &&
        !ledger.active(id + 1, 0x10000) && !ledger.active(0xffffffffU, 0x10000), "invalid reference accepted");
}
void invalid_capacity() {
    for (const auto limit : {0U, 65U, 0xffffffffU}) {
        LoaderMappingLedger ledger(limit);
        error(ledger.admit(0x10000, 1), "loader_mapping_invalid_limit");
        error(ledger.retire(0x10000, 1), "loader_mapping_invalid_limit");
        require(ledger.history().empty() && !ledger.active_count() && !ledger.unload_count(), "invalid capacity mutated state");
    }
}
void no_capacity_refund_after_unload() {
    LoaderMappingLedger ledger(1);
    const auto id = admitted(ledger, 0x10000, 1);
    retired(ledger, 0x10000, 2, id);
    error(ledger.admit(0x10000, 3), "loader_mapping_limit");
    error(ledger.admit(0x20000, 4), "loader_mapping_limit");
    require(ledger.history().size() == 1 && !ledger.active_count(), "retire refunded lifetime budget");
}
void maximum_lifetime_budget() {
    LoaderMappingLedger ledger;
    for (std::uint32_t i = 0; i < max_loader_mappings; ++i) {
        const auto id = admitted(ledger, 0x10000, 2 * i + 1);
        require(id == i + 1, "generation reused");
        retired(ledger, 0x10000, 2 * i + 2, id);
    }
    require(ledger.history().size() == 64 && ledger.unload_count() == 64 && !ledger.active_count(), "history bound failed");
    error(ledger.admit(0x10000, 129), "loader_mapping_limit");
}
void event_counter_never_wraps() {
    LoaderMappingLedger ledger;
    const auto id = admitted(ledger, 0x10000, std::numeric_limits<std::uint32_t>::max());
    error(ledger.retire(0x10000, 0), "loader_mapping_event_order");
    error(ledger.admit(0x20000, 1), "loader_mapping_event_order");
    require(ledger.active(id, 0x10000) && ledger.history().size() == 1, "counter wrap accepted");
}
}
int main() {
    struct Test { const char* name; void (*run)(); };
    constexpr Test tests[]{
        {"normal_lifetime", normal_lifetime}, {"reuse_never_revives_old_identity", reuse_never_revives_old_identity},
        {"duplicate_base_rejection_is_atomic", duplicate_base_rejection_is_atomic},
        {"unknown_and_duplicate_unload_are_atomic", unknown_and_duplicate_unload_are_atomic},
        {"ordering_rejection", ordering_rejection}, {"zero_base_and_stale_queries", zero_base_and_stale_queries},
        {"invalid_capacity", invalid_capacity}, {"no_capacity_refund_after_unload", no_capacity_refund_after_unload},
        {"maximum_lifetime_budget", maximum_lifetime_budget}, {"event_counter_never_wraps", event_counter_never_wraps}};
    unsigned passed{};
    for (const auto& test : tests) {
        try { test.run(); ++passed; std::cout << "PASS " << test.name << '\n'; }
        catch (const std::exception& failure) { std::cerr << "FAIL " << test.name << ": " << failure.what() << '\n'; }
    }
    std::cout << passed << "/10 loader mapping tests passed\n";
    return passed == 10 ? 0 : 1;
}
