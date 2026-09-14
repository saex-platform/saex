#include "saex/engine/cd_stream_tables_policy.generated.hpp"
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main() {
    unsigned checks{};
    const auto check=[&](bool value){++checks;if(!value)throw std::runtime_error("cd stream spec check "+std::to_string(checks));};
    try {
        const auto spec=reviewed_cd_stream_tables_spec;const auto manager=reviewed_file_manager_spec;
        CdStreamTablesCode code{};check(cd_stream_tables_code(spec,manager,code));
        for(const auto r:{0U,4095U,UINT32_MAX}){auto bad=spec;bad.target_rva=r;check(!cd_stream_tables_code(bad,manager,code));}
        {auto bad=spec;bad.names_rva++;check(!cd_stream_tables_code(bad,manager,code));}
        {auto bad=spec;bad.disk_iat_rva++;check(!cd_stream_tables_code(bad,manager,code));}
        {auto bad=spec;bad.disk_iat_rva=spec.target_rva;check(!cd_stream_tables_code(bad,manager,code));}
        {auto bad=manager;bad.manager.image_size=0;check(!cd_stream_tables_code(spec,bad,code));}
        {auto bad=manager;bad.manager.image_base=0xffff0000;check(!cd_stream_tables_code(spec,bad,code));}
        {auto bad=manager;bad.return_prefix[1]=std::byte{6};check(!cd_stream_tables_code(spec,bad,code));}
        CdStreamTableWindow before{},after{};before.fill(std::byte{0xa7});after=before;
        check(cd_stream_tables_memory(before,after,false));check(!cd_stream_tables_memory(before,after,true));
        for(std::size_t i=4;i<132;++i)after[i]=std::byte{};
        for(std::size_t i=140;i<2188;i+=64)after[i]=std::byte{};
        check(cd_stream_tables_memory(before,after,true));check(!cd_stream_tables_memory(before,after,false));
        for(std::size_t i=0;i<after.size();++i){auto bad=after;bad[i]^=std::byte{1};check(!cd_stream_tables_memory(before,bad,true));}
        EventDispatchRegisters caller{0x100100,0xb7005c,0,0x111,0x222,0,24,0x100900,0x202};
        auto entry=caller;entry.esp-=8;
        check(cd_stream_tables_frame(1,caller,entry,0x8e4010));
        auto terminal=caller;terminal.esp-=48;terminal.eax=caller.esp-12;terminal.ecx=caller.esp-16;
        terminal.edx=caller.esp-24;terminal.edi=0x8e4090;terminal.flags=0x246;
        check(cd_stream_tables_frame(2,caller,terminal,0x8e4010));
        for(auto member:{&EventDispatchRegisters::esp,&EventDispatchRegisters::eax,&EventDispatchRegisters::ebx,
            &EventDispatchRegisters::ecx,&EventDispatchRegisters::edx,&EventDispatchRegisters::esi,
            &EventDispatchRegisters::edi,&EventDispatchRegisters::ebp}) {
            auto bad=terminal;bad.*member^=1;check(!cd_stream_tables_frame(2,caller,bad,0x8e4010));
            bad=entry;bad.*member^=1;check(!cd_stream_tables_frame(1,caller,bad,0x8e4010));
        }
        for(auto bit:{0x100U,0x400U,1U,0x40U,0x800U}){auto bad=terminal;bad.flags^=bit;check(!cd_stream_tables_frame(2,caller,bad,0x8e4010));}
        check(!cd_stream_tables_frame(0,caller,entry,0x8e4010));check(!cd_stream_tables_frame(3,caller,terminal,0x8e4010));
        std::cout<<"PASS cd stream tables: "<<checks<<" portable checks\n";return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
