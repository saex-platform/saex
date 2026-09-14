#include "proxy_fixture_support.hpp"
#include "saex/engine/platform_suppression_policy.generated.hpp"
int wmain(int argc,wchar_t** argv) {
    if (argc!=13) return 2; // Eight EXEs + proxy, codec root/leaf, ASI.
    try {
        wchar_t buffer[32768]{}; const auto length=GetSystemDirectoryW(buffer,32768);
        require(length && length<32768,"system directory"); const std::wstring system(buffer);
        std::vector<std::unique_ptr<LoaderFile>> files; std::vector<const LoaderFile*> pins;
        for (const auto& p:reviewed_entry_specs) if (p.origin==LoaderOrigin::system_x86) {
            const std::wstring name(p.name.begin(),p.name.end());
            files.push_back(std::make_unique<LoaderFile>((system+L"\\"+name).c_str()));
            require(files.back()->valid(),"system pin"); pins.push_back(files.back().get());
        }
        Fixture dll(argv[9],true),root(argv[10],true); LoaderFile leaf(argv[11]),asi_file(argv[12]);
        require(leaf.valid() && asi_file.valid(),"pins"); pins.insert(pins.end(),{&dll.file,&root.file,&leaf,&asi_file});
        std::string artifact;
        for (const auto c:std::wstring_view(argv[12])) { require(c>=32 && c<=126,"ASCII artifact"); artifact.push_back(static_cast<char>(c)); }
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
            const auto code=exe.at<std::array<std::byte,128>>(crt.application.target_rva);
            std::size_t prefix_bytes{};
            for (std::size_t i=16;i<100;++i) if (code[i]==std::byte{0xff} && code[i+1]==std::byte{0x15}) { prefix_bytes=i; break; }
            require(prefix_bytes!=0,"platform CALL not found");
            PlatformStartupSpec platform{std::span(code).first(prefix_bytes),crt.application.target_rva+static_cast<DWORD>(prefix_bytes),
                exe.symbol("platform_call_slot"),152,{}, {}, std::string_view(dll.file.identity().name.data())};
            platform.return_prefix=exe.at<std::array<std::byte,16>>(platform.call_rva+6);
            platform.function={dll.symbol("platform_system_canary"),dll.at<std::array<std::byte,20>>(dll.symbol("platform_system_canary")),dll.layout.image_base,0};
            if (mode==1) platform.stack_bytes=148;
            if (mode==2) platform.iat_rva=1;
            if (mode==3) platform.return_prefix[15]^=std::byte{1};
            if (mode==4) platform.function.prefix[19]^=std::byte{1};
            if (mode==5) platform.function.rva++;
            if (mode==6) platform.module="absent.dll";
            auto suppression=reviewed_suppression_spec;
            const auto get_error=exe.symbol("suppression_last_error_call");
            suppression.last_error_iat_rva=exe.at<DWORD>(get_error+2)-exe.layout.image_base;
            if (mode==10) suppression.last_error_iat_rva=1;
            if (mode==11) suppression.last_error_function.rva++;
            if (mode==12) suppression.last_error_function.prefix[19]^=std::byte{1};
            LoaderLimits limits{}; if (mode==7) limits.events=1;
            if (mode==9) {
                LoaderTrace foreign{}; std::thread thread([&] { foreign=LoaderObservation::run_platform_suppression(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,tail,crt,app,platform,suppression); });
                thread.join(); require(foreign.reason=="loader_owner_thread" && !child.stopped(),"foreign owner");
            }
            const auto result=mode==8 ? LoaderObservation::run_to_application_entry(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,tail,crt,app)
                : LoaderObservation::run_platform_suppression(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,tail,crt,app,platform,suppression,limits);
            std::cout<<"case="<<cases<<" variant="<<variant<<" mode="<<mode<<" reason="<<result.reason
                <<" region="<<result.startup_return.last_region_rva<<" protection="<<result.startup_return.last_region_protect<<'\n';
            if (result.reason=="suppression_context_failed" || result.reason=="suppression_restore_failed") {
                const auto& t=result.platform_suppression.transaction;
                std::cout<<"transaction="<<t.failure<<" restored="<<t.restored
                    <<" flags="<<t.original.flags<<'/'<<t.replacement.flags<<'/'<<t.readback.flags
                    <<" dr6="<<t.replacement.debug[4]<<'/'<<t.readback.debug[4]
                    <<" dr7="<<t.replacement.debug[5]<<'/'<<t.readback.debug[5]<<'\n';
                const auto& a=t.original; const auto& b=t.restore_readback;
                std::cout<<"restore flags="<<a.flags<<'/'<<b.flags<<" eip="<<a.eip<<'/'<<b.eip<<" esp="<<a.esp<<'/'<<b.esp
                    <<" eax="<<a.eax<<'/'<<b.eax<<" dr0="<<a.debug[0]<<'/'<<b.debug[0]<<" dr6="<<a.debug[4]<<'/'<<b.debug[4]<<'\n';
            }
            require(result.exit_confirmed && child.stopped(),"child cleanup");
            require(!result.bootstrap.stack_written && !result.bootstrap.calls_armed,"bootstrap invoked");
            require(!markers.exists(L".startup-after-call") && !markers.exists(L".asi-export-called") && !markers.exists(L".binding-function-called"),"boundary escaped");
            require(!markers.exists(L".platform-api-called"),"system API canary executed");
            return result;
        };
        const auto normal=run(0); const auto& s=normal.platform_startup;
        require(normal.reason=="platform_suppression_verified" && s.verified && s.continued && s.call_reached &&
            s.shape_valid && s.stack_valid && s.registers_valid && s.arguments_valid && s.arguments==platform_startup_arguments &&
            normal.application_entry.verified,"platform boundary incomplete");
        const auto& v=normal.platform_suppression;
        require(v.verified && v.return_reached && v.continued && v.transaction.applied && v.transaction.restored &&
            v.stack_preserved && v.last_error_preserved && v.last_error_before==0x1234abcd && v.last_error_after==0x1234abcd,
            "suppression state incomplete");
        require(run(0,10).reason=="suppression_invalid_spec","bad suppression spec");
        for (int i:{11,12}) {
            const auto bad=run(0,i); require(bad.reason=="suppression_last_error_identity" && !bad.platform_suppression.transaction.write_attempted,"bad last-error target wrote state");
        }
        for (int i:{1,2,3,4}) require(run(i).reason=="platform_stack_or_registers","bad frame accepted");
        require(run(5).reason=="platform_arguments","bad arguments accepted");
        require(run(6).reason=="platform_precondition_shape","bad IAT accepted");
        const auto fault=run(7);
        require(fault.reason=="entry_unexpected_exception" && fault.platform_startup.continued &&
            !fault.platform_startup.call_reached && !fault.platform_startup.verified,"fault escaped or continuation reported as proof");
        for (int i:{1,2}) require(run(0,i).reason=="platform_invalid_spec","invalid spec accepted");
        for (int i:{3,4,5,6}) require(run(0,i).reason=="platform_precondition_shape","invalid target accepted");
        require(run(0,7).reason=="loader_event_limit","event limit ignored");
        const auto legacy=run(0,8); require(legacy.reason=="application_entry_verified" && !legacy.platform_startup.armed,"legacy advanced");
        require(run(0,9).reason=="platform_suppression_verified","owner recovery");
        {
            Markers markers(argv[1]); STARTUPINFOW startup{}; startup.cb=sizeof(startup);
            startup.dwFlags=STARTF_USESHOWWINDOW; startup.wShowWindow=SW_HIDE;
            PROCESS_INFORMATION process{}; std::wstring command=L"\""+std::wstring(argv[1])+L"\"";
            require(CreateProcessW(argv[1],command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process)!=0,"canary control create");
            const auto waited=WaitForSingleObject(process.hProcess,5000); DWORD code{};
            if (waited!=WAIT_OBJECT_0) { TerminateProcess(process.hProcess,99); WaitForSingleObject(process.hProcess,5000); }
            const auto read=GetExitCodeProcess(process.hProcess,&code);
            CloseHandle(process.hThread); CloseHandle(process.hProcess);
            require(waited==WAIT_OBJECT_0 && read && code==96 && markers.exists(L".platform-api-called"),"canary positive control");
        }
        const auto scenarios=cases; DWORD before{},after{};
        require(GetProcessHandleCount(GetCurrentProcess(),&before)!=0,"handles before");
        for (unsigned i=0;i<12;++i) require(run(0).platform_suppression.verified,"warm return");
        require(GetProcessHandleCount(GetCurrentProcess(),&after)!=0 && before==after,"handle leak");
        std::cout<<"PASS platform suppression: "<<scenarios<<" scenarios, canary control and 12 warm cycles; handles "<<before<<" -> "<<after<<'\n';
        return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
