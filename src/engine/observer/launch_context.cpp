#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "saex/engine/launch_context.hpp"
#include <algorithm>
#include <cstring>

namespace saex::engine {
namespace {
struct Handle {
    HANDLE value{INVALID_HANDLE_VALUE};
    ~Handle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};
bool absolute_drive_path(std::wstring_view path) noexcept {
    return path.size() >= 3 && ((path[0] >= L'A' && path[0] <= L'Z') || (path[0] >= L'a' && path[0] <= L'z')) &&
        path[1] == L':' && (path[2] == L'\\' || path[2] == L'/') && path.find(L'\0') == path.npos;
}
std::wstring_view key(std::wstring_view entry) noexcept {
    const auto equal = entry.find(L'=', entry.starts_with(L'=') ? 1 : 0);
    return equal == entry.npos ? std::wstring_view{} : entry.substr(0, equal);
}
bool valid_entry(std::wstring_view entry) noexcept {
    if (entry.empty() || entry.size() > 32767 || entry.find(L'\0') != entry.npos) return false;
    const auto name = key(entry);
    if (name.empty()) return false;
    if (name[0] == L'=') {
        // Preserve the documented per-drive current-directory entries (=C:=...).
        return name.size() == 3 && name[2] == L':' &&
            ((name[1] >= L'A' && name[1] <= L'Z') || (name[1] >= L'a' && name[1] <= L'z')) &&
            absolute_drive_path(entry.substr(4));
    }
    return true;
}
int compare(std::wstring_view left, std::wstring_view right) noexcept {
    return CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(), static_cast<int>(right.size()), TRUE);
}
}

LaunchContext::LaunchContext(std::wstring_view directory) {
    struct Environment {
        LPWCH value{GetEnvironmentStringsW()};
        ~Environment() { if (value) FreeEnvironmentStringsW(value); }
    } snapshot;
    if (!snapshot.value) { error_ = "launch_environment_capture_failed"; return; }
    std::vector<std::wstring_view> entries;
    std::size_t offset{};
    while (snapshot.value[offset]) {
        if (entries.size() == max_environment_entries) { error_ = "launch_environment_limit"; return; }
        const auto begin = offset;
        while (snapshot.value[offset]) {
            if (++offset >= max_environment_units - 1) { error_ = "launch_environment_limit"; return; }
        }
        entries.emplace_back(snapshot.value + begin, offset - begin);
        ++offset;
    }
    prepare(directory, entries);
}
LaunchContext::LaunchContext(std::wstring_view directory, std::span<const std::wstring_view> entries) { prepare(directory, entries); }
LaunchContext::~LaunchContext() { if (directory_handle_) CloseHandle(directory_handle_); }

void LaunchContext::prepare(std::wstring_view directory, std::span<const std::wstring_view> entries) {
    // Deliberately restrict this first local harness to ordinary absolute drive paths.
    if (!absolute_drive_path(directory) || directory.size() >= MAX_PATH) { error_ = "launch_directory_input"; return; }
    if (entries.size() > max_environment_entries) { error_ = "launch_environment_limit"; return; }
    std::size_t units = entries.empty() ? 2 : 1;
    for (const auto entry : entries) {
        if (!valid_entry(entry)) { error_ = "launch_environment_entry"; return; }
        if (entry.size() + 1 > max_environment_units - units) { error_ = "launch_environment_limit"; return; }
        units += entry.size() + 1;
    }
    std::vector<std::wstring_view> ordered(entries.begin(), entries.end());
    std::sort(ordered.begin(), ordered.end(), [](auto a, auto b) { return compare(key(a), key(b)) == CSTR_LESS_THAN; });
    for (std::size_t i = 1; i < ordered.size(); ++i) {
        if (compare(key(ordered[i - 1]), key(ordered[i])) != CSTR_LESS_THAN) { error_ = "launch_environment_duplicate"; return; }
    }
    std::vector<wchar_t> block;
    block.reserve(units);
    for (const auto entry : ordered) { block.insert(block.end(), entry.begin(), entry.end()); block.push_back(L'\0'); }
    block.push_back(L'\0');
    if (ordered.empty()) block.push_back(L'\0');
    Sha256 hash{};
    static_assert(sizeof(wchar_t) == 2);
    if (BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0, reinterpret_cast<PUCHAR>(block.data()),
        static_cast<ULONG>(block.size() * sizeof(wchar_t)), reinterpret_cast<PUCHAR>(hash.data()), static_cast<ULONG>(hash.size())) < 0) {
        error_ = "launch_environment_hash_failed"; return;
    }
    const std::wstring input(directory);
    Handle pin{CreateFileW(input.c_str(), FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr)};
    FILE_BASIC_INFO info{};
    if (pin.value == INVALID_HANDLE_VALUE || !GetFileInformationByHandleEx(pin.value, FileBasicInfo, &info, sizeof(info)) ||
        !(info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) { error_ = "launch_directory_unavailable"; return; }
    std::array<wchar_t, 32768> path{};
    const auto length = GetFinalPathNameByHandleW(pin.value, path.data(), static_cast<DWORD>(path.size()), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (!length || length >= path.size()) { error_ = "launch_directory_resolution"; return; }
    const std::wstring_view resolved(path.data(), length);
    if (!resolved.starts_with(L"\\\\?\\") || !absolute_drive_path(resolved.substr(4)) || resolved.size() - 4 >= MAX_PATH) {
        error_ = "launch_directory_resolution"; return;
    }
    directory_ = resolved.substr(4); // All potentially throwing work precedes ownership transfer.
    environment_ = std::move(block);
    environment_hash_ = hash;
    entry_count_ = ordered.size();
    directory_handle_ = pin.value; pin.value = INVALID_HANDLE_VALUE;
    ready_ = true; error_ = {};
}
bool LaunchContext::directory_matches() const noexcept {
    if (!ready_) return false;
    Handle current{CreateFileW(directory_.c_str(), FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr)};
    FILE_ID_INFO expected{}, actual{};
    return current.value != INVALID_HANDLE_VALUE &&
        GetFileInformationByHandleEx(directory_handle_, FileIdInfo, &expected, sizeof(expected)) &&
        GetFileInformationByHandleEx(current.value, FileIdInfo, &actual, sizeof(actual)) &&
        expected.VolumeSerialNumber == actual.VolumeSerialNumber &&
        std::memcmp(expected.FileId.Identifier, actual.FileId.Identifier, sizeof(expected.FileId.Identifier)) == 0;
}
}
