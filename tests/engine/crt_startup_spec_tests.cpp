#include "saex/engine/crt_startup_policy.generated.hpp"
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main() {
    try {
        unsigned count{};
        const auto check=[&](bool value) { ++count; if (!value) throw std::runtime_error("CRT spec failure"); };
        check(valid_crt_startup_spec(reviewed_crt_spec));
        auto s=reviewed_crt_spec; s.io_return_stack_offset=0; check(!valid_crt_startup_spec(s));
        s=reviewed_crt_spec; s.io_return_stack_offset=4097; check(!valid_crt_startup_spec(s));
        s=reviewed_crt_spec; s.tables={}; check(!valid_crt_startup_spec(s));
        s=reviewed_crt_spec; s.application=s.initialize; check(!valid_crt_startup_spec(s));
        s=reviewed_crt_spec; s.io.call[0]=std::byte{0xe9}; check(!valid_crt_startup_spec(s));
        s=reviewed_crt_spec; s.application.image_base+=65536; check(!valid_crt_startup_spec(s));
        s=reviewed_crt_spec; s.initialize.image_size+=4096; check(!valid_crt_startup_spec(s));
        std::array<std::uint32_t,2049> values{};
        std::array<CrtInitializerTable,2> tables{{{reviewed_crt_tables[0].rva,values},{reviewed_crt_tables[0].rva,std::span(values).first(1)}}};
        s=reviewed_crt_spec; s.tables=std::span(tables).first(1); check(!valid_crt_startup_spec(s));
        tables[0].targets=std::span(values).first(1); values[0]=UINT32_MAX; check(!valid_crt_startup_spec(s));
        values[0]=1; check(!valid_crt_startup_spec(s)); values[0]=0;
        tables[0].rva=1; check(!valid_crt_startup_spec(s));
        tables[0].rva=s.io.call_rva; check(!valid_crt_startup_spec(s));
        tables[0].rva=reviewed_crt_tables[0].rva; s.tables=tables; check(!valid_crt_startup_spec(s));
        s.tables=std::span(tables).first(1); check(valid_crt_startup_spec(s));
        std::cout<<count<<" CRT spec checks passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
