#include "saex/engine/cd_stream_disk.hpp"
#include <algorithm>
namespace saex::engine {
namespace {
template<std::size_t N> void word(std::array<std::byte,N>& b,unsigned i,std::uint32_t v) noexcept {
    for(unsigned j=0;j<4;++j)b[i+j]=std::byte((v>>(j*8))&255U);
}
}
bool cd_stream_disk_code(const CdStreamDiskSpec& s,const CdStreamTablesSpec& t,
    const FileManagerEntrySpec& m,CdStreamDiskCode& out) noexcept {
    CdStreamTablesCode parent{};
    if(!cd_stream_tables_code(t,m,parent))return false;
    const auto base=m.manager.image_base,size=m.manager.image_size;
    if(s.flags_rva<4100 || s.flags_rva%4 || s.thunk_rva<4096 || s.thunk_rva>16*1024*1024-6 ||
        s.thunk_slot_rva<4096 || s.thunk_slot_rva>16*1024*1024-4 || s.thunk_slot_rva%4 ||
        (s.thunk_rva<s.thunk_slot_rva+4 && s.thunk_slot_rva<s.thunk_rva+6) ||
        s.implementation.rva<4096 || s.implementation.rva>16*1024*1024-20)return false;
    std::array<std::byte,20> prefix{};
    if(!bootstrap_export_prefix(s.implementation,s.implementation.preferred_base,prefix))return false;
    const std::array<std::array<std::uint32_t,2>,6> spans{{{t.target_rva,137},{s.flags_rva-4,20},
        {s.allocator_rva,1},{t.handles_rva-4,cd_stream_table_bytes},{m.buffer_rva-4,136},{t.disk_iat_rva,4}}};
    for(std::size_t i=0;i<spans.size();++i) {
        if(spans[i][0]<4096 || spans[i][0]>size-spans[i][1])return false;
        for(std::size_t j=0;j<i;++j)if(spans[i][0]<spans[j][0]+spans[j][1] && spans[j][0]<spans[i][0]+spans[i][1])return false;
    }
    constexpr unsigned char raw[]{0xff,0x15,0,0,0,0,0x8b,0x4c,0x24,4,0x33,0xc0,0x81,0xf9,0,8,0,0,
        0x77,5,0xb8,0,0,0,0x20,0x56,0x6a,0,0x51,0x0d,0,0,0,0x40,0x68,0,8,0,0,
        0xc7,5,0,0,0,0,1,0,0,0,0xa3,0,0,0,0,0xc7,5,0,0,0,0,0,0,0,0,0xe8,0,0,0,0};
    std::transform(std::begin(raw),std::end(raw),out.body.begin(),[](auto v){return std::byte{v};});
    word(out.body,2,base+t.disk_iat_rva);word(out.body,41,base+s.flags_rva+8);
    word(out.body,50,base+s.flags_rva);word(out.body,56,base+s.flags_rva+4);
    word(out.body,65,s.allocator_rva-t.target_rva-137);
    return true;
}
bool cd_stream_disk_geometry(const CdStreamDiskGeometry& g) noexcept {
    // This observer's admitted logical-sector profile, not a universal Windows limit.
    return g.bytes_per_sector>=512 && g.bytes_per_sector<=65536 && !(g.bytes_per_sector&(g.bytes_per_sector-1)) &&
        g.sectors_per_cluster && !(g.sectors_per_cluster&(g.sectors_per_cluster-1)) &&
        g.sectors_per_cluster<=UINT32_MAX/g.bytes_per_sector && g.total_clusters && g.free_clusters<=g.total_clusters;
}
CdStreamDiskQueryResult cd_stream_disk_query_result(std::uint32_t result,
    const std::array<std::uint32_t,4>& locals,CdStreamDiskGeometry& output) noexcept {
    output={};
    if(!result)return CdStreamDiskQueryResult::failed;
    output={locals[3],locals[0],locals[2],locals[1]};
    return cd_stream_disk_geometry(output)?CdStreamDiskQueryResult::accepted:CdStreamDiskQueryResult::geometry_rejected;
}
std::uint32_t cd_stream_disk_flags(std::uint32_t sector) noexcept { return sector<=2048?0x60000000U:0x40000000U; }
bool cd_stream_disk_frame(std::uint32_t stage,const EventDispatchRegisters& c,const EventDispatchRegisters& r,
    std::uint32_t handles,const CdStreamDiskGeometry& g,const EventDispatchRegisters* api_return) noexcept {
    if(stage<1 || stage>4 || c.esp<65600 || c.esp%4 || handles>UINT32_MAX-128 ||
        (c.flags&0x500U) || (r.flags&0x500U) || r.ebx!=c.ebx || r.esi!=c.esi || r.ebp!=c.ebp || r.edi!=handles+128)return false;
    if(stage<=2)return r.esp==c.esp-52 && r.eax==c.esp-12 && r.ecx==c.esp-16 && r.edx==c.esp-24 &&
        (r.flags&0x8c5U)==0x44U && !((r.flags^c.flags)&~0x108d5U);
    if(stage==3)return r.esp==c.esp-28;
    return api_return && r.edx==api_return->edx && !((r.flags^api_return->flags)&~0x108d5U) &&
        cd_stream_disk_geometry(g) && r.esp==c.esp-44 && r.ecx==g.bytes_per_sector &&
        r.eax==cd_stream_disk_flags(g.bytes_per_sector) && (r.flags&0x8c5U)==4U;
}
bool cd_stream_disk_memory(const std::array<std::uint32_t,5>& before,const std::array<std::uint32_t,5>& after,
    std::uint32_t flags,bool prepared) noexcept {
    return !prepared?before==after:after==std::array<std::uint32_t,5>{before[0],flags,0,1,before[4]};
}
}
