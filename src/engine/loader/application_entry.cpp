#include "saex/engine/application_entry.hpp"
#include <algorithm>
namespace saex::engine {
bool application_reentry_prefix(const ApplicationEntrySpec& s,std::uint32_t base,std::uint32_t slot,std::array<std::byte,31>& output) noexcept {
    if (!base || base%65536 || !slot || slot>16U*1024*1024-4 || slot%4 || !s.once_rva || s.once_rva>=16U*1024*1024 ||
        !s.reentry_rva || s.reentry_rva>16U*1024*1024-31 || base>UINT32_MAX-16U*1024*1024 ||
        (s.once_rva>=s.reentry_rva && s.once_rva<s.reentry_rva+31)) return false;
    const auto& b=s.reentry_normalized;
    constexpr std::array<std::uint32_t,15> offsets{0,1,2,3,4,9,10,11,12,17,18,23,24,25,26};
    constexpr std::array<unsigned,15> values{0x55,0x8b,0xec,0x80,0x3d,0,0x75,0x0c,0xe8,0xc6,0x05,1,0x5d,0xff,0x25};
    for (std::size_t i=0;i<offsets.size();++i) if (std::to_integer<unsigned>(b[offsets[i]])!=values[i]) return false;
    for (const auto at:{5U,19U,27U}) for (unsigned i=0;i<4;++i) if (b[at+i]!=std::byte{}) return false;
    std::uint32_t raw{}; for (unsigned i=0;i<4;++i) raw|=std::to_integer<std::uint32_t>(b[13+i])<<(8*i);
    const auto delta=raw<=INT32_MAX ? static_cast<std::int64_t>(raw):static_cast<std::int64_t>(raw)-0x100000000LL;
    const auto target=static_cast<std::int64_t>(s.reentry_rva)+17+delta;
    if (target<=0 || target>=16*1024*1024 || (target>=s.reentry_rva && target<s.reentry_rva+31)) return false;
    output=b;
    for (const auto at:{5U,19U,27U}) {
        const auto value=base+(at==27 ? slot:s.once_rva);
        for (unsigned i=0;i<4;++i) output[at+i]=std::byte((value>>(8*i))&255);
    }
    return true;
}
bool valid_application_entry_spec(const ApplicationEntrySpec& s,const CrtStartupSpec& crt) noexcept {
    if (!valid_crt_startup_spec(crt) || s.initializer_body.size()<16 || s.initializer_body.size()>256 ||
        s.initializer_body.size()>crt.io.image_size-crt.initialize.target_rva || crt.initialize.call_rva>crt.io.image_size-21 ||
        s.second_call_rva<4096 || s.second_call_rva>crt.io.image_size-22) return false;
    if (!std::equal(crt.initialize.target_prefix.begin(),crt.initialize.target_prefix.end(),s.initializer_body.begin())) return false;
    const std::array<std::uint32_t,7> starts{crt.io.call_rva,crt.io.target_rva,crt.initialize.call_rva,crt.initialize.target_rva,crt.application.call_rva,crt.application.target_rva,s.second_call_rva};
    const std::array<std::uint32_t,7> sizes{21,16,21,static_cast<std::uint32_t>(s.initializer_body.size()),5,16,22};
    for (std::size_t i=0;i<starts.size();++i) {
        for (std::size_t j=0;j<i;++j) if (starts[i]<starts[j]+sizes[j] && starts[j]<starts[i]+sizes[i]) return false;
        for (const auto& t:crt.tables) if (starts[i]<t.rva+t.targets.size()*4 && t.rva<starts[i]+sizes[i]) return false;
    }
    std::array<std::byte,31> normalized{};
    return application_reentry_prefix(s,0x10000000,0x1000,normalized);
}
}
