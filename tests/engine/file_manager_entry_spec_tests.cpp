#include "saex/engine/file_manager_entry_policy.generated.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main() {
    try {
        unsigned count{};const auto check=[&](bool v){++count;if(!v)throw std::runtime_error("file manager check "+std::to_string(count));};
        const auto original=reviewed_file_manager_spec;const auto p=reviewed_prelude_spec;const auto r=reviewed_routing_spec;
        const auto valid=[&](const auto& s){return valid_file_manager_entry_spec(s,p,r);};check(valid(original));
        for(auto member:{&FileManagerEntrySpec::buffer_rva,&FileManagerEntrySpec::cwd_rva,&FileManagerEntrySpec::suffix_rva})
            for(auto v:{0U,4095U,UINT32_MAX,original.manager.target_rva}){auto s=original;s.*member=v;check(!valid(s));}
        for(std::size_t i=0;i<16;++i){auto s=original;s.manager.target_prefix[i]^=std::byte{1};check(!valid(s));}
        for(std::size_t i=0;i<11;++i){auto s=original;s.return_prefix[i]^=std::byte{1};check(!valid(s));}
        auto s=original;s.return_prefix[15]^=std::byte{1};check(valid(s)); // Full sample is a runtime guard.
        s=original;s.manager.call_rva++;check(!valid(s));s=original;s.manager.image_base+=65536;check(!valid(s));
        s=original;s.buffer_rva=4100;std::array<std::byte,51> body{};check(file_manager_body(s,body));
        s.buffer_rva=4099;check(!file_manager_body(s,body));s=original;s.buffer_rva=s.manager.image_size-132;check(file_manager_body(s,body));
        ++s.buffer_rva;check(!file_manager_body(s,body));
        auto relocated=original;auto rp=r;auto pp=p;relocated.manager.image_base+=65536;rp.initializer.image_base+=65536;pp.empty.image_base+=65536;pp.localisation.image_base+=65536;
        std::array<std::byte,20> prelude_body{};check(game_prelude_body(pp,prelude_body));std::copy_n(prelude_body.begin(),16,pp.localisation.target_prefix.begin());
        check(file_manager_body(relocated,body));std::copy_n(body.begin(),16,relocated.manager.target_prefix.begin());check(valid_file_manager_entry_spec(relocated,pp,rp));
        // Data/code overlap with a correctly rebuilt prefix must still fail.
        for(auto v:{p.flags_rva,p.empty.target_rva+4,original.manager.target_rva+4,original.suffix_rva+4}) {
            s=original;s.buffer_rva=v;check(file_manager_body(s,body));std::copy_n(body.begin(),16,s.manager.target_prefix.begin());check(!valid(s));
        }
        const EventDispatchRegisters saved{0x100000,0,0,0xabc,0xdef,0,24,0x77,0x246};
        for(std::uint32_t stage=1;stage<=2;++stage) {
            auto current=saved;current.esp-=stage==1?4:16;
            const auto base=original.manager.image_base,ret=base+original.manager.call_rva+5,outer=base+r.initializer.call_rva+5;
            std::array<std::uint32_t,5> words{base+original.buffer_rva,128,saved.edi,ret,outer};
            if(stage==1){words[0]=ret;words[1]=outer;}
            const auto stack=std::span(words).first(stage==1?2U:5U);
            check(file_manager_entry_frame(original,p,r,stage,saved,current,stack));
            for(auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::eax,&EventDispatchRegisters::ebx,&EventDispatchRegisters::ecx,&EventDispatchRegisters::edx,&EventDispatchRegisters::esi,&EventDispatchRegisters::edi,&EventDispatchRegisters::ebp}) {
                auto bad=current;bad.*member^=1;check(!file_manager_entry_frame(original,p,r,stage,saved,bad,stack));
            }
            for(auto flag:{0x100U,0x400U,0x200U,1U}) {auto bad=current;bad.flags^=flag;check(!file_manager_entry_frame(original,p,r,stage,saved,bad,stack));}
            auto rf=current;rf.flags^=0x10000U;check(file_manager_entry_frame(original,p,r,stage,saved,rf,stack));
            for(auto& word:stack){word^=1;check(!file_manager_entry_frame(original,p,r,stage,saved,current,stack));word^=1;}
            check(!file_manager_entry_frame(original,p,r,stage,saved,current,{}));
            auto bad_saved=saved;bad_saved.flags|=0x100U;check(!file_manager_entry_frame(original,p,r,stage,bad_saved,current,stack));
        }
        check(!file_manager_entry_frame(original,p,r,0,saved,saved,{}));check(!file_manager_entry_frame(original,p,r,3,saved,saved,{}));
        std::cout<<count<<" file manager checks passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
