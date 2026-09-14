#include "saex/engine/cwd_query.hpp"
#include <algorithm>
namespace saex::engine {
namespace {
template<std::size_t N> void word(std::array<std::byte,N>& bytes,std::size_t at,std::uint32_t n) noexcept {
    for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte((n>>(i*8))&255U);
}
}
bool cwd_query_code(const CwdQuerySpec& s,const CwdLockSpec& p,std::array<std::byte,18>& wrapper,
    std::array<std::byte,28>& helper,std::array<std::byte,18>& query) noexcept {
    const auto& f=p.selector;
    if(!valid_frame_target_spec(f) || f.image_size<4096+157 || f.call_rva>f.image_size-23 ||
        s.helper_rva<4096 || s.helper_rva>f.image_size-157 ||
        s.cookie_rva<4096 || s.cookie_rva>f.image_size-4 || s.cookie_rva%4 ||
        s.iat_rva<4096 || s.iat_rva>f.image_size-4 || s.iat_rva%4)return false;
    wrapper={std::byte{0x59},std::byte{0x83},std::byte{0x65},std::byte{0xfc},std::byte{},std::byte{0xff},std::byte{0x75},std::byte{0x0c},
        std::byte{0xff},std::byte{0x75},std::byte{8},std::byte{0x6a},std::byte{},std::byte{0xe8}};
    word(wrapper,14,s.helper_rva-(f.call_rva+23));
    helper={std::byte{0x55},std::byte{0x8b},std::byte{0xec},std::byte{0x81},std::byte{0xec},std::byte{0x0c},std::byte{1},std::byte{},std::byte{},std::byte{0xa1},
        std::byte{},std::byte{},std::byte{},std::byte{},std::byte{0x33},std::byte{0x45},std::byte{4},std::byte{0x53},std::byte{0x8b},std::byte{0x5d},std::byte{8},
        std::byte{0x85},std::byte{0xdb},std::byte{0x89},std::byte{0x45},std::byte{0xfc},std::byte{0x74},std::byte{0x5f}};
    word(helper,10,f.image_base+s.cookie_rva);
    query={std::byte{0x8d},std::byte{0x85},std::byte{0xf4},std::byte{0xfe},std::byte{0xff},std::byte{0xff},std::byte{0x50},std::byte{0x68},
        std::byte{4},std::byte{1},std::byte{},std::byte{},std::byte{0xff},std::byte{0x15}};
    word(query,14,f.image_base+s.iat_rva);return true;
}
bool valid_cwd_query_spec(const CwdQuerySpec& s,const CwdLockSpec& p) noexcept {
    std::array<std::byte,18> w{},q{};std::array<std::byte,28> h{};
    if(!cwd_query_code(s,p,w,h,q) || s.thunk_slot_rva<4096 || s.thunk_slot_rva%4 || s.thunk_slot_rva>16*1024*1024-4)return false;
    for(const auto& e:{s.function,s.implementation}) {
        std::array<std::byte,20> prefix{};
        if(e.rva<4096 || e.rva>16*1024*1024-20 || !e.preferred_base || e.preferred_base%65536 ||
            !bootstrap_export_prefix(e,e.preferred_base,prefix))return false;
    }
    const std::array<std::byte,8> thunk{std::byte{0x8b},std::byte{0xff},std::byte{0x55},std::byte{0x8b},std::byte{0xec},std::byte{0x5d},std::byte{0xff},std::byte{0x25}};
    if(!std::equal(thunk.begin(),thunk.end(),s.function.prefix.begin()) || s.function.highlow_mask!=(1U<<8) ||
        s.function.preferred_base>UINT32_MAX-s.thunk_slot_rva)return false;
    auto expected=s.function.prefix;word(expected,8,s.function.preferred_base+s.thunk_slot_rva);
    if(expected!=s.function.prefix)return false;
    const std::array<std::array<std::uint32_t,2>,5> spans{{{p.selector.call_rva,23},{s.helper_rva,157},
        {s.cookie_rva,4},{s.iat_rva,4},{p.table_rva+52,16}}};
    for(std::size_t i=0;i<spans.size();++i) {
        if(spans[i][0]<4096 || spans[i][0]>p.selector.image_size-spans[i][1])return false;
        for(std::size_t j=0;j<i;++j)if(spans[i][0]<spans[j][0]+spans[j][1] && spans[j][0]<spans[i][0]+spans[i][1])return false;
    }return true;
}
bool cwd_query_directory(std::string_view path) noexcept {
    if(path.size()<3 || path.size()>=260 || !((path[0]>='A' && path[0]<='Z') || (path[0]>='a' && path[0]<='z')) || path[1]!=':' || path[2]!='\\')return false;
    for(std::size_t i=0;i<path.size();++i) {
        const auto c=static_cast<unsigned char>(path[i]);
        if(c<32 || c>126 || (i!=1 && c==':') || c=='/' || c=='"' || c=='<' || c=='>' || c=='|' || c=='?' || c=='*')return false;
    }return true;
}
std::string_view cwd_query_result(std::uint32_t length,const std::array<std::byte,260>& bytes,std::string_view expected) noexcept {
    if(!cwd_query_directory(expected))return "cwd_query_expected_directory";
    if(!length)return "cwd_query_directory_failed";
    if(length>=bytes.size())return "cwd_query_directory_truncated";
    if(bytes[length]!=std::byte{} || std::find(bytes.begin(),bytes.begin()+length,std::byte{})!=bytes.begin()+length)return "cwd_query_directory_termination";
    const std::string_view actual(reinterpret_cast<const char*>(bytes.data()),length);
    if(!cwd_query_directory(actual))return "cwd_query_directory_encoding";
    if(actual!=expected)return "cwd_query_directory_mismatch";
    // CFileMgr later appends a backslash AND NUL to its 128-byte destination.
    if(length>126)return "cwd_query_suffix_capacity";
    return {};
}
bool cwd_query_frame(std::uint32_t stage,const EventDispatchRegisters& s,const EventDispatchRegisters& r,
    std::uint32_t cookie,std::uint32_t helper_return) noexcept {
    if(stage<1 || stage>7 || s.esp<65536+300 || s.esp>UINT32_MAX-72 || s.esp%4 || s.ebp!=s.esp+44 ||
        s.ebx || s.esi || s.edi!=24 || (s.flags&0x500U) || (r.flags&0x500U) ||
        ((r.flags^s.flags)&~0x108d5U) || r.ebx || r.esi || r.edi!=24)return false;
    const auto esp=stage==1?s.esp-8:stage==2?s.esp-12:stage==5?s.esp-296:stage==6?s.esp-300:s.esp-288;
    if(r.esp!=esp || r.ebp!=(stage<=2?s.ebp:s.esp-16))return false;
    if(stage==7)return true; // Win32 DWORD result; volatile registers/flags may change.
    if(r.ecx!=7 || r.edx!=s.edx)return false;
    if(stage<=2)return r.eax==s.eax;
    if(stage<=4)return r.eax==(cookie^helper_return) && (r.flags&0xc5U)==0x44U;
    return r.eax==s.esp-284;
}
bool cwd_query_stack(std::uint32_t stage,const std::array<std::uint32_t,21>& before,
    const std::array<std::uint32_t,21>& now,std::uint32_t buffer,std::uint32_t helper_return) noexcept {
    if(stage<1 || stage>7)return false;
    auto expected=before;expected[1]=0;expected[2]=buffer;expected[3]=128;expected[13]=0;
    if(stage>=2)expected[0]=helper_return;
    return now==expected;
}
}
