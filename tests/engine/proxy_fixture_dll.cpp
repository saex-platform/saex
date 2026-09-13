#include "loader_fixture_marker.hpp"
#include <cstring>

extern "C" {
__declspec(dllexport) unsigned char* proxy_original_entry{};
__declspec(dllexport) void __cdecl proxy_restore();
__declspec(dllexport) void __cdecl proxy_iat_target() {
    mark_loader_phase(L".startup-entered");
    ExitProcess(90); // Test escape canary; never masquerades as GetStartupInfoA.
}
__declspec(dllexport, naked) void proxy_thunk() {
    __asm {
        call proxy_restore
        jmp dword ptr [proxy_original_entry]
    }
}
}
namespace {
unsigned char saved[5]{};
DWORD* startup_slot{};
}
extern "C" void __cdecl proxy_restore() {
    mark_loader_phase(L".proxy-entered");
#if defined(SAEX_PROXY_fault)
    DebugBreak();
#elif defined(SAEX_PROXY_stall)
    Sleep(INFINITE);
#endif
#if !defined(SAEX_PROXY_no_restore)
    std::memcpy(proxy_original_entry, saved, 5);
    FlushInstructionCache(GetCurrentProcess(), proxy_original_entry, 5);
#endif
#if !defined(SAEX_PROXY_bad_iat)
    DWORD previous{};
    if (!VirtualProtect(startup_slot, 4, PAGE_READWRITE, &previous)) ExitProcess(87);
    *startup_slot = reinterpret_cast<DWORD>(&proxy_iat_target);
    DWORD ignored{};
    if (!VirtualProtect(startup_slot, 4, previous, &ignored)) ExitProcess(88);
#endif
#if defined(SAEX_PROXY_bad_return)
    ++proxy_original_entry;
#endif
}
BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    mark_loader_phase(L".dll-entered");
    const auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
    // Trusted own fixture image only; this is not a platform PE parser or loader.
    const auto imports = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(base + nt->OptionalHeader.DataDirectory[1].VirtualAddress);
    for (unsigned i = 0; imports[i].Name && i < 64; ++i) {
        if (_stricmp(reinterpret_cast<const char*>(base + imports[i].Name), "kernel32.dll")) continue;
        const auto lookup = reinterpret_cast<const DWORD*>(base + imports[i].OriginalFirstThunk);
        for (unsigned j = 0; lookup[j] && j < 4096; ++j) {
            if (!(lookup[j] & IMAGE_ORDINAL_FLAG32) &&
                !std::strcmp(reinterpret_cast<const char*>(base + lookup[j] + 2), "GetStartupInfoA"))
                startup_slot = reinterpret_cast<DWORD*>(base + imports[i].FirstThunk + j * 4);
        }
    }
    if (!startup_slot) ExitProcess(86);
    proxy_original_entry = base + nt->OptionalHeader.AddressOfEntryPoint;
    std::memcpy(saved, proxy_original_entry, 5);
    DWORD previous{};
    if (!VirtualProtect(proxy_original_entry, 5, PAGE_EXECUTE_READWRITE, &previous)) ExitProcess(89);
    proxy_original_entry[0] = 0xe9;
    DWORD relative = reinterpret_cast<DWORD>(&proxy_thunk) - reinterpret_cast<DWORD>(proxy_original_entry) - 5;
#if defined(SAEX_PROXY_bad_redirect)
    ++relative;
#endif
    std::memcpy(proxy_original_entry + 1, &relative, 4);
#if defined(SAEX_PROXY_bad_suffix)
    proxy_original_entry[5] ^= 1;
#endif
    FlushInstructionCache(GetCurrentProcess(), proxy_original_entry, 5);
    return TRUE;
}
