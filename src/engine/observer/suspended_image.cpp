#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "saex/engine/suspended_image.hpp"
#include "saex/engine/launch_context.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <string>

namespace saex::engine {
namespace {
constexpr DWORD observer_exit_code = 0x53414558;
constexpr ULONGLONG timeout_ms = 5000;
void close(void*& value) noexcept { if (value) { CloseHandle(value); value = nullptr; } }
void close_event_file(DEBUG_EVENT& event) noexcept {
    if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT && event.u.CreateProcessInfo.hFile)
        CloseHandle(event.u.CreateProcessInfo.hFile);
    if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT && event.u.LoadDll.hFile)
        CloseHandle(event.u.LoadDll.hFile);
    // Debug process/thread handles are closed by Windows on continued exit events.
}
}

SuspendedImage::SuspendedImage(const wchar_t* executable, std::size_t size, const LaunchContext* context)
    : owner_thread_(GetCurrentThreadId()), image_bytes_(size) {
    const auto fail = [&](std::string_view reason) { error_ = reason; system_error_ = GetLastError(); };
    if (!executable || !*executable || !size || size > max_image_bytes) { error_ = "invalid_observer_input"; return; }
    if (context && !context->valid()) { error_ = "observer_invalid_context"; return; }
    if (context && !context->directory_matches()) { error_ = "observer_context_directory_changed"; return; }
    std::array<wchar_t, 32768> full_path{};
    const auto count = GetFullPathNameW(executable, static_cast<DWORD>(full_path.size()), full_path.data(), nullptr);
    if (!count || count > 32764) { fail("observer_path_failed"); return; }
    // Allocate all throwing objects BEFORE creating OS resources or the child.
    std::wstring command = L"\"" + std::wstring(full_path.data()) + L"\"";
    std::vector<wchar_t> environment;
    if (context) environment.assign(context->environment().begin(), context->environment().end());
    job_ = CreateJobObjectW(nullptr, nullptr);
    if (!job_) { fail("observer_job_failed"); return; }
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job_, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
        fail("observer_job_limits_failed"); return;
    }
    STARTUPINFOW startup{}; startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW; startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION created{};
    if (!CreateProcessW(full_path.data(), command.data(), nullptr, nullptr, FALSE,
        DEBUG_ONLY_THIS_PROCESS | CREATE_NO_WINDOW | (context ? CREATE_UNICODE_ENVIRONMENT : 0),
        context ? environment.data() : nullptr, context ? context->directory().c_str() : nullptr, &startup, &created)) {
        fail("observer_create_failed"); return;
    }
    process_ = created.hProcess; thread_ = created.hThread; process_id_ = created.dwProcessId;
    if (!AssignProcessToJobObject(job_, process_)) { fail("observer_job_assign_failed"); return; }
    DEBUG_EVENT event{};
    if (!WaitForDebugEvent(&event, static_cast<DWORD>(timeout_ms))) { fail("observer_create_event_timeout"); return; }
    // No unknown process is ever continued, read or terminated through this object.
    if (event.dwProcessId != process_id_) { close_event_file(event); error_ = "observer_event_identity"; return; }
    pending_ = true; pending_thread_ = event.dwThreadId;
    exit_seen_ = event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT;
    if (event.dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT) {
        close_event_file(event); error_ = "observer_first_event"; return;
    }
    image_file_ = event.u.CreateProcessInfo.hFile;
    base_ = reinterpret_cast<std::uintptr_t>(event.u.CreateProcessInfo.lpBaseOfImage);
    if (!image_file_ || !base_ || size > std::numeric_limits<std::uintptr_t>::max() - base_) {
        error_ = "observer_image_metadata"; return;
    }
}

bool SuspendedImage::same_file(void* verified) const noexcept {
    if (!error_.empty() || !pending_ || kill_requested_ || GetCurrentThreadId() != owner_thread_ || !verified) return false;
    FILE_ID_INFO expected{}, actual{};
    return GetFileInformationByHandleEx(verified, FileIdInfo, &expected, sizeof(expected)) &&
        GetFileInformationByHandleEx(image_file_, FileIdInfo, &actual, sizeof(actual)) &&
        expected.VolumeSerialNumber == actual.VolumeSerialNumber &&
        std::memcmp(expected.FileId.Identifier, actual.FileId.Identifier, sizeof(actual.FileId.Identifier)) == 0;
}

