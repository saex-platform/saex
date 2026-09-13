#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstring>
#include <string_view>

namespace {
bool same_directory(const wchar_t* expected) {
    const auto open = [](const wchar_t* path) {
        return CreateFileW(path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    };
    const auto actual = open(L".");
    const auto requested = open(expected);
    FILE_ID_INFO actual_id{}, requested_id{};
    const bool equal = actual != INVALID_HANDLE_VALUE && requested != INVALID_HANDLE_VALUE &&
        GetFileInformationByHandleEx(actual, FileIdInfo, &actual_id, sizeof(actual_id)) &&
        GetFileInformationByHandleEx(requested, FileIdInfo, &requested_id, sizeof(requested_id)) &&
        actual_id.VolumeSerialNumber == requested_id.VolumeSerialNumber &&
        std::memcmp(&actual_id.FileId, &requested_id.FileId, sizeof(actual_id.FileId)) == 0;
    if (actual != INVALID_HANDLE_VALUE) CloseHandle(actual);
    if (requested != INVALID_HANDLE_VALUE) CloseHandle(requested);
    return equal;
}
}

// Fully owned positive control. The suspended observer must never reach this main.
int main() {
    std::array<wchar_t, 32768> expected{};
    std::array<wchar_t, 128> token{};
    if (!GetEnvironmentVariableW(L"SAEX_CONTEXT_EXPECTED_DIRECTORY", expected.data(), static_cast<DWORD>(expected.size())) ||
        !GetEnvironmentVariableW(L"SAEX_CONTEXT_TOKEN", token.data(), static_cast<DWORD>(token.size()))) return 71;
    if (!same_directory(expected.data()) ||
        std::wstring_view(token.data()) != L"sabit-çığ=1") return 72;
    const auto file = CreateFileW(L"context.received", GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return 74;
    CloseHandle(file);
    return 73;
}
