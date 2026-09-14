#include "loader_fixture_marker.hpp"
#include <cwchar>

extern "C" {
#if defined(SAEX_ASI)
void asi_scan();
#endif
__declspec(dllexport) FARPROC binding_slots[8]{};
__declspec(dllexport) decltype(&GetProcAddress) binding_proc_slot = &GetProcAddress;
void binding_escape() { mark_loader_phase(L".binding-after-stop"); ExitProcess(90); }
__declspec(dllexport, naked) void binding_stop() {
    __asm {
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
#if defined(SAEX_ASI)
        call asi_scan
#else
        call binding_escape
#endif
    }
}
void __cdecl binding_populate(HMODULE module) {
    mark_loader_phase(L".codec-after-return");
    wchar_t path[32768]{};
    if (!GetModuleFileNameW(nullptr, path, 32768)) ExitProcess(91);
    if (std::wcsstr(path,L"binding_fixture_fault")) DebugBreak();
    if (std::wcsstr(path,L"binding_fixture_stall")) Sleep(INFINITE);
    constexpr const char* names[]{"binding_target_0","binding_target_1","binding_target_2","binding_target_3",
        "binding_target_4","binding_target_5","binding_target_6","binding_target_7"};
    for (unsigned i=0; i<8; ++i) binding_slots[i]=binding_proc_slot(module,names[i]);
    if (std::wcsstr(path,L"binding_fixture_missing")) binding_slots[3]=binding_proc_slot(module,"missing_export");
    if (std::wcsstr(path,L"binding_fixture_wrong")) binding_slots[3]=binding_slots[2];
    if (std::wcsstr(path,L"binding_fixture_unload")) FreeLibrary(module);
    if (std::wcsstr(path,L"binding_fixture_load")) LoadLibraryA("saex_proxy_fixture_dll_normal.dll");
    if (std::wcsstr(path,L"binding_fixture_drift")) {
        DWORD old{};
        auto target=reinterpret_cast<unsigned char*>(binding_slots[0]);
        if (!target || !VirtualProtect(target,16,PAGE_EXECUTE_READWRITE,&old)) ExitProcess(92);
        target[0]^=1; FlushInstructionCache(GetCurrentProcess(),target,16);
    }
    if (std::wcsstr(path,L"binding_fixture_stop_drift")) {
        DWORD old{};
        if (!VirtualProtect(reinterpret_cast<void*>(&binding_stop),16,PAGE_EXECUTE_READWRITE,&old)) ExitProcess(93);
        reinterpret_cast<unsigned char*>(&binding_stop)[0]=0x91;
        FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(&binding_stop),16);
    }
}
}
