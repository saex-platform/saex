#include "saex/engine/game_prelude_policy.generated.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main() {
    try {
        unsigned n{};const auto check=[&](bool v){++n;if(!v)throw std::runtime_error("prelude check "+std::to_string(n));};
        const auto p=reviewed_routing_spec;const auto original=reviewed_prelude_spec;
        const auto valid=[&](const auto& s){return valid_game_prelude_spec(s,p);};check(valid(original));
        for (auto v:{0U,4095U,4099U,UINT32_MAX,p.initializer.target_rva}) {auto s=original;s.flags_rva=v;check(!valid(s));}
        for (std::size_t i=0;i<16;++i) {auto s=original;s.empty.target_prefix[i]^=std::byte{1};check(!valid(s));}
        for (std::size_t i=0;i<16;++i) {auto s=original;s.localisation.target_prefix[i]^=std::byte{1};check(!valid(s));}
        auto s=original;s.empty.call_rva++;check(!valid(s));s=original;s.localisation.call_rva++;check(!valid(s));
        s=original;s.stop_prefix[0]^=std::byte{1};check(!valid(s));
        s=original;s.stop_prefix[15]^=std::byte{1};check(valid(s)); // Runtime exact sample rejects drift.
        auto parent=p;auto moved=original;parent.initializer.image_base+=65536;moved.empty.image_base+=65536;moved.localisation.image_base+=65536;
        std::array<std::byte,20> code{};check(game_prelude_body(moved,code));std::copy_n(code.begin(),16,moved.localisation.target_prefix.begin());check(valid_game_prelude_spec(moved,parent));
        const EventDispatchRegisters saved{0x100000,5,0,0xabc,0xdef,0,24,0x77,0x293};
        std::array<std::byte,16> before{};for (unsigned i=0;i<16;++i) before[i]=std::byte(i+0x11);
        for (std::uint32_t stage=1;stage<=5;++stage) {
            const bool nested=stage==2 || stage==4;auto r=saved;r.esp-=nested ? 8 : 4;
            if (stage==5) {r.eax=0;r.flags=0x246;}
            std::array<std::uint32_t,2> words{p.initializer.image_base+p.initializer.call_rva+5,0};
            if (nested) {words[1]=words[0];words[0]=p.initializer.image_base+(stage==2 ? original.empty.call_rva : original.localisation.call_rva)+5;}
            const auto stack=std::span(words).first(nested ? 2U : 1U);check(game_prelude_frame(original,p,stage,saved,r,stack));
            for (auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::eax,&EventDispatchRegisters::ebx,&EventDispatchRegisters::ecx,&EventDispatchRegisters::edx,&EventDispatchRegisters::esi,&EventDispatchRegisters::edi,&EventDispatchRegisters::ebp}) {
                auto bad=r;bad.*member^=1;check(!game_prelude_frame(original,p,stage,saved,bad,stack));
            }
            for (auto flag:{0x100U,0x400U,0x200U,1U}) {auto bad=r;bad.flags^=flag;check(!game_prelude_frame(original,p,stage,saved,bad,stack));}
            auto rf=r;rf.flags^=0x10000U;check(game_prelude_frame(original,p,stage,saved,rf,stack));
            for (auto& word:stack) {word^=1;check(!game_prelude_frame(original,p,stage,saved,r,stack));word^=1;}
            check(!game_prelude_frame(original,p,stage,saved,r,{}));
            auto now=before;if(stage==5) {now[4]=std::byte{1};now[5]=now[6]=std::byte{};}
            check(game_prelude_flags(stage,before,now));
            for (auto& b:now) {b^=std::byte{1};check(!game_prelude_flags(stage,before,now));b^=std::byte{1};}
        }
        check(!game_prelude_frame(original,p,0,saved,saved,{}));check(!game_prelude_frame(original,p,6,saved,saved,{}));
        check(!game_prelude_flags(0,before,before));check(!game_prelude_flags(6,before,before));
        std::cout<<n<<" game prelude checks passed\n";return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
