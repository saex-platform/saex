#include "loader_fixture_marker.hpp"
#include "saex/engine/bootstrap_api.h"
#include "saex/engine/observed_profile.generated.hpp"
#include <algorithm>
#include <cstring>

namespace {
constinit SaexBootstrapStatus value{};
unsigned calls{};
std::uint32_t invoke(unsigned function, std::uint32_t abi, SaexBootstrapStatus* output, std::uint32_t bytes) {
    if (abi != 1 || !output || bytes != sizeof(value)) ExitProcess(97);
    ++calls;
    char path[32768]{}; GetModuleFileNameA(nullptr, path, 32768);
    const auto variant = [&](const char* name) { return std::strstr(path, name) != nullptr; };
    if (calls == 1 && variant("bootstrap_fixture_frame_terminal")) {
        auto target = reinterpret_cast<unsigned char*>(GetProcAddress(GetModuleHandleW(nullptr),"frame_target"));
        DWORD old{};
        if (!target || !VirtualProtect(target,16,PAGE_EXECUTE_READWRITE,&old)) ExitProcess(98);
        target[15] ^= 1;
        FlushInstructionCache(GetCurrentProcess(),target,16);
    }
    if (variant("bootstrap_fixture_fault")) DebugBreak();
    if (variant("bootstrap_fixture_stall")) Sleep(INFINITE);
    value.abi_major = 1; value.struct_bytes = sizeof(value);
    const auto digest = saex::engine::observed_profile_source_digest;
    std::copy(digest.begin(), digest.end(), value.profile_source_digest);
    if (function == 0 && value.state == SAEX_BOOTSTRAP_DISCOVERED) {
        value.state = SAEX_BOOTSTRAP_OBSERVED_UNVERIFIED; value.reason = SAEX_BOOTSTRAP_RUNTIME_UNVERIFIED;
        value.observation_attempts = 1;
        const auto id = saex::engine::observed_profile.id;
        std::copy(id.begin(), id.end(), value.observed_profile_id);
        if (variant("bootstrap_fixture_reject")) {
            value.state = SAEX_BOOTSTRAP_REJECTED; value.reason = SAEX_BOOTSTRAP_FILE_REJECTED;
            std::memset(value.observed_profile_id, 0, sizeof(value.observed_profile_id));
        }
    }
    if (function == 2) value.state = SAEX_BOOTSTRAP_STOPPED;
    *output = value;
    if (variant("bootstrap_fixture_abi")) output->abi_major = 2;
    if (variant("bootstrap_fixture_digest")) output->profile_source_digest[63] ^= 1;
    if (variant("bootstrap_fixture_attach")) output->can_attach = 1;
    if (variant("bootstrap_fixture_trailing")) output->observed_profile_id[95] = 'x';
    if (calls == 4 && variant("bootstrap_fixture_repeat")) ++output->observation_attempts;
    if (calls == 7 && variant("bootstrap_fixture_terminal")) output->state = SAEX_BOOTSTRAP_OBSERVED_UNVERIFIED;
    if (variant("bootstrap_fixture_overrun")) reinterpret_cast<unsigned char*>(output)[sizeof(value)] = 0;
    return variant("bootstrap_fixture_busy") ? SAEX_BOOTSTRAP_BUSY : SAEX_BOOTSTRAP_OK;
}
}
extern "C" {
__declspec(dllexport) void FixtureStop();
std::uint32_t __cdecl fixture_drift() {
    auto pointer = reinterpret_cast<unsigned char*>(&FixtureStop);
    DWORD old{};
    if (!VirtualProtect(pointer,16,PAGE_EXECUTE_READWRITE,&old)) ExitProcess(98);
    pointer[0] ^= 1;
    FlushInstructionCache(GetCurrentProcess(),pointer,16);
    return 0;
}
std::uint32_t __cdecl fixture_initialize(std::uint32_t abi, SaexBootstrapStatus* output, std::uint32_t bytes) { return invoke(0,abi,output,bytes); }
std::uint32_t __cdecl fixture_query(std::uint32_t abi, SaexBootstrapStatus* output, std::uint32_t bytes) { return invoke(1,abi,output,bytes); }
std::uint32_t __cdecl fixture_stop(std::uint32_t abi, SaexBootstrapStatus* output, std::uint32_t bytes) { return invoke(2,abi,output,bytes); }
// Stable, relocation-free prefixes are metadata for our owned fixture, not GTA recipes.
#define PREFIX __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop \
    __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop
__declspec(dllexport,naked) void FixtureInitialize() { PREFIX __asm jmp fixture_initialize }
__declspec(dllexport,naked) void FixtureQuery() { PREFIX __asm jmp fixture_query }
__declspec(dllexport,naked) void FixtureStop() { PREFIX __asm jmp fixture_stop }
__declspec(dllexport,naked) void FixtureBadStack() { PREFIX __asm { xor eax,eax } __asm { ret 12 } }
__declspec(dllexport,naked) void FixtureBadRegister() { PREFIX __asm { inc ebx } __asm { ret } }
__declspec(dllexport,naked) void FixtureDrift() { PREFIX __asm jmp fixture_drift }
#undef PREFIX
}
