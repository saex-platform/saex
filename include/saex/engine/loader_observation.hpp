#pragma once
#include "saex/engine/suspended_image.hpp"
#include "saex/engine/bootstrap_lifecycle.hpp"
#include "saex/engine/frame_target.hpp"
#include "saex/engine/startup_return.hpp"
#include "saex/engine/crt_startup.hpp"
#include "saex/engine/application_entry.hpp"
#include "saex/engine/platform_startup.hpp"
#include "saex/engine/platform_suppression.hpp"
#include "saex/engine/cd_stream_channels.hpp"
#include <array>

namespace saex::engine {
inline constexpr std::uint64_t max_loader_file_bytes = 64ULL * 1024 * 1024;
struct LoaderFileIdentity {
    std::uint64_t volume{}, bytes{};
    std::array<std::byte, 16> file_id{};
    Sha256 sha256{};
    std::array<char, 128> name{};
};
// Same-handle size, file ID and bounded SHA-256. Does not load/parse executable code.
bool inspect_loader_file(void* handle, LoaderFileIdentity& output, std::uint64_t byte_budget = max_loader_file_bytes) noexcept;
class LoaderFile final {
public:
    explicit LoaderFile(const wchar_t* path, std::uint64_t byte_budget = max_loader_file_bytes) noexcept;
    ~LoaderFile();
    LoaderFile(const LoaderFile&) = delete;
    LoaderFile& operator=(const LoaderFile&) = delete;
    [[nodiscard]] bool valid() const noexcept { return valid_; }
    [[nodiscard]] void* handle() const noexcept { return handle_; }
    [[nodiscard]] const LoaderFileIdentity& identity() const noexcept { return identity_; }

private:
    void* handle_{};
    LoaderFileIdentity identity_{};
    bool valid_{};
};
struct LoaderLimits {
    std::uint32_t events{128}, modules{64}, threads{16}, milliseconds{5000};
    std::uint64_t bytes{256ULL * 1024 * 1024};
};
struct LoaderModule {
    LoaderFileIdentity file{};
    std::uintptr_t base{};
    std::uint32_t event_index{}, mapping_id{}, unload_event_index{};
    bool identity_read{}, admitted{};
};
struct StartupSample {
    std::uint32_t rva{}, length{};
    std::array<std::byte, 16> before{}, after{};
    bool read{}, match{};
};
struct BindingSlotSpec {
    std::string_view name;
    std::uint32_t slot_rva{}, target_rva{};
};
struct BindingSlotObservation {
    std::array<char, 32> name{};
    std::uint32_t slot_rva{}, target_rva{}, before{}, after{};
    std::array<std::byte, 16> target_before{}, target_after{};
    bool target_stable{}, match{};
};
struct LoaderTrace {
    std::array<LoaderModule, 64> modules{};
    std::uint32_t module_count{}, event_count{}, thread_count{}, exception_code{}, system_error{};
    std::uint32_t last_event_code{}, last_event_thread_id{};
    std::uint32_t active_module_count{}, unload_count{};
    std::uintptr_t exception_address{}, last_event_address{};
    std::uint64_t charged_bytes{};
    bool advanced{}, breakpoint_candidate{}, exit_confirmed{};
    bool entry_breakpoint_armed{}, initial_breakpoint_continued{}, entry_reached{}, entry_bytes_read{}, entry_bytes_match{};
    std::uintptr_t entry_address{};
    std::array<std::byte, 16> entry_before{}, entry_after{};
    bool proxy_validated{}, proxy_breakpoint_armed{}, proxy_continued{}, proxy_return_reached{};
    bool proxy_entry_restored{}, proxy_iat_verified{};
    std::uintptr_t proxy_base{}, proxy_return_address{};
    std::uint32_t proxy_mapping_id{}, proxy_iat_before{}, proxy_iat_after{};
    std::array<std::byte, 16> proxy_entry_after{};
    bool startup_breakpoint_armed{}, startup_continued{}, startup_reached{}, startup_iat_write_observed{};
    bool startup_target_stable{}, startup_callsite_verified{}, startup_argument_valid{};
    std::uintptr_t startup_address{}, startup_return_address{}, startup_argument_address{};
    std::array<std::byte, 16> startup_target_before{}, startup_target_after{};
    std::array<StartupSample, 4> startup_samples{};
    std::uint32_t startup_sample_count{};
    bool codec_breakpoint_armed{}, codec_continued{}, codec_reached{}, codec_shape_verified{}, codec_modules_verified{};
    std::uintptr_t codec_return_address{}, codec_handle{};
    std::uint32_t codec_arm_event{}, codec_module_count{};
    std::array<std::uint32_t, 3> codec_mapping_ids{};
    bool binding_breakpoint_armed{}, binding_continued{}, binding_reached{}, binding_verified{};
    std::uintptr_t binding_stop_address{};
    std::uint32_t binding_count{};
    std::array<BindingSlotObservation, 8> bindings{};
    bool asi_call_armed{}, asi_scan_continued{}, asi_call_reached{}, asi_path_verified{};
    bool asi_return_armed{}, asi_load_continued{}, asi_return_reached{}, asi_verified{};
    std::uintptr_t asi_call_address{}, asi_return_address{}, asi_handle{};
    std::uint32_t asi_arm_event{}, asi_mapping_id{}, asi_stack_before{}, asi_stack_after{};
    BootstrapLifecycleObservation bootstrap{};
    FrameTargetObservation frame_target{};
    StartupReturnObservation startup_return{};
    CrtStartupObservation crt_startup{};
    ApplicationEntryObservation application_entry{};
    PlatformStartupObservation platform_startup{};
    PlatformSuppressionObservation platform_suppression{};
    InstanceStartupObservation instance_startup{};
    EventDispatchObservation event_dispatch{};
    ApplicationRoutingObservation application_routing{};
    GamePreludeObservation game_prelude{};
    FileManagerEntryObservation file_manager_entry{};
    CwdSehObservation cwd_seh{};
    CwdLockObservation cwd_lock{};
    CwdAcquireObservation cwd_acquire{};
    CwdQueryObservation cwd_query{};
    CwdCopyObservation cwd_copy{};
    CwdReturnObservation cwd_return{};
    FileManagerReadyObservation file_manager_ready{};
    CdStreamTablesObservation cd_stream_tables{};
    CdStreamDiskObservation cd_stream_disk{};
    CdStreamChannelsObservation cd_stream_channels{};
    CdStreamAllocationObservation cd_stream_allocation{};
    std::string_view reason{"loader_not_started"};
};
struct EntryStopSpec {
    std::uint32_t rva{};
    std::array<std::byte, 16> expected{};
};
// Trusted, compiled experiment recipe; never populated by a remote caller.
// Thunk shape: CALL rel32 (5 bytes), JMP [absolute return slot] (6 bytes).
struct ProxyStopSpec {
    const LoaderFile* module{};
    std::uint32_t thunk_rva{}, call_target_rva{}, return_slot_rva{}, iat_rva{}, iat_target_rva{};
};
// Separate permission to run the original entry until the first proxy startup call.
// Samples are observations, not unpack/ABI approval. Trusted caller retains the span.
struct StartupStopSpec { std::span<const ImageAnchor> samples; };
// Separate permission for one reviewed PUSH name / CALL [LoadLibraryA IAT].
// Stop before MOV EDI,EAX at its return. modules[0] is the returned root DLL;
// all members must be newly mapped after arming and retained in pins.
struct CodecStopSpec {
    std::uint32_t sequence_rva{}, name_rva{}, load_slot_rva{};
    std::string_view requested_name;
    std::span<const LoaderFile* const> modules;
};
// Trusted exact-file recipe; direct root exports only, no forwarder resolution.
struct BindingStopSpec {
    std::uint32_t stop_rva{}, proc_slot_rva{};
    std::array<std::byte, 16> expected_stop{};
    std::span<const BindingSlotSpec> slots;
};
// One trusted artifact and exact absolute ASCII path, never an arbitrary ASI allowlist.
// Stops before CALL [the codec LoadLibraryA slot], then immediately after its return.
// No bootstrap export is invoked. The caller retains the pin and path for the call.
struct AsiStopSpec {
    const LoaderFile* module{};
    std::uint32_t call_rva{}, end_rva{};
    std::array<std::byte, 16> expected_return{};
    std::array<std::byte, 16> expected_end{};
    std::string_view requested_path;
};
// Development-only x86 experiment. Dedicated owner thread, borrowed pins retained
// for the entire call. An admitted mapping is NOT permission to call DLL initializers.
// run() stops on the first exception. run_to_entry() is a separate explicit permit.
// Known active mappings can retire on UNLOAD; every subsequent LOAD rechecks pins.
class LoaderObservation final {
public:
    static LoaderTrace run(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, LoaderLimits limits = {}) noexcept;
    // Separate, explicit initialization experiment: pinned DLL/TLS code MAY execute.
    // Arms main-thread DR0 at the reviewed PE entry. Never continues its hit.
    // Not a sandbox or a guarantee against hostile code changing debug registers.
    static LoaderTrace run_to_entry(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry, LoaderLimits limits = {}) noexcept;
    static LoaderTrace run_to_proxy_return(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, LoaderLimits limits = {}) noexcept;
    static LoaderTrace run_to_startup_call(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, LoaderLimits limits = {}) noexcept;
    static LoaderTrace run_to_codec_return(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup,
        const CodecStopSpec& codec, LoaderLimits limits = {}) noexcept;
    static LoaderTrace run_to_codec_bindings(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, LoaderLimits limits = {}) noexcept;
    static LoaderTrace run_to_asi_return(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, LoaderLimits limits = {}) noexcept;
    // Explicit invasive experiment: bounded stack data writes and main-thread EIP/ESP
    // redirection to this pinned artifact's audited exports. Always terminates child;
    // never resumes gameplay, injects code, or accepts an arbitrary process/function.
    static LoaderTrace run_bootstrap_lifecycle(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi,
        const BootstrapLifecycleSpec& bootstrap, LoaderLimits limits = {}) noexcept;
    // Same bootstrap execution boundary, plus read-only EXE candidate samples.
    // No native frame function is invoked or armed as a breakpoint.
    static LoaderTrace run_frame_target_observation(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const BootstrapLifecycleSpec& bootstrap,
        const FrameTargetSpec& frame, LoaderLimits limits = {}) noexcept;
    // Separate natural startup continuation. Allows the reviewed loader to change
    // this owned EXE image's protection; never calls bootstrap or rewrites its context.
    static LoaderTrace run_to_startup_return(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        LoaderLimits limits = {}) noexcept;
    // Natural CRT I/O return, then stop BEFORE the reviewed initializer CALL.
    static LoaderTrace run_to_crt_startup(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, LoaderLimits limits = {}) noexcept;
    // Explicit natural initializer execution. Stop before the first application instruction.
    static LoaderTrace run_to_application_entry(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, LoaderLimits limits = {}) noexcept;
    // Natural prologue execution only. Never executes the host-setting API call.
    static LoaderTrace run_to_platform_startup(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        LoaderLimits limits = {}) noexcept;
    // Invasive, owned-child-only context substitution at one verified CALL.
    // Synthesizes FALSE, retains last-error; never executes the API or patches memory.
    static LoaderTrace run_platform_suppression(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, LoaderLimits limits = {}) noexcept;
    // Natural named-event creation after the separately validated CALL suppression.
    // Existing-instance and failed creation stop before any window activation branch.
    static LoaderTrace run_instance_startup(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, LoaderLimits limits = {}) noexcept;
    // Natural dispatcher prologue only; stop before AppEventHandler CALL.
    static LoaderTrace run_event_dispatch(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, LoaderLimits limits = {}) noexcept;
    // Reviewed event-24 routing only; stop before the first game initializer CALL.
    static LoaderTrace run_application_routing(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, LoaderLimits limits = {}) noexcept;
    // First two initializer helpers only; stop before CFileMgr CALL.
    static LoaderTrace run_game_prelude(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, LoaderLimits limits = {}) noexcept;
    // Natural CFileMgr prologue only; stop before the CRT cwd CALL.
    static LoaderTrace run_file_manager_entry(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, LoaderLimits limits = {}) noexcept;
    // CRT SEH registration only; stop before the lock path.
    static LoaderTrace run_cwd_seh(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, LoaderLimits limits = {}) noexcept;
    // Select slot seven and compare with zero; no branch or critical-section call.
    static LoaderTrace run_cwd_lock(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, LoaderLimits limits = {}) noexcept;
    // Existing unowned CRT critical section: natural API and selector return.
    static LoaderTrace run_cwd_acquire(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, LoaderLimits limits = {}) noexcept;
    // Natural drive-zero directory API return; no copy/unlock/SEH removal.
    static LoaderTrace run_cwd_query(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, LoaderLimits limits = {}) noexcept;
    // Natural copy return; no argument cleanup, helper return, unlock or SEH removal.
    static LoaderTrace run_cwd_copy(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, const CwdCopySpec& copy, LoaderLimits limits = {}) noexcept;
    // Natural cookie check and helper return; wrapper arguments and lock remain live.
    static LoaderTrace run_cwd_return(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, const CwdCopySpec& copy, const CwdReturnSpec& completion, LoaderLimits limits = {}) noexcept;
    // Full cwd cleanup, lock release, SEH removal and natural CFileMgr return.
    static LoaderTrace run_file_manager_ready(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, const CwdCopySpec& copy, const CwdReturnSpec& completion, const FileManagerReadySpec& ready, LoaderLimits limits = {}) noexcept;
    static LoaderTrace run_cd_stream_tables(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, const CwdCopySpec& copy, const CwdReturnSpec& completion, const FileManagerReadySpec& ready, const CdStreamTablesSpec& tables, LoaderLimits limits = {}) noexcept;
    static LoaderTrace run_cd_stream_disk(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, const CwdCopySpec& copy, const CwdReturnSpec& completion, const FileManagerReadySpec& ready, const CdStreamTablesSpec& tables, const CdStreamDiskSpec& disk, LoaderLimits limits = {}) noexcept;
    static LoaderTrace run_cd_stream_allocation(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, const CwdCopySpec& copy, const CwdReturnSpec& completion, const FileManagerReadySpec& ready, const CdStreamTablesSpec& tables, const CdStreamDiskSpec& disk, const CdStreamAllocationSpec& allocation, LoaderLimits limits = {160}) noexcept;
    static LoaderTrace run_cd_stream_channels(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
        const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
        const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
        const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
        const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, const CwdCopySpec& copy, const CwdReturnSpec& completion, const FileManagerReadySpec& ready, const CdStreamTablesSpec& tables, const CdStreamDiskSpec& disk, const CdStreamAllocationSpec& allocation, const CdStreamChannelsSpec& channels, LoaderLimits limits = {160}) noexcept;

private:
    static LoaderTrace run_impl(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, LoaderLimits limits, const EntryStopSpec* entry,
        const ProxyStopSpec* proxy = nullptr, const StartupStopSpec* startup = nullptr,
        const CodecStopSpec* codec = nullptr, const BindingStopSpec* binding = nullptr,
        const AsiStopSpec* asi = nullptr, const BootstrapLifecycleSpec* bootstrap = nullptr, const FrameTargetSpec* frame = nullptr, const StartupReturnSpec* tail = nullptr, const CrtStartupSpec* crt = nullptr, const ApplicationEntrySpec* application = nullptr, const PlatformStartupSpec* platform = nullptr, const PlatformSuppressionSpec* suppression = nullptr, const InstanceStartupSpec* instance = nullptr, const EventDispatchSpec* dispatch = nullptr, const ApplicationRoutingSpec* routing = nullptr, const GamePreludeSpec* prelude = nullptr, const FileManagerEntrySpec* manager = nullptr, const CwdSehSpec* seh = nullptr, const CwdLockSpec* lock = nullptr, const CwdAcquireSpec* acquire = nullptr, const CwdQuerySpec* query = nullptr, const CwdCopySpec* copy = nullptr, const CwdReturnSpec* completion = nullptr, const FileManagerReadySpec* ready = nullptr, const CdStreamTablesSpec* tables = nullptr, const CdStreamDiskSpec* disk = nullptr, const CdStreamAllocationSpec* allocation = nullptr, const CdStreamChannelsSpec* channels = nullptr) noexcept;
};
}
