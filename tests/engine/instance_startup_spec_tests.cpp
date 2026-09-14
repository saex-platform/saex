#include "saex/engine/instance_startup_policy.generated.hpp"
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main() {
    try {
        unsigned n{};
        const auto check=[&](bool v) { ++n; if (!v) throw std::runtime_error("instance spec failure "+std::to_string(n)); };
        const auto valid=[](const auto& s) { return valid_instance_startup_spec(s,reviewed_platform_spec,reviewed_crt_spec,reviewed_suppression_spec); };
        const auto original=reviewed_instance_spec; check(valid(original));
        for (std::size_t i=0;i<original.normalized_body.size();++i) {
            auto s=original; s.normalized_body[i]^=std::byte{1}; check(!valid(s));
        }
        for (std::size_t i=0;i<9;++i) {
            auto s=original; s.body_address_rvas[i]=UINT32_MAX; check(!valid(s));
            s=original; s.body_address_rvas[i]=s.caller.target_rva; check(!valid(s));
        }
        for (const auto name:{"","a\nb","a\0b"}) {
            auto s=original; s.name=std::string_view(name,name[0]=='a' ? 3 : 0); check(!valid(s));
        }
        auto s=original; s.name_rva=UINT32_MAX; check(!valid(s));
        s=original; s.caller.call_rva++; check(!valid(s));
        s=original; s.caller.call[0]=std::byte{0x90}; check(!valid(s));
        s=original; s.caller.target_prefix[15]^=std::byte{1}; check(!valid(s));
        s=original; s.create_thunk_rva=0; check(!valid(s));
        s=original; s.create_thunk_slot_rva++; check(!valid(s));
        s=original; s.create_thunk_slot_rva=s.create_thunk_rva; check(!valid(s));
        s=original; s.create_function.highlow_mask=1; check(!valid(s));
        s=original; s.create_function.preferred_base=1; check(!valid(s));
        std::array<std::byte,91> a{},b{};
        check(instance_body(original,original.caller.image_base,a));
        check(instance_body(original,0x10000000,b));
        check(a[4]==std::byte{0} && b[4]==std::byte{0x10});
        for (auto base:{0U,1U,0xffff0000U}) check(!instance_body(original,base,b));
        s=original; s.caller.image_size=0; check(!instance_body(s,0x400000,b));
        std::cout<<n<<" instance spec checks passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
