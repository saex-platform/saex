#include "loader_fixture_marker.hpp"
extern "C" __declspec(dllexport) int codec_leaf_value() { return 42; }
BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) mark_loader_phase(L".codec-leaf-entered");
    return TRUE;
}
