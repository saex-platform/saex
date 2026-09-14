#include "saex/engine/cwd_lock.hpp"
#include <algorithm>
#include <bit>
namespace saex::engine {
bool cwd_lock_code(const CwdLockSpec& s,std::array<std::byte,17>& code) noexcept {
    const auto& f=s.selector;
    if(!valid_frame_target_spec(f) || f.target_rva>f.image_size-33 || s.table_rva<4096 || s.table_rva%4 || s.table_rva>f.image_size-68)return false;
    constexpr unsigned char raw[]{0x55,0x8b,0xec,0x8b,0x45,0x08,0x56,0x8d,0x34,0xc5,0,0,0,0,0x83,0x3e,0};
    for(std::size_t i=0;i<code.size();++i)code[i]=std::byte{raw[i]};
    const auto address=f.image_base+s.table_rva;
    for(unsigned i=0;i<4;++i)code[10+i]=std::byte((address>>(8*i))&255U);
    return true;
}
bool valid_cwd_lock_spec(const CwdLockSpec& s,const CwdSehSpec& p) noexcept {
    const auto& f=s.selector;std::array<std::byte,17> code{};
    if(!cwd_lock_code(s,code) || !valid_frame_target_spec(p.prologue) || f.image_base!=p.wrapper.image_base ||
        f.image_size!=p.wrapper.image_size || f.call_rva!=p.prologue.call_rva+7 ||
        p.stop_prefix[0]!=std::byte{0x6a} || p.stop_prefix[1]!=std::byte{7} ||
        !std::equal(f.call.begin(),f.call.end(),p.stop_prefix.begin()+2) ||
        !std::equal(f.target_prefix.begin(),f.target_prefix.end(),code.begin()) ||
        s.stop_prefix[0]!=std::byte{0x75} || s.stop_prefix[1]!=std::byte{0x13})return false;
    const std::array<std::array<std::uint32_t,2>,7> spans{{{p.wrapper.target_rva,28},{p.prologue.target_rva,59},
        {p.scope_rva,12},{p.handler_rva,16},{p.cleanup_rva,16},{f.target_rva,33},{s.table_rva+52,16}}};
    for(std::size_t i=0;i<spans.size();++i){
        if(spans[i][0]<4096 || spans[i][1]>f.image_size || spans[i][0]>f.image_size-spans[i][1])return false;
        for(std::size_t j=0;j<i;++j)if(spans[i][0]<spans[j][0]+spans[j][1] && spans[j][0]<spans[i][0]+spans[i][1])return false;
    }return true;
}
bool valid_cwd_lock_origin(const EventDispatchRegisters& s,const std::array<std::uint32_t,7>& tib) noexcept {
    return s.esp>=65568 && s.esp<=UINT32_MAX-68 && s.esp%4==0 && s.eax==s.esp+24 && s.ebp==s.esp+40 &&
        s.ebx==0 && s.esi==0 && s.edi==24 && !(s.flags&0x500U) && tib[0]==s.esp+24 &&
        tib[2]>=65536 && tib[2]<=s.esp-32 && tib[1]>=s.esp+68 && tib[6]>=65536 && tib[6]<=UINT32_MAX-56;
}
bool cwd_lock_frame(const CwdLockSpec& s,std::uint32_t stage,const EventDispatchRegisters& saved,const EventDispatchRegisters& r,std::uint32_t value) noexcept {
    std::array<std::byte,17> code{};
    if(!cwd_lock_code(s,code) || stage<1 || stage>2 || saved.esp<65568 || saved.esp>UINT32_MAX-68 || saved.esp%4 ||
        saved.eax!=saved.esp+24 || saved.ebp!=saved.esp+40 || saved.ebx || saved.esi || saved.edi!=24 || (saved.flags&0x500U) || (r.flags&0x500U) ||
        r.esp!=saved.esp-(stage==1?8U:16U) || r.ebp!=(stage==1?saved.ebp:saved.esp-12) || r.eax!=(stage==1?saved.eax:7U) ||
        r.esi!=(stage==1?saved.esi:s.selector.image_base+s.table_rva+56) || r.ebx!=saved.ebx || r.ecx!=saved.ecx || r.edx!=saved.edx || r.edi!=saved.edi)return false;
    if(stage==1)return ((r.flags^saved.flags)&~0x10000U)==0;
    // CMP unsigned dword,0: CF/AF/OF clear, SF/ZF/PF describe the unchanged value.
    const auto flags=(std::popcount(value&255U)%2==0?4U:0U) | (value==0?64U:0U) | ((value>>24)&128U);
    return (r.flags&0x8d5U)==flags && ((r.flags^saved.flags)&~0x108d5U)==0;
}
bool cwd_lock_memory(const CwdLockSpec& s,std::uint32_t stage,const EventDispatchRegisters& saved,
    const std::array<std::uint32_t,25>& before,const std::array<std::uint32_t,25>& now) noexcept {
    std::array<std::byte,17> code{};
    if(!cwd_lock_code(s,code) || stage<1 || stage>2 || saved.esp<65568 || saved.esp>UINT32_MAX-68 || saved.esp%4)return false;
    auto expected=before;expected[7]=7;expected[6]=s.selector.image_base+s.selector.call_rva+5;
    if(stage==2){expected[5]=saved.ebp;expected[4]=saved.esi;}
    return expected==now;
}
}