bool SuspendedImage::copy(std::uint32_t rva, std::span<std::byte> output) const noexcept {
    if (!error_.empty() || !pending_ || kill_requested_ || GetCurrentThreadId() != owner_thread_ ||
        output.empty() || output.size() > 1024 || rva > image_bytes_ || output.size() > image_bytes_ - rva) return false;
    auto cursor = base_ + rva;
    const auto end = cursor + output.size();
    while (cursor < end) {
        MEMORY_BASIC_INFORMATION region{};
        if (VirtualQueryEx(process_, reinterpret_cast<const void*>(cursor), &region, sizeof(region)) != sizeof(region) ||
            region.State != MEM_COMMIT || region.Type != MEM_IMAGE ||
            reinterpret_cast<std::uintptr_t>(region.AllocationBase) != base_ || (region.Protect & PAGE_GUARD)) return false;
        const auto protection = region.Protect & 0xffU;
        if (protection != PAGE_READONLY && protection != PAGE_READWRITE && protection != PAGE_WRITECOPY &&
            protection != PAGE_EXECUTE_READ && protection != PAGE_EXECUTE_READWRITE && protection != PAGE_EXECUTE_WRITECOPY) return false;
        const auto start = reinterpret_cast<std::uintptr_t>(region.BaseAddress);
        if (start > cursor || region.RegionSize > std::numeric_limits<std::uintptr_t>::max() - start ||
            start + region.RegionSize <= cursor) return false;
        cursor = std::min<std::uintptr_t>(end, start + region.RegionSize);
    }
    SIZE_T copied{};
    return ReadProcessMemory(process_, reinterpret_cast<const void*>(base_ + rva), output.data(), output.size(), &copied) && copied == output.size();
}

bool SuspendedImage::stop() noexcept {
    if (!process_) return true;
    if (stopped_) return true;
    if (GetCurrentThreadId() != owner_thread_) return false;
    const auto deadline = GetTickCount64() + timeout_ms;
    if (!kill_requested_) {
        // Default mode still holds the first event; loader mode holds its terminal
        // event. Kill always precedes cleanup continuation, never releasing that phase.
        kill_requested_ = TerminateProcess(process_, observer_exit_code) != 0;
        if (!kill_requested_) kill_requested_ = TerminateJobObject(job_, observer_exit_code) != 0;
        if (!kill_requested_) return false;
    }
    close(image_file_);
    if (pending_) {
        if (!ContinueDebugEvent(process_id_, pending_thread_, DBG_CONTINUE)) return false;
        pending_ = false;
    }
    unsigned events{};
    while (!exit_seen_ && events < 64) {
        const auto now = GetTickCount64();
        if (now >= deadline) break;
        DEBUG_EVENT event{};
        if (!WaitForDebugEvent(&event, static_cast<DWORD>(std::min<ULONGLONG>(250, deadline - now)))) {
            if (GetLastError() == ERROR_SEM_TIMEOUT) continue;
            return false;
        }
        ++events;
        close_event_file(event);
        if (event.dwProcessId != process_id_) return false;
        exit_seen_ = event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT;
        pending_ = true; pending_thread_ = event.dwThreadId;
        const auto status = event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT ? DBG_EXCEPTION_NOT_HANDLED : DBG_CONTINUE;
        if (!ContinueDebugEvent(process_id_, pending_thread_, status)) return false;
        pending_ = false;
    }
    if (!exit_seen_) return false;
    const auto now = GetTickCount64();
    const DWORD remaining = now < deadline ? static_cast<DWORD>(deadline - now) : 0;
    stopped_ = WaitForSingleObject(process_, remaining) == WAIT_OBJECT_0;
    return stopped_;
}

SuspendedImage::~SuspendedImage() {
    static_cast<void>(stop());
    close(image_file_);
    close(job_); // Last-resort kill ownership, also when explicit stop could not confirm exit.
    close(thread_);
    close(process_);
}
}
