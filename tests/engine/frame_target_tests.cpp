#include "saex/engine/frame_target.hpp"
#include "saex/engine/frame_target_policy.generated.hpp"
#include "saex/engine/startup_return_policy.generated.hpp"
#include <iostream>
#include <stdexcept>
using namespace saex::engine;
int main() {
    try {
        unsigned checks{};
        const auto check = [&](bool result) { ++checks; if (!result) throw std::runtime_error("frame contract check failed"); };
        check(valid_frame_target_spec(reviewed_frame_spec));
        check(valid_startup_image_protection(0x40)); check(valid_startup_image_protection(0x80));
        for (auto p:{0x140U,0x180U,0x20U,0x04U}) check(!valid_startup_image_protection(p));
        auto tail=reviewed_startup_return_spec;
        check(valid_startup_return_spec(tail));
        tail.protect_call_rva=0; check(!valid_startup_return_spec(tail));
        tail=reviewed_startup_return_spec; tail.forward_rva=tail.protect_call_rva+6; check(!valid_startup_return_spec(tail));
        tail=reviewed_startup_return_spec; tail.protect_slot_rva|=1; check(!valid_startup_return_spec(tail));
        tail=reviewed_startup_return_spec; tail.startup_slot_rva=tail.protect_slot_rva; check(!valid_startup_return_spec(tail));
        tail=reviewed_startup_return_spec; tail.protect_function.highlow_mask=0x300; check(!valid_startup_return_spec(tail));
        tail=reviewed_startup_return_spec; tail.startup_function=tail.protect_function; check(!valid_startup_return_spec(tail));
        FrameTargetSpec s{0x400000, 0x5000, 0x2000, 0x1000, {std::byte{0xe8}, std::byte{0xfb}, std::byte{0xef}, std::byte{0xff}, std::byte{0xff}}};
        s.target_prefix.fill(std::byte{0x90}); check(valid_frame_target_spec(s));
        FrameTargetSample sample{s.call, s.target_prefix, 1, 0, true, true, true};
        check(matches_frame_target(s, sample));
        sample.target_read=false; check(!matches_frame_target(s,sample)); sample.target_read=true;
        sample.call_read=false; check(!matches_frame_target(s,sample)); sample.call_read=true;
        sample.attempted=false; check(!matches_frame_target(s,sample)); sample.attempted=true;
        sample.target_prefix[15]^=std::byte{1}; check(!matches_frame_target(s,sample)); sample.target_prefix=s.target_prefix;
        sample.call[4]^=std::byte{1}; check(!matches_frame_target(s,sample));
        auto bad=s; bad.call[0]=std::byte{0xe9}; check(!valid_frame_target_spec(bad));
        bad=s; ++bad.target_rva; check(!valid_frame_target_spec(bad));
        bad=s; bad.image_base=0xffff0000; bad.image_size=0x10000; check(!valid_frame_target_spec(bad));
        bad=s; bad.image_base=1; check(!valid_frame_target_spec(bad));
        bad=s; bad.image_size=0; check(!valid_frame_target_spec(bad));
        bad=s; bad.image_size=0x10000001; check(!valid_frame_target_spec(bad));
        bad=s; bad.call_rva=UINT32_MAX; check(!valid_frame_target_spec(bad));
        bad=s; bad.target_rva=0x4ff1; check(!valid_frame_target_spec(bad));
        bad=s; bad.target_rva=0x2001; check(!valid_frame_target_spec(bad));
        bad=s; bad.target_rva=0x400; check(!valid_frame_target_spec(bad));
        bad=s; bad.call[1]=std::byte{0}; bad.call[2]=std::byte{0}; bad.call[3]=std::byte{0}; bad.call[4]=std::byte{0x80};
        check(!valid_frame_target_spec(bad));
        // Forward rel32 and exact image-end prefix are supported.
        s.call_rva=0x1000; s.target_rva=0x4ff0;
        s.call={std::byte{0xe8},std::byte{0xeb},std::byte{0x3f},std::byte{0},std::byte{0}};
        check(valid_frame_target_spec(s));
        std::cout << checks << " frame contract checks passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
