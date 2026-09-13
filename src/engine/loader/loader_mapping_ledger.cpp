#include "saex/engine/loader_mapping_ledger.hpp"

namespace saex::engine {
std::string_view LoaderMappingLedger::validate(std::uintptr_t base, std::uint32_t event) const noexcept {
    if (!limit_ || limit_ > max_loader_mappings) return "loader_mapping_invalid_limit";
    if (!base) return "loader_mapping_invalid_base";
    if (!event || event <= last_event_) return "loader_mapping_event_order";
    return {};
}
MappingTransition LoaderMappingLedger::admit(std::uintptr_t base, std::uint32_t event) noexcept {
    if (const auto error = validate(base, event); !error.empty()) return {0, error};
    for (const auto& mapping : history()) {
        if (mapping.base == base && !mapping.unload_event) return {0, "loader_mapping_base_active"};
    }
    if (count_ == limit_) return {0, "loader_mapping_limit"};
    const auto id = count_ + 1;
    history_[count_] = {base, id, event, 0};
    ++count_; ++active_; last_event_ = event;
    return {id, {}};
}
MappingTransition LoaderMappingLedger::retire(std::uintptr_t base, std::uint32_t event) noexcept {
    if (const auto error = validate(base, event); !error.empty()) return {0, error};
    for (std::uint32_t i = 0; i < count_; ++i) {
        auto& mapping = history_[i];
        if (mapping.base == base && !mapping.unload_event) {
            mapping.unload_event = event;
            --active_; last_event_ = event;
            return {mapping.id, {}};
        }
    }
    return {0, "loader_mapping_not_active"};
}
bool LoaderMappingLedger::active(std::uint32_t id, std::uintptr_t base) const noexcept {
    return id && id <= count_ && base && history_[id - 1].base == base && !history_[id - 1].unload_event;
}
}
