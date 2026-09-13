#include "proxy_fixture_support.hpp"
int wmain(int argc, wchar_t** argv) {
    if (argc != 17) return 2;
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
        unsigned cases{};
        auto run = [&](int variant, int mode = 0) {
            ++cases;
            const auto path = argv[1 + variant * 2];
            Markers markers(path); Fixture exe(path), dll(argv[2 + variant * 2], true);
            auto selected = pins; selected.push_back(&dll.file);
            EntryStopSpec entry{exe.layout.entry_rva, exe.at<std::array<std::byte, 16>>(exe.layout.entry_rva)};
            ProxyStopSpec proxy{&dll.file, dll.symbol("proxy_thunk"), dll.symbol("proxy_restore"),
                dll.symbol("proxy_original_entry"), exe.startup_slot(), dll.symbol("proxy_iat_target")};
            auto limits = LoaderLimits{};
            if (variant == 4) limits.milliseconds = 1500;
            if (mode == 1) proxy.module = nullptr;
            if (mode == 2) ++proxy.thunk_rva;
            if (mode == 3) ++proxy.call_target_rva;
            if (mode == 4) proxy.return_slot_rva = UINT32_MAX;
            if (mode == 5) proxy.iat_rva = exe.layout.image_size - 2;
            if (mode == 6) proxy.iat_rva = entry.rva;
            if (mode == 7) ++proxy.iat_target_rva;
            if (mode == 8) limits.events = 1;
            if (mode == 9) selected.pop_back();
            SuspendedImage child(path, exe.layout.image_size);
            require(child.error().empty(), "child creation");
            if (mode == 11) {
                LoaderTrace foreign{};
                std::thread other([&] { foreign = LoaderObservation::run_to_proxy_return(child, exe.file.handle(), selected, entry, proxy); });
                other.join();
                require(foreign.reason == "loader_owner_thread" && !foreign.advanced && !child.stopped(), "foreign owner changed child");
            }
            const auto result = mode == 10 ? LoaderObservation::run_to_entry(child, exe.file.handle(), selected, entry)
                : LoaderObservation::run_to_proxy_return(child, exe.file.handle(), selected, entry, proxy, limits);
            std::cout << "case=" << cases << " variant=" << variant << " mode=" << mode << " reason=" << result.reason
                << " entry=" << result.entry_reached << " proxy=" << result.proxy_continued << " return=" << result.proxy_return_reached << '\n';
            require(result.exit_confirmed && child.stopped() && child.stop(), "cleanup not confirmed");
            require(!markers.exists(L".main-entered"), "main executed");
            require(markers.exists(L".proxy-entered") == result.proxy_continued, "proxy execution canary mismatch");
            if (result.entry_reached) require(markers.exists(L".dll-entered"), "DLL canary missing");
            const auto repeat = LoaderObservation::run_to_proxy_return(child, exe.file.handle(), selected, entry, proxy);
            require(repeat.reason == "loader_child_identity_or_state" && !repeat.advanced, "terminal reuse");
            return result;
        };
        const auto normal = run(0);
        require(normal.reason == "proxy_return_verified" && normal.proxy_entry_restored && normal.proxy_iat_verified &&
            normal.proxy_validated && normal.proxy_breakpoint_armed && !normal.entry_bytes_match &&
            normal.proxy_iat_before != normal.proxy_iat_after && normal.proxy_entry_after == normal.entry_before, "normal proxy stop");
        require(run(1).reason == "proxy_entry_not_restored", "missing restore accepted");
        require(run(2).reason == "proxy_iat_mismatch", "missing IAT rewrite accepted");
        require(run(3).reason == "entry_unexpected_exception", "proxy exception continued");
        auto result = run(4);
        require((result.reason == "loader_wait_failed" || result.reason == "loader_timeout") && !result.proxy_return_reached, "stall continued");
        require(run(5).reason == "proxy_return_shape", "return pointer drift accepted");
        require(run(6).reason == "proxy_entry_redirect", "wrong redirect continued");
        require(run(7).reason == "proxy_entry_redirect", "changed entry suffix continued");
        for (int mode : {1, 4, 5, 9}) require(run(0, mode).reason == "proxy_invalid_spec", "bad spec accepted");
        for (int mode : {2, 3}) require(run(0, mode).reason == "proxy_entry_shape", "bad thunk accepted");
        require(run(0, 6).reason == "proxy_iat_before_not_system", "non-system IAT accepted");
        require(run(0, 7).reason == "proxy_iat_mismatch", "wrong target accepted");
        require(run(0, 8).reason == "loader_event_limit", "event limit ignored");
        result = run(0, 10);
        require(result.reason == "entry_boundary_modified" && !result.proxy_continued, "old mode continued proxy");
        require(run(0, 11).reason == "proxy_return_verified", "owner failed to recover");
        DWORD before{}, after{}; require(GetProcessHandleCount(GetCurrentProcess(), &before) != 0, "handle count before");
        for (int i = 0; i < 12; ++i) require(run(0).reason == "proxy_return_verified", "warm cycle failed");
        require(GetProcessHandleCount(GetCurrentProcess(), &after) != 0 && before == after, "warm handle leak");
        std::cout << "PASS proxy observer: 19 cases and 12 warm cycles; handles " << before << " -> " << after << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
