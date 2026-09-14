#include "loader_fixture_marker.hpp"
#include <cstring>

extern "C" {
void asi_escape() { mark_loader_phase(L".asi-after-return"); ExitProcess(90); }
void asi_end_escape() { mark_loader_phase(L".asi-end-escaped"); ExitProcess(91); }
__declspec(dllexport, naked) void asi_end() {
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
        call asi_end_escape
    }
}
__declspec(dllexport, naked) void asi_call() {
    __asm {
        call LoadLibraryA
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
        call asi_escape
    }
}
void asi_scan() {
    char path[260]{};
    const auto length = GetModuleFileNameA(nullptr, path, 260);
    if (!length || length >= 240) ExitProcess(92);
    if (std::strstr(path, "asi_fixture_empty")) { __asm { jmp asi_end } }
    if (std::strstr(path, "asi_fixture_scan_fault")) DebugBreak();
    if (std::strstr(path, "asi_fixture_scan_stall")) Sleep(INFINITE);
    const bool wrong = std::strstr(path, "asi_fixture_path") != nullptr;
    const bool unreadable = std::strstr(path, "asi_fixture_pointer") != nullptr;
    const bool no_null = std::strstr(path, "asi_fixture_unterminated") != nullptr;
    auto name = std::strrchr(path, '\\'); if (!name) ExitProcess(93);
    const bool real_bootstrap = std::strstr(path, "bootstrap_fixture_real") != nullptr;
    strcpy_s(name + 1, sizeof(path) - static_cast<std::size_t>(name + 1 - path),
        real_bootstrap ? "saex_bootstrap.dll" : wrong ? "other_asi.asi" : "saex_asi.asi");
    if (no_null) path[std::strlen(path)] = 'x';
    if (unreadable) {
        __asm {
            push 1
            jmp asi_call
        }
    }
    __asm {
        lea eax, path
        push eax
        jmp asi_call
    }
}
}
