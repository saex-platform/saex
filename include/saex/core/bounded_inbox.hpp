#pragma once
#include <saex/contracts/foundation.generated.hpp>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <span>
#include <vector>

namespace saex::core {
enum class Admission { admitted, closed, invalid_epoch, item_limit, byte_limit };
struct IngressCommand final {
    contracts::ResourceEpoch resource;
    std::vector<std::byte> payload;
};
struct InboxUsage final { std::size_t items; std::size_t charged_bytes; bool closed; };

// Logical bytes + fixed command charge. Allocator/queue overhead also needs RSS profiling.
class BoundedInbox final {
public:
    static constexpr std::size_t command_charge = sizeof(IngressCommand);
    BoundedInbox(std::size_t max_items, std::size_t max_bytes);
    [[nodiscard]] Admission push(contracts::ResourceEpoch resource, std::span<const std::byte> payload);
    [[nodiscard]] std::optional<IngressCommand> pop();
    [[nodiscard]] InboxUsage usage() const;
    void close();
private:
    const std::size_t max_items_;
    const std::size_t max_bytes_;
    mutable std::mutex mutex_;
    std::deque<IngressCommand> queue_;
    std::size_t bytes_{};
    bool closed_{};
};
} // namespace saex::core
