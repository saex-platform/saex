#include "proxy_fixture_support.hpp"

int wmain(int argc, wchar_t** argv) {
    if (argc != 12) return 2; // Seven EXEs, proxy, codec root, leaf, an unused pinned DLL.
    try {
        wchar_t buffer[32768]{}; const auto length = GetSystemDirectoryW(buffer, 32768);
        require(length && length < 32768, "system directory"); const std::wstring system(buffer);
        std::vector<std::unique_ptr<LoaderFile>> files;
        std::vector<const LoaderFile*> pins;
        for (const auto& spec : reviewed_entry_specs) if (spec.origin == LoaderOrigin::system_x86) {
            const std::wstring name(spec.name.begin(), spec.name.end());
            files.push_back(std::make_unique<LoaderFile>((system + L"\\" + name).c_str()));
            require(files.back()->valid(), "system pin"); pins.push_back(files.back().get());
        }
        Fixture dll(argv[8], true); LoaderFile root(argv[9]), leaf(argv[10]), unused(argv[11]);
        require(root.valid() && leaf.valid() && unused.valid(), "codec pins");
        pins.insert(pins.end(), {&dll.file, &root, &leaf, &unused});
        const auto sequence = dll.symbol("codec_load_sequence");
        require(dll.at<unsigned char>(sequence) == 0x68 && dll.at<WORD>(sequence + 5) == 0x15ff &&
            dll.at<WORD>(sequence + 11) == 0xf88b, "fixture load instruction shape");
        const auto slot = dll.at<DWORD>(sequence + 7) - dll.layout.image_base;
        unsigned cases{};
        auto run = [&](int variant, int mode = 0) {
            ++cases;
            auto local_pins = pins;
            const auto path = argv[variant + 1]; Markers markers(path); Fixture exe(path);
            EntryStopSpec entry{exe.layout.entry_rva, exe.at<std::array<std::byte, 16>>(exe.layout.entry_rva)};
            ProxyStopSpec proxy{&dll.file, dll.symbol("proxy_thunk"), dll.symbol("proxy_restore"),
                dll.symbol("proxy_original_entry"), exe.startup_slot(), dll.symbol("proxy_iat_target")};
            const auto rva = exe.symbol("startup_sample");
            ImageAnchor sample{"fixture-data", rva, 0, 16, exe.at<std::array<std::byte, 16>>(rva)};
            StartupStopSpec startup{std::span(&sample, 1)};
            std::array<const LoaderFile*, 4> chain{&root, &leaf, &unused, &root};
            CodecStopSpec codec{sequence, dll.symbol("codec_name"), slot, "saex_codec", std::span(chain).first(2)};
            auto limits = LoaderLimits{};
            if (variant == 3) limits.milliseconds = 1500;
            if (mode == 1) codec.sequence_rva = UINT32_MAX;
            if (mode == 2) codec.requested_name = "wrong";
            if (mode == 3) chain[0] = nullptr;
            if (mode == 4) codec.modules = {};
            if (mode == 5) chain[1] = chain[0];
            if (mode == 6) {
                const auto system_pin = std::find_if(local_pins.begin(), local_pins.end(), [](const auto pin) {
                    return std::string_view(pin->identity().name.data()) == "kernel32.dll";
                });
                require(system_pin != local_pins.end(), "preloaded fixture kernel32 pin"); chain[1] = *system_pin;
            }
            if (mode == 7) ++codec.load_slot_rva;
            if (mode == 8) std::erase(local_pins, &root);
            if (mode == 9) { std::erase(local_pins, &leaf); codec.modules = std::span(chain).first(1); }
            if (mode == 10) codec.modules = std::span(chain).first(3);
            if (mode == 11) std::swap(chain[0], chain[1]);
            if (mode == 13) limits.events = 1;
            if (mode == 15) ++codec.sequence_rva;
            if (mode == 16) codec.requested_name = "../saex_codec";
            if (mode == 17) codec.modules = chain;
            SuspendedImage child(path, exe.layout.image_size);
            require(child.error().empty(), "child create");
            if (mode == 14) {
                LoaderTrace foreign{};
                std::thread other([&] { foreign = LoaderObservation::run_to_codec_return(child, exe.file.handle(), local_pins, entry, proxy, startup, codec); });
                other.join(); require(foreign.reason == "loader_owner_thread" && !foreign.advanced && !child.stopped(), "foreign owner mutated child");
            }
            auto result = mode == 12 ? LoaderObservation::run_to_startup_call(child, exe.file.handle(), local_pins, entry, proxy, startup)
                : LoaderObservation::run_to_codec_return(child, exe.file.handle(), local_pins, entry, proxy, startup, codec, limits);
            std::cout << "case=" << cases << " variant=" << variant << " mode=" << mode << " reason=" << result.reason
                << " continued=" << result.codec_continued << " hit=" << result.codec_reached << '\n';
            require(result.exit_confirmed && child.stopped() && child.stop(), "child exit");
            require(!markers.exists(L".codec-after-return"), "codec return instruction escaped");
            require(markers.exists(L".startup-entered") == result.codec_continued, "startup body progression mismatch");
            if (result.reason == "codec_return_verified") {
                require(markers.exists(L".codec-dll-entered") && markers.exists(L".codec-leaf-entered"), "DLL initialization missing");
                require(result.codec_modules_verified && result.codec_module_count == 2 && result.codec_handle &&
                    result.codec_mapping_ids[0] && result.codec_mapping_ids[1], "codec evidence missing");
            }
            if (mode == 9) require(!markers.exists(L".codec-dll-entered") && !markers.exists(L".codec-leaf-entered"), "unreviewed DLL initializer ran");
            auto repeat = LoaderObservation::run_to_codec_return(child, exe.file.handle(), local_pins, entry, proxy, startup, codec);
            require(repeat.reason == "loader_child_identity_or_state" && !repeat.advanced, "terminal reuse");
            return result;
        };
        require(run(0).reason == "codec_return_verified", "normal codec return");
        require(run(1).reason == "codec_load_failed", "failed DllMain accepted");
        require(run(2).reason == "entry_unexpected_exception", "DLL exception ignored");
        auto result = run(3); require(result.reason == "loader_wait_failed" || result.reason == "loader_timeout", "DLL stall ignored");
        require(run(4).reason == "codec_return_shape", "return instruction drift accepted");
        require(run(5).reason == "codec_module_preloaded", "preloaded codec accepted");
        result = run(6); require(result.reason == "startup_iat_written" && result.startup_iat_write_observed &&
            result.codec_continued && !result.codec_reached, "IAT write during codec load missed");
        for (int mode : {1,4,7,16,17}) require(run(0, mode).reason == "codec_invalid_spec", "invalid codec spec accepted");
        for (int mode : {2,15}) require(run(0, mode).reason == "codec_call_shape", "wrong codec call accepted");
        for (int mode : {3,8}) require(run(0, mode).reason == "codec_invalid_pin", "invalid codec pin accepted");
        require(run(0, 5).reason == "codec_duplicate_pin", "duplicate codec accepted");
        require(run(0, 6).reason == "codec_module_preloaded", "preloaded dependency accepted");
        require(run(0, 9).reason == "loader_module_not_pinned", "unpinned dependency accepted");
        require(run(0, 10).reason == "codec_mapping_missing", "missing required module accepted");
        require(run(0, 11).reason == "codec_handle_mismatch", "wrong root accepted");
        result = run(0, 12); require(result.reason == "startup_call_verified" && !result.codec_continued, "legacy startup advanced");
        require(run(0, 13).reason == "loader_event_limit", "budget ignored");
        require(run(0, 14).reason == "codec_return_verified", "owner recovery failed");
        const auto scenarios = cases;
        {
            Markers markers(argv[1]); std::wstring command = L"\"" + std::wstring(argv[1]) + L"\"";
            STARTUPINFOW info{}; info.cb = sizeof(info); info.dwFlags = STARTF_USESHOWWINDOW; info.wShowWindow = SW_HIDE;
            PROCESS_INFORMATION process{};
            require(CreateProcessW(argv[1], command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &info, &process) != 0, "control create");
            const auto waited = WaitForSingleObject(process.hProcess, 5000);
            if (waited != WAIT_OBJECT_0) { TerminateProcess(process.hProcess, 96); WaitForSingleObject(process.hProcess, 5000); }
            DWORD code{}; const auto read = GetExitCodeProcess(process.hProcess, &code);
            CloseHandle(process.hThread); CloseHandle(process.hProcess);
            require(waited == WAIT_OBJECT_0 && read && code == 90 && markers.exists(L".codec-after-return") &&
                markers.exists(L".codec-dll-entered") && markers.exists(L".codec-leaf-entered"), "control canary missing");
        }
        DWORD before{}, after{}; require(GetProcessHandleCount(GetCurrentProcess(), &before) != 0, "handles before");
        for (int i = 0; i < 12; ++i) require(run(0).reason == "codec_return_verified", "warm failure");
        require(GetProcessHandleCount(GetCurrentProcess(), &after) != 0 && before == after, "codec handle leak");
        std::cout << "PASS codec observer: " << scenarios << " scenarios, positive control and 12 warm cycles; handles " << before << " -> " << after << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
