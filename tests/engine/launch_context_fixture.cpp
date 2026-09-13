#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include <string_view>

// Fully owned positive control. The suspended observer must never reach this main.
int main() {
    std::array<wchar_t, 32768> cwd{}, expected{};
    std::array<wchar_t, 128> token{};
    if (!GetCurrentDirectoryW(static_cast<DWORD>(cwd.size()), cwd.data()) ||
        !GetEnvironmentVariableW(L"SAEX_CONTEXT_EXPECTED_DIRECTORY", expected.data(), static_cast<DWORD>(expected.size())) ||
        !GetEnvironmentVariableW(L"SAEX_CONTEXT_TOKEN", token.data(), static_cast<DWORD>(token.size()))) return 71;
    if (CompareStringOrdinal(cwd.data(), -1, expected.data(), -1, TRUE) != CSTR_EQUAL ||
        std::wstring_view(token.data()) != L"sabit-çığ=1") return 72;
    const auto file = CreateFileW(L"context.received", GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return 74;
    CloseHandle(file);
    return 73;
}
