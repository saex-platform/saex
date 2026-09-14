#include "saex/engine/event_dispatch_policy.generated.hpp"
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main() {
    try {
        unsigned n{};const auto check=[&](bool value){++n;if(!value)throw std::runtime_error("dispatch check "+std::to_string(n));};
        const auto valid=[](const auto& s){return valid_event_dispatch_spec(s,reviewed_instance_spec);};
        const auto original=reviewed_dispatch_spec;check(valid(original));
        for (std::size_t i=0;i<16;++i) {auto s=original;s.dispatcher.target_prefix[i]^=std::byte{1};check(!valid(s));}
        for (auto r:{0U,1U,UINT32_MAX,original.dispatcher.target_rva}) {auto s=original;s.dispatcher.call_rva=r;check(!valid(s));}
        auto s=original;s.application.call_rva++;check(!valid(s));
        s=original;s.application.image_base+=65536;check(!valid(s));
        s=original;s.application_prefix[0]^=std::byte{1};check(!valid(s));
        s=original;s.application_prefix[20]^=std::byte{1};check(valid(s)); // Runtime sample must reject this drift.
        const EventDispatchRegisters saved{0x100000,0,0,0xabc,0xdef,0x55,0x66,0x77,0x202};
        const auto ret=original.dispatcher.image_base+original.dispatcher.call_rva+5;
        for (std::uint32_t stage=1;stage<=3;++stage) {
            auto r=saved; r.esp-=stage==1 ? 8 : stage==2 ? 12 : 28;
            std::array<std::uint32_t,7> words{};const std::size_t count=stage==1 ? 2 : stage==2 ? 3 : 7;
            if (stage==1) words={24,0};
            else if (stage==2) words={ret,24,0};
            else {r.esi=0;r.edi=24;words={24,0,saved.edi,saved.esi,ret,24,0};}
            const auto stack=std::span(words).first(count);
            check(event_dispatch_frame(original,stage,saved,r,stack));
            for (auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::eax,&EventDispatchRegisters::ebx,&EventDispatchRegisters::ecx,&EventDispatchRegisters::edx,&EventDispatchRegisters::esi,&EventDispatchRegisters::edi,&EventDispatchRegisters::ebp}) {
                auto bad=r;bad.*member^=1;check(!event_dispatch_frame(original,stage,saved,bad,stack));
            }
            for (auto flag:{0x100U,0x400U}) {auto bad=r;bad.flags|=flag;check(!event_dispatch_frame(original,stage,saved,bad,stack));}
            for (std::size_t i=0;i<count;++i) {words[i]^=1;check(!event_dispatch_frame(original,stage,saved,r,stack));words[i]^=1;}
            check(!event_dispatch_frame(original,stage,saved,r,stack.first(count-1)));
            auto bad=saved;bad.esp=16;check(!event_dispatch_frame(original,stage,bad,r,stack));
            bad=saved;bad.eax=1;check(!event_dispatch_frame(original,stage,bad,r,stack));
        }
        check(!event_dispatch_frame(original,0,saved,saved,{}));check(!event_dispatch_frame(original,4,saved,saved,{}));
        std::cout<<n<<" event dispatch spec/frame checks passed\n";return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
