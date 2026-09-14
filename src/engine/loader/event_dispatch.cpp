#include "saex/engine/event_dispatch.hpp"
#include <algorithm>
namespace saex::engine {
bool valid_event_dispatch_spec(const EventDispatchSpec& s,const InstanceStartupSpec& parent) noexcept {
    if (!valid_frame_target_spec(parent.caller) || !valid_frame_target_spec(s.dispatcher) || !valid_frame_target_spec(s.application)) return false;
    const auto& a=s.dispatcher; const auto& b=s.application;
    if (a.image_base!=parent.caller.image_base || a.image_size!=parent.caller.image_size || b.image_base!=a.image_base ||
        b.image_size!=a.image_size || a.call_rva!=parent.caller.call_rva+12 || a.target_rva>a.image_size-17 ||
        b.call_rva!=a.target_rva+12 || b.target_rva>a.image_size-21 ||
        !std::equal(event_dispatch_prologue.begin(),event_dispatch_prologue.end(),a.target_prefix.begin()) ||
        !std::equal(b.call.begin(),b.call.begin()+4,a.target_prefix.begin()+12) ||
        !std::equal(b.target_prefix.begin(),b.target_prefix.end(),s.application_prefix.begin())) return false;
    const std::array<std::array<std::uint32_t,2>,4> spans{{{a.call_rva-7,12},{a.target_rva,17},{b.target_rva,21},{parent.caller.target_rva,91}}};
    for (std::size_t i=0;i<spans.size();++i) for (std::size_t j=0;j<i;++j)
        if (spans[i][0]<spans[j][0]+spans[j][1] && spans[j][0]<spans[i][0]+spans[i][1]) return false;
    return true;
}
bool event_dispatch_frame(const EventDispatchSpec& s,std::uint32_t stage,const EventDispatchRegisters& saved,
    const EventDispatchRegisters& r,std::span<const std::uint32_t> stack) noexcept {
    if (stage<1 || stage>3 || !valid_frame_target_spec(s.dispatcher) || saved.esp<4096 || saved.esp%4 ||
        saved.eax || saved.ebx || (saved.flags&0x500U) || (r.flags&0x500U) || r.eax || r.ebx ||
        r.ecx!=saved.ecx || r.edx!=saved.edx || r.ebp!=saved.ebp) return false;
    const auto ret=s.dispatcher.image_base+s.dispatcher.call_rva+5;
    if (stage==3) {
        const std::array<std::uint32_t,7> expected{24,0,saved.edi,saved.esi,ret,24,0};
        return r.esp==saved.esp-28 && r.esi==0 && r.edi==24 && std::equal(stack.begin(),stack.end(),expected.begin(),expected.end());
    }
    if (r.esi!=saved.esi || r.edi!=saved.edi) return false;
    if (stage==1) return r.esp==saved.esp-8 && stack.size()==2 && stack[0]==24 && stack[1]==0;
    return r.esp==saved.esp-12 && stack.size()==3 && stack[0]==ret && stack[1]==24 && stack[2]==0;
}
}
