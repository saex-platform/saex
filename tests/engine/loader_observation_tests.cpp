#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "saex/engine/loader_observation.hpp"
#include "saex/engine/loader_policy.hpp"
#include "saex/engine/launch_context.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

using namespace saex::engine;
namespace {
void require(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
struct Handle {
    HANDLE value{};
    explicit Handle(HANDLE h) : value(h) {}
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
};
struct CopiedModule {
    std::wstring directory, path;
    explicit CopiedModule(const wchar_t* source, const wchar_t* name = L"saex_loader_fixture_dll.dll") {
        wchar_t temp[32768]{}; const auto length = GetTempPathW(32768, temp);
        require(length && length < 32768, "temp path");
        directory = std::wstring(temp) + L"saex-loader-identity-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
        path = directory + L"\\" + name;
        require(CreateDirectoryW(directory.c_str(), nullptr) != 0, "unique identity directory");
        if (!CopyFileW(source, path.c_str(), TRUE)) { RemoveDirectoryW(directory.c_str()); throw std::runtime_error("identity copy"); }
    }
    ~CopiedModule() { DeleteFileW(path.c_str()); RemoveDirectoryW(directory.c_str()); }
    CopiedModule(const CopiedModule&) = delete;
    CopiedModule& operator=(const CopiedModule&) = delete;
};
constexpr const wchar_t* suffixes[]{L".dll-entered", L".tls-entered", L".main-entered"};
void absent(const std::wstring& path) {
    for (const auto suffix : suffixes) require(GetFileAttributesW((path + suffix).c_str()) == INVALID_FILE_ATTRIBUTES, "initialization canary exists");
}
void control(const wchar_t* path) {
    absent(path);
    auto command = L"\"" + std::wstring(path) + L"\"";
    STARTUPINFOW startup{}; startup.cb = sizeof(startup);
    PROCESS_INFORMATION child{};
    require(CreateProcessW(path, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child) != 0, "canary create");
    Handle p(child.hProcess), t(child.hThread);
    if (WaitForSingleObject(p.value, 5000) != WAIT_OBJECT_0) {
        TerminateProcess(p.value, 82); WaitForSingleObject(p.value, 5000); throw std::runtime_error("canary timeout");
    }
    DWORD code{}; require(GetExitCodeProcess(p.value, &code) && code == 73, "canary exit");
    for (const auto suffix : suffixes) {
        const auto marker = std::wstring(path) + suffix;
        require(GetFileAttributesW(marker.c_str()) != INVALID_FILE_ATTRIBUTES, "positive DLL/TLS/main canary missing");
        require(DeleteFileW(marker.c_str()) != 0, "canary cleanup");
    }
}
std::uint32_t image_size(void* file) {
    LARGE_INTEGER zero{}; require(SetFilePointerEx(file, zero, nullptr, FILE_BEGIN) != 0, "fixture seek");
    std::array<std::byte, 4096> bytes{}; DWORD read{};
    require(ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) && read == bytes.size(), "fixture read");
    std::uint32_t pe{}, size{}; std::memcpy(&pe, bytes.data() + 60, 4);
    require(pe < bytes.size() - 84, "fixture PE range"); std::memcpy(&size, bytes.data() + pe + 80, 4);
    return size;
}
void dump(const LoaderTrace& trace) {
    std::cout << trace.reason << " events=" << trace.event_count << " modules=" << trace.module_count << " active=" << trace.active_module_count << " unloads=" << trace.unload_count << " exception=" << trace.exception_code << '\n';
    for (std::uint32_t i = 0; i < trace.module_count; ++i) std::cout << "  " << trace.modules[i].file.name.data() << " admitted=" << trace.modules[i].admitted << " mapping=" << trace.modules[i].mapping_id << " retiredAt=" << trace.modules[i].unload_event_index << '\n';
}
void mapping_invariants(const LoaderTrace& trace) {
    std::uint32_t active{}, retired{}, admitted{};
    for (std::uint32_t i = 0; i < trace.module_count; ++i) {
        const auto& module = trace.modules[i];
        if (!module.mapping_id) { require(!module.unload_event_index, "unverified mapping retired"); continue; }
        require(module.admitted && module.identity_read && module.mapping_id == ++admitted, "mapping admitted without identity or reused ID");
        if (module.unload_event_index) {
            ++retired;
            require(module.unload_event_index > module.event_index && module.unload_event_index <= trace.event_count, "invalid retire order");
        } else {
            ++active;
            for (std::uint32_t j = i + 1; j < trace.module_count; ++j) {
                const auto& peer = trace.modules[j];
                require(!peer.mapping_id || peer.unload_event_index || peer.base != module.base, "duplicate active address");
            }
        }
    }
    require(active == trace.active_module_count && retired == trace.unload_count, "active/history counters diverged");
}
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 3) return 2;
    try {
        control(argv[1]);
        LoaderFile executable(argv[1]), fixture_dll(argv[2]), other(argv[0]);
        require(executable.valid() && fixture_dll.valid() && other.valid(), "fixture pins");
        const auto size = image_size(executable.handle());
        wchar_t path[32768]{}; const auto length = GetSystemDirectoryW(path, 32768);
        require(length && length < 32768, "system directory");
        const std::wstring system(path);
        LoaderFile ntdll((system + L"\\ntdll.dll").c_str()), kernel32((system + L"\\kernel32.dll").c_str()), kernelbase((system + L"\\kernelbase.dll").c_str());
        const std::array<const LoaderFile*, 4> pins{&ntdll, &kernel32, &kernelbase, &fixture_dll};
        for (const auto pin : pins) require(pin->valid(), "system pin");
        LoaderFile missing(L"Z:\\saex-impossible-missing-file.dll"), invalid(nullptr);
        require(!missing.valid() && !invalid.valid(), "invalid file accepted");
        LoaderFileIdentity output{}; output.bytes = 27;
        require(!inspect_loader_file(nullptr, output) && output.bytes == 27, "failed metadata changed output");
        {
            Handle write(CreateFileW(argv[2], GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
            require(write.value == INVALID_HANDLE_VALUE, "pinned module allowed writer");
        }
        const auto run = [&](std::span<const LoaderFile* const> selected, LoaderLimits limits = {}, void* expected = nullptr) {
            SuspendedImage child(argv[1], size);
            require(child.error().empty(), "loader child create");
            Handle process(OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, child.process_id()));
            require(process.value != nullptr, "observer process handle");
            const auto trace = LoaderObservation::run(child, expected ? expected : executable.handle(), selected, limits);
            mapping_invariants(trace);
            require(trace.exit_confirmed && child.stopped() && child.stop(), "loader cleanup not confirmed");
            DWORD code{};
            require(WaitForSingleObject(process.value, 0) == WAIT_OBJECT_0 && GetExitCodeProcess(process.value, &code) && code == 0x53414558, "loader process survived or wrong exit");
            require(!LoaderObservation::run(child, executable.handle(), selected).advanced, "terminal observer advanced again");
            absent(argv[1]); return trace;
        };
        auto result = run(pins); dump(result);
        require(result.breakpoint_candidate && result.advanced && result.module_count == 4, "fixture did not reach held loader breakpoint");
        require(result.last_event_code == EXCEPTION_DEBUG_EVENT && result.last_event_thread_id != 0, "terminal debug event metadata");
        require(result.last_event_address == result.exception_address && result.active_module_count == 4 && !result.unload_count, "initial mapping state");
        {
            // The imported fixture DLL is absent beside this EXE. Resolve it once
            // through explicit PATH and once through explicit cwd, never ambient state.
            CopiedModule isolated(argv[1], L"saex context fixture.exe");
            LoaderFile isolated_file(isolated.path.c_str());
            require(isolated_file.valid(), "isolated executable pin");
            LoaderFile apphelp((system + L"\\apphelp.dll").c_str());
            require(apphelp.valid(), "isolated fixture compatibility pin");
            const std::array<const LoaderFile*, 5> context_pins{&ntdll, &kernel32, &kernelbase, &fixture_dll, &apphelp};
            const std::wstring dll_path(argv[2]);
            auto dll_directory = dll_path.substr(0, dll_path.find_last_of(L"\\/"));
            std::replace(dll_directory.begin(), dll_directory.end(), L'/', L'\\');
            std::array<wchar_t, MAX_PATH> windows{};
            require(GetWindowsDirectoryW(windows.data(), static_cast<UINT>(windows.size())) != 0, "Windows directory");
            const auto root_entry = L"SystemRoot=" + std::wstring(windows.data());
            for (const bool through_path : {true, false}) {
                const auto path_entry = L"PATH=" + (through_path ? dll_directory : isolated.directory);
                const std::array<std::wstring_view, 2> environment{root_entry, path_entry};
                LaunchContext context(through_path ? isolated.directory : dll_directory, environment);
                require(context.valid(), "isolated fixture context");
                SuspendedImage child(isolated.path.c_str(), size, &context);
                require(child.error().empty(), "explicit context observer create");
                const auto trace = LoaderObservation::run(child, isolated_file.handle(), context_pins);
                mapping_invariants(trace);
                dump(trace);
                const bool fixture_found = std::any_of(trace.modules.begin(), trace.modules.begin() + trace.module_count,
                    [&](const auto& module) { return module.admitted && module.file.file_id == fixture_dll.identity().file_id && module.file.volume == fixture_dll.identity().volume; });
                require(trace.exit_confirmed && trace.breakpoint_candidate && fixture_found, "explicit cwd/PATH did not resolve imported fixture DLL");
                absent(isolated.path);
            }
        }
        {
            const std::wstring full(argv[1]); const auto game = full.substr(0, full.find_last_of(L"\\/"));
            std::array<LoaderPinSpec, 4> specs{};
            for (std::size_t i = 0; i < pins.size(); ++i) {
                const auto& id = pins[i]->identity();
                specs[i] = {i == 3 ? LoaderOrigin::game_root : LoaderOrigin::system_x86, id.name.data(), id.bytes, id.sha256};
            }
            PreparedLoaderPolicy policy(specs, executable.identity().sha256, executable.identity().sha256, game, system);
            require(policy.error().empty() && policy.pins().size() == 4, "exact policy preparation");
            result = run(policy.pins()); require(result.breakpoint_candidate, "prepared policy did not reach breakpoint");
            const auto reject_policy = [&](std::span<const LoaderPinSpec> bad, const Sha256& engine, const char* reason) {
                PreparedLoaderPolicy rejected(bad, executable.identity().sha256, engine, game, system);
                require(rejected.error() == reason && rejected.pins().empty(), "invalid policy exposed partial pins");
            };
            reject_policy(specs, other.identity().sha256, "loader_policy_engine_mismatch");
            reject_policy({}, executable.identity().sha256, "loader_policy_input");
            auto bad = specs; bad[3].sha256[0] ^= std::byte{1};
            reject_policy(bad, executable.identity().sha256, "loader_policy_file_mismatch");
            bad = specs; ++bad[3].bytes;
            reject_policy(bad, executable.identity().sha256, "loader_policy_file_mismatch");
            bad = specs; bad[3].origin = LoaderOrigin::system_x86;
            reject_policy(bad, executable.identity().sha256, "loader_policy_file_unavailable");
            bad = specs; bad[3].name = "../fixture.dll";
            reject_policy(bad, executable.identity().sha256, "loader_policy_spec");
            bad = specs; bad[3].name = "saex_bootstrap.asi";
            reject_policy(bad, executable.identity().sha256, "loader_policy_spec");
            PreparedLoaderPolicy asi_missing(bad, executable.identity().sha256, executable.identity().sha256, game, system, true);
            require(asi_missing.error() == "loader_policy_file_unavailable" && asi_missing.failed_module() == "saex_bootstrap.asi", "ASI filename opt-in not applied");
            for (auto name : {"other.asi", "../saex_bootstrap.asi", "SAEX_BOOTSTRAP.asi"}) {
                bad = specs; bad[3].name = name;
                PreparedLoaderPolicy denied(bad, executable.identity().sha256, executable.identity().sha256, game, system, true);
                require(denied.error() == "loader_policy_spec", "ASI name scope expanded");
            }
            bad = specs; bad[3].name = "saex_bootstrap.asi"; bad[3].origin = LoaderOrigin::system_x86;
            PreparedLoaderPolicy asi_system(bad, executable.identity().sha256, executable.identity().sha256, game, system, true);
            require(asi_system.error() == "loader_policy_spec", "ASI system origin allowed");
            bad = specs; bad[3].name = bad[0].name;
            reject_policy(bad, executable.identity().sha256, "loader_policy_duplicate");
            bad = specs; bad[3].origin = static_cast<LoaderOrigin>(99);
            reject_policy(bad, executable.identity().sha256, "loader_policy_spec");
            bad = specs; bad[3].sha256 = {};
            reject_policy(bad, executable.identity().sha256, "loader_policy_spec");
            bad = specs; bad[3].bytes = max_loader_file_bytes + 1;
            reject_policy(bad, executable.identity().sha256, "loader_policy_spec");
            std::array<LoaderPinSpec, 5> excessive{};
            constexpr std::array extra_names{"a.dll", "b.dll", "c.dll", "d.dll", "e.dll"};
            for (std::size_t i = 0; i < excessive.size(); ++i) excessive[i] = {LoaderOrigin::game_root, extra_names[i], max_loader_file_bytes, specs[0].sha256};
            reject_policy(excessive, executable.identity().sha256, "loader_policy_byte_limit");
            std::array<LoaderPinSpec, 65> too_many{};
            reject_policy(too_many, executable.identity().sha256, "loader_policy_input");
            LoaderFile limited(argv[2], fixture_dll.identity().bytes - 1), zero_budget(argv[2], 0);
            require(!limited.valid() && !zero_budget.valid(), "file read budget exceeded");
        }
        // Warm Windows/BCrypt once before measuring handles across complete ownership cycles.
        DWORD before{}; require(GetProcessHandleCount(GetCurrentProcess(), &before) != 0, "handle baseline");
        for (unsigned i = 0; i < 12; ++i) {
            result = run(pins);
            require(result.breakpoint_candidate && result.reason == "loader_breakpoint_candidate", "repeated loader trace");
        }
        DWORD after{}; require(GetProcessHandleCount(GetCurrentProcess(), &after) && after == before, "loader handle growth");
        result = run({}); require(result.reason == "loader_module_not_pinned" && result.module_count == 1, "empty policy allowed module");
        result = run(std::span(pins).first(3));
        require(result.reason == "loader_module_not_pinned" && std::string_view(result.modules[result.module_count - 1].file.name.data()) == "saex_loader_fixture_dll.dll", "unapproved local DLL not stopped");
        {
            CopiedModule copy(argv[2]); LoaderFile different(copy.path.c_str());
            require(different.valid() && different.identity().sha256 == fixture_dll.identity().sha256 &&
                different.identity().file_id != fixture_dll.identity().file_id, "copy identity control");
            const std::array<const LoaderFile*, 4> copied_pins{&ntdll, &kernel32, &kernelbase, &different};
            result = run(copied_pins);
            require(result.reason == "loader_module_not_pinned" && !result.breakpoint_candidate, "same name and hash from a different file accepted");
        }
        result = run(pins, {}, other.handle()); require(!result.advanced && result.reason == "loader_child_identity_or_state", "wrong exe advanced");
        const std::array<const LoaderFile*, 1> bad_pin{&missing};
        result = run(bad_pin); require(!result.advanced && result.reason == "loader_invalid_pin", "bad pin advanced");
        auto limits = LoaderLimits{}; limits.events = 1;
        result = run(pins, limits); require(!result.advanced && result.reason == "loader_event_limit", "event limit failed");
        limits = {}; limits.modules = 1;
        result = run(pins, limits); require(result.reason == "loader_module_limit" && result.module_count == 1, "module limit failed");
        limits = {}; limits.bytes = 1;
        result = run(pins, limits); require(result.reason == "loader_file_or_byte_limit" && result.charged_bytes == 0, "byte limit failed");
        for (unsigned selector = 0; selector < 6; ++selector) {
            limits = {};
            switch (selector) {
            case 0: limits.events = 129; break;
            case 1: limits.modules = 65; break;
            case 2: limits.threads = 17; break;
            case 3: limits.milliseconds = 5001; break;
            case 4: limits.bytes = 0; break;
            default: limits.milliseconds = 0; break;
            }
            result = run(pins, limits); require(!result.advanced && result.reason == "loader_invalid_limits", "invalid limits advanced");
        }
        {
            SuspendedImage child(argv[1], size); LoaderTrace foreign{};
            std::thread wrong([&] { foreign = LoaderObservation::run(child, executable.handle(), pins); }); wrong.join();
            require(foreign.reason == "loader_owner_thread" && !foreign.advanced && !child.stopped(), "foreign thread changed child");
            result = LoaderObservation::run(child, executable.handle(), pins);
            require(result.breakpoint_candidate && result.exit_confirmed, "owner could not finish"); absent(argv[1]);
        }
        std::cout << "PASS loader observation: DLL/TLS/main positive controls, held breakpoint, deny/identity/budgets/thread/terminal cleanup, 12 warm cycles handles " << before << " -> " << after << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
