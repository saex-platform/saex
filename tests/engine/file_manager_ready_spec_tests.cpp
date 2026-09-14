#include "saex/engine/file_manager_ready_policy.generated.hpp"
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main(){try {
    unsigned count{};const auto check=[&](bool ok){if(!ok)throw std::runtime_error("ready assertion "+std::to_string(count));++count;};
    const auto valid=[&](const auto& s){FileManagerReadyCode c{};return file_manager_ready_code(s,reviewed_cwd_return_spec,reviewed_cwd_lock_spec,reviewed_cwd_seh_spec,reviewed_file_manager_spec,c);};
    const auto spec=reviewed_file_manager_ready_spec;check(valid(spec));
    for(auto member:{&FileManagerReadySpec::unlock_rva,&FileManagerReadySpec::iat_rva,&FileManagerReadySpec::epilogue_rva})
        for(auto r:{0U,4095U,UINT32_MAX,reviewed_file_manager_spec.buffer_rva,reviewed_cwd_seh_spec.cleanup_rva}){auto bad=spec;bad.*member=r;check(!valid(bad));}
    auto bad=spec;bad.function.highlow_mask=1;check(!valid(bad));bad=spec;bad.function.rva=UINT32_MAX;check(!valid(bad));
    EventDispatchRegisters h{};h.esp=0x200100;h.ebp=h.esp+44;h.flags=0x202;
    EventDispatchRegisters caller{};caller.esp=h.esp+68;caller.ebp=0x210000;caller.edi=24;caller.flags=0x202;
    const std::uint32_t root=0xb71ae0;
    const std::array<std::uint32_t,9> stacks{h.esp-16,h.esp-20,h.esp-12,h.esp,h.esp+48,h.esp+52,h.esp+60,h.esp+60,h.esp+68};
    for(unsigned length:{3U,125U,126U})for(unsigned stage=1;stage<=9;++stage) {
        auto r=caller;r.esp=stacks[stage-1];r.ebp=stage<=3?h.esp-12:stage==4?h.ebp:caller.ebp;
        r.edi=stage==7 || stage==8?root+length:24;
        r.eax=stage<=2?7:stage==3?0xdeadbeef:stage<=6?root:stage==7?root&0xffffff00U:(root&0xffff0000U)|0x5c;
        const auto frame=[&](const auto& v){return file_manager_ready_frame(stage,h,caller,v,root,length);};check(frame(r));
        for(auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::ebp,&EventDispatchRegisters::edi,&EventDispatchRegisters::ebx,&EventDispatchRegisters::esi}){auto v=r;v.*member^=1;check(!frame(v));}
        auto v=r;v.eax^=1;check(frame(v)==(stage==3));
        for(auto flag:{0x100U,0x200U,0x400U,0x2000U}){v=r;v.flags^=flag;check(!frame(v));}
        v=r;v.ecx=0xabababab;v.edx=UINT32_MAX;check(frame(v));
        check(!file_manager_ready_frame(0,h,caller,r,root,length));check(!file_manager_ready_frame(10,h,caller,r,root,length));
    }
    for(unsigned length=3;length<=126;++length) {
        std::array<std::byte,136> before{};before.fill(std::byte{0x66});
        for(unsigned i=0;i<length;++i) { before[i+4]=std::byte{0x61}; }
        before[length+4]=std::byte{};
        check(file_manager_ready_buffer(before,before,length,false));auto after=before;after[length+4]=std::byte{0x5c};after[length+5]=std::byte{};
        check(file_manager_ready_buffer(before,after,length,true));check(!file_manager_ready_buffer(before,before,length,true));
        for(unsigned at:{0U,3U,4U,length+4,length+5,132U,135U}){auto changed=after;changed[at]^=std::byte{1};check(!file_manager_ready_buffer(before,changed,length,true));}
        check(!file_manager_ready_buffer(before,after,127,true));
    }
    std::cout<<count<<" file manager ready checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
