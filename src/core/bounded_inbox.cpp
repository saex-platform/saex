#include <saex/core/bounded_inbox.hpp>
#include <stdexcept>

namespace saex::core {
BoundedInbox::BoundedInbox(std::size_t max_items, std::size_t max_bytes)
    : max_items_(max_items), max_bytes_(max_bytes) {
    if (max_items == 0 || max_bytes < command_charge) throw std::invalid_argument("Invalid inbox budget");
}
Admission BoundedInbox::push(contracts::ResourceEpoch resource, std::span<const std::byte> payload) {
    std::lock_guard lock(mutex_);
    if (closed_) return Admission::closed;
    if (resource.value == 0) return Admission::invalid_epoch;
    if (queue_.size() >= max_items_) return Admission::item_limit;
    const auto remaining = max_bytes_ - bytes_;
    if (remaining < command_charge || payload.size() > remaining - command_charge) return Admission::byte_limit;
    queue_.push_back(IngressCommand{resource, {payload.begin(), payload.end()}});
    bytes_ += command_charge + payload.size();
    return Admission::admitted;
}
std::optional<IngressCommand> BoundedInbox::pop() {
    std::lock_guard lock(mutex_);
    if (queue_.empty()) return std::nullopt;
    auto command = std::move(queue_.front());
    queue_.pop_front();
    bytes_ -= command_charge + command.payload.size();
    return command;
}
InboxUsage BoundedInbox::usage() const {
    std::lock_guard lock(mutex_);
    return {queue_.size(), bytes_, closed_};
}
void BoundedInbox::close() { std::lock_guard lock(mutex_); closed_ = true; }
} // namespace saex::core
