#include "saex/engine/cd_stream_disk_policy.generated.hpp"
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
namespace { unsigned checks{};void check(bool v){++checks;if(!v)throw std::runtime_error("disk contract check "+std::to_string(checks));} }
int main() {
    try {
        const auto& m=reviewed_file_manager_spec;const auto& t=reviewed_cd_stream_tables_spec;
        const auto& s=reviewed_cd_stream_disk_spec;CdStreamDiskCode code{};
        check(cd_stream_disk_code(s,t,m,code));check(code.body[64]==std::byte{0xe8});
        for(auto member:{&CdStreamDiskSpec::flags_rva,&CdStreamDiskSpec::allocator_rva,&CdStreamDiskSpec::thunk_rva,&CdStreamDiskSpec::thunk_slot_rva})
            for(auto bad:{0U,1U,UINT32_MAX}){auto v=s;v.*member=bad;check(!cd_stream_disk_code(v,t,m,code));}
        auto overlap=s;overlap.flags_rva=t.handles_rva;check(!cd_stream_disk_code(overlap,t,m,code));
        overlap=s;overlap.allocator_rva=t.target_rva+74;check(!cd_stream_disk_code(overlap,t,m,code));
        auto invalid=m;invalid.manager.image_base=UINT32_MAX;check(!cd_stream_disk_code(s,t,invalid,code));
        for(auto sector:{512U,1024U,2048U,4096U,8192U,16384U,32768U,65536U}) {
            CdStreamDiskGeometry g{8,sector,0,100};check(cd_stream_disk_geometry(g));
            check(cd_stream_disk_flags(sector)==(sector<=2048?0x60000000U:0x40000000U));
            CdStreamDiskGeometry out{};
            for(auto result:{1U,2U,UINT32_MAX}) {
                check(cd_stream_disk_query_result(result,{sector,100,0,8},out)==CdStreamDiskQueryResult::accepted);
                check(out.sectors_per_cluster==8 && out.bytes_per_sector==sector && out.free_clusters==0 && out.total_clusters==100);
            }
        }
        for(auto sector:{0U,1U,4U,256U,511U,513U,2049U,65537U,131072U,UINT32_MAX})check(!cd_stream_disk_geometry({8,sector,1,100}));
        for(auto g:{CdStreamDiskGeometry{0,512,1,100},{3,512,1,100},{0x80000000U,512,1,100},{8,512,101,100},{8,512,0,0}})check(!cd_stream_disk_geometry(g));
        for(auto poison:{0U,512U,0xccccccccU,UINT32_MAX}) {
            CdStreamDiskGeometry out{8,512,1,100};
            check(cd_stream_disk_query_result(0,{poison,poison,poison,poison},out)==CdStreamDiskQueryResult::failed);
            check(!out.sectors_per_cluster && !out.bytes_per_sector && !out.free_clusters && !out.total_clusters);
        }
        CdStreamDiskGeometry out{};check(cd_stream_disk_query_result(1,{0,100,1,8},out)==CdStreamDiskQueryResult::geometry_rejected);
        const EventDispatchRegisters caller{0x110000,1,2,3,4,5,6,7,0x202};
        // Register layout is populated by member name to retain ABI-independent test intent.
        auto c=caller;c.esp=0x110000;c.flags=0x202;
        auto r=c;r.edi=0x880080;r.esp=c.esp-52;r.eax=c.esp-12;r.ecx=c.esp-16;r.edx=c.esp-24;r.flags=0x246;
        const CdStreamDiskGeometry g{8,512,1,100};
        for(unsigned stage:{1U,2U}) {
            check(cd_stream_disk_frame(stage,c,r,0x880000,g));
            for(auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::eax,&EventDispatchRegisters::ecx,&EventDispatchRegisters::edx,&EventDispatchRegisters::ebx,&EventDispatchRegisters::esi,&EventDispatchRegisters::edi,&EventDispatchRegisters::ebp}) {
                auto bad=r;(bad.*member)^=4;check(!cd_stream_disk_frame(stage,c,bad,0x880000,g));
            }
        }
        r.esp=c.esp-28;r.eax=0;r.ecx=43;r.edx=67;r.flags=0x202;check(cd_stream_disk_frame(3,c,r,0x880000,g));
        const auto returned=r;
        r.esp=c.esp-44;r.eax=0x60000000;r.ecx=512;r.flags=0x206;check(cd_stream_disk_frame(4,c,r,0x880000,g,&returned));
        r.ecx=4096;r.eax=0x40000000;check(cd_stream_disk_frame(4,c,r,0x880000,{8,4096,1,100},&returned));
        r.eax=0x60000000;check(!cd_stream_disk_frame(4,c,r,0x880000,{8,4096,1,100},&returned));
        for(unsigned stage:{0U,5U,UINT32_MAX})check(!cd_stream_disk_frame(stage,c,r,0x880000,g));
        r.ecx=512;r.flags=0x206|0x400;check(!cd_stream_disk_frame(4,c,r,0x880000,g,&returned));
        r.flags=0x206|0x100;check(!cd_stream_disk_frame(4,c,r,0x880000,g,&returned));
        r.flags=0x206;r.edx^=4;check(!cd_stream_disk_frame(4,c,r,0x880000,g,&returned));r.edx^=4;
        check(!cd_stream_disk_frame(4,c,r,0x880000,g));
        const std::array<std::uint32_t,5> before{11,22,33,44,55},after{11,0x60000000,0,1,55};
        check(cd_stream_disk_memory(before,before,0,false));check(cd_stream_disk_memory(before,after,0x60000000,true));
        for(unsigned i=0;i<5;++i)for(unsigned bit=0;bit<32;++bit){auto bad=after;bad[i]^=1U<<bit;check(!cd_stream_disk_memory(before,bad,0x60000000,true));}
        std::cout<<"PASS cd stream disk: "<<checks<<" portable checks\n";return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
