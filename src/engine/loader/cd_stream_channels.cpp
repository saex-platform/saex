#include "saex/engine/cd_stream_channels.hpp"
#include <algorithm>
#include <bit>
namespace saex::engine {
namespace { void word(std::array<std::byte,64>& b,unsigned offset,std::uint32_t value) noexcept {
    for(unsigned j=0;j<4;++j)b[offset+j]=std::byte((value>>(j*8))&255U);
} }
bool cd_stream_channels_code(const CdStreamChannelsSpec& s,const CdStreamTablesSpec& t,
    const FileManagerEntrySpec& m,std::array<std::byte,64>& out) noexcept {
    const auto size=m.manager.image_size,base=m.manager.image_base;
    if(size<4096 || base>UINT32_MAX-size || t.target_rva>UINT32_MAX-137 || t.names_rva<8 || s.pointer_rva<4)return false;
    const std::array<std::array<std::uint32_t,2>,7> spans{{{t.target_rva+137,64},{s.error_iat_rva,4},{s.local_iat_rva,4},
        {s.pointer_rva-4,12},{s.filename_rva,16},{s.open_rva,5},{t.names_rva-8,8}}};
    for(std::size_t i=0;i<spans.size();++i) {
        if(spans[i][0]<4096 || spans[i][0]>size-spans[i][1] || (i>=1 && i<=3 && spans[i][0]%4))return false;
        for(std::size_t j=0;j<i;++j)if(spans[i][0]<spans[j][0]+spans[j][1] && spans[j][0]<spans[i][0]+spans[i][1])return false;
    }
    for(auto slot:{s.error_thunk_slot_rva,s.local_thunk_slot_rva})if(slot<4096 || slot>16*1024*1024-4 || slot%4)return false;
    for(const auto& f:{s.error_thunk,s.error_implementation,s.local_thunk,s.local_implementation}) {
        std::array<std::byte,20> b{};
        if(f.rva<4096 || f.rva>16*1024*1024-20 || !bootstrap_export_prefix(f,f.preferred_base,b))return false;
    }
    const auto thunk=[](const BootstrapExportSpec& f,std::uint32_t slot,bool local) {
        constexpr unsigned char prologue[]{0x8b,0xff,0x55,0x8b,0xec,0x5d,0xff,0x25};
        const unsigned at=local?8U:2U;
        for(unsigned i=0;i<at;++i)if(f.prefix[i]!=std::byte{prologue[(local?0U:6U)+i]})return false;
        std::uint32_t operand{};for(unsigned i=0;i<4;++i)operand|=std::to_integer<std::uint32_t>(f.prefix[at+i])<<(8*i);
        return slot<=UINT32_MAX-f.preferred_base && operand==f.preferred_base+slot && f.highlow_mask==(1U<<at);
    };
    if(!thunk(s.error_thunk,s.error_thunk_slot_rva,false) || !thunk(s.local_thunk,s.local_thunk_slot_rva,true))return false;
    out={std::byte{0x83},std::byte{0xc4},std::byte{0x0c},std::byte{0x6a},std::byte{0x00},std::byte{0x8b},std::byte{0xf8},std::byte{0xff},std::byte{0x15}};
    word(out,9,base+s.error_iat_rva);
    constexpr unsigned char tail[]{0x8b,0x44,0x24,0x1c,0x8d,0x0c,0x40,0xc1,0xe1,0x04,0x51,0x6a,0x40,0xc7,0x05,0,0,0,0,0,0,0,0,0xa3,0,0,0,0,0xff,0x15,0,0,0,0,0x6a,0,0x68,0,0,0,0,0xa3,0,0,0,0,0xe8,0,0,0,0};
    static_assert(sizeof(tail)==51);for(unsigned i=0;i<51;++i)out[13+i]=std::byte{tail[i]};
    word(out,28,base+t.names_rva-4);word(out,37,base+t.names_rva-8);word(out,43,base+s.local_iat_rva);
    word(out,50,base+s.filename_rva);word(out,55,base+s.pointer_rva);word(out,60,s.open_rva-(t.target_rva+137)-64);return true;
}
bool cd_stream_channels_frame(std::uint32_t stage,const EventDispatchRegisters& c,const EventDispatchRegisters& r,
    const EventDispatchRegisters& e,const EventDispatchRegisters& l) noexcept {
    if(stage<1 || stage>9 || c.esp<65536 || c.esp>UINT32_MAX-44 || c.esp%4 || (r.flags&0x500U) || (c.flags&0x500U) ||
        r.ebx!=c.ebx || r.esi!=c.esi || r.edi!=c.eax || r.ebp!=c.ebp)return false;
    constexpr unsigned offset[]{0,8,4,4,12,4,0,0,12,4};
    if(r.esp!=c.esp+offset[stage])return false;
    if(stage<=3) {
        const auto sum=c.esp+12;
        const auto flags=(std::popcount(sum&255U)%2==0?4U:0U) | ((c.esp^12U^sum)&0x10U) |
            (sum&0x80000000U?0x80U:0U) | ((~(c.esp^12U)&(c.esp^sum))&0x80000000U?0x800U:0U);
        return r.eax==c.eax && r.ecx==c.ecx && r.edx==c.edx && (r.flags&0x8d5U)==flags && !((r.flags^c.flags)&~0x108d5U);
    }
    if(stage==4 || stage==8)return true; // Win32 volatile registers have no success ABI for VOID.
    if(stage<=7)return r.eax==5 && r.ecx==240 && r.edx==e.edx && (r.flags&0xc5U)==4 && !((r.flags^e.flags)&~0x108d5U);
    return r.eax==l.eax && r.ecx==l.ecx && r.edx==l.edx && !((r.flags^l.flags)&~0x10000U);
}
bool cd_stream_channels_range(std::uint32_t p,const CdStreamAllocationLayout& b) noexcept {
    return p>=65536 && p%4==0 && p<=UINT32_MAX-cd_stream_channel_bytes && b.raw>=65536 && b.bytes>=2560 &&
        b.bytes<=67584 && b.raw<=UINT32_MAX-b.bytes && !(p<b.raw+b.bytes && b.raw<p+cd_stream_channel_bytes);
}
bool cd_stream_channels_memory(std::span<const std::byte> b) noexcept {
    return b.size()==cd_stream_channel_bytes && std::all_of(b.begin(),b.end(),[](auto v){return v==std::byte{};});
}
bool cd_stream_channels_tables(const CdStreamTableWindow& before,const CdStreamTableWindow& after,bool prepared) noexcept {
    for(std::size_t i=0;i<before.size();++i) {
        const auto expected=prepared && i>=132 && i<140?(i==132?std::byte{5}:std::byte{}):before[i];
        if(after[i]!=expected)return false;
    }return true;
}
}
