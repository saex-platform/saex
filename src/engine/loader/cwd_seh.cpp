#include "saex/engine/cwd_seh.hpp"
#include <algorithm>
#include <bit>
namespace saex::engine {
bool cwd_seh_code(const CwdSehSpec& s,std::array<std::byte,16>& wrapper,std::array<std::byte,59>& prologue,std::array<std::byte,12>& scope) noexcept {
    const auto& f=s.wrapper;
    if(!valid_frame_target_spec(f) || !valid_frame_target_spec(s.prologue) || s.scope_rva<4096 || s.scope_rva>f.image_size-12 ||
        s.handler_rva<4096 || s.handler_rva>f.image_size-16 || s.cleanup_rva<4096 || s.cleanup_rva>f.image_size-16 ||
        s.prologue.target_rva>f.image_size-59 || f.target_rva>f.image_size-28) return false;
    constexpr unsigned char raw[]{0x68,0,0,0,0,0x64,0xa1,0,0,0,0,0x50,0x8b,0x44,0x24,0x10,0x89,0x6c,0x24,0x10,
        0x8d,0x6c,0x24,0x10,0x2b,0xe0,0x53,0x56,0x57,0x8b,0x45,0xf8,0x89,0x65,0xe8,0x50,0x8b,0x45,0xfc,
        0xc7,0x45,0xfc,0xff,0xff,0xff,0xff,0x89,0x45,0xf8,0x8d,0x45,0xf0,0x64,0xa3,0,0,0,0,0xc3};
    for(std::size_t i=0;i<prologue.size();++i)prologue[i]=std::byte{raw[i]};
    wrapper={std::byte{0x6a},std::byte{12},std::byte{0x68}};scope={};
    const auto write=[](auto& bytes,unsigned offset,std::uint32_t value){for(unsigned j=0;j<4;++j)bytes[offset+j]=std::byte((value>>(j*8))&255U);};
    write(wrapper,3,f.image_base+s.scope_rva);write(prologue,1,f.image_base+s.handler_rva);
    std::copy(s.prologue.call.begin(),s.prologue.call.end(),wrapper.begin()+7);std::copy_n(s.stop_prefix.begin(),4,wrapper.begin()+12);
    write(scope,0,UINT32_MAX);write(scope,8,f.image_base+s.cleanup_rva);return true;
}
bool valid_cwd_seh_spec(const CwdSehSpec& s,const FileManagerEntrySpec& p) noexcept {
    const auto& f=s.wrapper;const auto& h=s.prologue;const auto& m=p.manager;
    std::array<std::byte,16> w{};std::array<std::byte,59> code{};std::array<std::byte,12> scope{};std::array<std::byte,51> parent{};
    if(!file_manager_body(p,parent) || !cwd_seh_code(s,w,code,scope) || f.image_base!=m.image_base || h.image_base!=m.image_base ||
        f.image_size!=m.image_size || h.image_size!=m.image_size || f.call_rva!=m.target_rva+11 || f.target_rva!=p.cwd_rva ||
        h.call_rva!=f.target_rva+7 || !std::equal(f.call.begin(),f.call.end(),parent.begin()+11) ||
        f.target_prefix!=w || !std::equal(h.target_prefix.begin(),h.target_prefix.end(),code.begin()))return false;
    const std::array<std::array<std::uint32_t,2>,9> spans{{{m.call_rva,21},{m.target_rva,51},{p.buffer_rva-4,136},{p.suffix_rva,2},
        {f.target_rva,28},{h.target_rva,59},{s.scope_rva,12},{s.handler_rva,16},{s.cleanup_rva,16}}};
    for(std::size_t i=0;i<spans.size();++i){
        if(spans[i][0]<4096 || spans[i][0]>f.image_size-spans[i][1])return false;
        for(std::size_t j=0;j<i;++j)if(spans[i][0]<spans[j][0]+spans[j][1] && spans[j][0]<spans[i][0]+spans[i][1])return false;
    }return true;
}
bool valid_cwd_seh_origin(const EventDispatchRegisters& saved,const std::array<std::uint32_t,7>& t) noexcept {
    if(saved.esp<65596 || saved.esp>UINT32_MAX-20 || saved.esp%4 || t[2]<65536 || t[1]<=t[2] ||
        saved.esp-60<t[2] || saved.esp+20>t[1] || t[6]<65536 || t[6]>UINT32_MAX-56)return false;
    return t[0]==UINT32_MAX || (t[0]%4==0 && t[0]>=saved.esp+20 && t[0]<=t[1]-8);
}
bool cwd_seh_frame(const CwdSehSpec& s,std::uint32_t stage,const EventDispatchRegisters& saved,const EventDispatchRegisters& r) noexcept {
    if(stage<1 || stage>3 || saved.esp<65596 || saved.esp>UINT32_MAX-20 || saved.esp%4 || saved.eax || saved.ebx || saved.esi || saved.edi!=24 ||
        (saved.flags&0x500U) || (r.flags&0x500U) || r.ebx!=saved.ebx || r.ecx!=saved.ecx || r.edx!=saved.edx || r.esi!=saved.esi || r.edi!=saved.edi ||
        r.eax!=(stage==3?saved.esp-24:saved.eax) || r.ebp!=(stage==3?saved.esp-8:saved.ebp) ||
        r.esp!=saved.esp-(stage==1?4U:stage==2?16U:48U) || !valid_frame_target_spec(s.wrapper))return false;
    if(stage<3)return ((r.flags^saved.flags)&~0x10000U)==0;
    // Only SUB ESP,12 sets flags: operand was saved ESP-24, result saved ESP-36.
    const auto lhs=saved.esp-24,res=lhs-12;
    const auto flags=(lhs<12?1U:0U) | ((std::popcount(res&255U)%2)==0?4U:0U) | ((lhs^12U^res)&16U) |
        (res==0?64U:0U) | ((res>>24)&128U) | ((((lhs^12U)&(lhs^res))>>20)&0x800U);
    return (r.flags&0x8d5U)==flags && ((r.flags^saved.flags)&~0x108d5U)==0;
}
bool cwd_seh_memory(const CwdSehSpec& s,std::uint32_t stage,const EventDispatchRegisters& saved,
    const std::array<std::uint32_t,7>& before_tib,const std::array<std::uint32_t,7>& now_tib,
    const std::array<std::uint32_t,20>& before_stack,const std::array<std::uint32_t,20>& now_stack) noexcept {
    if(stage<1 || stage>3 || !valid_cwd_seh_origin(saved,before_tib) || !valid_frame_target_spec(s.wrapper))return false;
    auto stack=before_stack;auto tib=before_tib;const auto base=s.wrapper.image_base,ret=base+s.prologue.call_rva+5;
    stack[14]=base+s.wrapper.call_rva+5;
    if(stage==2){stack[13]=12;stack[12]=base+s.scope_rva;stack[11]=ret;}
    if(stage==3){
        stack[2]=ret;stack[3]=saved.edi;stack[4]=saved.esi;stack[5]=saved.ebx;stack[7]=saved.esp-48;
        stack[9]=before_tib[0];stack[10]=base+s.handler_rva;stack[11]=base+s.scope_rva;stack[12]=UINT32_MAX;stack[13]=saved.ebp;
        tib[0]=saved.esp-24;
    }
    return tib==now_tib && stack==now_stack;
}
}
