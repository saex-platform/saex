#include "saex/engine/cd_stream_tables.hpp"
#include <algorithm>
namespace saex::engine {
namespace {
template<std::size_t N> void word(std::array<std::byte,N>& b,unsigned i,std::uint32_t value) noexcept {
    for(unsigned j=0;j<4;++j)b[i+j]=std::byte((value>>(j*8))&255U);
}
}
bool cd_stream_tables_code(const CdStreamTablesSpec& s,const FileManagerEntrySpec& m,CdStreamTablesCode& out) noexcept {
    const auto base=m.manager.image_base,size=m.manager.image_size,caller=m.manager.call_rva+5;
    if(size<4096 || base>UINT32_MAX-size || s.handles_rva<4100 || s.handles_rva%4 ||
        s.handles_rva>size-cd_stream_table_bytes || s.names_rva!=s.handles_rva+136 || s.disk_iat_rva%4)return false;
    const std::array<std::array<std::uint32_t,2>,5> spans{{{caller,7},{s.target_rva,74},
        {s.handles_rva-4,cd_stream_table_bytes},{s.disk_iat_rva,4},{m.buffer_rva-4,136}}};
    for(std::size_t i=0;i<spans.size();++i) {
        if(spans[i][0]<4096 || spans[i][0]>size-spans[i][1])return false;
        for(std::size_t j=0;j<i;++j)if(spans[i][0]<spans[j][0]+spans[j][1] && spans[j][0]<spans[i][0]+spans[i][1])return false;
    }
    out.caller={std::byte{0x6a},std::byte{5},std::byte{0xe8},{},{},{},{}};
    word(out.caller,3,s.target_rva-caller-7);
    constexpr unsigned char raw[]{0x83,0xec,0x10,0x57,0xb9,0x20,0,0,0,0x33,0xc0,0xbf,0,0,0,0,
        0xba,0,0,0,0,0xf3,0xab,0xeb,7,0x8d,0xa4,0x24,0,0,0,0,0xc6,2,0,0x83,0xc2,0x40,
        0x81,0xfa,0,0,0,0,0x7c,0xf2,0x8d,0x44,0x24,8,0x50,0x8d,0x4c,0x24,0x10,0x51,
        0x8d,0x54,0x24,0x0c,0x52,0x8d,0x44,0x24,0x1c,0x50,0x6a,0,0xff,0x15,0,0,0,0};
    std::transform(std::begin(raw),std::end(raw),out.body.begin(),[](auto v){return std::byte{v};});
    word(out.body,12,base+s.handles_rva);word(out.body,17,base+s.names_rva);
    word(out.body,40,base+s.names_rva+2048);word(out.body,70,base+s.disk_iat_rva);
    return std::equal(out.caller.begin(),out.caller.end(),m.return_prefix.begin());
}
bool cd_stream_tables_memory(const CdStreamTableWindow& before,const CdStreamTableWindow& after,bool initialized) noexcept {
    for(std::size_t i=0;i<before.size();++i) {
        const bool written=(i>=4 && i<132) || (i>=140 && i<2188 && (i-140)%64==0);
        if(after[i]!=(initialized && written?std::byte{}:before[i]))return false;
    }
    return true;
}
bool cd_stream_tables_frame(std::uint32_t stage,const EventDispatchRegisters& c,
    const EventDispatchRegisters& r,std::uint32_t handles) noexcept {
    if(stage<1 || stage>2 || c.esp<65600 || c.esp%4 || handles>UINT32_MAX-128 ||
        (c.flags&0x500U) || (r.flags&0x500U) || c.ebx!=r.ebx || c.esi!=r.esi || c.ebp!=r.ebp)return false;
    if(stage==1)return r.esp==c.esp-8 && r.eax==c.eax && r.ecx==c.ecx && r.edx==c.edx && r.edi==c.edi &&
        !((r.flags^c.flags)&~0x10000U);
    return r.esp==c.esp-48 && r.eax==c.esp-12 && r.ecx==c.esp-16 && r.edx==c.esp-24 &&
        r.edi==handles+128 && (r.flags&0x8c5U)==0x44U && !((r.flags^c.flags)&~0x108d5U);
}
}
