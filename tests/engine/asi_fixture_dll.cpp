#include "loader_fixture_marker.hpp"
#include <cwchar>
#include <cstring>
extern "C" __declspec(dllexport) void asi_export() {
    mark_loader_phase(L".asi-export-called"); ExitProcess(94);
}
BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    mark_loader_phase(L".asi-entered");
    wchar_t path[32768]{};
    if (!GetModuleFileNameW(nullptr, path, 32768)) return FALSE;
    if (std::wcsstr(path, L"asi_fixture_fail")) return FALSE;
    if (std::wcsstr(path, L"asi_fixture_fault")) DebugBreak();
    if (std::wcsstr(path, L"asi_fixture_stall")) Sleep(INFINITE);
    if (std::wcsstr(path, L"asi_fixture_iat_write")) {
        const auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        const auto imports = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(base + nt->OptionalHeader.DataDirectory[1].VirtualAddress);
        DWORD* slot{};
        for (unsigned i = 0; i < 64 && imports[i].Name; ++i) {
            if (_stricmp(reinterpret_cast<const char*>(base + imports[i].Name), "kernel32.dll")) continue;
            const auto lookup = reinterpret_cast<const DWORD*>(base + imports[i].OriginalFirstThunk);
            for (unsigned j = 0; j < 4096 && lookup[j]; ++j)
                if (!(lookup[j] & IMAGE_ORDINAL_FLAG32) && !std::strcmp(reinterpret_cast<const char*>(base + lookup[j] + 2), "GetStartupInfoA"))
                    slot = reinterpret_cast<DWORD*>(base + imports[i].FirstThunk + j * 4);
        }
        DWORD old{};
        if (!slot || !VirtualProtect(slot, 4, PAGE_READWRITE, &old)) return FALSE;
        *reinterpret_cast<volatile DWORD*>(slot) = *slot;
    }
    if (std::wcsstr(path, L"asi_fixture_drift")) {
        const auto proxy = GetModuleHandleW(L"saex_asi_proxy.dll");
        const auto function = reinterpret_cast<unsigned char*>(GetProcAddress(proxy, "asi_call"));
        DWORD old{};
        if (!function || !VirtualProtect(function, 22, PAGE_EXECUTE_READWRITE, &old)) return FALSE;
        function[7] = 0x91; FlushInstructionCache(GetCurrentProcess(), function, 22);
    }
    return TRUE;
}
