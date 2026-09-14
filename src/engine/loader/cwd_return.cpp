#include "saex/engine/cwd_return.hpp"
#include <algorithm>
namespace saex::engine {
namespace {
template<std::size_t N> void word(std::array<std::byte,N>& b,unsigned i,std::uint32_t v) noexcept {
    for(unsigned j=0;j<4;++j)b[i+j]=std::byte((v>>(8*j))&255U);
}
std::uint32_t logic_flags(std::uint32_t v) noexcept {
    unsigned ones{};for(unsigned i=0;i<8;++i)ones+=(v>>i)&1U;
    return (ones%2?0U:4U)|(v?0U:0x40U)|((v>>31)*0x80U);
}
}
bool cwd_return_code(const CwdReturnSpec& s,const CwdCopySpec& p,const CwdQuerySpec& q,const CwdLockSpec& l,CwdReturnCode& c) noexcept {
    CwdCopyCode parent{};if(!cwd_copy_code(p,q,l,parent) || s.checker_rva<4096 || s.checker_rva>l.selector.image_size-9 ||
        l.selector.call_rva>l.selector.image_size-39)return false;
    const std::array<std::array<std::uint32_t,2>,8> spans{{{q.helper_rva,247},{p.copier_rva,248},
        {l.selector.call_rva,23},{l.selector.call_rva+23,16},{q.cookie_rva,4},{q.iat_rva,4},{l.table_rva+52,16},{s.checker_rva,9}}};
    for(std::size_t i=0;i<spans.size();++i)for(std::size_t j=0;j<i;++j)
        if(spans[i][0]<spans[j][0]+spans[j][1] && spans[j][0]<spans[i][0]+spans[i][1])return false;
    c.cleanup={std::byte{0x59},std::byte{0x59},std::byte{0xe9},std::byte{0x51},std::byte{0xff},std::byte{0xff},std::byte{0xff}};
    c.epilogue={std::byte{0x8b},std::byte{0x4d},std::byte{0xfc},std::byte{0x33},std::byte{0x4d},std::byte{4},std::byte{0x5b},std::byte{0xe8},{},{},{},{},std::byte{0xc9},std::byte{0xc3}};
    word(c.epilogue,8,s.checker_rva-(q.helper_rva+75));
    c.checker={std::byte{0x3b},std::byte{0x0d},{},{},{},{},std::byte{0x75},std::byte{1},std::byte{0xc3}};
    if(l.selector.image_base>UINT32_MAX-q.cookie_rva)return false;
    word(c.checker,2,l.selector.image_base+q.cookie_rva);
    return std::equal(c.cleanup.begin(),c.cleanup.end(),p.return_prefix.begin());
}
bool cwd_return_frame(std::uint32_t stage,const EventDispatchRegisters& h,const EventDispatchRegisters& c,
    const EventDispatchRegisters& r,std::uint32_t cookie,std::uint32_t destination) noexcept {
    if(stage<1 || stage>6 || h.esp<65536+304 || h.esp>UINT32_MAX-72 || h.esp%4 || h.ebp!=h.esp+44 ||
        !destination || destination>UINT32_MAX-128 || c.esp!=h.esp-296 || c.ebp!=h.esp-16 ||
        c.eax!=destination || c.ebx || c.esi || c.edi!=24 || (c.flags&0x500U) || (r.flags&0x500U) ||
        ((c.flags^r.flags)&~0x108d5U) || r.eax!=destination || r.edx!=c.edx || r.ebx || r.esi || r.edi!=24 ||
        r.ebp!=(stage==6?h.ebp:c.ebp))return false;
    const auto esp=stage==6?h.esp-8:(stage==2 || stage==5)?h.esp-284:h.esp-288;
    if(r.esp!=esp || r.ecx!=(stage==1?h.esp-284:cookie))return false;
    if(stage==1)return !((r.flags^c.flags)&~0x10000U);
    if(stage<=3)return (r.flags&0x8c5U)==logic_flags(cookie); // XOR leaves AF undefined.
    return (r.flags&0x8d5U)==0x44U; // CMP cookie,cookie; only the equal path is admitted.
}
}
