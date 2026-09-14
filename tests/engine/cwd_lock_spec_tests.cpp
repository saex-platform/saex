#include "saex/engine/cwd_lock_policy.generated.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main(){try{
    unsigned count{};const auto check=[&](bool v){++count;if(!v)throw std::runtime_error("cwd lock check "+std::to_string(count));};
    const auto original=reviewed_cwd_lock_spec;const auto parent=reviewed_cwd_seh_spec;
    const auto valid=[&](const auto& s){return valid_cwd_lock_spec(s,parent);};check(valid(original));
    for(auto v:{0U,4095U,UINT32_MAX,original.table_rva+1,original.selector.target_rva}){auto s=original;s.table_rva=v;check(!valid(s));}
    for(std::size_t i=0;i<16;++i){auto s=original;s.selector.target_prefix[i]^=std::byte{1};check(!valid(s));}
    for(std::size_t i=0;i<5;++i){auto s=original;s.selector.call[i]^=std::byte{1};check(!valid(s));}
    auto s=original;s.selector.call_rva++;check(!valid(s));s=original;s.stop_prefix[1]^=std::byte{1};check(!valid(s));
    s=original;s.stop_prefix[15]^=std::byte{1};check(valid(s)); // Remaining stop bytes are checked against live code.
    auto moved=original;auto mp=parent;mp.wrapper.image_base+=65536;mp.prologue.image_base+=65536;moved.selector.image_base+=65536;
    std::array<std::byte,17> code{};check(cwd_lock_code(moved,code));std::copy_n(code.begin(),16,moved.selector.target_prefix.begin());check(valid_cwd_lock_spec(moved,mp));
    const EventDispatchRegisters saved{0x100000,0x100018,0,0xabc,0xdef,0,24,0x100028,0x212};
    const std::array<std::uint32_t,7> tib{0x100018,0x110000,0xf0000,0,0,0,0x700000};
    check(valid_cwd_lock_origin(saved,tib));
    for(auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::eax,&EventDispatchRegisters::ebx,&EventDispatchRegisters::esi,&EventDispatchRegisters::edi,&EventDispatchRegisters::ebp}){
        auto bad=saved;bad.*member^=1;check(!valid_cwd_lock_origin(bad,tib));
    }
    for(auto pair:{std::array<std::uint32_t,2>{0,0x10001c},{1,0x100040},{2,0xfffe4},{6,0},{6,UINT32_MAX}}){auto t=tib;t[pair[0]]=pair[1];check(!valid_cwd_lock_origin(saved,t));}
    for(auto esp:{0U,65567U,UINT32_MAX,0xfffffffcU}){auto bad=saved;bad.esp=esp;check(!valid_cwd_lock_origin(bad,tib));}
    std::array<std::uint32_t,25> before{};for(unsigned i=0;i<25;++i)before[i]=0x11110000+i;
    // Independently specified flags for empty, odd/even parity, sign bit and all bits set.
    for(auto pair:{std::array<std::uint32_t,2>{0,0x246},{1,0x202},{3,0x206},{0x80000000,0x286},{UINT32_MAX,0x286},{0x12345678,0x206}}){
        for(std::uint32_t stage=1;stage<=2;++stage){
            auto current=saved;current.esp-=stage==1?8:16;
            if(stage==2){current.eax=7;current.ebp=saved.esp-12;current.esi=original.selector.image_base+original.table_rva+56;current.flags=pair[1];}
            check(cwd_lock_frame(original,stage,saved,current,pair[0]));
            for(auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::eax,&EventDispatchRegisters::ebx,&EventDispatchRegisters::ecx,&EventDispatchRegisters::edx,&EventDispatchRegisters::esi,&EventDispatchRegisters::edi,&EventDispatchRegisters::ebp}){
                auto bad=current;bad.*member^=1;check(!cwd_lock_frame(original,stage,saved,bad,pair[0]));
            }
            for(auto flag:{1U,4U,16U,64U,128U,0x800U,0x100U,0x200U,0x400U}){auto bad=current;bad.flags^=flag;check(!cwd_lock_frame(original,stage,saved,bad,pair[0]));}
            auto rf=current;rf.flags^=0x10000U;check(cwd_lock_frame(original,stage,saved,rf,pair[0]));
        }
    }
    for(std::uint32_t stage=1;stage<=2;++stage){
        auto now=before;now[7]=7;now[6]=original.selector.image_base+original.selector.call_rva+5;
        if(stage==2){now[5]=saved.ebp;now[4]=saved.esi;}
        check(cwd_lock_memory(original,stage,saved,before,now));
        for(auto& value:now){value^=1;check(!cwd_lock_memory(original,stage,saved,before,now));value^=1;}
    }
    check(!cwd_lock_frame(original,0,saved,saved,0));check(!cwd_lock_frame(original,3,saved,saved,0));
    check(!cwd_lock_memory(original,0,saved,before,before));check(!cwd_lock_memory(original,3,saved,before,before));
    std::cout<<count<<" cwd lock checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
