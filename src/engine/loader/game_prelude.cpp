#include "saex/engine/game_prelude.hpp"
#include <algorithm>
namespace saex::engine {
bool game_prelude_body(const GamePreludeSpec& s,std::array<std::byte,20>& out) noexcept {
    const auto& f=s.localisation;
    if (!valid_frame_target_spec(f) || s.flags_rva<4100 || s.flags_rva>f.image_size-12) return false;
    std::array<std::byte,20> b{std::byte{0x32},std::byte{0xc0},std::byte{0xc6},std::byte{0x05}};
    b[8]=std::byte{1};b[9]=b[14]=std::byte{0xa2};b[19]=std::byte{0xc3};
    for (unsigned i=0;i<3;++i) {
        const auto a=f.image_base+s.flags_rva+i;const auto offset=i==0 ? 4U : i==1 ? 10U : 15U;
        for (unsigned j=0;j<4;++j) b[offset+j]=std::byte((a>>(j*8))&0xffU);
    }
    out=b;return true;
}
bool valid_game_prelude_spec(const GamePreludeSpec& s,const ApplicationRoutingSpec& p) noexcept {
    const auto& a=s.empty;const auto& b=s.localisation;const auto& f=p.initializer;
    if (!valid_frame_target_spec(f) || !valid_frame_target_spec(a) || !valid_frame_target_spec(b) ||
        a.image_base!=f.image_base || b.image_base!=f.image_base || a.image_size!=f.image_size || b.image_size!=f.image_size ||
        a.call_rva!=f.target_rva || b.call_rva!=a.call_rva+5 || a.target_prefix[0]!=std::byte{0xc3} ||
        !std::all_of(a.target_prefix.begin()+1,a.target_prefix.end(),[](auto v){return v==std::byte{0x90};}) ||
        !std::equal(a.call.begin(),a.call.end(),f.target_prefix.begin()) ||
        !std::equal(b.call.begin(),b.call.end(),f.target_prefix.begin()+5) ||
        !std::equal(s.stop_prefix.begin(),s.stop_prefix.begin()+6,f.target_prefix.begin()+10)) return false;
    std::array<std::byte,20> body{};
    if (!game_prelude_body(s,body) || !std::equal(b.target_prefix.begin(),b.target_prefix.end(),body.begin())) return false;
    const std::array<std::array<std::uint32_t,2>,5> spans{{{f.call_rva,5},{f.target_rva,26},{a.target_rva,16},{b.target_rva,20},{s.flags_rva-4,16}}};
    for (std::size_t i=0;i<spans.size();++i) {
        if (spans[i][0]<4096 || spans[i][0]>f.image_size-spans[i][1]) return false;
        for (std::size_t j=0;j<i;++j) if (spans[i][0]<spans[j][0]+spans[j][1] && spans[j][0]<spans[i][0]+spans[i][1]) return false;
    }
    return true;
}
bool game_prelude_frame(const GamePreludeSpec& s,const ApplicationRoutingSpec& p,std::uint32_t stage,
    const EventDispatchRegisters& saved,const EventDispatchRegisters& r,std::span<const std::uint32_t> stack) noexcept {
    if (stage<1 || stage>5 || !valid_game_prelude_spec(s,p) || saved.esp<4096 || saved.esp%4 || saved.eax!=5 ||
        saved.ebx || saved.esi || saved.edi!=24 || (saved.flags&0x500U) || (r.flags&0x500U) ||
        r.ebx!=saved.ebx || r.ecx!=saved.ecx || r.edx!=saved.edx || r.esi!=saved.esi || r.edi!=saved.edi || r.ebp!=saved.ebp ||
        r.eax!=(stage==5 ? 0U : 5U)) return false;
    if (stage==5 && ((r.flags&0x8c5U)!=0x44U || ((r.flags^saved.flags)&~0x108d5U))) return false; // XOR flags; AF undefined, RF belongs to breakpoint.
    if (stage<5 && ((r.flags^saved.flags)&~0x10000U)) return false; // CPU breakpoint RF is not an application change.
    const auto ret=p.initializer.image_base+p.initializer.call_rva+5;
    if (stage==2 || stage==4) {
        const auto child_ret=p.initializer.image_base+(stage==2 ? s.empty.call_rva : s.localisation.call_rva)+5;
        return r.esp==saved.esp-8 && stack.size()==2 && stack[0]==child_ret && stack[1]==ret;
    }
    return r.esp==saved.esp-4 && stack.size()==1 && stack[0]==ret;
}
bool game_prelude_flags(std::uint32_t stage,const std::array<std::byte,16>& before,const std::array<std::byte,16>& current) noexcept {
    if (stage<1 || stage>5) return false;
    auto expected=before;
    if (stage==5) {expected[4]=std::byte{1};expected[5]=expected[6]=std::byte{};}
    return current==expected;
}
}
