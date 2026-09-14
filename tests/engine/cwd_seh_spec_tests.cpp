#include "saex/engine/cwd_seh_policy.generated.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main(){try{
    unsigned count{};const auto check=[&](bool v){++count;if(!v)throw std::runtime_error("cwd SEH check "+std::to_string(count));};
    const auto original=reviewed_cwd_seh_spec;const auto parent=reviewed_file_manager_spec;
    const auto valid=[&](const auto& s){return valid_cwd_seh_spec(s,parent);};check(valid(original));
    for(auto member:{&CwdSehSpec::scope_rva,&CwdSehSpec::handler_rva,&CwdSehSpec::cleanup_rva})
        for(auto v:{0U,4095U,UINT32_MAX,original.wrapper.target_rva}){auto s=original;s.*member=v;check(!valid(s));}
    for(std::size_t i=0;i<16;++i){auto s=original;s.wrapper.target_prefix[i]^=std::byte{1};check(!valid(s));s=original;s.prologue.target_prefix[i]^=std::byte{1};check(!valid(s));}
    auto s=original;s.prologue.call_rva++;check(!valid(s));s=original;s.stop_prefix[15]^=std::byte{1};check(valid(s));
    auto moved=original;auto mp=parent;mp.manager.image_base+=65536;moved.wrapper.image_base+=65536;moved.prologue.image_base+=65536;
    std::array<std::byte,16> w{};std::array<std::byte,59> code{};std::array<std::byte,12> scope{};
    check(cwd_seh_code(moved,w,code,scope));moved.wrapper.target_prefix=w;std::copy_n(code.begin(),16,moved.prologue.target_prefix.begin());check(valid_cwd_seh_spec(moved,mp));
    const EventDispatchRegisters saved{0x100000,0,0,0xabc,0xdef,0,24,0x77,0x246};
    const std::array<std::uint32_t,7> tib{0x100200,0x110000,0xf0000,0x1234,0x5678,0x9abc,0x700000};
    check(valid_cwd_seh_origin(saved,tib));auto bad_tib=tib;bad_tib[0]=UINT32_MAX;check(valid_cwd_seh_origin(saved,bad_tib));
    for(auto head:{0U,saved.esp-24,saved.esp+16,tib[1]-4,0x100201U}){bad_tib=tib;bad_tib[0]=head;check(!valid_cwd_seh_origin(saved,bad_tib));}
    bad_tib=tib;bad_tib[2]=saved.esp-56;check(!valid_cwd_seh_origin(saved,bad_tib));bad_tib=tib;bad_tib[1]=saved.esp+16;check(!valid_cwd_seh_origin(saved,bad_tib));
    for(auto esp:{0U,59U,65595U,UINT32_MAX,0x100001U}){auto bad=saved;bad.esp=esp;check(!valid_cwd_seh_origin(bad,tib));}
    std::array<std::uint32_t,20> before{};for(unsigned i=0;i<20;++i)before[i]=0x11110000+i;
    for(std::uint32_t stage=1;stage<=3;++stage){
        auto current=saved;current.esp-=stage==1?4:stage==2?16:48;
        if(stage==3){current.eax=saved.esp-24;current.ebp=saved.esp-8;current.flags=0x212;}
        check(cwd_seh_frame(original,stage,saved,current));
        for(auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::eax,&EventDispatchRegisters::ebx,&EventDispatchRegisters::ecx,&EventDispatchRegisters::edx,&EventDispatchRegisters::esi,&EventDispatchRegisters::edi,&EventDispatchRegisters::ebp}) {
            auto bad=current;bad.*member^=1;check(!cwd_seh_frame(original,stage,saved,bad));
        }
        for(auto flag:{1U,4U,16U,64U,128U,0x800U,0x100U,0x200U,0x400U}){auto bad=current;bad.flags^=flag;check(!cwd_seh_frame(original,stage,saved,bad));}
        auto rf=current;rf.flags^=0x10000U;check(cwd_seh_frame(original,stage,saved,rf));
        auto stack=before;auto now=tib;const auto base=original.wrapper.image_base,ret=base+original.prologue.call_rva+5;
        stack[14]=base+original.wrapper.call_rva+5;
        if(stage==2){stack[13]=12;stack[12]=base+original.scope_rva;stack[11]=ret;}
        if(stage==3){stack[2]=ret;stack[3]=24;stack[4]=stack[5]=0;stack[7]=0xfffd0;stack[9]=tib[0];stack[10]=base+original.handler_rva;
            stack[11]=base+original.scope_rva;stack[12]=UINT32_MAX;stack[13]=saved.ebp;now[0]=0xfffe8;}
        check(cwd_seh_memory(original,stage,saved,tib,now,before,stack));
        for(auto& v:stack){v^=1;check(!cwd_seh_memory(original,stage,saved,tib,now,before,stack));v^=1;}
        for(auto& v:now){v^=1;check(!cwd_seh_memory(original,stage,saved,tib,now,before,stack));v^=1;}
    }
    // A different stack alignment changes AF/PF; these are independently calculated examples.
    for(auto pair:{std::array<std::uint32_t,2>{4,0x202},std::array<std::uint32_t,2>{8,0x216}}){
        auto from=saved;from.esp+=pair[0];auto to=from;to.esp-=48;to.eax=from.esp-24;to.ebp=from.esp-8;to.flags=pair[1];
        check(cwd_seh_frame(original,3,from,to));
    }
    check(!cwd_seh_frame(original,0,saved,saved));check(!cwd_seh_frame(original,4,saved,saved));
    check(!cwd_seh_memory(original,0,saved,tib,tib,before,before));check(!cwd_seh_memory(original,4,saved,tib,tib,before,before));
    std::cout<<count<<" cwd SEH checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
