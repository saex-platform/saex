#include "proxy_fixture_support.hpp"

int wmain(int argc, wchar_t** argv) {
    if (argc != 14) return 2; // Nine EXEs, proxy, codec root/leaf and an unused pinned DLL.
    try {
        wchar_t buffer[32768]{}; const auto length=GetSystemDirectoryW(buffer,32768);
        require(length && length<32768,"system directory"); const std::wstring system(buffer);
        std::vector<std::unique_ptr<LoaderFile>> files; std::vector<const LoaderFile*> pins;
        for (const auto& spec:reviewed_entry_specs) if(spec.origin==LoaderOrigin::system_x86) {
            const std::wstring name(spec.name.begin(),spec.name.end());
            files.push_back(std::make_unique<LoaderFile>((system+L"\\"+name).c_str()));
            require(files.back()->valid(),"system pin"); pins.push_back(files.back().get());
        }
        Fixture dll(argv[10],true), root(argv[11],true); LoaderFile leaf(argv[12]), unused(argv[13]);
        require(leaf.valid() && unused.valid(),"codec pins"); pins.insert(pins.end(),{&dll.file,&root.file,&leaf,&unused});
        const auto sequence=dll.symbol("codec_load_sequence");
        const auto load_slot=dll.at<DWORD>(sequence+7)-dll.layout.image_base;
        constexpr const char* names[]{"binding_target_0","binding_target_1","binding_target_2","binding_target_3",
            "binding_target_4","binding_target_5","binding_target_6","binding_target_7"};
        unsigned cases{};
        auto run=[&](int variant,int mode=0) {
            ++cases; const auto path=argv[variant+1]; Markers markers(path); Fixture exe(path);
            EntryStopSpec entry{exe.layout.entry_rva,exe.at<std::array<std::byte,16>>(exe.layout.entry_rva)};
            ProxyStopSpec proxy{&dll.file,dll.symbol("proxy_thunk"),dll.symbol("proxy_restore"),dll.symbol("proxy_original_entry"),exe.startup_slot(),dll.symbol("proxy_iat_target")};
            const auto rva=exe.symbol("startup_sample");
            ImageAnchor sample{"fixture",rva,0,16,exe.at<std::array<std::byte,16>>(rva)};
            StartupStopSpec startup{std::span(&sample,1)};
            std::array<const LoaderFile*,2> chain{&root.file,&leaf};
            CodecStopSpec codec{sequence,dll.symbol("codec_name"),load_slot,"saex_codec",chain};
            std::array<BindingSlotSpec,9> slots{};
            for(unsigned i=0;i<8;++i) slots[i]={names[i],dll.symbol("binding_slots")+i*4,root.symbol(names[i])};
            const auto stop=dll.symbol("binding_stop");
            BindingStopSpec binding{stop,dll.symbol("binding_proc_slot"),dll.at<std::array<std::byte,16>>(stop),std::span(slots).first(8)};
            LoaderLimits limits{}; if(variant==4) limits.milliseconds=1500;
            if(mode==1) binding.slots={};
            if(mode==2) binding.slots=slots;
            if(mode==3) slots[0].name="bad\"name";
            if(mode==4) slots[1].slot_rva=slots[0].slot_rva;
            if(mode==5) ++slots[0].slot_rva;
            if(mode==6) binding.stop_rva=UINT32_MAX;
            if(mode==7) binding.expected_stop[0]^=std::byte{1};
            if(mode==8) slots[0].target_rva=static_cast<std::uint32_t>(max_image_bytes-16);
            if(mode==9) ++slots[0].target_rva;
            if(mode==10) slots[0].slot_rva=binding.proc_slot_rva;
            if(mode==13) limits.events=1;
            if(mode==14) binding.proc_slot_rva=slots[0].slot_rva;
            if(mode==15) slots[1].name=slots[0].name;
            SuspendedImage child(path,exe.layout.image_size); require(child.error().empty(),"child create");
            if(mode==12) {
                LoaderTrace foreign{};
                std::thread other([&]{foreign=LoaderObservation::run_to_codec_bindings(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding);});
                other.join(); require(foreign.reason=="loader_owner_thread" && !child.stopped(),"foreign owner changed child");
            }
            auto result=mode==11 ? LoaderObservation::run_to_codec_return(child,exe.file.handle(),pins,entry,proxy,startup,codec)
                : LoaderObservation::run_to_codec_bindings(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,limits);
            std::cout<<"case="<<cases<<" variant="<<variant<<" mode="<<mode<<" reason="<<result.reason<<" continued="<<result.binding_continued<<'\n';
            require(result.exit_confirmed && child.stopped() && child.stop(),"child exit");
            require(!markers.exists(L".binding-after-stop") && !markers.exists(L".binding-function-called"),"binding escaped or called export");
            require(markers.exists(L".codec-after-return")==result.binding_continued,"binding body progress mismatch");
            if(result.binding_verified) {
                require(result.binding_reached && result.binding_count==8,"binding evidence missing");
                for(unsigned i=0;i<8;++i) require(result.bindings[i].before==0 && result.bindings[i].match && result.bindings[i].target_stable,"binding evidence mismatch");
            }
            const auto repeat=LoaderObservation::run_to_codec_bindings(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding);
            require(repeat.reason=="loader_child_identity_or_state" && !repeat.advanced,"terminal reuse");
            return result;
        };
        require(run(0).reason=="codec_bindings_verified","normal binding");
        for(int variant:{1,2,5}) require(run(variant).reason=="binding_slot_mismatch","missing/wrong/drift binding accepted");
        require(run(3).reason=="entry_unexpected_exception","binding fault ignored");
        auto result=run(4); require(result.reason=="loader_wait_failed" || result.reason=="loader_timeout","binding stall ignored");
        require(run(6).reason=="binding_mapping_retired","retired binding root accepted");
        require(run(7).reason=="binding_unexpected_load","unexpected load accepted");
        require(run(8).reason=="binding_return_shape","stop drift accepted");
        for(int mode:{1,2,6}) require(run(0,mode).reason=="binding_invalid_spec","invalid spec accepted");
        for(int mode:{3,5}) require(run(0,mode).reason=="binding_invalid_slot","invalid slot accepted");
        for(int mode:{4,15}) require(run(0,mode).reason=="binding_duplicate_slot","duplicate slot accepted");
        for(int mode:{7,14}) require(run(0,mode).reason=="binding_precondition_shape","shape precondition bypass");
        require(run(0,8).reason=="binding_precondition_read","target range bypass");
        require(run(0,9).reason=="binding_slot_mismatch","wrong target accepted");
        require(run(0,10).reason=="binding_preexisting_slot","prepopulated slot accepted");
        result=run(0,11); require(result.reason=="codec_return_verified" && !result.binding_continued,"legacy codec advanced");
        require(run(0,12).reason=="codec_bindings_verified","owner recovery failed");
        require(run(0,13).reason=="loader_event_limit","event limit ignored");
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
            require(waited==WAIT_OBJECT_0 && read && code==90 && markers.exists(L".binding-after-stop") &&
                !markers.exists(L".binding-function-called"),"positive control failed");
        }
        DWORD before{},after{}; require(GetProcessHandleCount(GetCurrentProcess(),&before)!=0,"handles before");
        for(int i=0;i<12;++i) require(run(0).reason=="codec_bindings_verified","warm failure");
        require(GetProcessHandleCount(GetCurrentProcess(),&after)!=0 && before==after,"binding handle leak");
        std::cout<<"PASS bindings: "<<scenarios<<" scenarios, positive control and 12 warm cycles; handles "<<before<<" -> "<<after<<'\n';
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n'; return 1;}
}
