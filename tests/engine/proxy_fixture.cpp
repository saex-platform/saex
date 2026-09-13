#include "loader_fixture_marker.hpp"
extern "C" __declspec(dllimport) void __cdecl proxy_iat_target();
int main() {
    mark_loader_phase(L".main-entered");
    STARTUPINFOA info{};
    GetStartupInfoA(&info); // Retain the real import slot for the owned DLL fixture.
    proxy_iat_target();
    return 73;
}
