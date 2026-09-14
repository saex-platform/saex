#include "saex/engine/application_routing.hpp"
#include <algorithm>
namespace saex::engine {
namespace {
bool layout(const ApplicationRoutingSpec& s,const EventDispatchSpec& p) noexcept {
    const auto& f=s.initializer; const auto& a=p.application;
    if (!valid_frame_target_spec(f) || !valid_frame_target_spec(a) || !valid_frame_target_spec(p.dispatcher) ||
        f.image_base!=a.image_base || f.image_size!=a.image_size || p.dispatcher.image_base!=a.image_base ||
        p.dispatcher.image_size!=a.image_size || a.call_rva!=p.dispatcher.target_rva+12 ||
        s.indices[24]!=std::byte{5} || s.target_rvas[5]!=f.call_rva) return false;
    const auto range=[&](std::uint32_t r,std::uint32_t n) {return r>=4096 && n<=f.image_size && r<=f.image_size-n;};
    const std::array<std::array<std::uint32_t,2>,8> spans{{{a.target_rva,27},{s.detour_rva,12},
        {s.index_rva,39},{s.table_rva,44},{f.call_rva,5},{f.target_rva,16},
        {p.dispatcher.target_rva,17},{p.dispatcher.call_rva-7,12}}};
    for (std::size_t i=0;i<spans.size();++i) {
        if (!range(spans[i][0],spans[i][1])) return false;
        for (std::size_t j=0;j<i;++j)
            if (spans[i][0]<spans[j][0]+spans[j][1] && spans[j][0]<spans[i][0]+spans[i][1]) return false;
    }
    for (const auto v:s.indices) if (std::to_integer<unsigned>(v)>=s.target_rvas.size()) return false;
    for (const auto r:s.target_rvas) if (!range(r,1)) return false;
    return true;
}
void word(std::byte* bytes,std::uint32_t value) noexcept {
    for (unsigned i=0;i<4;++i) bytes[i]=std::byte((value>>(i*8))&0xffU);
}
}
bool application_routing_code(const ApplicationRoutingSpec& s,const EventDispatchSpec& p,ApplicationRoutingCode& out) noexcept {
    if (!layout(s,p)) return false;
    const auto r=p.application.target_rva,base=p.application.image_base;
    ApplicationRoutingCode code{};
    constexpr std::array<std::byte,9> prefix{std::byte{0x8b},std::byte{0x44},std::byte{0x24},std::byte{0x04},
        std::byte{0x83},std::byte{0xf8},std::byte{0x26},std::byte{0x0f},std::byte{0x87}};
    std::copy(prefix.begin(),prefix.end(),code.entry.begin());
    word(code.entry.data()+9,s.target_rvas[10]-(r+13));
    code.entry[13]=code.entry[14]=std::byte{0x90}; code.entry[15]=std::byte{0xe9};
    word(code.entry.data()+16,s.detour_rva-(r+20)); code.entry[20]=std::byte{0xff};
    code.detour[0]=std::byte{0x0f};code.detour[1]=std::byte{0xb6};code.detour[2]=std::byte{0x80};
    word(code.detour.data()+3,base+s.index_rva);code.detour[7]=std::byte{0xe9};
    word(code.detour.data()+8,r+20-(s.detour_rva+12));
    code.indirect[0]=std::byte{0xff};code.indirect[1]=std::byte{0x24};code.indirect[2]=std::byte{0x85};
    word(code.indirect.data()+3,base+s.table_rva);
    for (std::size_t i=0;i<code.targets.size();++i) code.targets[i]=base+s.target_rvas[i];
    out=code; return true;
}
bool valid_application_routing_spec(const ApplicationRoutingSpec& s,const EventDispatchSpec& p) noexcept {
    ApplicationRoutingCode code{};
    return application_routing_code(s,p,code) && code.entry==p.application_prefix &&
        std::equal(p.application.target_prefix.begin(),p.application.target_prefix.end(),code.entry.begin());
}
bool application_routing_frame(const ApplicationRoutingSpec& s,const EventDispatchSpec& p,std::uint32_t stage,
    const EventDispatchRegisters& saved,const EventDispatchRegisters& r,std::span<const std::uint32_t> stack) noexcept {
    if (stage<1 || stage>4 || !valid_application_routing_spec(s,p) || saved.esp<4096 || saved.esp%4 ||
        saved.eax || saved.ebx || (saved.flags&0x500U) || (r.flags&0x500U) || r.esp!=saved.esp-32 ||
        r.ebx || r.ecx!=saved.ecx || r.edx!=saved.edx || r.ebp!=saved.ebp || r.esi || r.edi!=24 ||
        r.eax!=(stage==1 ? 0U : stage==2 ? 24U : 5U)) return false;
    const std::array<std::uint32_t,8> expected{p.application.image_base+p.application.call_rva+5,24,0,
        saved.edi,saved.esi,p.dispatcher.image_base+p.dispatcher.call_rva+5,24,0};
    return std::equal(stack.begin(),stack.end(),expected.begin(),expected.end());
}
}
