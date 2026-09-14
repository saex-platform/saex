#include "saex/engine/platform_startup_policy.generated.hpp"
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main() {
    try {
        unsigned n{};
        const auto check=[&](bool v) { ++n; if (!v) throw std::runtime_error("platform spec failure"); };
        const auto valid=[](const auto& s) { return valid_platform_startup_spec(s,reviewed_crt_spec); };
        check(valid(reviewed_platform_spec));
        auto s=reviewed_platform_spec; s.stack_bytes=0; check(!valid(s));
        s=reviewed_platform_spec; s.prologue={}; check(!valid(s));
        s=reviewed_platform_spec; s.call_rva++; check(!valid(s));
        for (auto r:{0U,1U,UINT32_MAX,reviewed_crt_tables[0].rva,reviewed_crt_spec.application.target_rva}) {
            s=reviewed_platform_spec; s.iat_rva=r; check(!valid(s));
        }
        s=reviewed_platform_spec; s.function.rva=UINT32_MAX; check(!valid(s));
        s=reviewed_platform_spec; s.function.rva=0; check(!valid(s));
        s=reviewed_platform_spec; s.function.preferred_base=0; s.function.highlow_mask=0; check(!valid(s));
        s=reviewed_platform_spec; s.function.highlow_mask=3; check(!valid(s));
        s=reviewed_platform_spec; s.function.preferred_base=1; check(!valid(s));
        s=reviewed_platform_spec; s.module=""; check(!valid(s));
        s=reviewed_platform_spec; s.module="../user32.dll"; check(!valid(s));
        std::array<std::byte,20> a{},b{};
        check(bootstrap_export_prefix(reviewed_platform_spec.function,0x10000000,a));
        check(bootstrap_export_prefix(reviewed_platform_spec.function,0x20000000,b));
        check(a[6]==std::byte{0x10} && b[6]==std::byte{0x20});
        check(platform_startup_arguments==std::array<std::uint32_t,4>{8193,0,0,2});
        std::cout<<n<<" platform spec checks passed\n";return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
