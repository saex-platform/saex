#include "saex/engine/file_manager_ready.hpp"
#include <algorithm>
namespace saex::engine {
namespace {
template<std::size_t N> void word(std::array<std::byte,N>& b,unsigned i,std::uint32_t v) noexcept {
    for(unsigned j=0;j<4;++j)b[i+j]=std::byte((v>>(8*j))&255U);
}
}
bool file_manager_ready_code(const FileManagerReadySpec& s,const CwdReturnSpec& p,const CwdLockSpec& l,
    const CwdSehSpec& seh,const FileManagerEntrySpec& m,FileManagerReadyCode& c) noexcept {
    const auto& f=l.selector;std::array<std::byte,51> body{};
    if(!file_manager_body(m,body) || !valid_cwd_lock_spec(l,seh) || f.image_base!=m.manager.image_base ||
        f.image_size!=m.manager.image_size || s.iat_rva%4 || s.function.rva<4096 ||
        s.function.rva>16*1024*1024-20 || !s.function.preferred_base || s.function.preferred_base%65536 ||
        s.function.highlow_mask)return false;
    const std::array<std::array<std::uint32_t,2>,10> spans{{{f.call_rva+23,24},{seh.cleanup_rva,9},
        {s.unlock_rva,21},{s.epilogue_rva,17},{s.iat_rva,4},{l.table_rva+52,16},
        {m.manager.target_rva,51},{m.manager.call_rva,21},{m.buffer_rva-4,136},{m.suffix_rva,2}}};
    for(std::size_t i=0;i<spans.size();++i) {
        if(spans[i][0]<4096 || spans[i][0]>f.image_size-spans[i][1])return false;
        for(std::size_t j=0;j<i;++j)if(spans[i][0]<spans[j][0]+spans[j][1] && spans[j][0]<spans[i][0]+spans[i][1])return false;
    }
    c.wrapper={std::byte{0x83},std::byte{0xc4},std::byte{0x0c},std::byte{0x89},std::byte{0x45},std::byte{0xe4},
        std::byte{0x83},std::byte{0x4d},std::byte{0xfc},std::byte{0xff},std::byte{0xe8},{},{},{},{},
        std::byte{0x8b},std::byte{0x45},std::byte{0xe4},std::byte{0xe8},{},{},{},{},std::byte{0xc3}};
    word(c.wrapper,11,seh.cleanup_rva-(f.call_rva+38));word(c.wrapper,19,s.epilogue_rva-(f.call_rva+46));
    c.cleanup={std::byte{0x6a},std::byte{7},std::byte{0xe8},{},{},{},{},std::byte{0x59},std::byte{0xc3}};
    word(c.cleanup,3,s.unlock_rva-(seh.cleanup_rva+7));
    c.unlock={std::byte{0x55},std::byte{0x8b},std::byte{0xec},std::byte{0x8b},std::byte{0x45},std::byte{8},
        std::byte{0xff},std::byte{0x34},std::byte{0xc5},{},{},{},{},std::byte{0xff},std::byte{0x15},{},{},{},{},std::byte{0x5d},std::byte{0xc3}};
    word(c.unlock,9,f.image_base+l.table_rva);word(c.unlock,15,f.image_base+s.iat_rva);
    c.epilogue={std::byte{0x8b},std::byte{0x4d},std::byte{0xf0},std::byte{0x64},std::byte{0x89},std::byte{0x0d},{},{},{},{},
        std::byte{0x59},std::byte{0x5f},std::byte{0x5e},std::byte{0x5b},std::byte{0xc9},std::byte{0x51},std::byte{0xc3}};
    return std::equal(p.return_prefix.begin(),p.return_prefix.end(),c.wrapper.begin());
}
bool file_manager_ready_frame(std::uint32_t stage,const EventDispatchRegisters& h,const EventDispatchRegisters& caller,
    const EventDispatchRegisters& r,std::uint32_t destination,std::uint32_t length) noexcept {
    if(stage<1 || stage>9 || h.esp<65556 || h.esp>UINT32_MAX-68 || h.esp%4 || h.ebp!=h.esp+44 ||
        caller.esp!=h.esp+68 || !destination || destination>UINT32_MAX-128 || length<3 || length>126 ||
        caller.ebx || caller.esi || caller.edi!=24 || r.ebx || r.esi || (r.flags&0x500U) ||
        ((r.flags^h.flags)&~0x108d5U))return false;
    const auto t=h.esp;
    const std::array<std::uint32_t,9> stacks{t-16,t-20,t-12,t,t+48,t+52,t+60,t+60,t+68};
    if(r.esp!=stacks[stage-1] || r.ebp!=(stage<=3?t-12:stage==4?h.ebp:caller.ebp) ||
        r.edi!=((stage==7 || stage==8)?destination+length:caller.edi))return false;
    if(stage<=2)return r.eax==7;
    if(stage==3)return true; // VOID API: volatile registers are not a success value.
    if(stage<=6)return r.eax==destination;
    return r.eax==(stage==7?(destination&0xffffff00U):((destination&0xffff0000U)|0x5cU));
}
bool file_manager_ready_buffer(const std::array<std::byte,136>& before,const std::array<std::byte,136>& now,
    std::uint32_t length,bool written) noexcept {
    if(length<3 || length>126 || before[length+4]!=std::byte{})return false;
    for(std::uint32_t i=0;i<length;++i)if(before[i+4]==std::byte{})return false;
    auto expected=before;
    if(written){expected[length+4]=std::byte{0x5c};expected[length+5]=std::byte{};}
    return now==expected;
}
}
