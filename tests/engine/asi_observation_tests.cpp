#include "proxy_fixture_support.hpp"

int wmain(int argc, wchar_t** argv) {
    if (argc != 18) return 2; // 12 EXEs, proxy, codec root/leaf, ASI and an unused DLL.
    try {
        wchar_t buffer[32768]{}; const auto length = GetSystemDirectoryW(buffer,32768);
        require(length && length < 32768, "system directory"); const std::wstring system(buffer);
        std::vector<std::unique_ptr<LoaderFile>> files; std::vector<const LoaderFile*> pins;
        for (const auto& spec : reviewed_entry_specs) if (spec.origin == LoaderOrigin::system_x86) {
            const std::wstring name(spec.name.begin(),spec.name.end());
            files.push_back(std::make_unique<LoaderFile>((system+L"\\"+name).c_str()));
            require(files.back()->valid(), "system pin"); pins.push_back(files.back().get());
        }
        Fixture dll(argv[13],true), root(argv[14],true);
        LoaderFile leaf(argv[15]), artifact(argv[16]), unused(argv[17]);
        require(leaf.valid() && artifact.valid() && unused.valid(), "fixture pins");
        pins.insert(pins.end(), {&dll.file,&root.file,&leaf,&artifact,&unused});
        const std::wstring wide_path(argv[16]); std::string artifact_path;
        for (const auto ch : wide_path) { require(ch >= 32 && ch <= 126,"fixture requires ASCII path"); artifact_path.push_back(static_cast<char>(ch)); }
        std::replace(artifact_path.begin(), artifact_path.end(), '/', '\\');
        const auto sequence = dll.symbol("codec_load_sequence");
        const auto load_slot = dll.at<DWORD>(sequence+7)-dll.layout.image_base;
        const auto call = dll.symbol("asi_call"), end = dll.symbol("asi_end");
        constexpr const char* names[]{"binding_target_0","binding_target_1","binding_target_2","binding_target_3",
            "binding_target_4","binding_target_5","binding_target_6","binding_target_7"};
        unsigned cases{};
        auto run = [&](int variant,int mode=0) {
            ++cases; const auto path=argv[variant+1]; Markers markers(path); Fixture exe(path);
            EntryStopSpec entry{exe.layout.entry_rva,exe.at<std::array<std::byte,16>>(exe.layout.entry_rva)};
            ProxyStopSpec proxy{&dll.file,dll.symbol("proxy_thunk"),dll.symbol("proxy_restore"),dll.symbol("proxy_original_entry"),exe.startup_slot(),dll.symbol("proxy_iat_target")};
            const auto rva=exe.symbol("startup_sample");
            ImageAnchor sample{"fixture",rva,0,16,exe.at<std::array<std::byte,16>>(rva)};
            StartupStopSpec startup{std::span(&sample,1)};
            std::array<const LoaderFile*,2> chain{&root.file,&leaf};
            CodecStopSpec codec{sequence,dll.symbol("codec_name"),load_slot,"saex_codec",chain};
            std::array<BindingSlotSpec,8> slots{};
            for(unsigned i=0;i<8;++i) slots[i]={names[i],dll.symbol("binding_slots")+i*4,root.symbol(names[i])};
            const auto stop=dll.symbol("binding_stop");
            BindingStopSpec binding{stop,dll.symbol("binding_proc_slot"),dll.at<std::array<std::byte,16>>(stop),slots};
            AsiStopSpec asi{&artifact,call,end,dll.at<std::array<std::byte,16>>(call+6),dll.at<std::array<std::byte,16>>(end),artifact_path};
            std::string long_path(260,'a'); long_path.replace(0,3,"C:\\");
            std::string nul_path("C:\\a\0.asi",9);
            LoaderLimits limits{}; if (variant==6 || variant==9) limits.milliseconds=1500;
            if(mode==1) asi.module=nullptr;
            if(mode==2) asi.call_rva=0;
            if(mode==3) asi.end_rva=asi.call_rva;
            if(mode==4) asi.expected_return[0]^=std::byte{1};
            if(mode==5) asi.expected_end[0]^=std::byte{1};
            if(mode==6) asi.module=&root.file;
            if(mode==7) asi.requested_path="relative.asi";
            if(mode==8) asi.requested_path=long_path;
            if(mode==9) asi.requested_path=nul_path;
            if(mode==10) asi.requested_path="C:\\nonascii\xc3\xa7.asi";
            if(mode==13) limits.events=1;
            if(mode==14) ++asi.call_rva;
            if(mode==15) asi.module=&unused;
            SuspendedImage child(path,exe.layout.image_size); require(child.error().empty(),"child create");
            if(mode==12) {
                LoaderTrace foreign{};
                std::thread other([&]{foreign=LoaderObservation::run_to_asi_return(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi);});
                other.join(); require(foreign.reason=="loader_owner_thread" && !child.stopped(),"foreign owner changed child");
            }
            auto result=mode==11 ? LoaderObservation::run_to_codec_bindings(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding)
                : LoaderObservation::run_to_asi_return(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,limits);
            std::cout<<"case="<<cases<<" variant="<<variant<<" mode="<<mode<<" reason="<<result.reason<<" loaded="<<result.asi_verified<<'\n';
            require(result.exit_confirmed && child.stopped() && child.stop(),"child exit");
            for(auto suffix:{L".asi-after-return",L".asi-end-escaped",L".asi-export-called",L".binding-function-called"})
                require(!markers.exists(suffix),"observer escaped or called export");
            if(!result.asi_load_continued) require(!markers.exists(L".asi-entered"),"ASI initializer ran before permission");
            if(result.asi_verified) {
                require(result.binding_verified && result.asi_call_reached && result.asi_path_verified && result.asi_return_reached &&
                    result.asi_mapping_id && result.asi_handle && result.asi_stack_after==result.asi_stack_before+4 && markers.exists(L".asi-entered"),"ASI evidence missing");
            }
            require(LoaderObservation::run_to_asi_return(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi).reason=="loader_child_identity_or_state","terminal reuse");
            return result;
        };
        require(run(0).reason=="asi_return_verified","normal ASI");
        for(int i:{1,3}) require(run(i).reason=="asi_path_mismatch","wrong/unterminated path accepted");
        require(run(2).reason=="asi_path_unreadable","invalid pointer accepted");
        require(run(4).reason=="asi_candidate_missing","empty scan escaped");
        for(int i:{5,8}) require(run(i).reason=="entry_unexpected_exception","fault ignored");
        for(int i:{6,9}) {const auto r=run(i); require(r.reason=="loader_wait_failed" || r.reason=="loader_timeout","stall ignored");}
        require(run(7).reason=="asi_mapping_retired","failed initializer accepted");
        require(run(10).reason=="asi_return_shape","return drift accepted");
        const auto watched=run(11);
        require(watched.reason=="startup_iat_written" && watched.startup_iat_write_observed,"ASI IAT watch lost");
        for(int mode:{1,2,3}) require(run(0,mode).reason=="asi_invalid_spec","invalid spec accepted");
        for(int mode:{4,5,14}) require(run(0,mode).reason=="asi_precondition_shape","wrong boundary accepted");
        require(run(0,6).reason=="asi_module_preloaded","preloaded root accepted");
        for(int mode:{7,8,9,10}) require(run(0,mode).reason=="asi_invalid_path","invalid path accepted");
        const auto legacy=run(0,11); require(legacy.reason=="codec_bindings_verified" && !legacy.asi_scan_continued,"legacy advanced");
        require(run(0,12).reason=="asi_return_verified","owner recovery");
        require(run(0,13).reason=="loader_event_limit","event budget bypass");
        require(run(0,15).reason=="asi_mapping_missing","wrong pinned identity accepted");
        const auto scenarios=cases;
        {
            Markers markers(argv[1]); std::wstring command=L"\""+std::wstring(argv[1])+L"\"";
            STARTUPINFOW info{}; info.cb=sizeof(info); info.dwFlags=STARTF_USESHOWWINDOW; info.wShowWindow=SW_HIDE;
            PROCESS_INFORMATION process{};
            require(CreateProcessW(argv[1],command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&info,&process)!=0,"control create");
            const auto waited=WaitForSingleObject(process.hProcess,5000);
            if(waited!=WAIT_OBJECT_0) {TerminateProcess(process.hProcess,96); WaitForSingleObject(process.hProcess,5000);}
            DWORD code{}; const auto read=GetExitCodeProcess(process.hProcess,&code);
            CloseHandle(process.hThread); CloseHandle(process.hProcess);
            require(waited==WAIT_OBJECT_0 && read && code==90 && markers.exists(L".asi-entered") &&
                markers.exists(L".asi-after-return") && !markers.exists(L".asi-export-called"),"positive control failed");
        }
        DWORD before{},after{}; require(GetProcessHandleCount(GetCurrentProcess(),&before)!=0,"handles before");
        for(int i=0;i<12;++i) require(run(0).reason=="asi_return_verified","warm failure");
        require(GetProcessHandleCount(GetCurrentProcess(),&after)!=0 && before==after,"ASI handle leak");
        std::cout<<"PASS ASI: "<<scenarios<<" scenarios, positive control and 12 warm cycles; handles "<<before<<" -> "<<after<<'\n';
        return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
