#include "loader_fixture_marker.hpp"
extern "C" __declspec(dllimport) void __cdecl proxy_iat_target();
extern "C" {
#define PREFIX __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop \
    __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop
__declspec(dllexport,align(4096)) unsigned char startup_sample[4096]{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
__declspec(dllexport,naked) void frame_target() { PREFIX __asm ret }
__declspec(dllexport,naked) void frame_caller() { __asm call frame_target __asm ret }
__declspec(dllexport,naked) void crt_application_target() { PREFIX __asm int 3 }
__declspec(dllexport,naked) void crt_application_call() { __asm call crt_application_target __asm ret }
__declspec(dllexport) void (*crt_table[3])(){nullptr,&frame_target,&crt_application_target};
void crt_after_startup() {
    mark_loader_phase(L".crt-after-startup");
#if defined(SAEX_CRT_crash)
    DebugBreak();
#elif defined(SAEX_CRT_hang)
    Sleep(INFINITE);
#endif
}
__declspec(dllexport,naked) void crt_io_target() {
    __asm {
        sub esp,48h
        push ebx
        push ebp
        push esi
        push edi
        lea eax,[esp+10h]
        push eax
        call GetStartupInfoA
        call crt_after_startup
        xor eax,eax
    }
#if defined(SAEX_CRT_io_error)
    __asm dec eax
#elif defined(SAEX_CRT_saved_register)
    __asm inc dword ptr [esp+0ch]
#endif
    __asm {
        pop edi
        pop esi
        pop ebp
        pop ebx
        add esp,48h
        ret
    }
}
void crt_between() {
    mark_loader_phase(L".crt-io-returned");
#if defined(SAEX_CRT_table_change)
    crt_table[0]=&frame_target;
#elif defined(SAEX_CRT_frame_change)
    auto p=reinterpret_cast<unsigned char*>(&frame_target);
    p[15]^=1; FlushInstructionCache(GetCurrentProcess(),p,16);
#endif
}
void crt_initializer_canary() { mark_loader_phase(L".crt-initializer-entered"); ExitProcess(96); }
__declspec(dllexport,naked) void crt_initialize_target() { PREFIX __asm jmp crt_initializer_canary }
__declspec(dllexport,naked) void crt_initialize_call() { __asm call crt_initialize_target __asm int 3 }
__declspec(dllexport,naked) void crt_io_call() {
    __asm call crt_io_target
    PREFIX
    __asm call crt_between
    __asm jmp crt_initialize_call
}
#undef PREFIX
}
int main() {
    mark_loader_phase(L".main-entered");
    crt_io_call();
    proxy_iat_target();
    return 95;
}
