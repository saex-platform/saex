#pragma once
#include "saex/engine/pe_image.hpp"
#include "saex/engine/profile_gate.hpp"
#include <vector>

namespace saex::engine {
// Keeps the same read/share-read handle alive through the subsequent image check.
class WindowsFileObservation {
public:
    explicit WindowsFileObservation(const wchar_t* path);
    ~WindowsFileObservation();
    WindowsFileObservation(const WindowsFileObservation&) = delete;
    WindowsFileObservation& operator=(const WindowsFileObservation&) = delete;
    [[nodiscard]] std::string_view error() const noexcept { return error_; }
    [[nodiscard]] void* handle() const noexcept { return handle_; }
    [[nodiscard]] std::span<const std::byte> bytes() const noexcept { return bytes_; }
    [[nodiscard]] const PeLayout& layout() const noexcept { return layout_; }
    [[nodiscard]] const Sha256& hash() const noexcept { return hash_; }
private:
    void* handle_{};
    std::vector<std::byte> bytes_;
    PeLayout layout_{};
    Sha256 hash_{};
    std::string_view error_;
};
}
