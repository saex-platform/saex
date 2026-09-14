#include "loader_fixture_marker.hpp"
#include <cwchar>
#include <cstring>
extern "C" __declspec(dllimport) int codec_leaf_value();
#define SAEX_BINDING_TARGET(n) extern "C" __declspec(dllexport) int binding_target_##n() { mark_loader_phase(L".binding-function-called"); ExitProcess(100 + n); }
SAEX_BINDING_TARGET(0)
SAEX_BINDING_TARGET(1)
SAEX_BINDING_TARGET(2)
SAEX_BINDING_TARGET(3)
SAEX_BINDING_TARGET(4)
SAEX_BINDING_TARGET(5)
SAEX_BINDING_TARGET(6)
SAEX_BINDING_TARGET(7)
#undef SAEX_BINDING_TARGET
BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    mark_loader_phase(L".codec-dll-entered");
    if (codec_leaf_value() != 42) return FALSE;
    wchar_t path[32768]{};
    if (!GetModuleFileNameW(nullptr, path, 32768)) return FALSE;
    if (std::wcsstr(path, L"codec_fixture_fail")) return FALSE;
    if (std::wcsstr(path, L"codec_fixture_fault")) DebugBreak();
    if (std::wcsstr(path, L"codec_fixture_stall")) Sleep(INFINITE);
    if (std::wcsstr(path, L"codec_fixture_iat_write")) {
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
    if (std::wcsstr(path, L"codec_fixture_drift")) {
        auto target = reinterpret_cast<unsigned char*>(GetProcAddress(GetModuleHandleW(L"saex_codec_proxy.dll"), "codec_load_sequence"));
        DWORD old{};
        if (!target || !VirtualProtect(target, 13, PAGE_EXECUTE_READWRITE, &old)) return FALSE;
        target[12] ^= 1;
        FlushInstructionCache(GetCurrentProcess(), target, 13);
    }
    return TRUE;
}
