#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "saex/engine/bootstrap_api.h"
#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* why) { if (!condition) throw std::runtime_error(why); }
SaexBootstrapFunction resolve(HMODULE module, const char* name) {
    auto address = GetProcAddress(module, name);
    require(address != nullptr, "missing C export");
    return std::bit_cast<SaexBootstrapFunction>(address);
}
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    HMODULE module{};
    try {
        static_assert(sizeof(void*) == 4);
        static_assert(offsetof(SaexBootstrapStatus, observed_profile_id) == 32);
        static_assert(offsetof(SaexBootstrapStatus, profile_source_digest) == 128);
        std::array<wchar_t, 32768> path{};
        const auto n = GetFullPathNameW(argv[1], static_cast<DWORD>(path.size()), path.data(), nullptr);
        require(n && n < path.size(), "module path");
        DWORD before{}; require(GetProcessHandleCount(GetCurrentProcess(), &before) != 0, "handle baseline");
        module = LoadLibraryExW(path.data(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        require(module != nullptr, "control load");
        DWORD loaded{}; GetProcessHandleCount(GetCurrentProcess(), &loaded);
        require(FreeLibrary(module) != 0, "control release"); module = nullptr;
        DWORD control{}; GetProcessHandleCount(GetCurrentProcess(), &control);
        // Process-wide cold loader/CRT handles are not SAEX-owned handle counts.
        // Record them separately; require exact stability on every measured warm cycle.
        DWORD warm{};
        for (unsigned cycle = 0; cycle <= 100; ++cycle) {
            // Test only our explicitly provided local build artifact. No GTA deployment.
            module = LoadLibraryExW(path.data(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
            require(module != nullptr, "bootstrap module load");
            const auto initialize = resolve(module, "SaexBootstrapInitialize");
            const auto query = resolve(module, "SaexBootstrapQuery");
            const auto stop = resolve(module, "SaexBootstrapStop");
            SaexBootstrapStatus status{};
            require(query(1, &status, sizeof(status)) == SAEX_BOOTSTRAP_OK && status.state == SAEX_BOOTSTRAP_DISCOVERED &&
                status.observation_attempts == 0, "load performed host inspection");
            status.reserved = 77; const auto unchanged = status;
            require(initialize(2, &status, sizeof(status)) == SAEX_BOOTSTRAP_ABI_MISMATCH &&
                std::memcmp(&unchanged, &status, sizeof(status)) == 0, "ABI mismatch side effects");
            require(initialize(1, &status, sizeof(status) - 1) == SAEX_BOOTSTRAP_INVALID_ARGUMENT, "short ABI output");
            require(query(1, nullptr, sizeof(status)) == SAEX_BOOTSTRAP_INVALID_ARGUMENT, "null ABI output");
            require(initialize(1, &status, sizeof(status)) == SAEX_BOOTSTRAP_OK && status.state == SAEX_BOOTSTRAP_REJECTED &&
                status.reason == SAEX_BOOTSTRAP_FILE_REJECTED && status.observation_attempts == 1 &&
                !status.can_attach && !status.bindings_loaded && !status.observed_profile_id[0], "non-GTA host approved");
            require(initialize(1, &status, sizeof(status)) == SAEX_BOOTSTRAP_OK && status.observation_attempts == 1, "repeated initialize");
            require(stop(1, &status, sizeof(status)) == SAEX_BOOTSTRAP_OK && status.state == SAEX_BOOTSTRAP_STOPPED, "module stop");
            require(initialize(1, &status, sizeof(status)) == SAEX_BOOTSTRAP_OK && status.state == SAEX_BOOTSTRAP_STOPPED, "module restart");
            require(FreeLibrary(module) != 0, "module release"); module = nullptr;
            require(GetModuleHandleW(path.data()) == nullptr, "module remains loaded");
            DWORD handles{}; require(GetProcessHandleCount(GetCurrentProcess(), &handles) != 0, "cycle handle count");
            if (cycle == 0) warm = handles;
            else require(handles == warm, "process handles changed across warm cycles");
        }
        DWORD after{}; require(GetProcessHandleCount(GetCurrentProcess(), &after) != 0 && after == warm, "warm handle count changed");
        std::cout << "{\"scope\":\"non-game-bootstrap-module\",\"loadStopUnloadCycles\":100,\"wrongHostRejected\":true,"
            "\"canAttach\":false,\"coldHandles\":" << before << ",\"loadOnlyHandles\":" << loaded
            << ",\"loadOnlyReleasedHandles\":" << control << ",\"warmHandles\":" << warm << ",\"finalHandles\":" << after << "}\n";
        return 0;
    } catch (const std::exception& e) {
        if (module) FreeLibrary(module);
        std::cerr << e.what() << '\n'; return 1;
    }
}
