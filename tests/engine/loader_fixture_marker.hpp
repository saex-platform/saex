#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
// Canary fixture only. Files are next to our test EXE, never inside a GTA install.
inline void mark_loader_phase(const wchar_t* suffix) {
    wchar_t path[32768]{};
    const auto n = GetModuleFileNameW(nullptr, path, 32768);
    if (!n || n > 32700) ExitProcess(80);
    DWORD at = n;
    while (*suffix) path[at++] = *suffix++;
    const auto file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) ExitProcess(81);
    CloseHandle(file);
}
