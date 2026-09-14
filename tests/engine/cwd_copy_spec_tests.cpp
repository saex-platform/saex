#include "saex/engine/cwd_copy_policy.generated.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main(){try {
    unsigned count{};const auto check=[&](bool value){++count;if(!value)throw std::runtime_error("cwd copy check "+std::to_string(count));};
    const auto original=reviewed_cwd_copy_spec;const auto query=reviewed_cwd_query_spec;const auto lock=reviewed_cwd_lock_spec;
    CwdCopyCode code{};check(cwd_copy_code(original,query,lock,code));
    for(auto address:{0U,4095U,UINT32_MAX,query.helper_rva,query.cookie_rva}){auto s=original;s.copier_rva=address;check(!cwd_copy_code(s,query,lock,code));}
    auto q=query;q.helper_rva=lock.selector.image_size-246;check(!cwd_copy_code(original,q,lock,code));
    q=query;q.return_prefix[0]^=std::byte{1};check(!cwd_copy_code(original,q,lock,code));
    check(cwd_copy_code(original,query,lock,code));std::uint32_t displacement{};
    for(unsigned i=0;i<4;++i)displacement|=std::to_integer<std::uint32_t>(code.call[9+i])<<(i*8);
    check(query.helper_rva+231+displacement==original.copier_rva);
    auto moved=lock;moved.selector.image_base+=65536;CwdCopyCode other{};check(cwd_copy_code(original,query,moved,other));
    check(code.checks==other.checks && code.call==other.call && code.entry==other.entry && code.body==other.body);
    std::array<std::byte,136> before{};before.fill(std::byte{0xa5});
    for(unsigned length=3;length<=126;++length) {
        std::array<std::byte,260> source{};source.fill(std::byte{0x55});
        for(unsigned i=0;i<length;++i)source[i]=std::byte('A'+i%23);
        source[length]=std::byte{};
        auto after=before;std::copy_n(source.begin(),length+1,after.begin()+4);
        check(cwd_copy_buffer(before,before,source,length,false));check(cwd_copy_buffer(before,after,source,length,true));
        check(!cwd_copy_buffer(before,after,source,length,false));check(!cwd_copy_buffer(before,before,source,length,true));
        for(unsigned i=0;i<after.size();++i){after[i]^=std::byte{1};check(!cwd_copy_buffer(before,after,source,length,true));after[i]^=std::byte{1};}
        source[length]=std::byte{1};check(!cwd_copy_buffer(before,after,source,length,true));source[length]=std::byte{};
        source[length/2]=std::byte{};check(!cwd_copy_buffer(before,after,source,length,true));
    }
    std::array<std::byte,260> empty{};for(auto length:{0U,2U,127U,260U,UINT32_MAX})check(!cwd_copy_buffer(before,before,empty,length,false));
    EventDispatchRegisters helper{};helper.esp=0x200100;helper.ebp=helper.esp+44;
    const auto source=helper.esp-284;const std::uint32_t destination=0xb71ae0;
    for(unsigned length:{3U,4U,15U,31U,83U,85U,125U,126U})for(unsigned stage=1;stage<=5;++stage) {
        EventDispatchRegisters api{};api.esp=helper.esp-288;api.ebp=helper.esp-16;api.eax=length;api.edx=0x1234;api.edi=24;api.flags=0x202;
        auto r=api;r.esp=stage<=2?helper.esp-288:stage==4?helper.esp-300:helper.esp-296;
        r.eax=stage<=2?length+1:stage==5?destination:source;r.ecx=destination;
        const auto value=stage==1?destination:length+1-128;
        r.flags=0x202U|(std::popcount(value&255U)%2?0U:4U)|(stage==1?0U:0x81U);
        if(stage==5){r.ecx=0xeeee;r.edx=0xffff;r.flags=0x202;}
        const auto valid=[&](auto v){return cwd_copy_frame(stage,helper,api,v,source,destination,length);};
        check(valid(r));
        for(auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::ebp,&EventDispatchRegisters::eax,&EventDispatchRegisters::ebx,&EventDispatchRegisters::esi,&EventDispatchRegisters::edi}){auto v=r;v.*member^=1;check(!valid(v));}
        for(auto member:{&EventDispatchRegisters::ecx,&EventDispatchRegisters::edx}){auto v=r;v.*member^=1;check(valid(v)==(stage==5));}
        for(unsigned bit:{0x100U,0x400U,0x200U,0x3000U,0x20000U}){auto v=r;v.flags^=bit;check(!valid(v));}
        if(stage<5){auto v=r;v.flags^=64;check(!valid(v));}
        auto v=r;v.flags^=0x10000;check(valid(v));
        check(!cwd_copy_frame(stage,helper,api,r,source+1,destination,length));
        check(!cwd_copy_frame(stage,helper,api,r,source,source+128,length));
        check(!cwd_copy_frame(stage,helper,api,r,source,UINT32_MAX-126,length));
        check(!cwd_copy_frame(0,helper,api,r,source,destination,length));check(!cwd_copy_frame(6,helper,api,r,source,destination,length));
    }
    std::cout<<count<<" cwd copy checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
