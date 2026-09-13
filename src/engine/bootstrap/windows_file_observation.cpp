#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include "saex/engine/windows_file_observation.hpp"
#include "saex/engine/observed_profile.generated.hpp"

namespace saex::engine {
WindowsFileObservation::WindowsFileObservation(const wchar_t* path) {
    handle_ = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle_ == INVALID_HANDLE_VALUE) { handle_ = nullptr; error_ = "file_open_failed"; return; }
    // Close the handle even if allocation throws before construction completes.
    try {
        LARGE_INTEGER length{};
        if (!GetFileSizeEx(handle_, &length) || length.QuadPart < 64 || length.QuadPart > max_image_bytes) {
            error_ = "engine_file_size"; return;
        }
        bytes_.resize(static_cast<std::size_t>(length.QuadPart));
        std::size_t offset{};
        while (offset < bytes_.size()) {
            DWORD got{};
            const auto request = static_cast<DWORD>(std::min<std::size_t>(65536, bytes_.size() - offset));
            if (!ReadFile(handle_, bytes_.data() + offset, request, &got, nullptr) || !got) {
                error_ = "file_read_failed"; return;
            }
            offset += got;
        }
        const auto parsed = parse_pe32(bytes_);
        if (parsed.error != PeError::none) { error_ = pe_error_name(parsed.error); return; }
        layout_ = parsed.layout;
        if (BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0, reinterpret_cast<PUCHAR>(bytes_.data()),
            static_cast<ULONG>(bytes_.size()), reinterpret_cast<PUCHAR>(hash_.data()), static_cast<ULONG>(hash_.size())) < 0) {
            error_ = "hash_failed"; return;
        }
        const auto result = match_file_observation(bytes_, hash_, layout_, observed_profile);
        if (result != ProfileResult::matched_observation) error_ = profile_result_name(result);
    } catch (...) { CloseHandle(handle_); handle_ = nullptr; throw; }
}
WindowsFileObservation::~WindowsFileObservation() { if (handle_) CloseHandle(handle_); }
}
