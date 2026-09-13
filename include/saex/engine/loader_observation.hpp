#pragma once
#include "saex/engine/suspended_image.hpp"
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
private:
    static LoaderTrace run_impl(SuspendedImage& child, void* executable_file,
        std::span<const LoaderFile* const> pins, LoaderLimits limits, const EntryStopSpec* entry,
        const ProxyStopSpec* proxy = nullptr, const StartupStopSpec* startup = nullptr) noexcept;
};
}
