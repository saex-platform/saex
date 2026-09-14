#include "saex/engine/file_manager_entry.hpp"
#include <algorithm>
namespace saex::engine {
bool file_manager_body(const FileManagerEntrySpec& s,std::array<std::byte,51>& out) noexcept {
    const auto& f=s.manager;
    if (!valid_frame_target_spec(f) || s.buffer_rva<4100 || s.buffer_rva>f.image_size-132 ||
        s.suffix_rva<4096 || s.suffix_rva>f.image_size-2 || s.cwd_rva<4096 || s.cwd_rva>f.image_size-16 ||
        f.target_rva>f.image_size-51) return false;
    std::array<std::byte,51> b{};
    constexpr unsigned char raw[]{0x57,0x68,0x80,0,0,0,0x68,0,0,0,0,0xe8,0,0,0,0,
        0xbf,0,0,0,0,0x83,0xc4,8,0x4f,0x8d,0xa4,0x24,0,0,0,0,
        0x8a,0x47,1,0x47,0x84,0xc0,0x75,0xf8,0x66,0xa1,0,0,0,0,0x66,0x89,7,0x5f,0xc3};
    for (std::size_t i=0;i<b.size();++i) b[i]=std::byte{raw[i]};
    const auto write=[&](unsigned offset,std::uint32_t value) {for(unsigned j=0;j<4;++j)b[offset+j]=std::byte((value>>(8*j))&255U);};
    write(7,f.image_base+s.buffer_rva);write(17,f.image_base+s.buffer_rva);write(42,f.image_base+s.suffix_rva);
    const auto delta=static_cast<std::int64_t>(s.cwd_rva)-f.target_rva-16;
    if (delta<INT32_MIN || delta>INT32_MAX) return false;
    write(12,static_cast<std::uint32_t>(delta));out=b;return true;
}
bool valid_file_manager_entry_spec(const FileManagerEntrySpec& s,const GamePreludeSpec& p,const ApplicationRoutingSpec& r) noexcept {
    const auto& f=s.manager;const auto& parent=r.initializer;std::array<std::byte,51> body{};
    if (!valid_game_prelude_spec(p,r) || !file_manager_body(s,body) || f.image_base!=parent.image_base || f.image_size!=parent.image_size ||
        f.call_rva!=p.localisation.call_rva+5 || !std::equal(f.call.begin(),f.call.end(),p.stop_prefix.begin()) ||
        !std::equal(s.return_prefix.begin(),s.return_prefix.begin()+11,p.stop_prefix.begin()+5) ||
        !std::equal(f.target_prefix.begin(),f.target_prefix.end(),body.begin())) return false;
    const std::array<std::array<std::uint32_t,2>,9> spans{{{parent.call_rva,5},{parent.target_rva,31},
        {p.empty.target_rva,16},{p.localisation.target_rva,20},{p.flags_rva-4,16},
        {f.target_rva,51},{s.cwd_rva,16},{s.buffer_rva-4,136},{s.suffix_rva,2}}};
    for(std::size_t i=0;i<spans.size();++i) {
        if(spans[i][0]<4096 || spans[i][0]>f.image_size-spans[i][1])return false;
        for(std::size_t j=0;j<i;++j)if(spans[i][0]<spans[j][0]+spans[j][1] && spans[j][0]<spans[i][0]+spans[i][1])return false;
    }
    return true;
}
bool file_manager_entry_frame(const FileManagerEntrySpec& s,const GamePreludeSpec& p,const ApplicationRoutingSpec& parent,
    std::uint32_t stage,const EventDispatchRegisters& saved,const EventDispatchRegisters& r,std::span<const std::uint32_t> stack) noexcept {
    if(stage<1 || stage>2 || !valid_file_manager_entry_spec(s,p,parent) || saved.esp<4096 || saved.esp%4 ||
        saved.eax || saved.ebx || saved.esi || saved.edi!=24 || (saved.flags&0x500U) || (r.flags&0x500U) ||
        ((saved.flags^r.flags)&~0x10000U) || r.eax!=saved.eax || r.ebx!=saved.ebx || r.ecx!=saved.ecx || r.edx!=saved.edx ||
        r.esi!=saved.esi || r.edi!=saved.edi || r.ebp!=saved.ebp) return false;
    const auto base=s.manager.image_base,outer=base+parent.initializer.call_rva+5,ret=base+s.manager.call_rva+5;
    if(stage==1)return r.esp==saved.esp-4 && stack.size()==2 && stack[0]==ret && stack[1]==outer;
    return r.esp==saved.esp-16 && stack.size()==5 && stack[0]==base+s.buffer_rva && stack[1]==128 &&
        stack[2]==saved.edi && stack[3]==ret && stack[4]==outer;
}
}
