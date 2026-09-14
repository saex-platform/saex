#include "saex/engine/application_routing_policy.generated.hpp"
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main() {
    try {
        unsigned n{};const auto check=[&](bool v){++n;if(!v)throw std::runtime_error("routing check "+std::to_string(n));};
        const auto p=reviewed_dispatch_spec;const auto original=reviewed_routing_spec;
        const auto valid=[&](const auto& s){return valid_application_routing_spec(s,p);};
        check(valid(original));ApplicationRoutingCode code{};check(application_routing_code(original,p,code));
        check(code.entry==p.application_prefix);check(code.targets[5]==0x53ec2b && code.detour[7]==std::byte{0xe9});
        for (auto member:{&ApplicationRoutingSpec::detour_rva,&ApplicationRoutingSpec::index_rva,&ApplicationRoutingSpec::table_rva})
            for (auto v:{0U,4095U,UINT32_MAX,p.application.target_rva,original.initializer.call_rva}) {auto s=original;s.*member=v;check(!valid(s));}
        for (std::size_t i=0;i<39;++i) {auto s=original;s.indices[i]=std::byte{11};check(!valid(s));}
        for (std::size_t i=0;i<11;++i) {auto s=original;s.target_rvas[i]=UINT32_MAX;check(!valid(s));}
        auto s=original;s.indices[24]=std::byte{6};check(!valid(s));
        s=original;s.target_rvas[5]++;check(!valid(s));
        s=original;s.initializer.call[1]^=std::byte{1};check(!valid(s));
        s=original;s.initializer.image_base+=65536;check(!valid(s));
        auto moved=p;auto relocated=original;
        for (auto* f:{&moved.dispatcher,&moved.application,&relocated.initializer}) f->image_base+=0x10000;
        ApplicationRoutingCode relocated_code{};check(application_routing_code(relocated,moved,relocated_code));
        check(valid_application_routing_spec(relocated,moved));check(relocated_code.entry==code.entry);
        check(relocated_code.targets[5]==code.targets[5]+0x10000 && relocated_code.indirect!=code.indirect && relocated_code.detour!=code.detour);
        const EventDispatchRegisters saved{0x100000,0,0,0xabc,0xdef,0x55,0x66,0x77,0x202};
        for (std::uint32_t stage=1;stage<=4;++stage) {
            auto r=saved;r.esp-=32;r.esi=0;r.edi=24;r.eax=stage==1 ? 0U : stage==2 ? 24U : 5U;
            std::array<std::uint32_t,8> words{p.application.image_base+p.application.call_rva+5,24,0,saved.edi,saved.esi,p.dispatcher.image_base+p.dispatcher.call_rva+5,24,0};
            check(application_routing_frame(original,p,stage,saved,r,words));
            for (auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::eax,&EventDispatchRegisters::ebx,&EventDispatchRegisters::ecx,&EventDispatchRegisters::edx,&EventDispatchRegisters::esi,&EventDispatchRegisters::edi,&EventDispatchRegisters::ebp}) {
                auto bad=r;bad.*member^=1;check(!application_routing_frame(original,p,stage,saved,bad,words));
            }
            for (auto flag:{0x100U,0x400U}) {auto bad=r;bad.flags|=flag;check(!application_routing_frame(original,p,stage,saved,bad,words));}
            for (auto& word:words) {word^=1;check(!application_routing_frame(original,p,stage,saved,r,words));word^=1;}
            check(!application_routing_frame(original,p,stage,saved,r,std::span(words).first(7)));
            auto bad=saved;bad.esp=16;check(!application_routing_frame(original,p,stage,bad,r,words));
            bad=saved;bad.eax=1;check(!application_routing_frame(original,p,stage,bad,r,words));
        }
        check(!application_routing_frame(original,p,0,saved,saved,{}));check(!application_routing_frame(original,p,5,saved,saved,{}));
        std::cout<<n<<" application routing checks passed\n";return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
