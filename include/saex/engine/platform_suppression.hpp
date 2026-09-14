#pragma once
#include "saex/engine/platform_startup.hpp"

namespace saex::engine {
// A local debugger experiment, never a server-supplied patch or Windows API emulator.
struct PlatformSuppressionSpec {
    std::uint32_t last_error_iat_rva{};
    BootstrapExportSpec last_error_function{};
};
[[nodiscard]] bool valid_platform_suppression_spec(const PlatformSuppressionSpec&,
    const PlatformStartupSpec&, const CrtStartupSpec&) noexcept;

// Only these context groups may be written. Segment selectors are read for identity.
// DR6 is represented with its event bits only; restore clears those cause bits.
// RF is explicit; the Windows port normalizes architecturally fixed EFLAGS bit 1.
struct SuppressionRegisters {
    std::uint32_t eip{}, esp{}, eax{}, ebx{}, ecx{}, edx{}, esi{}, edi{}, ebp{}, flags{};
    std::array<std::uint32_t,6> segments{}; // CS, SS, DS, ES, FS, GS
    std::array<std::uint32_t,6> debug{}; // DR0, DR1, DR2, DR3, DR6, DR7
    bool operator==(const SuppressionRegisters&) const = default;
};
class SuppressionContextPort {
protected:
    // Borrowed stack adapter; never delete through this interface. A trivial
    // destructor also keeps MSVC /Od's large trace return path free of EH copies.
    ~SuppressionContextPort() = default;
public:
    virtual bool read(SuppressionRegisters&) noexcept = 0;
    virtual bool write(const SuppressionRegisters&) noexcept = 0;
};
struct SuppressionTransaction {
    SuppressionRegisters original{}, replacement{}, readback{}, restore_readback{};
    std::string_view failure{"none"};
    bool write_attempted{}, applied{}, restore_attempted{}, restored{};
    // Caller must hold the owned main thread at its verified CALL debug event.
    bool apply(SuppressionContextPort&,const SuppressionRegisters&,std::uint32_t call_address) noexcept;
    bool restore(SuppressionContextPort&) noexcept;
};
struct PlatformSuppressionObservation {
    SuppressionTransaction transaction{};
    std::uint32_t return_address{}, teb_address{}, last_error_before{}, last_error_after{};
    bool continued{}, return_reached{}, last_error_read{}, last_error_preserved{}, stack_preserved{}, verified{};
};
}
