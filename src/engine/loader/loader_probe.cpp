#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "saex/engine/loader_observation.hpp"
#include "saex/engine/loader_policy.generated.hpp"
#include "saex/engine/entry_policy.generated.hpp"
#include "saex/engine/proxy_policy.generated.hpp"
#include "saex/engine/startup_policy.generated.hpp"
#include "saex/engine/observed_profile.generated.hpp"
#include "saex/engine/windows_file_observation.hpp"
#include "saex/engine/launch_context.hpp"
#include <iostream>
#include <algorithm>
#include <string>

namespace {
template<std::size_t N> std::string hex(const std::array<std::byte, N>& bytes) {
    constexpr char alphabet[] = "0123456789abcdef";
    std::string result;
    for (auto byte : bytes) {
        const auto n = std::to_integer<unsigned>(byte);
        result += alphabet[n >> 4U]; result += alphabet[n & 15U];
    }
    return result;
}
std::string json_path(std::wstring_view value) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result{"\""};
    for (const auto ch : value) {
        const auto code = static_cast<unsigned short>(ch);
        result += "\\u";
        for (int shift = 12; shift >= 0; shift -= 4) result += digits[(code >> shift) & 15];
    }
    return result + '"';
}
void print(const saex::engine::LoaderTrace& trace, std::uint32_t pid, std::string_view engine_hash,
    bool reviewed, std::string_view failed_module, const saex::engine::LaunchContext* context, bool entry_mode, bool proxy_mode, bool startup_mode) {
    const auto policy = reviewed ? saex::engine::reviewed_loader_policy_id : "windows-loader-three-file-observation-v1";
    const auto digest = reviewed ? saex::engine::reviewed_loader_policy_digest : "";
    std::cout << std::boolalpha << "{\"scope\":\"" << (startup_mode ? "bounded-startup-call-observation" : proxy_mode ? "bounded-proxy-return-observation" : entry_mode ? "bounded-entry-boundary-observation" : "bounded-loader-mapping-observation")
        << "\",\"schemaVersion\":1,\"canAttach\":false,"
        << "\"initializationVerified\":false,\"policy\":\"" << policy << "\",\"policySourceDigest\":\"" << digest
        << "\",\"failedPolicyModule\":\"" << failed_module << "\",\"engineSha256\":\"" << engine_hash
        << "\",\"childCreated\":" << (pid != 0) << ",\"childPid\":" << pid << ",\"childExitConfirmed\":" << trace.exit_confirmed
        << ",\"loaderAdvanced\":" << trace.advanced << ",\"breakpointCandidate\":" << trace.breakpoint_candidate
        << ",\"eventCount\":" << trace.event_count << ",\"threadCount\":" << trace.thread_count
        << ",\"lastEventCode\":" << trace.last_event_code << ",\"lastEventThreadId\":" << trace.last_event_thread_id
        << ",\"lastEventAddress\":" << trace.last_event_address << ",\"activeModuleCount\":" << trace.active_module_count
        << ",\"unloadCount\":" << trace.unload_count
        << ",\"chargedBytes\":" << trace.charged_bytes << ",\"exceptionCode\":" << trace.exception_code
        << ",\"exceptionAddress\":" << trace.exception_address << ",\"systemError\":" << trace.system_error
        << ",\"reason\":\"" << trace.reason << "\",\"launchContext\":";
    if (context) {
        std::cout << "{\"mode\":\"explicit-directory-environment-snapshot\",\"prepared\":" << context->valid()
            << ",\"directory\":" << json_path(context->directory()) << ",\"environmentSha256\":\""
            << (context->valid() ? hex(context->environment_hash()) : "") << "\",\"environmentEntries\":" << context->entry_count()
            << ",\"environmentCodeUnits\":" << context->environment().size() << '}';
    } else std::cout << "null";
    std::cout << ",\"entryObservation\":";
    if (entry_mode) {
        std::cout << "{\"executionPolicy\":\"" << saex::engine::reviewed_entry_policy_id
            << "\",\"executionPolicySourceDigest\":\"" << saex::engine::reviewed_entry_policy_digest << "\","
            << "\"dllInitializationAllowed\":true,\"breakpointArmed\":" << trace.entry_breakpoint_armed
            << ",\"initialBreakpointContinued\":" << trace.initial_breakpoint_continued
            << ",\"boundaryReached\":" << trace.entry_reached << ",\"address\":" << trace.entry_address
            << ",\"bytesRead\":" << trace.entry_bytes_read << ",\"bytesMatch\":" << trace.entry_bytes_match
            << ",\"beforeHex\":\"" << hex(trace.entry_before) << "\",\"afterHex\":\"" << hex(trace.entry_after) << "\"}";
    } else std::cout << "null";
    std::cout << ",\"proxyObservation\":";
    if (proxy_mode) {
        std::cout << "{\"executionPolicy\":\"" << saex::engine::reviewed_proxy_policy_id
            << "\",\"executionPolicySourceDigest\":\"" << saex::engine::reviewed_proxy_policy_digest
            << "\",\"proxyExecutionAllowed\":true,\"validated\":" << trace.proxy_validated
            << ",\"breakpointArmed\":" << trace.proxy_breakpoint_armed << ",\"continued\":" << trace.proxy_continued
            << ",\"returnReached\":" << trace.proxy_return_reached << ",\"entryRestored\":" << trace.proxy_entry_restored
            << ",\"iatVerified\":" << trace.proxy_iat_verified << ",\"moduleBase\":" << trace.proxy_base
            << ",\"mappingId\":" << trace.proxy_mapping_id << ",\"returnAddress\":" << trace.proxy_return_address
            << ",\"iatRva\":" << saex::engine::reviewed_proxy_spec.iat_rva << ",\"iatBefore\":" << trace.proxy_iat_before
            << ",\"iatAfter\":" << trace.proxy_iat_after << ",\"entryAfterHex\":\"" << hex(trace.proxy_entry_after) << "\"}";
    } else std::cout << "null";
    std::cout << ",\"startupObservation\":";
    if (startup_mode) {
        std::cout << "{\"executionPolicy\":\"" << saex::engine::reviewed_startup_policy_id
            << "\",\"executionPolicySourceDigest\":\"" << saex::engine::reviewed_startup_policy_digest
            << "\",\"entryExecutionAllowed\":true,\"breakpointArmed\":" << trace.startup_breakpoint_armed
            << ",\"continued\":" << trace.startup_continued << ",\"reached\":" << trace.startup_reached
            << ",\"iatWriteObserved\":" << trace.startup_iat_write_observed << ",\"targetStable\":" << trace.startup_target_stable
            << ",\"callsiteVerified\":" << trace.startup_callsite_verified << ",\"argumentValid\":" << trace.startup_argument_valid
            << ",\"address\":" << trace.startup_address << ",\"returnAddress\":" << trace.startup_return_address
            << ",\"argumentAddress\":" << trace.startup_argument_address << ",\"targetBeforeHex\":\"" << hex(trace.startup_target_before)
            << "\",\"targetAfterHex\":\"" << hex(trace.startup_target_after) << "\",\"samples\":[";
        for (std::uint32_t i = 0; i < trace.startup_sample_count; ++i) {
            if (i) std::cout << ',';
            const auto& sample = trace.startup_samples[i];
            std::cout << "{\"rva\":" << sample.rva << ",\"length\":" << sample.length << ",\"read\":" << sample.read
                << ",\"match\":" << sample.match << ",\"beforeHex\":\"" << hex(sample.before).substr(0, sample.length * 2)
                << "\",\"afterHex\":\"" << hex(sample.after).substr(0, sample.length * 2) << "\"}";
        }
        std::cout << "]}";
    } else std::cout << "null";
    std::cout << ",\"modules\":[";
    for (std::uint32_t i = 0; i < trace.module_count; ++i) {
        if (i) std::cout << ',';
        const auto& module = trace.modules[i];
        std::cout << "{\"name\":\"" << module.file.name.data() << "\",\"sha256\":\"" << hex(module.file.sha256)
            << "\",\"volume\":" << module.file.volume << ",\"fileId\":\"" << hex(module.file.file_id)
            << "\",\"bytes\":" << module.file.bytes << ",\"base\":" << module.base << ",\"eventIndex\":" << module.event_index
            << ",\"identityRead\":" << module.identity_read << ",\"admitted\":" << module.admitted
            << ",\"mappingId\":" << module.mapping_id << ",\"unloadEventIndex\":" << module.unload_event_index
            << ",\"activeAtObservationEnd\":" << (module.mapping_id != 0 && module.unload_event_index == 0) << '}';
    }
    std::cout << "]}\n";
}
}
int wmain(int argc, wchar_t** argv) {
    const bool startup_mode = argc == 4 && std::wstring_view(argv[1]) == L"--observe-startup-call";
    const bool proxy_mode = startup_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-proxy-return");
    const bool entry_mode = proxy_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-entry-boundary");
    const bool controlled = entry_mode || (argc == 4 && std::wstring_view(argv[1]) == L"--observe-context-loader");
    if (!controlled && (argc != 3 || (std::wstring_view(argv[1]) != L"--observe-loader" && std::wstring_view(argv[1]) != L"--observe-reviewed-loader"))) {
        std::cerr << "Usage: saex_engine_loader_probe --observe-loader|--observe-reviewed-loader <gta_sa.exe>\n"
            << "       saex_engine_loader_probe --observe-context-loader|--observe-entry-boundary|--observe-proxy-return|--observe-startup-call <gta_sa.exe> <absolute-working-directory>\n"; return 2;
    }
    using namespace saex::engine;
    const bool reviewed = controlled || std::wstring_view(argv[1]) == L"--observe-reviewed-loader";
    std::uint32_t pid{};
    bool exit_confirmed{};
    std::unique_ptr<LaunchContext> context;
    const auto emit = [&](const LoaderTrace& trace, std::uint32_t child_pid, std::string_view hash, std::string_view failed = {}) {
        print(trace, child_pid, hash, reviewed, failed, context.get(), entry_mode, proxy_mode, startup_mode);
    };
    try {
        if (controlled) {
            context = std::make_unique<LaunchContext>(argv[3]);
            if (!context->valid()) { LoaderTrace result{}; result.reason = context->error(); emit(result, 0, ""); return 1; }
        }
        std::array<wchar_t, 32768> buffer{};
        const auto absolute_size = GetFullPathNameW(argv[2], static_cast<DWORD>(buffer.size()), buffer.data(), nullptr);
        if (!absolute_size || absolute_size > 32764) { LoaderTrace result{}; result.reason = "loader_executable_path"; emit(result, 0, ""); return 1; }
        const std::wstring executable_path(buffer.data());
        WindowsFileObservation executable(executable_path.c_str());
        if (!executable.error().empty()) { LoaderTrace result{}; result.reason = executable.error(); emit(result, 0, ""); return 1; }
        const auto hash = hex(executable.hash());
        EntryStopSpec entry{};
        if (entry_mode) {
            entry.rva = executable.layout().entry_rva;
            const auto offset = raw_offset(executable.layout(), entry.rva, entry.expected.size());
            if (!offset) { LoaderTrace result{}; result.reason = "entry_file_range"; emit(result, 0, hash); return 1; }
            std::copy_n(executable.bytes().begin() + *offset, entry.expected.size(), entry.expected.begin());
        }
        const auto size = GetSystemDirectoryW(buffer.data(), static_cast<UINT>(buffer.size()));
        if (!size || size >= buffer.size()) { LoaderTrace result{}; result.reason = "loader_system_directory"; emit(result, 0, hash); return 1; }
        // In this x86 process Windows resolves System32 to the x86 system directory.
        // These three retained files permit mapping observation only. No local DLLs,
        // arbitrary OS directory wildcard, server-provided policy or mod allowlist.
        const std::wstring system(buffer.data());
        std::array<std::unique_ptr<LoaderFile>, 3> basic_files{};
        std::array<const LoaderFile*, 3> basic_pins{};
        std::unique_ptr<PreparedLoaderPolicy> prepared;
        std::span<const LoaderFile* const> pins;
        if (reviewed) {
            const auto separator = executable_path.find_last_of(L"\\/");
            const auto game = executable_path.substr(0, separator == 2 ? 3 : separator);
            const auto specs = entry_mode ? std::span<const LoaderPinSpec>(reviewed_entry_specs) : std::span<const LoaderPinSpec>(reviewed_loader_specs);
            prepared = std::make_unique<PreparedLoaderPolicy>(specs, reviewed_loader_engine_hash, executable.hash(), game, system);
            if (!prepared->error().empty()) {
                LoaderTrace result{}; result.reason = prepared->error(); emit(result, 0, hash, prepared->failed_module()); return 1;
            }
            pins = prepared->pins();
        } else {
            constexpr std::array names{L"ntdll.dll", L"kernel32.dll", L"kernelbase.dll"};
            for (std::size_t i = 0; i < names.size(); ++i) {
                basic_files[i] = std::make_unique<LoaderFile>((system + L"\\" + names[i]).c_str());
                if (!basic_files[i]->valid()) { LoaderTrace result{}; result.reason = "loader_system_pin_failed"; emit(result, 0, hash); return 1; }
                basic_pins[i] = basic_files[i].get();
            }
            pins = basic_pins;
        }
        auto proxy = reviewed_proxy_spec;
        if (proxy_mode) {
            for (const auto pin : pins)
                if (std::string_view(pin->identity().name.data()) == reviewed_proxy_module) proxy.module = pin;
            if (!proxy.module) { LoaderTrace result{}; result.reason = "proxy_policy_module_missing"; emit(result, 0, hash); return 1; }
        }
        LoaderTrace trace{};
        {
            SuspendedImage child(executable_path.c_str(), executable.layout().image_size, context.get());
            pid = child.process_id();
            auto reason = child.error();
            if (reason.empty() && !child.same_file(executable.handle())) reason = "observer_file_identity";
            if (reason.empty() && child.image_base() != executable.layout().image_base) reason = "observer_image_base";
            if (reason.empty()) {
                const auto check = check_mapped_observation(child, executable.bytes(), executable.layout(), observed_profile);
                if (check != ProfileResult::matched_observation) reason = profile_result_name(check);
            }
            if (reason.empty()) trace = startup_mode ? LoaderObservation::run_to_startup_call(child, executable.handle(), pins, entry, proxy, reviewed_startup_spec)
                : proxy_mode ? LoaderObservation::run_to_proxy_return(child, executable.handle(), pins, entry, proxy)
                : entry_mode ? LoaderObservation::run_to_entry(child, executable.handle(), pins, entry)
                : LoaderObservation::run(child, executable.handle(), pins);
            else { trace.reason = reason; trace.exit_confirmed = child.created() && child.stop(); }
            exit_confirmed = trace.exit_confirmed;
        } // Cleanup and all borrowed handles remain valid before output allocation/IO.
        emit(trace, pid, hash);
        const bool complete = startup_mode ? trace.reason == "startup_call_verified" : proxy_mode ? trace.reason == "proxy_return_verified"
            : entry_mode ? (trace.entry_reached && trace.entry_bytes_read) : trace.breakpoint_candidate;
        return complete && trace.exit_confirmed ? 3 : 1;
    } catch (const std::exception&) {
        LoaderTrace result{}; result.reason = "loader_probe_exception"; result.exit_confirmed = exit_confirmed;
        emit(result, pid, ""); return 1;
    }
}
