#include "saex/engine/cd_stream_allocation.hpp"
#include <algorithm>
#include <bit>
namespace saex::engine {
namespace {
template<std::size_t N> void word(std::array<std::byte,N>& b,unsigned i,std::uint32_t v) noexcept {
    for(unsigned j=0;j<4;++j)b[i+j]=std::byte((v>>(j*8))&255U);
}
bool alignment_valid(std::uint32_t a) noexcept {return a>=512 && a<=65536 && !(a&(a-1));}
std::uint32_t test_flags(std::uint32_t v) noexcept {
    return (std::popcount(v&255U)%2==0?4U:0U)|(v==0?64U:0U)|((v>>24)&128U);
}
}
bool cd_stream_allocation_request(std::uint32_t a,const std::array<std::uint32_t,4>& g) noexcept {
    return alignment_valid(a) && !g[0] && g[2]>=65536 && g[2]<0xffff0000U && g[2]%8==0 &&
        (g[3]==1 || (g[3]==3 && g[1]<2048+a));
}
bool cd_stream_allocation_layout(std::uint32_t raw,std::uint32_t a,CdStreamAllocationLayout& out) noexcept {
    out={};
    if(!alignment_valid(a) || raw<65536 || raw%8 || raw>UINT32_MAX-2048-a)return false;
    const auto aligned=(raw+a)&~(a-1);
    if(aligned<raw+4 || aligned>raw+a)return false;
    out={raw,aligned,aligned-4,2048+a};return true;
}
bool cd_stream_allocation_frame(std::uint32_t stage,const EventDispatchRegisters& c,const EventDispatchRegisters& r,
    std::uint32_t a,const CdStreamAllocationLayout& b,const EventDispatchRegisters& returned) noexcept {
    if(stage<1 || stage>10 || c.esp<65700 || c.esp%4 || !alignment_valid(a) || (c.flags&0x500U) ||
        (r.flags&0x500U) || r.ebx!=c.ebx || r.edi!=c.edi)return false;
    const auto active=stage>=3 && stage<=7;
    if(r.ebp!=(active?c.esp-40:c.ebp) || r.esi!=(active?2048+a:stage==9?~(a-1):stage>=2 && stage<=8?a:c.esi))return false;
    constexpr std::uint32_t delta[]{0,4,36,80,80,92,96,80,12,8,0};
    if(r.esp!=c.esp-delta[stage])return false;
    if(stage==1)return r.eax==c.eax && r.ecx==c.ecx && r.edx==c.edx && !((r.flags^c.flags)&~0x10000U);
    if(stage==2)return r.eax==2048+a && r.ecx==c.ecx && r.edx==c.edx;
    if(stage>=3 && stage<=6)return r.eax==c.esp-56 && r.ecx==c.ecx && r.edx==c.edx;
    if(stage==7)return true; // The Windows ABI permits volatile registers and flags to change.
    if(!b.raw)return false;
    if(stage==8)return r.eax==b.raw && !r.ecx && r.edx==returned.edx && (r.flags&0x8c5U)==test_flags(b.raw);
    return r.eax==b.aligned && r.ecx==b.raw && r.edx==returned.edx &&
        (r.flags&0x8c5U)==test_flags(b.aligned) && !((r.flags^returned.flags)&~0x108d5U);
}
bool cd_stream_allocation_code(const CdStreamAllocationSpec& s,const CdStreamDiskSpec& disk,
    const FileManagerEntrySpec& m,const CwdSehSpec& seh,const FileManagerReadySpec& ready,CdStreamAllocationCode& out) noexcept {
    const auto base=m.manager.image_base,size=m.manager.image_size;
    if(size<4096 || base>UINT32_MAX-size || s.function.rva<4096 || s.function.rva>16*1024*1024-20)return false;
    std::array<std::byte,20> prefix{};
    if(!bootstrap_export_prefix(s.function,s.function.preferred_base,prefix))return false;
    const std::array<std::array<std::uint32_t,2>,15> spans{{{disk.allocator_rva,35},{s.malloc_rva,18},{s.nh_rva,44},
        {s.heap_rva,111},{s.scope_rva,12},{s.new_mode_rva,4},{s.threshold_rva,4},{s.heap_handle_rva,4},{s.heap_mode_rva,4},{s.iat_rva,4},
        {seh.prologue.target_rva,59},{ready.epilogue_rva,17},{m.buffer_rva-4,136},{disk.flags_rva-4,20},{s.scope_cleanup_rva,1}}};
    for(std::size_t i=0;i<spans.size();++i) {
        if(spans[i][0]<4096 || spans[i][0]>size-spans[i][1])return false;
        if(i>=4 && i<=9 && spans[i][0]%4)return false;
        for(std::size_t j=0;j<i;++j)if(spans[i][0]<spans[j][0]+spans[j][1] && spans[j][0]<spans[i][0]+spans[i][1])return false;
    }
    out.aligned={std::byte{0x8b},std::byte{0x44},std::byte{0x24},std::byte{0x04},std::byte{0x56},std::byte{0x8b},std::byte{0x74},std::byte{0x24},std::byte{0x0c},std::byte{0x03},std::byte{0xc6},std::byte{0x50},std::byte{0xe8},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x8b},std::byte{0xc8},std::byte{0x8d},std::byte{0x04},std::byte{0x31},std::byte{0x83},std::byte{0xc4},std::byte{0x04},std::byte{0x4e},std::byte{0xf7},std::byte{0xd6},std::byte{0x23},std::byte{0xc6},std::byte{0x89},std::byte{0x48},std::byte{0xfc},std::byte{0x5e},std::byte{0xc3}};
    word(out.aligned,13,s.malloc_rva-(disk.allocator_rva)-17);
    out.malloc={std::byte{0xff},std::byte{0x35},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0xff},std::byte{0x74},std::byte{0x24},std::byte{0x08},std::byte{0xe8},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x59},std::byte{0x59},std::byte{0xc3}};
    word(out.malloc,2,base+s.new_mode_rva);
    word(out.malloc,11,s.nh_rva-(s.malloc_rva)-15);
    out.nh={std::byte{0x83},std::byte{0x7c},std::byte{0x24},std::byte{0x04},std::byte{0xe0},std::byte{0x77},std::byte{0x22},std::byte{0xff},std::byte{0x74},std::byte{0x24},std::byte{0x04},std::byte{0xe8},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x85},std::byte{0xc0},std::byte{0x59},std::byte{0x75},std::byte{0x16}};
    word(out.nh,12,s.heap_rva-(s.nh_rva)-16);
    out.heap_entry={std::byte{0x6a},std::byte{0x0c},std::byte{0x68},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0xe8},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x8b},std::byte{0x75},std::byte{0x08},std::byte{0x83},std::byte{0x3d},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x03},std::byte{0x75},std::byte{0x2e},std::byte{0x3b},std::byte{0x35},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x77},std::byte{0x26}};
    word(out.heap_entry,3,base+s.scope_rva);
    word(out.heap_entry,8,seh.prologue.target_rva-(s.heap_rva)-12);
    word(out.heap_entry,17,base+s.heap_mode_rva);
    word(out.heap_entry,26,base+s.threshold_rva);
    out.heap_tail={std::byte{0x85},std::byte{0xf6},std::byte{0x75},std::byte{0x01},std::byte{0x46},std::byte{0x83},std::byte{0x3d},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x01},std::byte{0x74},std::byte{0x06},std::byte{0x83},std::byte{0xc6},std::byte{0x0f},std::byte{0x83},std::byte{0xe6},std::byte{0xf0},std::byte{0x56},std::byte{0x6a},std::byte{0x00},std::byte{0xff},std::byte{0x35},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0xff},std::byte{0x15},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0xe8},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0xc3}};
    word(out.heap_tail,7,base+s.heap_mode_rva);
    word(out.heap_tail,25,base+s.heap_handle_rva);
    word(out.heap_tail,31,base+s.iat_rva);
    word(out.heap_tail,36,ready.epilogue_rva-(s.heap_rva+70)-40);
    return true;
}
}
