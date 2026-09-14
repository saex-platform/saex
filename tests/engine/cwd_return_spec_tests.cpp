#include "saex/engine/cwd_return_policy.generated.hpp"
#include <bit>
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main(){try{
    unsigned count{};const auto check=[&](bool v){++count;if(!v)throw std::runtime_error("cwd return assertion "+std::to_string(count));};
    CwdReturnCode code{};auto spec=reviewed_cwd_return_spec;
    const auto valid=[&](const auto& s){return cwd_return_code(s,reviewed_cwd_copy_spec,reviewed_cwd_query_spec,reviewed_cwd_lock_spec,code);};
    check(valid(spec));
    for(auto r:{0U,4095U,UINT32_MAX,reviewed_cwd_query_spec.helper_rva,reviewed_cwd_query_spec.cookie_rva,reviewed_cwd_copy_spec.copier_rva,reviewed_cwd_lock_spec.selector.call_rva+23}){auto s=spec;s.checker_rva=r;check(!valid(s));}
    auto copy=reviewed_cwd_copy_spec;copy.return_prefix[0]^=std::byte{1};check(!cwd_return_code(spec,copy,reviewed_cwd_query_spec,reviewed_cwd_lock_spec,code));
    EventDispatchRegisters h{};h.esp=0x200100;h.ebp=h.esp+44;
    EventDispatchRegisters c{};c.esp=h.esp-296;c.ebp=h.esp-16;c.eax=0xb71ae0;c.ecx=0x1111;c.edx=0xabcd;c.edi=24;c.flags=0x202;
    for(auto cookie:{0U,1U,0xaabbccddU,0x80000000U,UINT32_MAX})for(unsigned stage=1;stage<=6;++stage) {
        auto r=c;r.esp=stage==6?h.esp-8:(stage==2 || stage==5)?h.esp-284:h.esp-288;
        r.ebp=stage==6?h.ebp:c.ebp;r.ecx=stage==1?h.esp-284:cookie;
        if(stage==2 || stage==3)r.flags=0x202U|(std::popcount(cookie&255U)%2?0U:4U)|(cookie?0U:64U)|((cookie>>31)*128U);
        if(stage>=4)r.flags=0x246;
        const auto frame=[&](auto v){return cwd_return_frame(stage,h,c,v,cookie,c.eax);};check(frame(r));
        for(auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::ebp,&EventDispatchRegisters::eax,&EventDispatchRegisters::ebx,&EventDispatchRegisters::ecx,&EventDispatchRegisters::edx,&EventDispatchRegisters::esi,&EventDispatchRegisters::edi}){auto v=r;v.*member^=1;check(!frame(v));}
        for(unsigned bit:{1U,4U,64U,128U,0x100U,0x200U,0x400U,0x800U,0x2000U,0x20000U}){auto v=r;v.flags^=bit;check(!frame(v));}
        auto v=r;v.flags^=0x10000;check(frame(v));v=r;v.flags^=0x10;check(frame(v)==(stage==2 || stage==3));
        check(!cwd_return_frame(0,h,c,r,cookie,c.eax));check(!cwd_return_frame(7,h,c,r,cookie,c.eax));
        auto bad=h;bad.esp=280;check(!cwd_return_frame(stage,bad,c,r,cookie,c.eax));bad=h;bad.esp=UINT32_MAX;check(!cwd_return_frame(stage,bad,c,r,cookie,c.eax));
        auto old=c;old.flags|=0x400;check(!cwd_return_frame(stage,h,old,r,cookie,c.eax));
    }
    std::cout<<count<<" cwd return checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
