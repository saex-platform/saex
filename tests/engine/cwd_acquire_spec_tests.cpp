#include "saex/engine/cwd_acquire_policy.generated.hpp"
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main(){try{
    unsigned count{};const auto check=[&](bool v){++count;if(!v)throw std::runtime_error("cwd acquire check "+std::to_string(count));};
    const auto original=reviewed_cwd_acquire_spec;const auto parent=reviewed_cwd_lock_spec;
    const auto valid=[&](const auto& s){return valid_cwd_acquire_spec(s,parent);};check(valid(original));
    for(auto v:{0U,4095U,UINT32_MAX,original.iat_rva+1,parent.selector.target_rva,parent.table_rva+56}){auto s=original;s.iat_rva=v;check(!valid(s));}
    auto s=original;s.function.rva=0;check(!valid(s));s=original;s.function.highlow_mask=1;check(!valid(s));s=original;s.function.preferred_base++;check(!valid(s));
    s=original;s.function.prefix[0]^=std::byte{1};check(valid(s)); // Live pinned entry rejects this value.
    auto p=parent;p.selector.image_base+=65536;std::array<std::byte,11> a{},b{};
    check(cwd_acquire_code(original,parent,a));check(cwd_acquire_code(original,p,b));check(a!=b);
    const std::array<std::uint32_t,6> before{UINT32_MAX,UINT32_MAX,0,0,0,4000};
    auto after=before;after[1]=UINT32_MAX-1;after[2]=1;after[3]=123;
    check(cwd_acquire_object(0x200000,before,before,123,false));check(cwd_acquire_object(0x200000,before,after,123,true));
    for(auto address:{0U,1U,65535U,UINT32_MAX,0x200001U})check(!cwd_acquire_object(address,before,after,123,true));
    check(!cwd_acquire_object(0x200000,before,after,0,true));check(!cwd_acquire_object(0x200000,before,after,124,true));
    for(unsigned i=1;i<=4;++i){auto bad=before;bad[i]^=1;check(!cwd_acquire_object(0x200000,bad,bad,123,false));}
    for(auto& value:after){value^=1;check(!cwd_acquire_object(0x200000,before,after,123,true));value^=1;}
    const EventDispatchRegisters saved{0x100000,7,0,0xabc,0xdef,0x8e31f8,24,0x100004,0x206};
    for(std::uint32_t stage=1;stage<=5;++stage){
        auto r=saved;r.esp=stage==1?saved.esp:stage==2?saved.esp-4:stage==3?saved.esp-8:stage==4?saved.esp:saved.esp+12;
        if(stage>=4){r.eax=0xcccc;r.ecx=0xdddd;r.edx=0xeeee;r.flags=0x202;}
        if(stage==5){r.esi=0;r.ebp=0x100038;}
        check(cwd_acquire_frame(stage,saved,r,0x100038));
        for(auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::ebx,&EventDispatchRegisters::esi,&EventDispatchRegisters::edi,&EventDispatchRegisters::ebp}){
            auto bad=r;bad.*member^=1;check(!cwd_acquire_frame(stage,saved,bad,0x100038));
        }
        for(auto member:{&EventDispatchRegisters::eax,&EventDispatchRegisters::ecx,&EventDispatchRegisters::edx}){
            auto v=r;v.*member^=1;check(cwd_acquire_frame(stage,saved,v,0x100038)==(stage>=4));
        }
        for(auto flag:{1U,4U,16U,64U,128U,0x800U}){auto v=r;v.flags^=flag;check(cwd_acquire_frame(stage,saved,v,0x100038)==(stage>=4));}
        for(auto flag:{0x100U,0x400U,0x200U,0x3000U,0x20000U}){auto v=r;v.flags^=flag;check(!cwd_acquire_frame(stage,saved,v,0x100038));}
        auto v=r;v.flags^=0x10000U;check(cwd_acquire_frame(stage,saved,v,0x100038));
    }
    auto zero_branch=saved;zero_branch.flags|=64;check(!cwd_acquire_frame(1,zero_branch,zero_branch,0));
    check(!cwd_acquire_frame(0,saved,saved,0));check(!cwd_acquire_frame(6,saved,saved,0));
    std::cout<<count<<" cwd acquire checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
