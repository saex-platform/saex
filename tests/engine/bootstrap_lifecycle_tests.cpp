#include <memory>
#include "proxy_fixture_support.hpp"
#include "saex/engine/observed_profile.generated.hpp"
#include "saex/engine/bootstrap_artifact.generated.hpp"
#include "saex/engine/bootstrap_lifecycle_policy.generated.hpp"

int wmain(int argc, wchar_t** argv) {
    if (argc != 21) return 2; // 15 EXEs + proxy, codec, leaf, fake ASI, real bootstrap.
    try {
        BootstrapExportSpec relocated{}; relocated.preferred_base=0x10000000; relocated.highlow_mask=2;
        relocated.prefix[1]=std::byte{0x34}; relocated.prefix[2]=std::byte{0x12}; relocated.prefix[4]=std::byte{0x10};
        std::array<std::byte,20> normalized{};
        require(bootstrap_export_prefix(relocated,0x21000000,normalized) && normalized[4]==std::byte{0x21} &&
            normalized[1]==std::byte{0x34} && normalized[2]==std::byte{0x12},"positive HIGHLOW");
        require(bootstrap_export_prefix(relocated,0x01000000,normalized) && normalized[4]==std::byte{0x01},"negative HIGHLOW delta");
        relocated.highlow_mask=6; require(!bootstrap_export_prefix(relocated,0,normalized),"overlapping fixups");
        relocated.highlow_mask=0x20000; require(!bootstrap_export_prefix(relocated,0,normalized),"partial fixup");
        relocated.highlow_mask=2; relocated.preferred_base=1;
        require(!bootstrap_export_prefix(relocated,0,normalized),"bad preferred base");
        wchar_t buffer[32768]{}; const auto length = GetSystemDirectoryW(buffer,32768);
        require(length && length < 32768,"system directory"); const std::wstring system(buffer);
        std::vector<std::unique_ptr<LoaderFile>> files; std::vector<const LoaderFile*> pins;
        auto pin_system = [&](std::string_view name) {
            const std::wstring wide(name.begin(),name.end());
            files.push_back(std::make_unique<LoaderFile>((system+L"\\"+wide).c_str()));
            require(files.back()->valid(),"system pin"); pins.push_back(files.back().get());
        };
        for(const auto& spec:reviewed_entry_specs) if(spec.origin==LoaderOrigin::system_x86) pin_system(spec.name);
        pin_system("bcrypt.dll"); pin_system("bcryptprimitives.dll");
#if defined(_DEBUG)
        for(const auto& spec:reviewed_asi_debugModules) pin_system(spec.name);
#endif
        Fixture dll(argv[16],true), root(argv[17],true), fake(argv[19],true);
        LoaderFile leaf(argv[18]), real(argv[20]); require(leaf.valid() && real.valid(),"artifact pins");
        pins.insert(pins.end(),{&dll.file,&root.file,&leaf,&fake.file,&real});
        const auto sequence=dll.symbol("codec_load_sequence");
        const auto load_slot=dll.at<DWORD>(sequence+7)-dll.layout.image_base;
        const auto call=dll.symbol("asi_call"), scan_end=dll.symbol("asi_end");
        constexpr const char* names[]{"binding_target_0","binding_target_1","binding_target_2","binding_target_3",
            "binding_target_4","binding_target_5","binding_target_6","binding_target_7"};
        unsigned cases{};
        auto run=[&](int variant,int mode=0) {
            ++cases; const auto path=argv[variant+1]; Markers markers(path); Fixture exe(path);
            EntryStopSpec entry{exe.layout.entry_rva,exe.at<std::array<std::byte,16>>(exe.layout.entry_rva)};
            ProxyStopSpec proxy{&dll.file,dll.symbol("proxy_thunk"),dll.symbol("proxy_restore"),dll.symbol("proxy_original_entry"),exe.startup_slot(),dll.symbol("proxy_iat_target")};
            const auto sample_rva=exe.symbol("startup_sample");
            ImageAnchor sample{"fixture",sample_rva,0,16,exe.at<std::array<std::byte,16>>(sample_rva)};
            StartupStopSpec startup{std::span(&sample,1)};
            std::array<const LoaderFile*,2> chain{&root.file,&leaf};
            CodecStopSpec codec{sequence,dll.symbol("codec_name"),load_slot,"saex_codec",chain};
            std::array<BindingSlotSpec,8> slots{};
            for(unsigned i=0;i<8;++i) slots[i]={names[i],dll.symbol("binding_slots")+i*4,root.symbol(names[i])};
            const auto stop=dll.symbol("binding_stop");
            BindingStopSpec binding{stop,dll.symbol("binding_proc_slot"),dll.at<std::array<std::byte,16>>(stop),slots};
            const bool actual=variant==12;
            std::string artifact_path;
            for(const auto ch:std::wstring_view(argv[actual?20:19])) { require(ch>=32 && ch<=126,"ASCII path"); artifact_path.push_back(static_cast<char>(ch)); }
            std::replace(artifact_path.begin(),artifact_path.end(),'/','\\');
            AsiStopSpec asi{actual?&real:&fake.file,call,scan_end,dll.at<std::array<std::byte,16>>(call+6),dll.at<std::array<std::byte,16>>(scan_end),artifact_path};
            BootstrapLifecycleSpec spec{bootstrap_artifact_exports,observed_profile.id,observed_profile_source_digest};
            if(!actual) {
                constexpr const char* exports[]{"FixtureInitialize","FixtureQuery","FixtureStop"};
                for(unsigned i=0;i<3;++i) {const auto rva=fake.symbol(exports[i]); spec.exports[i]={rva,fake.at<std::array<std::byte,20>>(rva)};}
            }
            if(mode==1) spec.exports[0].rva=0;
            if(mode==2) spec.exports[1]=spec.exports[0];
            if(mode==3) spec.profile_id="invalid/id";
            if(mode==4) spec.profile_source_digest="short";
            if(mode==5) spec.exports[1].prefix[0]^=std::byte{1};
            if(mode==6 || mode==7 || mode==11) {
                const auto rva=fake.symbol(mode==6?"FixtureBadStack":mode==7?"FixtureBadRegister":"FixtureDrift");
                spec.exports[1]={rva,fake.at<std::array<std::byte,20>>(rva)};
            }
            LoaderLimits limits{}; if(variant==11) limits.milliseconds=1500;
            if(mode==10) limits.events=1;
            SuspendedImage child(path,exe.layout.image_size); require(child.error().empty(),"child create");
            FrameTargetSpec frame{static_cast<std::uint32_t>(child.image_base()),exe.layout.image_size,
                exe.symbol("frame_caller"),exe.symbol("frame_target")};
            frame.call=exe.at<std::array<std::byte,5>>(frame.call_rva);
            frame.target_prefix=exe.at<std::array<std::byte,16>>(frame.target_rva);
            const bool frame_mode=mode>=12 || variant>=13;
            if(mode==13) ++frame.image_base;
            if(mode==14) ++frame.image_size;
            if(mode==15) frame.target_prefix[15]^=std::byte{1};
            if(mode==16) {
                frame.target_rva=sample_rva;
                const auto relative=frame.target_rva-frame.call_rva-5;
                for(unsigned i=0;i<4;++i) frame.call[i+1]=static_cast<std::byte>((relative>>(i*8))&255U);
            }
            if(mode==9) {
                LoaderTrace foreign{};
                std::thread other([&]{foreign=LoaderObservation::run_bootstrap_lifecycle(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,spec);});
                other.join(); require(foreign.reason=="loader_owner_thread" && !child.stopped(),"foreign owner");
            }
            // Keep diagnostic records off the x86 caller stack as observation scopes grow.
            // One dispatch return slot also avoids one large /Od temporary per ternary arm.
            auto result=std::make_unique<LoaderTrace>();
            *result=[&]() -> LoaderTrace {
                if(frame_mode)return LoaderObservation::run_frame_target_observation(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,spec,frame,limits);
                if(mode==8)return LoaderObservation::run_to_asi_return(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi);
                return LoaderObservation::run_bootstrap_lifecycle(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,spec,limits);
            }();
            std::cout<<"case="<<cases<<" variant="<<variant<<" mode="<<mode<<" reason="<<result->reason<<" calls="<<result->bootstrap.calls_returned<<'\n';
            require(result->exit_confirmed && child.stopped() && child.stop(),"child exit");
            for(auto suffix:{L".asi-after-return",L".asi-end-escaped",L".asi-export-called",L".binding-function-called"}) require(!markers.exists(suffix),"boundary escaped");
            if(result->bootstrap.verified) {
                require(result->asi_verified && result->bootstrap.calls_armed==8 && result->bootstrap.calls_returned==8,"incomplete lifecycle");
                for(unsigned i=0;i<8;++i) {
                    const auto& c=result->bootstrap.calls[i];
                    require(c.armed && c.continued && c.returned && c.stack_valid && c.guards_valid && c.status_valid &&
                        c.export_index==bootstrap_call_sequence[i] && c.return_code==0,"call evidence");
                }
            }
            return result;
        };
        require(run(0)->reason=="bootstrap_lifecycle_verified","normal lifecycle");
        require(run(1)->reason=="bootstrap_lifecycle_host_rejected","fixture host rejection");
        for(int i:{2,3,4,5,6,7}) require(run(i)->reason=="bootstrap_status_mismatch","bad status accepted");
        require(run(8)->reason=="bootstrap_output_overrun","overrun accepted");
        require(run(9)->reason=="bootstrap_call_failed","busy accepted");
        require(run(10)->reason=="entry_unexpected_exception","fault ignored");
        const auto stalled=run(11); require(stalled->reason=="loader_wait_failed" || stalled->reason=="loader_timeout","stall ignored");
        const auto actual_result=run(12); require(actual_result->reason=="bootstrap_lifecycle_host_rejected" &&
            actual_result->bootstrap.calls[1].status.reason==SAEX_BOOTSTRAP_FILE_REJECTED,"real DLL unknown host failed");
        for(int mode:{1,2,3,4}) require(run(0,mode)->reason=="bootstrap_invalid_spec","invalid spec accepted");
        require(run(0,5)->reason=="bootstrap_export_shape","wrong prefix accepted");
        require(run(0,6)->reason=="bootstrap_calling_convention","stdcall accepted");
        require(run(0,7)->reason=="bootstrap_calling_convention","register corruption accepted");
        const auto legacy=run(0,8); require(legacy->reason=="asi_return_verified" && !legacy->bootstrap.calls_armed && !legacy->bootstrap.stack_written,"legacy invoked exports");
        require(run(0,9)->reason=="bootstrap_lifecycle_verified","owner recovery");
        require(run(0,10)->reason=="loader_event_limit","budget ignored");
        require(run(0,11)->reason=="bootstrap_return_shape","post-call prefix drift ignored");
        const auto framed=run(0,12);
        require(framed->reason=="frame_target_samples_verified" && framed->frame_target.verified,"frame phases incomplete");
        for(const auto& sample:framed->frame_target.samples) require(sample.attempted && sample.call_read && sample.target_read &&
            sample.match && sample.thread_id==framed->last_event_thread_id,"frame sample missing");
        require(framed->frame_target.samples[0].event_index==0 && framed->frame_target.samples[1].event_index>0 &&
            framed->frame_target.samples[2].event_index>framed->frame_target.samples[1].event_index,"frame phase order");
        for(int mode:{13,14}) { const auto r=run(0,mode); require(r->reason=="frame_invalid_spec" && !r->advanced,"bad frame recipe advanced"); }
        const auto wrong=run(0,15); require(wrong->reason=="frame_create_sample_rejected" && !wrong->advanced &&
            wrong->frame_target.samples[0].target_read,"wrong prefix advanced");
        const auto inaccessible=run(0,16); require(inaccessible->reason=="frame_create_sample_rejected" &&
            !inaccessible->frame_target.samples[0].target_read && !inaccessible->advanced,"non-executable target read");
        const auto asi_drift=run(13); require(asi_drift->reason=="frame_asi_sample_rejected" && asi_drift->asi_verified &&
            !asi_drift->bootstrap.stack_written && !asi_drift->frame_target.samples[2].attempted,"ASI target drift ignored");
        const auto terminal_drift=run(14); require(terminal_drift->reason=="frame_terminal_sample_rejected" &&
            terminal_drift->bootstrap.verified && !terminal_drift->frame_target.verified,"terminal target drift ignored");
        require(!legacy->frame_target.samples[0].attempted && !legacy->frame_target.verified,"legacy frame mode leaked");
        const auto scenarios=cases;
        DWORD before{},after{}; require(GetProcessHandleCount(GetCurrentProcess(),&before)!=0,"handles before");
        for(int i=0;i<12;++i) require(run(i%2?12:0,12)->bootstrap.verified,"warm lifecycle failure");
        require(GetProcessHandleCount(GetCurrentProcess(),&after)!=0 && before==after,"lifecycle handle leak");
        std::cout<<"PASS bootstrap lifecycle: "<<scenarios<<" scenarios and 12 warm cycles; handles "<<before<<" -> "<<after<<'\n';
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n'; return 1;}
}
