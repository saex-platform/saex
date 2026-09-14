#include "saex/engine/cd_stream_channels_policy.generated.hpp"
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
namespace {unsigned checks{};void check(bool v){++checks;if(!v)throw std::runtime_error("channels check "+std::to_string(checks));}}
int main(){try {
    const auto& s=reviewed_cd_stream_channels_spec;const auto& t=reviewed_cd_stream_tables_spec;const auto& m=reviewed_file_manager_spec;
    std::array<std::byte,64> code{};check(cd_stream_channels_code(s,t,m,code));
    for(auto member:{&CdStreamChannelsSpec::error_iat_rva,&CdStreamChannelsSpec::local_iat_rva,&CdStreamChannelsSpec::pointer_rva,
        &CdStreamChannelsSpec::filename_rva,&CdStreamChannelsSpec::open_rva,&CdStreamChannelsSpec::error_thunk_slot_rva,&CdStreamChannelsSpec::local_thunk_slot_rva})
        for(auto value:{0U,1U,UINT32_MAX}){auto bad=s;bad.*member=value;check(!cd_stream_channels_code(bad,t,m,code));}
    auto bad=s;bad.local_iat_rva=s.error_iat_rva;check(!cd_stream_channels_code(bad,t,m,code));
    bad=s;bad.local_thunk_slot_rva+=4;check(!cd_stream_channels_code(bad,t,m,code));
    bad=s;bad.error_thunk.highlow_mask=0;check(!cd_stream_channels_code(bad,t,m,code));
    auto image=m;image.manager.image_base=UINT32_MAX;check(!cd_stream_channels_code(s,t,image,code));
    std::array<std::byte,240> memory{};check(cd_stream_channels_memory(memory));
    for(auto& v:memory){v=std::byte{1};check(!cd_stream_channels_memory(memory));v={};}
    check(!cd_stream_channels_memory(std::span(memory).first(239)));check(!cd_stream_channels_memory({}));
    CdStreamTableWindow before{},after{};before.fill(std::byte{0xa7});after=before;
    check(cd_stream_channels_tables(before,after,false));check(!cd_stream_channels_tables(before,after,true));
    std::fill(after.begin()+132,after.begin()+140,std::byte{});after[132]=std::byte{5};check(cd_stream_channels_tables(before,after,true));
    for(auto& v:after){v^=std::byte{1};check(!cd_stream_channels_tables(before,after,true));v^=std::byte{1};}
    CdStreamAllocationLayout block{};check(cd_stream_allocation_layout(0x200000,512,block));
    for(auto p:{0U,1U,65532U,0x300001U,0xfffffff0U,0x200000U,0x1ffffcU,0x2009fcU})check(!cd_stream_channels_range(p,block));
    check(cd_stream_channels_range(0x200a00,block));check(cd_stream_channels_range(0x200000-240,block));check(!cd_stream_channels_range(0x300000,{}));
    EventDispatchRegisters c{};c.esp=0x110000;c.eax=0x200200;c.ebx=2;c.ecx=3;c.edx=4;c.esi=5;c.edi=6;c.ebp=7;c.flags=0x202;
    auto e=c;e.edx=9;e.flags=0x202;auto l=c;l.eax=0x300000;l.ecx=8;l.edx=9;l.flags=0x246;
    constexpr unsigned offsets[]{0,8,4,4,12,4,0,0,12,4};
    for(unsigned stage=1;stage<=9;++stage){auto r=c;r.esp+=offsets[stage];r.edi=c.eax;
        if(stage<=3)r.flags=0x206;
        if(stage>=5 && stage<=7){r.eax=5;r.ecx=240;r.edx=e.edx;r.flags=0x206;}
        if(stage==9){r.eax=l.eax;r.ecx=l.ecx;r.edx=l.edx;r.flags=l.flags;}
        check(cd_stream_channels_frame(stage,c,r,e,l));
        for(auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::ebx,&EventDispatchRegisters::esi,&EventDispatchRegisters::edi,&EventDispatchRegisters::ebp}){
            auto wrong=r;wrong.*member^=4;check(!cd_stream_channels_frame(stage,c,wrong,e,l));}
        for(auto bit:{0x100U,0x400U}){auto wrong=r;wrong.flags|=bit;check(!cd_stream_channels_frame(stage,c,wrong,e,l));}
        for(auto member:{&EventDispatchRegisters::eax,&EventDispatchRegisters::ecx,&EventDispatchRegisters::edx}){
            auto wrong=r;wrong.*member^=4;check(cd_stream_channels_frame(stage,c,wrong,e,l)==(stage==4 || stage==8));}
        if(stage!=4 && stage!=8){auto wrong=r;wrong.flags^=0x40;check(!cd_stream_channels_frame(stage,c,wrong,e,l));}
    }
    for(auto stage:{0U,10U,UINT32_MAX})check(!cd_stream_channels_frame(stage,c,c,e,l));
    std::cout<<"PASS cd stream channels: "<<checks<<" portable checks\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
