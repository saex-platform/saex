#include "loader_fixture_marker.hpp"
extern "C" __declspec(dllimport) int loader_fixture_value();
void NTAPI loader_tls(PVOID, DWORD reason, PVOID) {
    if (reason == DLL_PROCESS_ATTACH) mark_loader_phase(L".tls-entered");
}
#pragma section(".CRT$XLB", long, read)
extern "C" __declspec(allocate(".CRT$XLB")) const PIMAGE_TLS_CALLBACK loader_tls_slot = loader_tls;
#pragma comment(linker, "/INCLUDE:__tls_used")
#pragma comment(linker, "/INCLUDE:_loader_tls_slot")
int main() {
    mark_loader_phase(L".main-entered");
    return loader_fixture_value();
}
