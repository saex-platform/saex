#include "saex/engine/platform_suppression.hpp"
#include <algorithm>

namespace saex::engine {
bool valid_platform_suppression_spec(const PlatformSuppressionSpec& s,
    const PlatformStartupSpec& p,const CrtStartupSpec& crt) noexcept {
    if (!valid_platform_startup_spec(p,crt) || s.last_error_iat_rva<4096 ||
        s.last_error_iat_rva%4 || s.last_error_iat_rva>crt.application.image_size-4 ||
        s.last_error_iat_rva==p.iat_rva ||
        (s.last_error_iat_rva<p.call_rva+22 && crt.application.target_rva<s.last_error_iat_rva+4)) return false;
    const auto& f=s.last_error_function;
    // Audited x86 GetLastError: MOV EAX,FS:[34h]; RET. No heuristic TEB layout.
    constexpr std::array prefix{std::byte{0x64},std::byte{0xa1},std::byte{0x34},std::byte{},std::byte{},std::byte{},std::byte{0xc3}};
    return f.rva && f.rva<16U*1024*1024-20 && f.preferred_base && f.preferred_base%65536==0 &&
        !f.highlow_mask && std::equal(prefix.begin(),prefix.end(),f.prefix.begin());
}
bool SuppressionTransaction::apply(SuppressionContextPort& port,const SuppressionRegisters& expected,
    std::uint32_t call_address) noexcept {
    if (write_attempted || restore_attempted || failure!="none") return false;
    if (!call_address || call_address>UINT32_MAX-6 ||
        expected.eip!=call_address || expected.esp<4096 || expected.esp%4 || expected.esp>UINT32_MAX-16 ||
        (expected.flags&0x500U) || expected.debug[0]!=call_address || expected.debug[4]!=1 ||
        (expected.debug[5]&0xffff20ffU)!=0x00d00015U) { failure="precondition"; return false; }
    SuppressionRegisters current{};
    if (!port.read(current)) { failure="initial_read"; return false; }
    if (current!=expected) { failure="stale_context"; return false; }
    original=current; replacement=current;
    replacement.eip=call_address+6; replacement.esp+=16; replacement.eax=0; // suppressed, not API success
    replacement.flags&=~0x10000U; // allow the immediate return-site execution breakpoint
    replacement.debug[0]=replacement.eip; replacement.debug[4]=0;
    write_attempted=true;
    if (!port.write(replacement)) failure="write";
    else if (!port.read(readback)) failure="readback";
    else if (readback!=replacement) failure="readback_mismatch";
    if (failure!="none") { (void)restore(port); return false; }
    applied=true; return true;
}
bool SuppressionTransaction::restore(SuppressionContextPort& port) noexcept {
    if (!write_attempted || restore_attempted) return false;
    restore_attempted=true;
    auto target=original;
    target.debug[4]=0; // Retire the debug event, never replay its DR6 cause bits.
    restored=port.write(target) && port.read(restore_readback) && restore_readback==target;
    return restored;
}
}
