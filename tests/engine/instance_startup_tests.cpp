#include "proxy_fixture_support.hpp"
#include "saex/engine/instance_startup_policy.generated.hpp"
int wmain(int argc,wchar_t** argv) {
    if (argc!=6) return 2; // EXE + proxy, codec root/leaf, ASI.
    try {
        wchar_t buffer[32768]{}; const auto length=GetSystemDirectoryW(buffer,32768);
        require(length && length<32768,"system directory"); const std::wstring system(buffer);
        std::vector<std::unique_ptr<LoaderFile>> files; std::vector<const LoaderFile*> pins;
        for (const auto& p:reviewed_entry_specs) if (p.origin==LoaderOrigin::system_x86) {
            const std::wstring name(p.name.begin(),p.name.end());
            files.push_back(std::make_unique<LoaderFile>((system+L"\\"+name).c_str()));
            require(files.back()->valid(),"system pin"); pins.push_back(files.back().get());
        }
        Fixture dll(argv[2],true),root(argv[3],true); LoaderFile leaf(argv[4]),asi_file(argv[5]);
        require(leaf.valid() && asi_file.valid(),"pins"); pins.insert(pins.end(),{&dll.file,&root.file,&leaf,&asi_file});
        std::string artifact;
        for (const auto c:std::wstring_view(argv[5])) { require(c>=32 && c<=126,"ASCII artifact"); artifact.push_back(static_cast<char>(c)); }
        std::replace(artifact.begin(),artifact.end(),'/','\\');
        unsigned cases{};
        auto run=[&](int mode=0) {
            ++cases; const auto path=argv[1]; Markers markers(path); Fixture exe(path);
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
            auto suppression=reviewed_suppression_spec;
            suppression.last_error_iat_rva=exe.at<DWORD>(exe.symbol("suppression_last_error_call")+2)-exe.layout.image_base;
            auto instance=reviewed_instance_spec;
            instance.caller={static_cast<DWORD>(child.image_base()),exe.layout.image_size,platform.call_rva+6,exe.symbol("instance_target")};
            instance.caller.call=exe.at<std::array<std::byte,5>>(instance.caller.call_rva);
            instance.caller.target_prefix=exe.at<std::array<std::byte,16>>(instance.caller.target_rva);
            instance.normalized_body=exe.at<std::array<std::byte,91>>(instance.caller.target_rva);
            for (std::size_t i=0;i<instance_address_offsets.size();++i) {
                const auto offset=instance_address_offsets[i]; DWORD value{};
                std::memcpy(&value,instance.normalized_body.data()+offset,4);
                instance.body_address_rvas[i]=value-exe.layout.image_base;
                std::fill_n(instance.normalized_body.begin()+offset,4,std::byte{});
            }
            instance.name="Local\\SAEX.InstanceFixture.v1"; instance.name_rva=exe.symbol("instance_name");
            std::array<std::byte,91> relocated{};
            require(instance_body(instance,static_cast<DWORD>(child.image_base()),relocated),"fixture relocation");
            std::copy_n(relocated.begin(),16,instance.caller.target_prefix.begin());
            require(valid_instance_startup_spec(instance,platform,crt,suppression),"fixture instance spec");
            if (mode==1) instance.caller.call_rva++;
            if (mode==2) instance.normalized_body[29]=std::byte{0x74};
            if (mode==3) instance.name="Local\\SAEX.InstanceFixture.v2";
            if (mode==4) instance.create_thunk_rva++;
            if (mode==5) instance.create_thunk_slot_rva+=4;
            if (mode==6) instance.create_function.rva++;
            if (mode==7) instance.create_function.prefix[19]^=std::byte{1};
            if (mode==8) instance.name_rva++;
            if (mode==9) instance.body_address_rvas[1]=instance.body_address_rvas[2];
            LoaderLimits limits{}; if (mode==10) limits.events=1;
            if (mode==12) {
                LoaderTrace foreign{}; std::thread worker([&] { foreign=LoaderObservation::run_instance_startup(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,tail,crt,app,platform,suppression,instance); });
                worker.join(); require(foreign.reason=="loader_owner_thread" && !child.stopped(),"foreign owner");
            }
            const auto result=[&]() -> LoaderTrace {
                if (mode==11) return LoaderObservation::run_platform_suppression(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,tail,crt,app,platform,suppression);
                return LoaderObservation::run_instance_startup(child,exe.file.handle(),pins,entry,proxy,startup,codec,binding,asi,tail,crt,app,platform,suppression,instance,limits);
            }();
            std::cout<<"case="<<cases<<" mode="<<mode<<" reason="<<result.reason<<" stage="<<result.instance_startup.stage
                <<" error="<<result.instance_startup.last_error<<'\n';
            require(result.exit_confirmed && child.stopped(),"child cleanup");
            require(!result.bootstrap.stack_written && !result.bootstrap.calls_armed,"bootstrap invoked");
            for (const auto suffix:{L".startup-after-call",L".asi-export-called",L".binding-function-called",L".platform-api-called",L".instance-window-called"})
                require(!markers.exists(suffix),"canary boundary escaped");
            return result;
        };
        const auto normal=run(); const auto& n=normal.instance_startup;
        require(normal.reason=="instance_startup_verified" && n.verified && n.suppression_boundary_validated &&
            n.continued && n.create_call_reached && n.create_returned && n.event_identity_valid && n.getter_returned &&
            n.arguments_valid && n.caller_returned && n.stack_preserved && n.event_handle && !n.last_error,"normal incomplete");
        require(normal.platform_suppression.transaction.applied && !normal.platform_suppression.transaction.restore_attempted &&
            !normal.platform_suppression.verified && normal.platform_suppression.last_error_preserved,"suppression phase claims");
        const auto absent=OpenEventA(SYNCHRONIZE,FALSE,"Local\\SAEX.InstanceFixture.v1");
        if (absent) CloseHandle(absent);
        require(!absent && GetLastError()==ERROR_FILE_NOT_FOUND,"event lifetime leak");
        for (int i:{1,2,9}) require(run(i).reason=="instance_invalid_spec","invalid spec");
        for (int i:{3,4,5,6,7,8}) {
            const auto bad=run(i); require(bad.reason=="instance_precondition_shape" && !bad.instance_startup.continued,"bad shape advanced");
        }
        require(run(10).reason=="loader_event_limit","event budget");
        const auto legacy=run(11); require(legacy.platform_suppression.verified && legacy.platform_suppression.transaction.restored && !legacy.instance_startup.armed,"legacy mode advanced");
        require(run(12).instance_startup.verified,"owner recovery");
        {
            const auto held=CreateEventA(nullptr,TRUE,FALSE,"Local\\SAEX.InstanceFixture.v1");
            require(held && GetLastError()!=ERROR_ALREADY_EXISTS,"own existing event");
            const auto existing=run(); const auto state=WaitForSingleObject(held,0);
            CloseHandle(held);
            require(existing.reason=="instance_existing_detected" && existing.instance_startup.last_error==183 &&
                existing.instance_startup.event_identity_valid && existing.instance_startup.existing_detected &&
                !existing.instance_startup.caller_returned && !existing.instance_startup.verified && state==WAIT_TIMEOUT,"existing event changed or branch entered");
        }
        {
            const auto held=CreateMutexA(nullptr,FALSE,"Local\\SAEX.InstanceFixture.v1"); require(held,"own collision mutex");
            const auto collision=run(); CloseHandle(held);
            require(collision.reason=="instance_create_failed" && collision.instance_startup.create_returned &&
                !collision.instance_startup.event_handle && collision.instance_startup.last_error==ERROR_INVALID_HANDLE &&
                !collision.instance_startup.getter_returned,"NULL handle accepted");
        }
        {
            const auto held=CreateEventA(nullptr,TRUE,FALSE,"Local\\SAEX.InstanceFixture.v1"); require(held,"canary event");
            Markers markers(argv[1]); STARTUPINFOW startup{}; startup.cb=sizeof(startup);
            startup.dwFlags=STARTF_USESHOWWINDOW; startup.wShowWindow=SW_HIDE;
            PROCESS_INFORMATION process{}; std::wstring command=L"\""+std::wstring(argv[1])+L"\" --instance-control";
            const auto created=CreateProcessW(argv[1],command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process);
            if (!created) CloseHandle(held);
            require(created!=0,"canary create");
            const auto waited=WaitForSingleObject(process.hProcess,5000); DWORD code{};
            if (waited!=WAIT_OBJECT_0) { TerminateProcess(process.hProcess,99); WaitForSingleObject(process.hProcess,5000); }
            const auto read=GetExitCodeProcess(process.hProcess,&code);
            CloseHandle(process.hThread); CloseHandle(process.hProcess); CloseHandle(held);
            require(waited==WAIT_OBJECT_0 && read && code==94 && markers.exists(L".instance-window-called"),"window canary insensitive");
        }
        const auto scenarios=cases; DWORD before{},after{};
        require(GetProcessHandleCount(GetCurrentProcess(),&before)!=0,"handles before");
        for (unsigned i=0;i<12;++i) require(run().instance_startup.verified,"warm return");
        require(GetProcessHandleCount(GetCurrentProcess(),&after)!=0 && before==after,"handle leak");
        std::cout<<"PASS instance startup: "<<scenarios<<" scenarios, window canary control and 12 warm cycles; handles "<<before<<" -> "<<after<<'\n';
        return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
