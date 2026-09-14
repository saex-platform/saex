#include "saex/engine/platform_suppression_policy.generated.hpp"
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
namespace {
unsigned checks{};
void require(bool ok,const char* reason) { ++checks; if (!ok) throw std::runtime_error(reason); }
struct Port final : SuppressionContextPort {
    SuppressionRegisters value{};
    unsigned reads{},writes{},fail_read{},fail_write{},drift_read{};
    bool read(SuppressionRegisters& r) noexcept override {
        ++reads; if (reads==fail_read) return false;
        r=value; if (reads==drift_read) ++r.ecx; return true;
    }
    bool write(const SuppressionRegisters& r) noexcept override {
        ++writes; value=r; // A failing write may already have changed state.
        return writes!=fail_write;
    }
};
SuppressionRegisters before() {
    return {0x748727,0x120000,77,0,22,33,44,55,66,0x10246,
        {0x23,0x2b,0x2b,0x2b,0x53,0}, {0x748727,0x858100,0x70001000,0,1,0x00d00015}};
}
}
int main() {
    try {
        const auto valid=[](const PlatformSuppressionSpec& s) { return valid_platform_suppression_spec(s,reviewed_platform_spec,reviewed_crt_spec); };
        require(valid(reviewed_suppression_spec),"reviewed spec rejected");
        for (const auto r: {0U,1U,reviewed_platform_spec.iat_rva,UINT32_MAX}) {
            auto s=reviewed_suppression_spec; s.last_error_iat_rva=r; require(!valid(s),"invalid IAT");
        }
        auto spec=reviewed_suppression_spec; spec.last_error_function.prefix[2]=std::byte{0x30}; require(!valid(spec),"guessed TEB accepted");
        spec=reviewed_suppression_spec; spec.last_error_function.highlow_mask=1; require(!valid(spec),"relocation accepted");
        spec=reviewed_suppression_spec; spec.last_error_function.rva=0; require(!valid(spec),"zero export");
        Port port{}; port.value=before(); SuppressionTransaction t{};
        require(t.apply(port,before(),before().eip),"apply failed");
        auto changed=before(); changed.eip+=6; changed.esp+=16; changed.eax=0; changed.flags&=~0x10000U;
        changed.debug[0]=changed.eip; changed.debug[4]=0;
        require(port.value==changed && t.applied && t.write_attempted,"unexpected register delta");
        require(!t.apply(port,changed,changed.eip) && port.writes==1,"double application");
        auto restored=before(); restored.debug[4]=0;
        require(t.restore(port) && port.value==restored,"restore mismatch");
        require(!t.restore(port) && port.writes==2,"double restore");
        for (int mode=0;mode<8;++mode) {
            Port fault{}; fault.value=before(); SuppressionTransaction x{};
            if (mode==0) fault.fail_read=1;
            if (mode==1) fault.drift_read=1;
            if (mode==2) fault.fail_write=1;
            if (mode==3) fault.fail_read=2;
            if (mode==4) fault.drift_read=2;
            if (mode==5) { fault.fail_read=2; fault.fail_write=2; }
            if (mode==6) { fault.drift_read=2; fault.fail_read=3; }
            if (mode==7) { fault.fail_write=1; fault.drift_read=2; }
            require(!x.apply(fault,before(),before().eip) && !x.applied,"fault became success");
            if (mode<2) require(!x.write_attempted && !fault.writes,"precondition wrote context");
            else require(x.restore_attempted && x.restored==(mode<5),"rollback evidence wrong");
            const auto writes=fault.writes;
            require(!x.apply(fault,before(),before().eip) && writes==fault.writes,"failed transaction retried a write");
        }
        for (int mode=0;mode<7;++mode) {
            Port p{}; auto r=before();
            if (mode==0) r.esp=UINT32_MAX-3;
            if (mode==1) r.esp++;
            if (mode==2) r.flags|=0x100;
            if (mode==3) r.debug[4]=3;
            if (mode==4) r.debug[5]=1;
            if (mode==5) r.debug[0]++;
            if (mode==6) r.eip++;
            p.value=r; SuppressionTransaction x{};
            require(!x.apply(p,r,before().eip) && !p.writes,"unsafe context accepted");
        }
        std::cout<<checks<<" suppression transaction/spec checks passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
