#pragma once
#include "saex/engine/profile_gate.hpp"

namespace saex::engine {
// Current process only; no PID input or arbitrary foreign-process reader.
class WindowsImageReader final : public ImageReader {
public:
    WindowsImageReader(const void* base, std::size_t image_bytes) noexcept;
    bool copy(std::uint32_t rva, std::span<std::byte> output) const noexcept override;
private:
    std::uintptr_t base_{};
    std::size_t size_{};
};
}
