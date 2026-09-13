#include "proxy_fixture_support.hpp"

int wmain(int argc, wchar_t** argv) {
    if (argc != 11) return 2; // Nine EXE variants, one common proxy DLL.
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
        Fixture dll(argv[10], true); pins.push_back(&dll.file);
        unsigned cases{};
        auto run = [&](int variant, int mode = 0) {
            ++cases;
            const auto path = argv[variant + 1]; Markers markers(path); Fixture exe(path);
            EntryStopSpec entry{exe.layout.entry_rva, exe.at<std::array<std::byte, 16>>(exe.layout.entry_rva)};
            ProxyStopSpec proxy{&dll.file, dll.symbol("proxy_thunk"), dll.symbol("proxy_restore"),
                dll.symbol("proxy_original_entry"), exe.startup_slot(), dll.symbol("proxy_iat_target")};
            const auto rva = exe.symbol("startup_sample");
            std::array<ImageAnchor, 5> samples{};
            samples[0] = {"fixture-data", rva, 0, 16, exe.at<std::array<std::byte, 16>>(rva)};
            StartupStopSpec startup{std::span(samples).first(1)};
            auto limits = LoaderLimits{};
            if (variant == 2) limits.milliseconds = 1500;
            if (mode == 1) startup.samples = {};
            if (mode == 2) startup.samples = samples;
            if (mode == 3) samples[0].length = 17;
            if (mode == 4) samples[0].bytes[0] ^= std::byte{1};
            if (mode == 5) { samples[1] = samples[0]; startup.samples = std::span(samples).first(2); }
            if (mode == 6) ++proxy.iat_rva;
            if (mode == 7) limits.events = 1;
            if (mode == 10) samples[0].rva = exe.layout.image_size - 8;
            SuspendedImage child(path, exe.layout.image_size);
            require(child.error().empty(), "child create");
            if (mode == 9) {
                LoaderTrace foreign{};
                std::thread other([&] { foreign = LoaderObservation::run_to_startup_call(child, exe.file.handle(), pins, entry, proxy, startup); });
                other.join(); require(foreign.reason == "loader_owner_thread" && !foreign.advanced && !child.stopped(), "foreign owner mutated child");
            }
            auto result = mode == 8 ? LoaderObservation::run_to_proxy_return(child, exe.file.handle(), pins, entry, proxy)
                : LoaderObservation::run_to_startup_call(child, exe.file.handle(), pins, entry, proxy, startup, limits);
            std::cout << "case=" << cases << " variant=" << variant << " mode=" << mode << " reason=" << result.reason
                << " continued=" << result.startup_continued << " hit=" << result.startup_reached << '\n';
            require(result.exit_confirmed && child.stopped() && child.stop(), "child exit");
            require(!markers.exists(L".startup-entered"), "startup target body ran");
            require(markers.exists(L".main-entered") == result.startup_continued, "entry/main progression mismatch");
            if (result.startup_continued) require(markers.exists(L".dll-entered") && markers.exists(L".proxy-entered"), "proxy canaries missing");
            auto repeat = LoaderObservation::run_to_startup_call(child, exe.file.handle(), pins, entry, proxy, startup);
            require(repeat.reason == "loader_child_identity_or_state" && !repeat.advanced, "terminal reuse");
            return result;
        };
        auto result = run(0);
        require(result.reason == "startup_call_verified" && result.startup_reached && result.startup_target_stable &&
            result.startup_callsite_verified && result.startup_argument_valid && result.startup_samples[0].read && result.startup_samples[0].match,
            "normal startup call");
        require(run(1).reason == "entry_unexpected_exception", "entry exception continued");
        result = run(2); require((result.reason == "loader_wait_failed" || result.reason == "loader_timeout") && !result.startup_reached, "entry stall ignored");
        require(run(3).reason == "startup_argument_region", "null argument accepted");
        require(run(4).reason == "startup_callsite_shape", "register call accepted");
        result = run(5); require(result.reason == "startup_iat_written" && result.startup_iat_write_observed && !result.startup_reached, "IAT write missed");
        require(run(6).reason == "startup_target_drift", "target mutation accepted");
        result = run(7); require(result.reason == "startup_call_verified" && result.startup_samples[0].read && !result.startup_samples[0].match, "sample mutation hidden");
        require(run(8).reason == "startup_sample_unreadable", "unreadable sample accepted");
        for (int mode : {1, 2, 6}) require(run(0, mode).reason == "startup_invalid_spec", "invalid spec accepted");
        require(run(0, 3).reason == "startup_invalid_sample", "invalid sample accepted");
        for (int mode : {4, 10}) require(run(0, mode).reason == "startup_sample_precondition", "sample precondition bypassed");
        require(run(0, 5).reason == "startup_sample_overlap", "sample overlap accepted");
        require(run(0, 7).reason == "loader_event_limit", "budget ignored");
        result = run(0, 8); require(result.reason == "proxy_return_verified" && !result.startup_continued, "legacy proxy mode advanced entry");
        require(run(0, 9).reason == "startup_call_verified", "owner recovery failed");
        // Positive control proves the target marker is reachable when no observer holds it.
        {
            Markers markers(argv[1]);
            std::wstring command = L"\"" + std::wstring(argv[1]) + L"\"";
            STARTUPINFOW info{}; info.cb = sizeof(info); info.dwFlags = STARTF_USESHOWWINDOW; info.wShowWindow = SW_HIDE;
            PROCESS_INFORMATION process{};
            require(CreateProcessW(argv[1], command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &info, &process) != 0, "control create");
            const auto waited = WaitForSingleObject(process.hProcess, 5000);
            if (waited != WAIT_OBJECT_0) { TerminateProcess(process.hProcess, 96); WaitForSingleObject(process.hProcess, 5000); }
            DWORD code{}; const auto read = GetExitCodeProcess(process.hProcess, &code);
            CloseHandle(process.hThread); CloseHandle(process.hProcess);
            require(waited == WAIT_OBJECT_0 && read && code == 90 && markers.exists(L".main-entered") && markers.exists(L".startup-entered"), "control canary not reached");
        }
        DWORD before{}, after{}; require(GetProcessHandleCount(GetCurrentProcess(), &before) != 0, "handles before");
        for (int i = 0; i < 12; ++i) require(run(0).reason == "startup_call_verified", "warm failure");
        require(GetProcessHandleCount(GetCurrentProcess(), &after) != 0 && before == after, "startup handle leak");
        std::cout << "PASS startup observer: 19 scenarios, positive control and 12 warm cycles; handles " << before << " -> " << after << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
