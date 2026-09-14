#include "saex/engine/instance_startup.hpp"
#include <algorithm>
namespace saex::engine {
bool instance_body(const InstanceStartupSpec& s,std::uint32_t base,std::array<std::byte,91>& output) noexcept {
    if (s.caller.image_size<4096 || !base || base%65536 || base>UINT32_MAX-s.caller.image_size) return false;
    auto result=s.normalized_body;
    for (std::size_t i=0;i<instance_address_offsets.size();++i) {
        const auto offset=instance_address_offsets[i],rva=s.body_address_rvas[i];
        if (rva<4096 || rva>s.caller.image_size-4) return false;
        for (unsigned j=0;j<4;++j) {
            if (result[offset+j]!=std::byte{}) return false;
            result[offset+j]=std::byte((base+rva)>>(8*j));
        }
    }
    output=result; return true;
}
bool valid_instance_startup_spec(const InstanceStartupSpec& s,const PlatformStartupSpec& p,
    const CrtStartupSpec& crt,const PlatformSuppressionSpec& suppression) noexcept {
    if (!valid_platform_suppression_spec(suppression,p,crt) || !valid_frame_target_spec(s.caller) ||
        s.caller.image_base!=crt.application.image_base || s.caller.image_size!=crt.application.image_size ||
        s.caller.call_rva!=p.call_rva+6 || s.caller.target_rva>s.caller.image_size-91 ||
        s.name.empty() || s.name.size()>96 || s.name_rva<4096 || s.name_rva>s.caller.image_size-s.name.size()-1 ||
        !std::all_of(s.name.begin(),s.name.end(),[](char c){return c>=32 && c<=126;}) ||
        s.body_address_rvas[2]!=suppression.last_error_iat_rva || s.body_address_rvas[0]!=s.body_address_rvas[4] ||
        s.body_address_rvas[6]!=s.body_address_rvas[8]) return false;
    for (const auto v:s.body_address_rvas) if (v%4) return false;
    const auto begin=s.caller.target_rva,end=begin+91;
    if (begin<p.call_rva+22 && crt.application.target_rva<end) return false;
    for (const auto v:s.body_address_rvas) if (v<end && begin<v+4) return false;
    if (s.name_rva<end && begin<s.name_rva+s.name.size()+1) return false;
    for (const auto& f:{s.create_function}) {
        std::array<std::byte,20> bytes{};
        if (!f.rva || f.rva>=16U*1024*1024-20 || !f.preferred_base || f.preferred_base%65536 ||
            !bootstrap_export_prefix(f,0x10000000,bytes)) return false;
    }
    if (s.create_thunk_rva<4096 || s.create_thunk_rva>16U*1024*1024-6 ||
        s.create_thunk_slot_rva<4096 || s.create_thunk_slot_rva%4 || s.create_thunk_slot_rva>16U*1024*1024-4 ||
        (s.create_thunk_rva<s.create_thunk_slot_rva+4 && s.create_thunk_slot_rva<s.create_thunk_rva+6) ||
        s.create_function.highlow_mask) return false;
    // The branch/call opcode graph is fixed; only image operands are relocated.
    constexpr std::array<unsigned char,91> shape{0xa1,0,0,0,0,0x50,0x6a,1,0x6a,0,0x6a,0,0xff,0x15,0,0,0,0,
        0xff,0x15,0,0,0,0,0x3d,0xb7,0,0,0,0x75,0x39,0x8b,0x0d,0,0,0,0,0x8b,0x15,0,0,0,0,0x51,0x52,
        0xff,0x15,0,0,0,0,0x85,0xc0,0x74,0x0d,0x50,0xff,0x15,0,0,0,0,0xb8,1,0,0,0,0xc3,
        0xa1,0,0,0,0,0x8b,8,0x51,0xff,0x15,0,0,0,0,0xb8,1,0,0,0,0xc3,0x33,0xc0,0xc3};
    for (std::size_t i=0;i<shape.size();++i) if (s.normalized_body[i]!=std::byte(shape[i])) return false;
    std::array<std::byte,91> expected{};
    return instance_body(s,s.caller.image_base,expected) &&
        std::equal(s.caller.target_prefix.begin(),s.caller.target_prefix.end(),expected.begin());
}
}
