#include "loader_fixture_marker.hpp"
extern "C" __declspec(dllimport) int loader_fixture_value();
void NTAPI entry_tls(PVOID, DWORD reason, PVOID) {
    if (reason != DLL_PROCESS_ATTACH) return;
    mark_loader_phase(L".tls-entered");
#if defined(SAEX_ENTRY_mutate)
    // Owned fixture only: mutate its PE entry before the observer's hardware hit.
    const auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
    const auto entry = base + nt->OptionalHeader.AddressOfEntryPoint;
    DWORD previous{};
    if (!VirtualProtect(entry, 1, PAGE_EXECUTE_READWRITE, &previous)) ExitProcess(84);
    *entry = 0xcc; // Must never execute: DR0 is an instruction fault, before this byte.
    FlushInstructionCache(GetCurrentProcess(), entry, 1);
    DWORD ignored{}; if (!VirtualProtect(entry, 1, previous, &ignored)) ExitProcess(85);
#elif defined(SAEX_ENTRY_fault)
    DebugBreak();
#elif defined(SAEX_ENTRY_stall)
    Sleep(INFINITE);
#endif
}
#pragma section(".CRT$XLB", long, read)
extern "C" __declspec(allocate(".CRT$XLB")) const PIMAGE_TLS_CALLBACK entry_tls_slot = entry_tls;
#pragma comment(linker, "/INCLUDE:__tls_used")
#pragma comment(linker, "/INCLUDE:_entry_tls_slot")
int main() {
    mark_loader_phase(L".main-entered");
    return loader_fixture_value();
}
