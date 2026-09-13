#pragma once
#include "saex/engine/profile_gate.hpp"

namespace saex::engine {
class LaunchContext;
// Owned child only. Construct/read/stop/destroy on the same thread; no attach/PID API.
// Default observation holds the first create event until kill. Only the separate
// LoaderObservation friend can advance it for an explicit bounded loader experiment.
// CREATE_SUSPENDED is intentionally absent: it also prevents that first event.
class SuspendedImage final : public ImageReader {
public:
    SuspendedImage(const wchar_t* executable, std::size_t image_bytes, const LaunchContext* context = nullptr);
    ~SuspendedImage();
    SuspendedImage(const SuspendedImage&) = delete;
    SuspendedImage& operator=(const SuspendedImage&) = delete;
    bool copy(std::uint32_t rva, std::span<std::byte> output) const noexcept override;
    [[nodiscard]] bool same_file(void* verified_file) const noexcept;
    [[nodiscard]] bool stop() noexcept;
    [[nodiscard]] std::string_view error() const noexcept { return error_; }
    [[nodiscard]] std::uint32_t system_error() const noexcept { return system_error_; }
    [[nodiscard]] std::uint32_t process_id() const noexcept { return process_id_; }
    [[nodiscard]] std::uintptr_t image_base() const noexcept { return base_; }
    [[nodiscard]] bool stopped() const noexcept { return stopped_; }
    [[nodiscard]] bool created() const noexcept { return process_ != nullptr; }
private:
    friend class LoaderObservation;
    void* job_{};
    void* process_{};
    void* thread_{};
    void* image_file_{};
    std::uint32_t owner_thread_{}, process_id_{}, pending_thread_{}, system_error_{};
    std::uintptr_t base_{};
    std::size_t image_bytes_{};
    bool pending_{}, kill_requested_{}, exit_seen_{}, stopped_{}, loader_started_{};
    std::string_view error_;
};
}
