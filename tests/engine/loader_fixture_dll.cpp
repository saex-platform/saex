#include "loader_fixture_marker.hpp"
extern "C" __declspec(dllexport) int loader_fixture_value() { return 73; }
BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) mark_loader_phase(L".dll-entered");
    return TRUE;
}
