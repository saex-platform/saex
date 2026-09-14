#include "saex/engine/cwd_query_policy.generated.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main(){try{
    unsigned count{};const auto check=[&](bool v){++count;if(!v)throw std::runtime_error("cwd query check "+std::to_string(count));};
    const auto original=reviewed_cwd_query_spec;const auto parent=reviewed_cwd_lock_spec;
    const auto valid=[&](const auto& s){return valid_cwd_query_spec(s,parent);};check(valid(original));
    for(auto member:{&CwdQuerySpec::helper_rva,&CwdQuerySpec::iat_rva,&CwdQuerySpec::cookie_rva,&CwdQuerySpec::thunk_slot_rva})
        for(auto value:{0U,4095U,UINT32_MAX}){auto s=original;s.*member=value;check(!valid(s));}
    for(auto member:{&CwdQuerySpec::iat_rva,&CwdQuerySpec::cookie_rva}){auto s=original;s.*member=original.helper_rva+1;check(!valid(s));}
    auto s=original;s.cookie_rva=s.iat_rva;check(!valid(s));s=original;s.function.highlow_mask=0;check(!valid(s));
    s=original;s.implementation.highlow_mask=1U<<19;check(!valid(s));
    for(unsigned i=0;i<12;++i){s=original;s.function.prefix[i]^=std::byte{1};check(!valid(s));}
    std::array<std::byte,18> w{},q{},w2{},q2{};std::array<std::byte,28> h{},h2{};
    check(cwd_query_code(original,parent,w,h,q));auto relocated=parent;relocated.selector.image_base+=65536;
    check(cwd_query_code(original,relocated,w2,h2,q2));check(w==w2 && h!=h2 && q!=q2);
    for(const auto path:{"","C:","C:relative","\\\\server\\share","C:/path","C:\\bad*","C:\\bad:rest","C:\\bad\"","C:\\bad\x80"})check(!cwd_query_directory(path));
    check(cwd_query_directory("C:\\"));check(cwd_query_directory("C:\\dir name"));
    std::array<std::byte,260> bytes{};
    for(unsigned n=3;n<260;++n) {
        std::string path="C:\\"+std::string(n-3,'a');bytes.fill(std::byte{0xa5});
        std::transform(path.begin(),path.end(),bytes.begin(),[](char c){return std::byte(static_cast<unsigned char>(c));});bytes[n]=std::byte{};
        check(cwd_query_result(n,bytes,path)==(n<=126?"":"cwd_query_suffix_capacity"));
    }
    const std::string path="C:\\example";
    bytes.fill(std::byte{0xa5});std::transform(path.begin(),path.end(),bytes.begin(),[](char c){return std::byte(static_cast<unsigned char>(c));});bytes[path.size()]=std::byte{};
    const auto length=static_cast<std::uint32_t>(path.size());
    check(cwd_query_result(length,bytes,path).empty());
    check(cwd_query_result(0,bytes,path)=="cwd_query_directory_failed");
    for(auto n:{260U,261U,UINT32_MAX})check(cwd_query_result(n,bytes,path)=="cwd_query_directory_truncated");
    check(cwd_query_result(length,bytes,"C:\\other")=="cwd_query_directory_mismatch");
    check(cwd_query_result(length,bytes,"relative")=="cwd_query_expected_directory");
    for(unsigned i=0;i<length;++i){auto bad=bytes;bad[i]=std::byte{};check(cwd_query_result(length,bad,path)=="cwd_query_directory_termination");}
    auto bad=bytes;bad[length]=std::byte{1};check(cwd_query_result(length,bad,path)=="cwd_query_directory_termination");
    bad=bytes;bad[3]=std::byte{0x80};check(cwd_query_result(length,bad,path)=="cwd_query_directory_encoding");
    const EventDispatchRegisters saved{0x100000,0xabc,0,0x123,0x456,0,24,0x10002c,0x202};
    constexpr unsigned cookie=0xaabbccdd,ret=0x836eb6;
    for(unsigned stage=1;stage<=7;++stage) {
        auto r=saved;r.ecx=7;r.flags=0x246;
        r.esp=stage==1?saved.esp-8:stage==2?saved.esp-12:stage==5?saved.esp-296:stage==6?saved.esp-300:saved.esp-288;
        if(stage>=3)r.ebp=saved.esp-16;
        if(stage==3 || stage==4)r.eax=cookie^ret;
        if(stage==5 || stage==6)r.eax=saved.esp-284;
        if(stage==7){r.eax=length;r.ecx=0xeeee;r.edx=0xffff;r.flags=0x202;}
        check(cwd_query_frame(stage,saved,r,cookie,ret));
        for(auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::ebp,&EventDispatchRegisters::ebx,&EventDispatchRegisters::esi,&EventDispatchRegisters::edi}){
            auto v=r;v.*member^=1;check(!cwd_query_frame(stage,saved,v,cookie,ret));
        }
        for(auto member:{&EventDispatchRegisters::eax,&EventDispatchRegisters::ecx,&EventDispatchRegisters::edx}){
            auto v=r;v.*member^=1;check(cwd_query_frame(stage,saved,v,cookie,ret)==(stage==7));
        }
        for(auto flag:{0x100U,0x400U,0x200U,0x3000U,0x20000U}){auto v=r;v.flags^=flag;check(!cwd_query_frame(stage,saved,v,cookie,ret));}
        if(stage==3 || stage==4){auto v=r;v.flags^=64;check(!cwd_query_frame(stage,saved,v,cookie,ret));}
        auto v=r;v.flags^=0x10000;check(cwd_query_frame(stage,saved,v,cookie,ret));
        std::array<std::uint32_t,21> before{};before.fill(0xdeadbeef);auto after=before;
        after[1]=0;after[2]=0xb71ae0;after[3]=128;after[13]=0;if(stage>=2)after[0]=ret;
        check(cwd_query_stack(stage,before,after,0xb71ae0,ret));
        for(auto& word:after){word^=1;check(!cwd_query_stack(stage,before,after,0xb71ae0,ret));word^=1;}
    }
    check(!cwd_query_frame(0,saved,saved,cookie,ret));check(!cwd_query_frame(8,saved,saved,cookie,ret));
    std::cout<<count<<" cwd query checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
