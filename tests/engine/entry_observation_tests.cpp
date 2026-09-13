#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "saex/engine/loader_observation.hpp"
#include "saex/engine/loader_policy.generated.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace saex::engine;
namespace {
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
constexpr const wchar_t* suffixes[]{L".dll-entered", L".tls-entered", L".main-entered"};
struct Markers {
    std::wstring path;
    explicit Markers(const wchar_t* source) : path(source) {
        for (auto suffix : suffixes) require(!exists(suffix), "preexisting fixture marker");
    }
    bool exists(const wchar_t* suffix) const { return GetFileAttributesW((path + suffix).c_str()) != INVALID_FILE_ATTRIBUTES; }
    ~Markers() { for (auto suffix : suffixes) DeleteFileW((path + suffix).c_str()); }
};
struct Fixture {
    LoaderFile file;
    PeLayout layout;
    EntryStopSpec entry;
    explicit Fixture(const wchar_t* path) : file(path) {
        require(file.valid(), "fixture file pin");
        std::ifstream stream(std::filesystem::path(path), std::ios::binary);
        const std::vector<char> raw{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
        const auto bytes = std::as_bytes(std::span(raw));
        auto parsed = parse_pe32(bytes); require(parsed.error == PeError::none, "fixture PE parse"); layout = parsed.layout;
        entry.rva = layout.entry_rva;
        auto offset = raw_offset(layout, entry.rva, entry.expected.size()); require(offset.has_value(), "fixture entry range");
        std::copy_n(bytes.begin() + *offset, entry.expected.size(), entry.expected.begin());
    }
};
void dump(const LoaderTrace& trace) {
    std::cout << trace.reason << " armed=" << trace.entry_breakpoint_armed << " continued=" << trace.initial_breakpoint_continued
        << " reached=" << trace.entry_reached << " events=" << trace.event_count << " exception=" << trace.exception_code << '\n';
    for (std::uint32_t i = 0; i < trace.module_count; ++i)
        std::cout << "  " << trace.modules[i].file.name.data() << " admitted=" << trace.modules[i].admitted << '\n';
}
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 6) return 2;
    try {
        wchar_t buffer[32768]{}; const auto length = GetSystemDirectoryW(buffer, 32768);
        require(length && length < 32768, "system directory"); const std::wstring system(buffer);
        LoaderFile fixture_dll(argv[2]);
        std::vector<std::unique_ptr<LoaderFile>> system_files;
        std::vector<const LoaderFile*> pins;
        // Explicit reviewed system name set, with fresh retained identities for our own fixture.
        // This fixture policy does not approve a GTA hash or modify the production mapping recipe.
        for (const auto& spec : reviewed_loader_specs) if (spec.origin == LoaderOrigin::system_x86) {
            const std::wstring name(spec.name.begin(), spec.name.end());
            system_files.push_back(std::make_unique<LoaderFile>((system + L"\\" + name).c_str()));
            pins.push_back(system_files.back().get());
        }
        pins.push_back(&fixture_dll);
        for (auto pin : pins) require(pin->valid(), "DLL pin");
        auto run = [&](const wchar_t* path, int mode = 0) {
            Markers markers(path); Fixture fixture(path);
            SuspendedImage child(path, fixture.layout.image_size);
            require(child.error().empty(), "entry child create");
            auto spec = fixture.entry; auto limits = LoaderLimits{};
            if (mode == 1) spec.expected[0] ^= std::byte{1};
            if (mode == 2) spec.rva = 0;
            if (mode == 3) spec.rva = fixture.layout.image_size - 8;
            if (mode == 4) limits.events = 1;
            if (mode == 5) limits.milliseconds = 1000;
            if (mode == 9) {
                const auto end = fixture.layout.sections.begin() + fixture.layout.section_count;
                const auto section = std::find_if(fixture.layout.sections.begin(), end,
                    [](const auto& s) { return s.raw_size >= 16 && !(s.characteristics & IMAGE_SCN_MEM_EXECUTE); });
                require(section != end, "non-executable fixture section");
                spec.rva = section->rva; require(child.copy(spec.rva, spec.expected), "data section bytes");
            }
            if (mode == 8) {
                LoaderTrace foreign{};
                std::thread other([&] { foreign = LoaderObservation::run_to_entry(child, fixture.file.handle(), pins, spec); });
                other.join();
                require(foreign.reason == "loader_owner_thread" && !child.stopped() && !foreign.advanced, "foreign owner changed child");
            }
            auto selected = mode == 6 ? std::span<const LoaderFile* const>(pins).first(pins.size() - 1) : std::span<const LoaderFile* const>(pins);
            const auto result = mode == 7 ? LoaderObservation::run(child, fixture.file.handle(), selected, limits)
                : LoaderObservation::run_to_entry(child, fixture.file.handle(), selected, spec, limits);
            dump(result);
            require(result.exit_confirmed && child.stopped() && child.stop(), "entry child exit not confirmed");
            require(!markers.exists(L".main-entered"), "fixture main executed");
            if (result.entry_reached) {
                require(markers.exists(L".dll-entered") && markers.exists(L".tls-entered"), "DLL/TLS initialization did not precede entry stop");
                require(result.entry_breakpoint_armed && result.initial_breakpoint_continued && result.entry_bytes_read &&
                    result.exception_code == EXCEPTION_SINGLE_STEP && result.entry_address == child.image_base() + fixture.entry.rva,
                    "entry hit identity");
            } else if (!result.initial_breakpoint_continued) {
                require(!markers.exists(L".dll-entered") && !markers.exists(L".tls-entered"), "early rejection allowed initializer");
            } else {
                require(markers.exists(L".dll-entered") && markers.exists(L".tls-entered"), "fault/stall did not reach fixture initializer");
            }
            const auto repeat = LoaderObservation::run_to_entry(child, fixture.file.handle(), pins, fixture.entry);
            require(!repeat.advanced && repeat.reason == "loader_child_identity_or_state", "terminal child reused");
            return result;
        };
        auto result = run(argv[1]); require(result.reason == "entry_boundary_reached" && result.entry_bytes_match, "normal entry not held");
        result = run(argv[3]); require(result.reason == "entry_boundary_modified" && !result.entry_bytes_match && result.entry_after[0] == std::byte{0xcc}, "initializer patch not observed");
        result = run(argv[4]); require(result.reason == "entry_unexpected_exception" && !result.entry_reached && result.initial_breakpoint_continued && result.exception_code == EXCEPTION_BREAKPOINT, "initializer exception accepted");
        result = run(argv[5], 5); require((result.reason == "loader_wait_failed" || result.reason == "loader_timeout") && !result.entry_reached && result.initial_breakpoint_continued, "initializer timeout not bounded");
        for (int mode = 1; mode <= 3; ++mode) {
            result = run(argv[1], mode); require(result.reason == "entry_precondition_bytes" && !result.advanced, "invalid entry spec advanced");
        }
        result = run(argv[1], 4); require(result.reason == "loader_event_limit" && !result.advanced, "entry event budget bypassed");
        result = run(argv[1], 6); require(result.reason == "loader_module_not_pinned" && !result.initial_breakpoint_continued, "unpinned DLL initialized");
        result = run(argv[1], 7); require(result.reason == "loader_breakpoint_candidate" && !result.entry_breakpoint_armed && !result.initial_breakpoint_continued, "old mode advanced into init");
        result = run(argv[1], 8); require(result.entry_reached, "owner could not recover after foreign call");
        result = run(argv[1], 9); require(result.reason == "entry_precondition_region" && !result.advanced, "data region used as executable entry");
        DWORD before{}, after{}; require(GetProcessHandleCount(GetCurrentProcess(), &before) != 0, "warm handles before");
        for (int i = 0; i < 12; ++i) require(run(argv[1]).entry_reached, "warm entry stop failed");
        require(GetProcessHandleCount(GetCurrentProcess(), &after) != 0 && before == after, "entry observer leaked warm handles");
        std::cout << "PASS entry observer: 12 scenario cases and 12 warm cycles; DLL/TLS, modified entry, fault/stall, bytes/range, pins/budget, old mode, ownership, cleanup; handles " << before << " -> " << after << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
