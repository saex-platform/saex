#include "saex/engine/crt_startup.hpp"

namespace saex::engine {
bool valid_crt_startup_spec(const CrtStartupSpec& s) noexcept {
    const auto size=s.io.image_size;
    if (!valid_frame_target_spec(s.io) || !valid_frame_target_spec(s.initialize) || !valid_frame_target_spec(s.application) ||
        s.io.image_base!=s.initialize.image_base || s.io.image_base!=s.application.image_base ||
        size!=s.initialize.image_size || size!=s.application.image_size || s.io.call_rva>size-21 ||
        s.io_return_stack_offset<16 || s.io_return_stack_offset>4096 || s.io_return_stack_offset%4 ||
        s.tables.empty() || s.tables.size()>3) return false;
    const std::array<std::uint32_t,6> starts{s.io.call_rva,s.io.target_rva,s.initialize.call_rva,s.initialize.target_rva,s.application.call_rva,s.application.target_rva};
    constexpr std::array<std::uint32_t,6> lengths{21,16,5,16,5,16};
    const auto overlap=[](std::uint32_t a,std::uint32_t n,std::uint32_t b,std::uint32_t m) { return a<b+m && b<a+n; };
    for (std::size_t i=0;i<starts.size();++i)
        for (std::size_t j=0;j<i;++j) if (overlap(starts[i],lengths[i],starts[j],lengths[j])) return false;
    std::size_t total{};
    for (std::size_t i=0;i<s.tables.size();++i) {
        const auto& table=s.tables[i];
        if (table.targets.empty() || table.targets.size()>2048-total || table.rva<4096 || table.rva%4 || table.rva>size ||
            table.targets.size()>(size-table.rva)/4) return false;
        total+=table.targets.size();
        const auto bytes=static_cast<std::uint32_t>(table.targets.size()*4);
        for (const auto target:table.targets) if (target && (target<4096 || target>=size)) return false;
        for (std::size_t j=0;j<starts.size();++j) if (overlap(table.rva,bytes,starts[j],lengths[j])) return false;
        for (std::size_t j=0;j<i;++j)
            if (overlap(table.rva,bytes,s.tables[j].rva,static_cast<std::uint32_t>(s.tables[j].targets.size()*4))) return false;
    }
    return true;
}
}
