#include "saex/engine/application_entry_policy.generated.hpp"
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main() {
    try {
        unsigned n{};
        const auto check=[&](bool v) { ++n; if (!v) throw std::runtime_error("application spec failure"); };
        const auto valid=[](const auto& s) { return valid_application_entry_spec(s,reviewed_crt_spec); };
        check(valid(reviewed_application_spec));
        auto s=reviewed_application_spec; s.initializer_body={}; check(!valid(s));
        std::array<std::byte,257> bytes{}; s.initializer_body=bytes; check(!valid(s));
        s=reviewed_application_spec; s.initializer_body=std::span(bytes).first(16); check(!valid(s));
        for (auto r:{0U,UINT32_MAX,reviewed_crt_spec.initialize.call_rva,reviewed_crt_tables[0].rva}) {
            s=reviewed_application_spec; s.second_call_rva=r; check(!valid(s));
        }
        for (unsigned i:{0,5,11,19,27}) { s=reviewed_application_spec; s.reentry_normalized[i]^=std::byte{1}; check(!valid(s)); }
        s=reviewed_application_spec; s.once_rva=s.reentry_rva; check(!valid(s));
        s=reviewed_application_spec; s.once_rva=UINT32_MAX; check(!valid(s));
        s=reviewed_application_spec; s.reentry_rva=UINT32_MAX; check(!valid(s));
        std::array<std::byte,31> a{},b{};
        check(application_reentry_prefix(reviewed_application_spec,0x10000000,0x4028,a));
        check(application_reentry_prefix(reviewed_application_spec,0x20000000,0x4028,b));
        for (unsigned i:{8,22,30}) check(a[i]==std::byte{0x10} && b[i]==std::byte{0x20});
        check(!application_reentry_prefix(reviewed_application_spec,0x10000001,0x4028,a));
        check(!application_reentry_prefix(reviewed_application_spec,0xff000000,0x4028,a));
        check(!application_reentry_prefix(reviewed_application_spec,0x10000000,1,a));
        std::cout<<n<<" application spec checks passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
