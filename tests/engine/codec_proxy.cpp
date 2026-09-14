#include "loader_fixture_marker.hpp"
#include <cstring>

extern "C" {
#if defined(SAEX_BINDINGS)
void __cdecl binding_populate(HMODULE module);
void binding_stop();
#endif
__declspec(dllexport) char codec_name[] = "saex_codec";
void __cdecl codec_before_load() { mark_loader_phase(L".startup-entered"); }
void __cdecl codec_after_load() {
    mark_loader_phase(L".codec-after-return");
    ExitProcess(90); // Escape canary, not a GetStartupInfoA implementation.
}
__declspec(dllexport, naked) void codec_load_sequence() {
    __asm {
        push offset codec_name
        call LoadLibraryA
        mov edi, eax
#if defined(SAEX_BINDINGS)
        push eax
        call binding_populate
        add esp, 4
        jmp binding_stop
#else
        call codec_after_load
#endif
    }
}
__declspec(dllexport, naked) void __cdecl proxy_iat_target() {
    __asm {
        call codec_before_load
        jmp codec_load_sequence
    }
}
}
