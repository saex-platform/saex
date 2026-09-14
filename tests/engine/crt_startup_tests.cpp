#include "system_fixture_support.hpp"
#include "saex/engine/crt_startup_policy.generated.hpp"
int wmain(int argc,wchar_t** argv) {
    if (argc!=12) return 2; // Seven EXEs + proxy, codec root/leaf, ASI.
    try {
        wchar_t buffer[32768]{}; const auto length=GetSystemDirectoryW(buffer,32768);
        require(length && length<32768,"system directory"); const std::wstring system(buffer);
        SystemFixtureExports system_exports(system);
        std::vector<std::unique_ptr<LoaderFile>> files; std::vector<const LoaderFile*> pins;
        for (const auto& p:reviewed_entry_specs) if (p.origin==LoaderOrigin::system_x86) {
            const std::wstring name(p.name.begin(),p.name.end());
            files.push_back(std::make_unique<LoaderFile>((system+L"\\"+name).c_str()));
            require(files.back()->valid(),"system pin"); pins.push_back(files.back().get());
        }
        Fixture dll(argv[8],true),root(argv[9],true); LoaderFile leaf(argv[10]),asi_file(argv[11]);
        require(leaf.valid() && asi_file.valid(),"pins"); pins.insert(pins.end(),{&dll.file,&root.file,&leaf,&asi_file});
        std::string artifact;
        for (const auto c:std::wstring_view(argv[11])) { require(c>=32 && c<=126,"ASCII artifact"); artifact.push_back(static_cast<char>(c)); }
        std::replace(artifact.begin(),artifact.end(),'/','\\');
        unsigned cases{};
        auto run=[&](int variant,int mode=0) {
            ++cases; const auto path=argv[variant+1]; Markers markers(path); Fixture exe(path);
            EntryStopSpec entry{exe.layout.entry_rva,exe.at<std::array<std::byte,16>>(exe.layout.entry_rva)};
            ProxyStopSpec proxy{&dll.file,dll.symbol("proxy_thunk"),dll.symbol("proxy_restore"),dll.symbol("proxy_original_entry"),exe.startup_slot(),dll.symbol("proxy_iat_target")};
            const auto rva=exe.symbol("startup_sample"); ImageAnchor sample{"fixture",rva,0,16,exe.at<std::array<std::byte,16>>(rva)};
            StartupStopSpec startup{std::span(&sample,1)};
            const auto sequence=dll.symbol("codec_load_sequence");
            const auto load_slot=dll.at<DWORD>(sequence+7)-dll.layout.image_base;
            std::array<const LoaderFile*,2> chain{&root.file,&leaf}; CodecStopSpec codec{sequence,dll.symbol("codec_name"),load_slot,"saex_codec",chain};
            std::array<BindingSlotSpec,8> slots{};
            for (unsigned i=0;i<8;++i) {
                static constexpr const char* names[]{"binding_target_0","binding_target_1","binding_target_2","binding_target_3","binding_target_4","binding_target_5","binding_target_6","binding_target_7"};
                slots[i]={names[i],dll.symbol("binding_slots")+4*i,root.symbol(names[i])};
            }
            const auto stop=dll.symbol("binding_stop"),call=dll.symbol("asi_call"),end=dll.symbol("asi_end");
            BindingStopSpec binding{stop,dll.symbol("binding_proc_slot"),dll.at<std::array<std::byte,16>>(stop),slots};
            AsiStopSpec asi{&asi_file,call,end,dll.at<std::array<std::byte,16>>(call+6),dll.at<std::array<std::byte,16>>(end),artifact};
            SuspendedImage child(path,exe.layout.image_size); require(child.error().empty(),"child create");
            auto tail=reviewed_startup_return_spec;
            tail.protect_function=system_exports.protect; tail.startup_function=system_exports.startup;
            tail.protect_call_rva=dll.symbol("protect_call");
            tail.protect_slot_rva=dll.at<DWORD>(tail.protect_call_rva+2)-dll.layout.image_base;
            tail.startup_slot_rva=dll.symbol("original_startup"); tail.forward_rva=dll.symbol("startup_forward");
            tail.protect_return_prefix=dll.at<std::array<std::byte,16>>(tail.protect_call_rva+6);
            tail.frame={static_cast<DWORD>(child.image_base()),exe.layout.image_size,exe.symbol("frame_caller"),exe.symbol("frame_target")};
            tail.frame.call=exe.at<std::array<std::byte,5>>(tail.frame.call_rva);
            tail.frame.target_prefix=exe.at<std::array<std::byte,16>>(tail.frame.target_rva);
            const auto frame=[&](const char* call,const char* target) {
                FrameTargetSpec f{static_cast<DWORD>(child.image_base()),exe.layout.image_size,exe.symbol(call),exe.symbol(target)};
                f.call=exe.at<std::array<std::byte,5>>(f.call_rva); f.target_prefix=exe.at<std::array<std::byte,16>>(f.target_rva); return f;
            };
            std::array<std::uint32_t,3> targets{0,exe.symbol("frame_target"),exe.symbol("crt_application_target")};
            std::array tables{CrtInitializerTable{exe.symbol("crt_table"),targets}};
            CrtStartupSpec crt{frame("crt_io_call","crt_io_target"),frame("crt_initialize_call","crt_initialize_target"),
                frame("crt_application_call","crt_application_target"),{},88,tables};
            crt.io_return_prefix=exe.at<std::array<std::byte,16>>(crt.io.call_rva+5);
            if (mode==1) crt.io_return_stack_offset=1;
            if (mode==2) tables[0].rva=crt.io.call_rva;
            if (mode==3) crt.io_return_prefix[15]^=std::byte{1};
            if (mode==4) crt.application.target_prefix[15]^=std::byte{1};
            if (mode==5) targets[0]=exe.symbol("frame_target");
            if (mode==6) crt.io_return_stack_offset=84;
            LoaderLimits limits{}; if (variant==2) limits.milliseconds=1500;
            if (mode==7) limits.events=1;
            if (mode==9) {
                LoaderTrace foreign{}; std::thread thread([&] { foreign=LoaderObservation::run_to_crt_startup(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,tail,crt); });
                thread.join(); require(foreign.reason=="loader_owner_thread" && !child.stopped(),"foreign owner");
            }
            const auto result=mode==8 ? LoaderObservation::run_to_startup_return(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,tail)
                : LoaderObservation::run_to_crt_startup(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,tail,crt,limits);
            std::cout<<"case="<<cases<<" variant="<<variant<<" mode="<<mode<<" reason="<<result.reason
                <<" region="<<result.startup_return.last_region_rva<<" protection="<<result.startup_return.last_region_protect<<'\n';
            require(result.exit_confirmed && child.stopped(),"child cleanup");
            require(!result.bootstrap.stack_written && !result.bootstrap.calls_armed,"bootstrap invoked");
            require(!markers.exists(L".startup-after-call") && !markers.exists(L".asi-export-called") && !markers.exists(L".binding-function-called"),"boundary escaped");
            require(!markers.exists(L".crt-initializer-entered"),"initializer executed");
            if (!result.crt_startup.continued) require(!markers.exists(L".crt-after-startup"),"legacy startup advanced");
            if (!result.startup_return.protect_continued) require(!markers.exists(L".tail-after-protect"),"protect escaped");
            return result;
        };
        const auto normal=run(0); const auto& s=normal.crt_startup;
        require(normal.reason=="crt_initializer_boundary_verified" && s.verified && s.stage==2 && s.samples_verified==3 &&
            s.stack_valid && s.registers_valid && s.checked_slots==3 && s.nonzero_slots==2,"CRT boundary incomplete");
        require(run(1).reason=="entry_unexpected_exception","fault escaped");
        const auto stall=run(2); require(stall.reason=="loader_wait_failed" || stall.reason=="loader_timeout","stall escaped");
        require(run(3).reason=="crt_io_failed","I/O failure ignored");
        require(run(4).reason=="crt_io_calling_convention","saved register corruption ignored");
        require(run(5).reason=="crt_shape_drift","table drift ignored");
        require(run(6).reason=="crt_frame_drift","frame drift ignored");
        for (int i:{1,2}) require(run(0,i).reason=="crt_invalid_spec","invalid spec accepted");
        for (int i:{3,4,5}) require(run(0,i).reason=="crt_precondition_shape","invalid shape accepted");
        require(run(0,6).reason=="crt_stack_precondition","wrong enclosing frame accepted");
        require(run(0,7).reason=="loader_event_limit","event limit ignored");
        const auto legacy=run(0,8); require(legacy.reason=="startup_return_verified" && !legacy.crt_startup.armed,"legacy advanced");
        require(run(0,9).reason=="crt_initializer_boundary_verified","owner recovery");
        const auto scenarios=cases; DWORD before{},after{};
        require(GetProcessHandleCount(GetCurrentProcess(),&before)!=0,"handles before");
        for (unsigned i=0;i<12;++i) require(run(0).crt_startup.verified,"warm return");
        require(GetProcessHandleCount(GetCurrentProcess(),&after)!=0 && before==after,"handle leak");
        std::cout<<"PASS CRT startup: "<<scenarios<<" scenarios and 12 warm cycles; handles "<<before<<" -> "<<after<<'\n';
        return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
