#include "saex/engine/startup_return.hpp"
namespace saex::engine {
bool valid_startup_return_spec(const StartupReturnSpec& s) noexcept {
    if (!valid_frame_target_spec(s.frame)) return false;
    const std::array<std::uint32_t,4> starts{s.protect_call_rva,s.protect_slot_rva,s.startup_slot_rva,s.forward_rva};
    constexpr std::array<std::uint32_t,4> lengths{22,4,4,6};
    for (std::size_t i=0;i<starts.size();++i) {
        if (!starts[i] || starts[i] > 16U*1024*1024-lengths[i]) return false;
        for (std::size_t j=0;j<i;++j) if (starts[i]<starts[j]+lengths[j] && starts[j]<starts[i]+lengths[i]) return false;
    }
    if (s.protect_slot_rva%4 || s.startup_slot_rva%4) return false;
    for (const auto* f:{&s.protect_function,&s.startup_function}) {
        std::array<std::byte,20> normalized{};
        if (!f->rva || f->rva>16U*1024*1024-20 || !bootstrap_export_prefix(*f,f->preferred_base,normalized)) return false;
    }
    return s.protect_function.rva+20<=s.startup_function.rva || s.startup_function.rva+20<=s.protect_function.rva;
}
}
