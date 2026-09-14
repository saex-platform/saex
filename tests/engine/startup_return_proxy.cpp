#include "loader_fixture_marker.hpp"
#include <cstring>
extern "C" {
__declspec(dllexport) char codec_name[]="saex_codec";
__declspec(dllexport) FARPROC binding_slots[8]{};
__declspec(dllexport) decltype(&GetProcAddress) binding_proc_slot=&GetProcAddress;
__declspec(dllexport) decltype(&GetStartupInfoA) original_startup=&GetStartupInfoA;
DWORD saved_stack{}, bad_register{};
void __cdecl binding_populate(HMODULE m) {
    constexpr const char* names[]{"binding_target_0","binding_target_1","binding_target_2","binding_target_3",
        "binding_target_4","binding_target_5","binding_target_6","binding_target_7"};
    for (unsigned i=0;i<8;++i) binding_slots[i]=binding_proc_slot(m,names[i]);
}
#define PREFIX __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop \
    __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop
#if defined(SAEX_APPLICATION_FIXTURE)
void platform_api_canary() { mark_loader_phase(L".platform-api-called"); ExitProcess(96); }
__declspec(dllexport,naked) void platform_system_canary() {
    PREFIX
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm jmp platform_api_canary
}
#endif
__declspec(dllexport,naked) void startup_forward() { __asm jmp dword ptr [original_startup] }
__declspec(naked) void finish_startup() {
    __asm {
        mov esp,saved_stack
        pop edi
        pop esi
        pop ebx
        pop ebp
        cmp bad_register,0
        je intact
        inc esi
    intact:
#if defined(SAEX_APPLICATION_FIXTURE)
        ret
#else
        jmp startup_forward
#endif
    }
}
__declspec(dllexport,naked) void protect_call() { __asm call VirtualProtect PREFIX __asm ret }
__declspec(naked) void __cdecl invoke_protect(void*,DWORD,DWORD,DWORD*) {
    __asm {
        mov eax,[esp+4]
        mov ecx,[esp+8]
        mov edx,[esp+12]
        push dword ptr [esp+16]
        push edx
        push ecx
        push eax
        jmp protect_call
    }
}
__declspec(dllexport,naked) void asi_end() { PREFIX __asm int 3 }
__declspec(dllexport,naked) void asi_call() { __asm call LoadLibraryA PREFIX __asm ret }
__declspec(naked) void __cdecl invoke_asi(const char*) {
    __asm {
        mov eax,[esp+4]
        push eax
        jmp asi_call
    }
}
void run_tail() {
    char path[260]{}; if (!GetModuleFileNameA(nullptr,path,260)) ExitProcess(80);
#if defined(SAEX_APPLICATION_FIXTURE)
    // Application faults are selected in the EXE after the CRT barrier.
    const auto variant=[](const char*) { return false; };
#else
    const auto variant=[&](const char* name) { return std::strstr(path,name)!=nullptr; };
#endif
    const bool extra=variant("fixture_extra"),fault=variant("fixture_fault"),stall=variant("fixture_stall"),
        wrong_size=variant("fixture_size"),wrong_flags=variant("fixture_flags"),wrong_pointer=variant("fixture_pointer"),
        drift=variant("fixture_frame_drift");
    bad_register=variant("fixture_register");
    auto name=std::strrchr(path,'\\'); if (!name) ExitProcess(81);
    strcpy_s(name+1,sizeof(path)-static_cast<std::size_t>(name+1-path),"saex_asi.asi");
    invoke_asi(path);
    mark_loader_phase(L".tail-after-asi");
    if (extra) invoke_asi(path);
    if (fault) DebugBreak();
    if (stall) Sleep(INFINITE);
    const auto base=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    if (drift) {
        auto target=reinterpret_cast<unsigned char*>(GetProcAddress(GetModuleHandleW(nullptr),"frame_target"));
        DWORD old{}; if (!target || !VirtualProtect(target,16,PAGE_EXECUTE_READWRITE,&old)) ExitProcess(82);
        target[15]^=1; FlushInstructionCache(GetCurrentProcess(),target,16);
    }
    const auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    const auto nt=reinterpret_cast<IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);
    DWORD old{};
    invoke_protect(base,nt->OptionalHeader.SizeOfImage-(wrong_size?4096:0),
        wrong_flags?PAGE_READWRITE:PAGE_EXECUTE_READWRITE,wrong_pointer?nullptr:&old);
    mark_loader_phase(L".tail-after-protect");
}
__declspec(dllexport,naked) void binding_stop() { PREFIX __asm call run_tail __asm jmp finish_startup }
__declspec(dllexport,naked) void codec_load_sequence() {
    __asm {
        push offset codec_name
        call LoadLibraryA
        mov edi,eax
        push eax
        call binding_populate
        add esp,4
        jmp binding_stop
    }
}
#if defined(SAEX_APPLICATION_FIXTURE)
#define FIRST_LOAD application_first_load
__declspec(dllexport) unsigned char application_once{};
#else
#define FIRST_LOAD proxy_iat_target
#endif
__declspec(dllexport,naked) void FIRST_LOAD() {
    __asm {
        push ebp
        push ebx
        push esi
        push edi
        mov saved_stack,esp
        jmp codec_load_sequence
    }
}
#if defined(SAEX_APPLICATION_FIXTURE)
__declspec(dllexport,naked) void proxy_iat_target() {
    __asm {
        push ebp
        mov ebp,esp
        cmp byte ptr [application_once],0
        jne ready
        call application_first_load
        mov byte ptr [application_once],1
    ready:
        pop ebp
        jmp dword ptr [original_startup]
    }
}
#endif
#undef FIRST_LOAD
#undef PREFIX
}
