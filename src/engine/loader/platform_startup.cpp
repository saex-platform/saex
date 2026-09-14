#include "saex/engine/platform_startup.hpp"
#include <algorithm>
namespace saex::engine {
bool valid_platform_startup_spec(const PlatformStartupSpec& s,const CrtStartupSpec& crt) noexcept {
    if (s.module.empty() || s.module.size()>127 || !std::all_of(s.module.begin(),s.module.end(),[](char c) {
        return (c>='a' && c<='z') || (c>='0' && c<='9') || c=='_' || c=='.' || c=='-'; })) return false;
    if (!valid_crt_startup_spec(crt) || s.stack_bytes!=152 || s.prologue.size()<16 || s.prologue.size()>256 ||
        s.prologue.size()>crt.application.image_size-crt.application.target_rva ||
        s.call_rva!=crt.application.target_rva+s.prologue.size() || s.call_rva>crt.application.image_size-22 ||
        s.iat_rva<4096 || s.iat_rva%4 || s.iat_rva>crt.application.image_size-4 ||
        !std::equal(crt.application.target_prefix.begin(),crt.application.target_prefix.end(),s.prologue.begin())) return false;
    const auto start=crt.application.target_rva,end=s.call_rva+22;
    if (s.iat_rva<end && start<s.iat_rva+4) return false;
    for (const auto& table:crt.tables) if ((start<table.rva+table.targets.size()*4 && table.rva<end) ||
        (s.iat_rva<table.rva+table.targets.size()*4 && table.rva<s.iat_rva+4)) return false;
    for (const auto span:std::array<std::array<std::uint32_t,2>,5>{{
        {crt.io.call_rva,21},{crt.io.target_rva,16},{crt.initialize.call_rva,21},
        {crt.initialize.target_rva,16},{crt.application.call_rva,5}}}) {
        if ((start<span[0]+span[1] && span[0]<end) || (s.iat_rva<span[0]+span[1] && span[0]<s.iat_rva+4)) return false;
    }
    std::array<std::byte,20> prefix{};
    return s.function.rva && s.function.rva<16U*1024*1024-20 && s.function.preferred_base &&
        s.function.preferred_base%65536==0 && bootstrap_export_prefix(s.function,0x10000000,prefix);
}
}
