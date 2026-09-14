#include "saex/engine/cwd_copy.hpp"
#include <algorithm>
namespace saex::engine {
namespace {
template<std::size_t N> constexpr std::array<std::byte,(N-1)/2> bytes(const char (&text)[N]) {
    std::array<std::byte,(N-1)/2> result{};
    const auto digit=[](char c){return c<='9'?c-'0':c-'a'+10;};
    for(std::size_t i=0;i<result.size();++i)result[i]=std::byte((digit(text[i*2])<<4)|digit(text[i*2+1]));
    return result;
}
std::uint32_t parity(std::uint32_t v) noexcept {
    unsigned ones{};for(unsigned i=0;i<8;++i)ones+=(v>>i)&1U;
    return ones%2?0U:4U;
}
}
bool cwd_copy_code(const CwdCopySpec& s,const CwdQuerySpec& q,const CwdLockSpec& lock,CwdCopyCode& c) noexcept {
    if(!valid_cwd_query_spec(q,lock) || lock.selector.image_size<4096+248 ||
        s.copier_rva<4096 || s.copier_rva>lock.selector.image_size-248 ||
        q.helper_rva>lock.selector.image_size-247)return false;
    const std::array<std::array<std::uint32_t,2>,6> spans{{{q.helper_rva,247},{s.copier_rva,248},
        {lock.selector.call_rva,23},{q.cookie_rva,4},{q.iat_rva,4},{lock.table_rva+52,16}}};
    for(std::size_t i=0;i<spans.size();++i)
        for(std::size_t j=0;j<i;++j)if(spans[i][0]<spans[j][0]+spans[j][1] && spans[j][0]<spans[i][0]+spans[i][1])return false;
    c.checks=bytes("85c074ac403d0401000077a48b4d0c85c97525");
    c.capacity=bytes("3b45107e10");
    c.call=bytes("8d85f4feffff5051e800000000");
    const auto delta=s.copier_rva-(q.helper_rva+231);
    for(unsigned i=0;i<4;++i)c.call[9+i]=std::byte((delta>>(8*i))&255U);
    c.entry=bytes("578b7c2408eb6e");
    // Reviewed shared strcpy loop; the unrelated gap/strcat entry is never entered.
    c.body=bytes("8b4c240cf7c103000000741d8a1183c10184d27466881783c701f7c10300000075eaeb05891783c704bafffefe7e8b0103d083f0ff33c28b1183c104a90001018174e184d2743484f67427f7c20000ff007412f7c2000000ff7402ebc789178b4424085fc36689178b442408c64702005fc36689178b4424085fc388178b4424085fc3");
    return std::equal(q.return_prefix.begin(),q.return_prefix.end(),c.checks.begin());
}
bool cwd_copy_frame(std::uint32_t stage,const EventDispatchRegisters& h,const EventDispatchRegisters& a,
    const EventDispatchRegisters& r,std::uint32_t source,std::uint32_t destination,std::uint32_t length) noexcept {
    if(stage<1 || stage>5 || length<3 || length>126 || h.esp<65536+304 || h.esp>UINT32_MAX-72 || h.esp%4 ||
        h.ebp!=h.esp+44 || source!=h.esp-284 || !destination || destination>UINT32_MAX-128 ||
        (source<destination+128 && destination<source+260) ||
        a.esp!=h.esp-288 || a.ebp!=h.esp-16 || a.eax!=length ||
        a.ebx || a.esi || a.edi!=24 || (a.flags&0x500U) || (r.flags&0x500U) ||
        ((r.flags^a.flags)&~0x108d5U) || r.ebp!=a.ebp || r.ebx || r.esi || r.edi!=24)return false;
    const auto esp=stage<=2?h.esp-288:stage==4?h.esp-300:h.esp-296;
    if(r.esp!=esp || r.eax!=(stage<=2?length+1:stage==5?destination:source))return false;
    if(stage==5)return true; // cdecl result in EAX; ECX/EDX/arithmetic flags are volatile.
    if(r.ecx!=destination || r.edx!=a.edx)return false;
    if(stage==1)return (r.flags&0x8c5U)==(parity(destination)|((destination>>31)*0x80U));
    // CMP length+1,128: all admitted lengths produce a negative signed difference.
    return (r.flags&0x8d5U)==(0x81U|parity(length+1-128));
}
bool cwd_copy_buffer(const std::array<std::byte,136>& before,const std::array<std::byte,136>& after,
    const std::array<std::byte,260>& source,std::uint32_t length,bool copied) noexcept {
    if(length<3 || length>126 || source[length]!=std::byte{} ||
        std::find(source.begin(),source.begin()+length,std::byte{})!=source.begin()+length)return false;
    for(std::size_t i=0;i<before.size();++i) {
        const auto expected=copied && i>=4 && i-4<=length?source[i-4]:before[i];
        if(after[i]!=expected)return false;
    }
    return true;
}
}
