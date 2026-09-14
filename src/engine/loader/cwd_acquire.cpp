#include "saex/engine/cwd_acquire.hpp"
namespace saex::engine {
bool cwd_acquire_code(const CwdAcquireSpec& s,const CwdLockSpec& p,std::array<std::byte,11>& code) noexcept {
    const auto& f=p.selector;
    if(!valid_frame_target_spec(f) || s.iat_rva<4096 || s.iat_rva%4 || s.iat_rva>f.image_size-4 ||
        f.target_rva>f.image_size-49 || f.call_rva>f.image_size-21)return false;
    code={std::byte{0xff},std::byte{0x36},std::byte{0xff},std::byte{0x15},std::byte{},std::byte{},std::byte{},std::byte{},std::byte{0x5e},std::byte{0x5d},std::byte{0xc3}};
    const auto address=f.image_base+s.iat_rva;
    for(unsigned i=0;i<4;++i)code[i+4]=std::byte((address>>(8*i))&255U);
    return true;
}
bool valid_cwd_acquire_spec(const CwdAcquireSpec& s,const CwdLockSpec& p) noexcept {
    std::array<std::byte,11> code{};
    if(!cwd_acquire_code(s,p,code) || p.stop_prefix[0]!=std::byte{0x75} || p.stop_prefix[1]!=std::byte{0x13} ||
        s.function.rva<4096 || s.function.rva>16*1024*1024-20 || s.function.highlow_mask ||
        !s.function.preferred_base || s.function.preferred_base%65536)return false;
    const std::array<std::array<std::uint32_t,2>,4> spans{{{p.selector.target_rva,49},{p.selector.call_rva,21},{p.table_rva+52,16},{s.iat_rva,4}}};
    for(std::size_t i=0;i<spans.size();++i){
        if(spans[i][0]<4096 || spans[i][0]>p.selector.image_size-spans[i][1])return false;
        for(std::size_t j=0;j<i;++j)if(spans[i][0]<spans[j][0]+spans[j][1] && spans[j][0]<spans[i][0]+spans[i][1])return false;
    }return true;
}
bool cwd_acquire_object(std::uint32_t address,const std::array<std::uint32_t,6>& before,const std::array<std::uint32_t,6>& now,
    std::uint32_t thread_id,bool acquired) noexcept {
    // Internal layout is pinned to the reviewed x86 Windows image, not a public serialization format.
    if(address<65536 || address>UINT32_MAX-24 || address%4 || !thread_id ||
        before[1]!=UINT32_MAX || before[2] || before[3] || before[4])return false;
    auto expected=before;
    if(acquired){expected[1]=UINT32_MAX-1;expected[2]=1;expected[3]=thread_id;}
    return now==expected;
}
bool cwd_acquire_frame(std::uint32_t stage,const EventDispatchRegisters& saved,const EventDispatchRegisters& r,std::uint32_t restored_ebp) noexcept {
    if(stage<1 || stage>5 || saved.esp<65544 || saved.esp>UINT32_MAX-84 || saved.esp%4 || saved.eax!=7 ||
        saved.ebx || saved.edi!=24 || saved.ebp!=saved.esp+4 || (saved.flags&0x540U) || (r.flags&0x500U) ||
        r.ebx!=saved.ebx || r.edi!=saved.edi)return false;
    if(r.esp!=(stage==1?saved.esp:stage==2?saved.esp-4:stage==3?saved.esp-8:stage==4?saved.esp:saved.esp+12) ||
        r.esi!=(stage==5?0U:saved.esi) || r.ebp!=(stage==5?restored_ebp:saved.ebp))return false;
    // EnterCriticalSection returns VOID: volatile registers/flags are not a success code.
    if(stage>=4)return ((r.flags^saved.flags)&~0x108d5U)==0;
    return r.eax==saved.eax && r.ecx==saved.ecx && r.edx==saved.edx && ((r.flags^saved.flags)&~0x10000U)==0;
}
}
