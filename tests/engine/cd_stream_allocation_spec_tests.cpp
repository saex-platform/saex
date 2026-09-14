#include "saex/engine/cd_stream_allocation_policy.generated.hpp"
#include <bit>
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
namespace { unsigned checks{};void check(bool v){++checks;if(!v)throw std::runtime_error("allocation contract check "+std::to_string(checks));} }
int main() {
    try {
        const auto& s=reviewed_cd_stream_allocation_spec;const auto& d=reviewed_cd_stream_disk_spec;
        const auto& m=reviewed_file_manager_spec;const auto& seh=reviewed_cwd_seh_spec;const auto& ready=reviewed_file_manager_ready_spec;
        CdStreamAllocationCode code{};check(cd_stream_allocation_code(s,d,m,seh,ready,code));
        for(auto member:{&CdStreamAllocationSpec::malloc_rva,&CdStreamAllocationSpec::nh_rva,&CdStreamAllocationSpec::heap_rva,
            &CdStreamAllocationSpec::scope_rva,&CdStreamAllocationSpec::new_mode_rva,&CdStreamAllocationSpec::threshold_rva,
            &CdStreamAllocationSpec::heap_handle_rva,&CdStreamAllocationSpec::heap_mode_rva,&CdStreamAllocationSpec::iat_rva,&CdStreamAllocationSpec::scope_cleanup_rva})
            for(auto bad:{0U,1U,UINT32_MAX}){auto v=s;v.*member=bad;check(!cd_stream_allocation_code(v,d,m,seh,ready,code));}
        auto overlap=s;overlap.new_mode_rva=s.heap_handle_rva;check(!cd_stream_allocation_code(overlap,d,m,seh,ready,code));
        auto invalid=m;invalid.manager.image_base=UINT32_MAX;check(!cd_stream_allocation_code(s,d,invalid,seh,ready,code));
        for(auto a:{512U,1024U,2048U,4096U,8192U,16384U,32768U,65536U}) {
            check(cd_stream_allocation_request(a,{0,0,0x200000,1}));
            check(cd_stream_allocation_request(a,{0,2048+a-1,0x200000,3}));
            check(!cd_stream_allocation_request(a,{0,2048+a,0x200000,3}));
            check(!cd_stream_allocation_request(a,{1,0,0x200000,1}));
            for(auto mode:{0U,2U,4U,UINT32_MAX})check(!cd_stream_allocation_request(a,{0,0,0x200000,mode}));
            for(auto heap:{0U,8U,0x200001U,0xffff0000U})check(!cd_stream_allocation_request(a,{0,0,heap,1}));
            // Every 8-byte heap residue within each supported sector size.
            for(unsigned offset=0;offset<a;offset+=8) {
                const auto raw=0x200000+offset;CdStreamAllocationLayout b{};
                check(cd_stream_allocation_layout(raw,a,b));
                check(b.raw==raw && b.aligned%a==0 && b.aligned>raw && b.aligned<=raw+a &&
                    b.metadata==b.aligned-4 && b.metadata>=raw && b.aligned+2048<=raw+b.bytes && b.bytes==2048+a);
            }
            for(auto raw:{0U,1U,65528U,0x200001U,0xfffffff8U}) {
                CdStreamAllocationLayout b{1,2,3,4};check(!cd_stream_allocation_layout(raw,a,b));
                check(!b.raw && !b.aligned && !b.metadata && !b.bytes);
            }
        }
        for(auto a:{0U,1U,256U,511U,513U,65537U,UINT32_MAX}) {
            CdStreamAllocationLayout b{};check(!cd_stream_allocation_layout(0x200000,a,b));check(!cd_stream_allocation_request(a,{0,0,0x200000,1}));
        }
        EventDispatchRegisters c{};c.esp=0x110000;c.eax=0x60000000;c.ebx=2;c.ecx=512;c.edx=4;c.esi=5;c.edi=6;c.ebp=7;c.flags=0x206;
        CdStreamAllocationLayout b{};check(cd_stream_allocation_layout(0x200008,512,b));
        auto returned=c;returned.edx=0x3344;returned.flags=0x202;
        constexpr unsigned offsets[]{0,4,36,80,80,92,96,80,12,8,0};
        for(unsigned stage=1;stage<=10;++stage) {
            auto r=c;r.esp-=offsets[stage];
            if(stage==2){r.eax=2560;r.esi=512;}
            if(stage>=3 && stage<=7){r.ebp=c.esp-40;r.esi=2560;r.eax=c.esp-56;}
            if(stage>=8){r.edx=returned.edx;r.eax=stage==8?b.raw:b.aligned;r.ecx=stage==8?0:b.raw;
                r.esi=stage==8?512:stage==9?~511U:c.esi;r.flags=0x202|(std::popcount(r.eax&255U)%2==0?4U:0U);}
            check(cd_stream_allocation_frame(stage,c,r,512,b,returned));
            for(auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::ebx,&EventDispatchRegisters::edi,&EventDispatchRegisters::esi,&EventDispatchRegisters::ebp}) {
                auto bad=r;(bad.*member)^=4;check(!cd_stream_allocation_frame(stage,c,bad,512,b,returned));
            }
            for(auto flag:{0x100U,0x400U}){auto bad=r;bad.flags|=flag;check(!cd_stream_allocation_frame(stage,c,bad,512,b,returned));}
            if(stage!=7)for(auto member:{&EventDispatchRegisters::eax,&EventDispatchRegisters::ecx,&EventDispatchRegisters::edx}) {
                auto bad=r;(bad.*member)^=4;check(!cd_stream_allocation_frame(stage,c,bad,512,b,returned));
            }
            if(stage>=8){auto bad=r;bad.flags^=0x40;check(!cd_stream_allocation_frame(stage,c,bad,512,b,returned));check(!cd_stream_allocation_frame(stage,c,r,512,{},returned));}
        }
        for(auto stage:{0U,11U,UINT32_MAX})check(!cd_stream_allocation_frame(stage,c,c,512,b,returned));
        std::cout<<"PASS cd stream allocation: "<<checks<<" portable checks\n";return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
