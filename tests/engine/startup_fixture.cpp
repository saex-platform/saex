#include "loader_fixture_marker.hpp"
#include <cstring>
extern "C" __declspec(dllimport) void __cdecl proxy_iat_target();
extern "C" __declspec(dllexport, align(4096)) unsigned char startup_sample[4096]{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
#if defined(SAEX_FRAME_FIXTURE)
extern "C" __declspec(dllexport, naked) void frame_target() {
    __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop
    __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop
    __asm ret
}
extern "C" __declspec(dllexport, naked) void frame_caller() { __asm call frame_target __asm ret }
#endif

int main() {
    mark_loader_phase(L".main-entered");
    STARTUPINFOA info{};
#if defined(SAEX_FRAME_ASI_DRIFT)
    DWORD old{};
    auto target = reinterpret_cast<unsigned char*>(&frame_target);
    if (!VirtualProtect(target,16,PAGE_EXECUTE_READWRITE,&old)) ExitProcess(98);
    target[15] ^= 1;
    FlushInstructionCache(GetCurrentProcess(),target,16);
#endif
#if defined(SAEX_STARTUP_preload)
    if (!LoadLibraryA("saex_codec")) ExitProcess(97);
#endif
#if defined(SAEX_STARTUP_fault)
    DebugBreak();
#elif defined(SAEX_STARTUP_stall)
    Sleep(INFINITE);
#elif defined(SAEX_STARTUP_sample_change)
    startup_sample[0] ^= 1;
#elif defined(SAEX_STARTUP_sample_unreadable)
    DWORD previous{};
    if (!VirtualProtect(startup_sample, 4096, PAGE_NOACCESS, &previous)) ExitProcess(91);
#elif defined(SAEX_STARTUP_target_drift)
    const auto module = GetModuleHandleW(L"saex_proxy_fixture_dll_normal.dll");
    const auto target = GetProcAddress(module, "proxy_iat_target");
    if (!target) ExitProcess(92);
    DWORD previous{};
    if (!VirtualProtect(reinterpret_cast<void*>(target), 16, PAGE_EXECUTE_READWRITE, &previous)) ExitProcess(93);
    reinterpret_cast<unsigned char*>(target)[0] ^= 1;
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(target), 16);
#elif defined(SAEX_STARTUP_iat_write)
    const auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
    const auto imports = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(base + nt->OptionalHeader.DataDirectory[1].VirtualAddress);
    DWORD* slot{};
    for (unsigned i = 0; i < 64 && imports[i].Name; ++i) {
        if (_stricmp(reinterpret_cast<const char*>(base + imports[i].Name), "kernel32.dll")) continue;
        const auto lookup = reinterpret_cast<const DWORD*>(base + imports[i].OriginalFirstThunk);
        for (unsigned j = 0; j < 4096 && lookup[j]; ++j) {
            if (!(lookup[j] & IMAGE_ORDINAL_FLAG32) && !std::strcmp(reinterpret_cast<const char*>(base + lookup[j] + 2), "GetStartupInfoA"))
                slot = reinterpret_cast<DWORD*>(base + imports[i].FirstThunk + j * 4);
        }
    }
    DWORD previous{};
    if (!slot || !VirtualProtect(slot, 4, PAGE_READWRITE, &previous)) ExitProcess(94);
    // Writing the SAME value must still trip the main-thread DR1 watch.
    *reinterpret_cast<volatile DWORD*>(slot) = *slot;
#endif
#if defined(SAEX_STARTUP_bad_arg)
    GetStartupInfoA(nullptr);
#elif defined(SAEX_STARTUP_call_register)
    auto volatile function = &GetStartupInfoA;
    function(&info);
#else
    GetStartupInfoA(&info);
#endif
    mark_loader_phase(L".startup-after-call");
    proxy_iat_target(); // Keep the fixture DLL statically imported; must never reach here.
    return 95;
}
