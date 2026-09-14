#include "proxy_fixture_support.hpp"
#include "saex/engine/application_entry_policy.generated.hpp"
int wmain(int argc,wchar_t** argv) {
    if (argc!=19) return 2; // Fourteen EXEs + proxy, codec root/leaf, ASI.
    try {
        wchar_t buffer[32768]{}; const auto length=GetSystemDirectoryW(buffer,32768);
        require(length && length<32768,"system directory"); const std::wstring system(buffer);
        std::vector<std::unique_ptr<LoaderFile>> files; std::vector<const LoaderFile*> pins;
        for (const auto& p:reviewed_entry_specs) if (p.origin==LoaderOrigin::system_x86) {
            const std::wstring name(p.name.begin(),p.name.end());
            files.push_back(std::make_unique<LoaderFile>((system+L"\\"+name).c_str()));
            require(files.back()->valid(),"system pin"); pins.push_back(files.back().get());
        }
        Fixture dll(argv[15],true),root(argv[16],true); LoaderFile leaf(argv[17]),asi_file(argv[18]);
        require(leaf.valid() && asi_file.valid(),"pins"); pins.insert(pins.end(),{&dll.file,&root.file,&leaf,&asi_file});
        std::string artifact;
        for (const auto c:std::wstring_view(argv[18])) { require(c>=32 && c<=126,"ASCII artifact"); artifact.push_back(static_cast<char>(c)); }
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
            auto body=exe.at<std::array<std::byte,32>>(crt.initialize.target_rva);
            ApplicationEntrySpec app{body,exe.at<std::array<std::byte,16>>(crt.initialize.call_rva+5),
                exe.at<std::array<std::byte,16>>(exe.symbol("application_second_call")+6),
                exe.symbol("application_second_call"),proxy.iat_target_rva,dll.symbol("application_once"),
                dll.at<std::array<std::byte,31>>(proxy.iat_target_rva)};
            for (unsigned i:{5,19,27}) std::fill_n(app.reentry_normalized.begin()+i,4,std::byte{});
            if (mode==1) app.second_call_rva=0;
            if (mode==2) app.reentry_normalized[11]=std::byte{0};
            if (mode==3) app.initializer_return[15]^=std::byte{1};
            if (mode==4) body[31]^=std::byte{1};
            if (mode==5) app.second_return[15]^=std::byte{1};
            if (mode==6) app.once_rva++;
            LoaderLimits limits{}; if (variant==2) limits.milliseconds=1500;
            if (mode==7) limits.events=1;
            if (mode==9) {
                LoaderTrace foreign{}; std::thread thread([&] { foreign=LoaderObservation::run_to_application_entry(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,tail,crt,app); });
                thread.join(); require(foreign.reason=="loader_owner_thread" && !child.stopped(),"foreign owner");
            }
            const auto result=mode==8 ? LoaderObservation::run_to_crt_startup(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,tail,crt)
                : LoaderObservation::run_to_application_entry(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,tail,crt,app,limits);
            std::cout<<"case="<<cases<<" variant="<<variant<<" mode="<<mode<<" reason="<<result.reason
                <<" region="<<result.startup_return.last_region_rva<<" protection="<<result.startup_return.last_region_protect<<'\n';
            require(result.exit_confirmed && child.stopped(),"child cleanup");
            require(!result.bootstrap.stack_written && !result.bootstrap.calls_armed,"bootstrap invoked");
            require(!markers.exists(L".startup-after-call") && !markers.exists(L".asi-export-called") && !markers.exists(L".binding-function-called"),"boundary escaped");
            require(!markers.exists(L".application-body-entered"),"application first instruction executed");
            if (!result.application_entry.continued) require(!markers.exists(L".application-initializer-entered"),"initializer escaped permit");
            return result;
        };
        const auto normal=run(0); const auto& s=normal.application_entry;
        require(normal.reason=="application_entry_verified" && s.verified && s.stage==5 &&
            s.initializer_returned && s.initializer_abi_valid && s.second_returned && s.second_abi_valid &&
            s.once_valid && s.arguments_valid && s.entry_reached && s.entry_stack_valid &&
            normal.crt_startup.samples_verified==3,"application entry incomplete");
        require(run(1).reason=="entry_unexpected_exception","fault escaped");
        const auto stall=run(2); require(stall.reason=="loader_wait_failed" || stall.reason=="loader_timeout","stall escaped");
        require(run(3).reason=="application_initializer_failed","initializer failure ignored");
        for (int i:{4,5,6}) require(run(i).reason=="application_shape_drift","shape drift ignored");
        require(run(7).reason=="application_initializer_abi","register corruption ignored");
        require(run(8).reason=="application_second_argument","bad second argument ignored");
        for (int i:{9,10}) require(run(i).reason=="application_arguments","bad application argument ignored");
        for (int i:{11,12}) require(run(i).reason=="application_command_line","bad command line ignored");
        require(run(13).reason=="loader_module_not_pinned","unpinned initializer DLL accepted");
        for (int i:{1,2}) require(run(0,i).reason=="application_invalid_spec","invalid spec accepted");
        for (int i:{3,4,5,6}) require(run(0,i).reason=="application_precondition_shape","invalid shape accepted");
        require(run(0,7).reason=="loader_event_limit","event limit ignored");
        const auto legacy=run(0,8); require(legacy.reason=="crt_initializer_boundary_verified" && !legacy.application_entry.armed,"legacy advanced");
        require(run(0,9).reason=="application_entry_verified","owner recovery");
        const auto scenarios=cases; DWORD before{},after{};
        require(GetProcessHandleCount(GetCurrentProcess(),&before)!=0,"handles before");
        for (unsigned i=0;i<12;++i) require(run(0).application_entry.verified,"warm return");
        require(GetProcessHandleCount(GetCurrentProcess(),&after)!=0 && before==after,"handle leak");
        std::cout<<"PASS application entry: "<<scenarios<<" scenarios and 12 warm cycles; handles "<<before<<" -> "<<after<<'\n';
        return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
