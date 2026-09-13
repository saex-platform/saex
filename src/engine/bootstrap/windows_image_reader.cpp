#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "saex/engine/windows_image_reader.hpp"
#include <algorithm>
#include <limits>

namespace saex::engine {
WindowsImageReader::WindowsImageReader(const void* base, std::size_t size) noexcept
    : base_(reinterpret_cast<std::uintptr_t>(base)), size_(size) {}

bool WindowsImageReader::copy(std::uint32_t rva, std::span<std::byte> output) const noexcept {
    if (!base_ || !size_ || size_ > max_image_bytes || output.empty() || output.size() > 1024 ||
        rva > size_ || output.size() > size_ - rva || size_ > std::numeric_limits<std::uintptr_t>::max() - base_)
        return false;
    auto cursor = base_ + rva;
    const auto end = cursor + output.size();
    while (cursor < end) {
        MEMORY_BASIC_INFORMATION region{};
        if (VirtualQuery(reinterpret_cast<const void*>(cursor), &region, sizeof(region)) != sizeof(region) ||
            region.State != MEM_COMMIT || region.Type != MEM_IMAGE ||
            reinterpret_cast<std::uintptr_t>(region.AllocationBase) != base_ || (region.Protect & PAGE_GUARD)) return false;
        const auto protection = region.Protect & 0xffU;
        if (protection != PAGE_READONLY && protection != PAGE_READWRITE && protection != PAGE_WRITECOPY &&
            protection != PAGE_EXECUTE_READ && protection != PAGE_EXECUTE_READWRITE && protection != PAGE_EXECUTE_WRITECOPY)
            return false;
        const auto start = reinterpret_cast<std::uintptr_t>(region.BaseAddress);
        if (start > cursor || region.RegionSize > std::numeric_limits<std::uintptr_t>::max() - start ||
            start + region.RegionSize <= cursor) return false;
        cursor = std::min<std::uintptr_t>(end, start + region.RegionSize);
    }
    SIZE_T copied{};
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(base_ + rva),
        output.data(), output.size(), &copied) && copied == output.size();
}
}
