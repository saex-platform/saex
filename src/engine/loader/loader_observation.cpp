#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "saex/engine/loader_observation.hpp"
#include "saex/engine/loader_mapping_ledger.hpp"
#include <algorithm>
#include <cstring>

namespace saex::engine {
bool inspect_loader_file(void* handle, LoaderFileIdentity& output, std::uint64_t byte_budget) noexcept {
    if (!handle || handle == INVALID_HANDLE_VALUE) return false;
    LoaderFileIdentity result{};
    FILE_ID_INFO id{}; LARGE_INTEGER size{}, zero{};
    if (!GetFileInformationByHandleEx(handle, FileIdInfo, &id, sizeof(id)) ||
        !GetFileSizeEx(handle, &size) || size.QuadPart <= 0 ||
        static_cast<std::uint64_t>(size.QuadPart) > std::min(byte_budget, max_loader_file_bytes)) return false;
    std::array<wchar_t, 32768> path{};
    const auto length = GetFinalPathNameByHandleW(handle, path.data(), static_cast<DWORD>(path.size()), FILE_NAME_NORMALIZED);
    if (!length || length >= path.size()) return false;
    auto start = length;
    while (start && path[start - 1] != L'\\' && path[start - 1] != L'/') --start;
    if (length - start == 0 || length - start >= result.name.size()) return false;
    for (DWORD i = start; i < length; ++i) {
        auto ch = path[i];
        if (ch >= L'A' && ch <= L'Z') ch += L'a' - L'A';
        if (!((ch >= L'a' && ch <= L'z') || (ch >= L'0' && ch <= L'9') || ch == L'.' || ch == L'_' || ch == L'-' || ch == L' ')) return false;
        result.name[i - start] = static_cast<char>(ch);
    }
    result.volume = id.VolumeSerialNumber;
    std::memcpy(result.file_id.data(), id.FileId.Identifier, result.file_id.size());
    result.bytes = static_cast<std::uint64_t>(size.QuadPart);
    if (!SetFilePointerEx(handle, zero, nullptr, FILE_BEGIN)) return false;
    BCRYPT_HASH_HANDLE hash{};
    if (BCryptCreateHash(BCRYPT_SHA256_ALG_HANDLE, &hash, nullptr, 0, nullptr, 0, 0) < 0) return false;
    std::array<unsigned char, 65536> buffer{};
    std::uint64_t remaining = result.bytes;
    bool ok = true;
    while (remaining && ok) {
        DWORD read{};
        const auto requested = static_cast<DWORD>(std::min<std::uint64_t>(remaining, buffer.size()));
        ok = ReadFile(handle, buffer.data(), requested, &read, nullptr) && read && read <= requested;
        if (ok) { ok = BCryptHashData(hash, buffer.data(), read, 0) >= 0; remaining -= read; }
    }
    if (ok) ok = BCryptFinishHash(hash, reinterpret_cast<PUCHAR>(result.sha256.data()), static_cast<ULONG>(result.sha256.size()), 0) >= 0;
    BCryptDestroyHash(hash);
    if (ok) output = result;
    return ok;
}
LoaderFile::LoaderFile(const wchar_t* path, std::uint64_t byte_budget) noexcept {
    if (!path || !*path) return;
    handle_ = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle_ == INVALID_HANDLE_VALUE) { handle_ = nullptr; return; }
    valid_ = inspect_loader_file(handle_, identity_, byte_budget);
}
LoaderFile::~LoaderFile() { if (handle_) CloseHandle(handle_); }

LoaderTrace LoaderObservation::run(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, LoaderLimits limits) noexcept {
    return run_impl(child, executable, pins, limits, nullptr);
}
LoaderTrace LoaderObservation::run_to_entry(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry, LoaderLimits limits) noexcept {
    return run_impl(child, executable, pins, limits, &entry);
}
LoaderTrace LoaderObservation::run_impl(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, LoaderLimits limits, const EntryStopSpec* entry) noexcept {
    static_assert(sizeof(void*) == 4, "loader experiment requires a same-bitness x86 observer");
    LoaderTrace trace{};
    // Wrong-thread calls cannot change the owner's pending debug event or cleanup.
    if (GetCurrentThreadId() != child.owner_thread_) { trace.reason = "loader_owner_thread"; return trace; }
    const auto finish = [&](std::string_view reason) {
        trace.reason = reason;
        trace.exit_confirmed = child.created() && child.stop();
        if (child.created() && !trace.exit_confirmed) trace.reason = "loader_exit_unconfirmed";
        return trace;
    };
    if (!limits.events || limits.events > 128 || !limits.modules || limits.modules > 64 ||
        !limits.threads || limits.threads > 16 || !limits.milliseconds || limits.milliseconds > 5000 ||
        !limits.bytes || limits.bytes > 256ULL * 1024 * 1024 || pins.size() > 64)
        return finish("loader_invalid_limits");
    for (const auto pin : pins) if (!pin || !pin->valid()) return finish("loader_invalid_pin");
    if (!child.error_.empty() || child.loader_started_ || !child.same_file(executable)) return finish("loader_child_identity_or_state");
    if (entry) {
        if (!entry->rva || entry->rva >= child.image_bytes_ || entry->expected.size() > child.image_bytes_ - entry->rva ||
            !child.copy(entry->rva, trace.entry_before) || trace.entry_before != entry->expected)
            return finish("entry_precondition_bytes");
        trace.entry_address = child.base_ + entry->rva;
        MEMORY_BASIC_INFORMATION region{};
        if (VirtualQueryEx(child.process_, reinterpret_cast<void*>(trace.entry_address), &region, sizeof(region)) != sizeof(region) ||
            region.Type != MEM_IMAGE || region.State != MEM_COMMIT || reinterpret_cast<std::uintptr_t>(region.AllocationBase) != child.base_ ||
            (region.Protect & PAGE_GUARD) || !(region.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
            return finish("entry_precondition_region");
    }
    if (entry) {
        // Debug event holds every child thread; only alter the owned main thread's DR state.
        // No guessed OS/GTA function address and no writes to instruction bytes or IP/stack.
        CONTEXT context{}; context.ContextFlags = CONTEXT_CONTROL | CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(child.thread_, &context)) {
            trace.system_error = GetLastError(); return finish("entry_context_read_failed");
        }
        if (context.Dr0 || context.Dr1 || context.Dr2 || context.Dr3 || (context.Dr7 & 0xffff20ffU) ||
            (context.EFlags & 0x100U)) return finish("entry_debug_registers_busy");
        std::array<std::byte, 16> current_entry{};
        if (!child.copy(entry->rva, current_entry) || current_entry != trace.entry_before)
            return finish("entry_preinitialization_drift");
        context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        context.Dr0 = static_cast<DWORD>(trace.entry_address); context.Dr6 = 0; context.Dr7 |= 1U;
        if (!SetThreadContext(child.thread_, &context)) {
            trace.system_error = GetLastError(); return finish("entry_context_write_failed");
        }
        CONTEXT verified{}; verified.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(child.thread_, &verified) || verified.Dr0 != trace.entry_address ||
            (verified.Dr7 & 0xffff20ffU) != 1) return finish("entry_breakpoint_arm_failed");
        trace.entry_breakpoint_armed = true;
    }
    child.loader_started_ = true;
    LoaderMappingLedger mappings(limits.modules);
    trace.event_count = 1; trace.thread_count = 1;
    trace.last_event_code = CREATE_PROCESS_DEBUG_EVENT; trace.last_event_thread_id = child.pending_thread_;
    const auto deadline = GetTickCount64() + limits.milliseconds;
    for (;;) {
        if (trace.event_count >= limits.events) return finish("loader_event_limit");
        if (GetTickCount64() >= deadline) return finish("loader_timeout");
        if (!ContinueDebugEvent(child.process_id_, child.pending_thread_, DBG_CONTINUE)) {
            trace.system_error = GetLastError(); return finish("loader_continue_failed");
        }
        child.pending_ = false; trace.advanced = true;
        if (trace.entry_breakpoint_armed && trace.breakpoint_candidate) trace.initial_breakpoint_continued = true;
        DEBUG_EVENT event{};
        const auto now = GetTickCount64();
        if (now >= deadline) return finish("loader_timeout");
        if (!WaitForDebugEvent(&event, static_cast<DWORD>(deadline - now))) {
            trace.system_error = GetLastError(); return finish("loader_wait_failed");
        }
        ++trace.event_count;
        trace.last_event_code = event.dwDebugEventCode; trace.last_event_thread_id = event.dwThreadId;
        trace.last_event_address = 0;
        // Dedicated debugger thread is required. Never resume an unrelated process.
        if (event.dwProcessId != child.process_id_) {
            if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT && event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
            if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT && event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
            return finish("loader_event_identity");
        }
        child.pending_ = true; child.pending_thread_ = event.dwThreadId;
        if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            trace.last_event_address = reinterpret_cast<std::uintptr_t>(event.u.LoadDll.lpBaseOfDll);
            const auto file = event.u.LoadDll.hFile;
            if (trace.module_count >= limits.modules) {
                if (file) CloseHandle(file);
                return finish("loader_module_limit");
            }
            auto& module = trace.modules[trace.module_count++];
            module.base = reinterpret_cast<std::uintptr_t>(event.u.LoadDll.lpBaseOfDll);
            module.event_index = trace.event_count;
            LARGE_INTEGER bytes{};
            const bool budget = file && GetFileSizeEx(file, &bytes) && bytes.QuadPart > 0 &&
                static_cast<std::uint64_t>(bytes.QuadPart) <= limits.bytes - trace.charged_bytes;
            if (!budget) { if (file) CloseHandle(file); return finish("loader_file_or_byte_limit"); }
            trace.charged_bytes += static_cast<std::uint64_t>(bytes.QuadPart);
            const bool inspected = inspect_loader_file(file, module.file);
            if (file) CloseHandle(file);
            module.identity_read = inspected;
            if (!inspected || !module.base) return finish("loader_module_metadata");
            for (const auto pin : pins) {
                const auto& expected = pin->identity();
                if (expected.volume == module.file.volume && expected.file_id == module.file.file_id &&
                    expected.bytes == module.file.bytes && expected.sha256 == module.file.sha256) { module.admitted = true; break; }
            }
            if (!module.admitted) return finish("loader_module_not_pinned");
            const auto transition = mappings.admit(module.base, trace.event_count);
            if (!transition.error.empty()) return finish(transition.error);
            module.mapping_id = transition.mapping_id;
            trace.active_module_count = mappings.active_count();
        } else if (event.dwDebugEventCode == UNLOAD_DLL_DEBUG_EVENT) {
            trace.last_event_address = reinterpret_cast<std::uintptr_t>(event.u.UnloadDll.lpBaseOfDll);
            const auto transition = mappings.retire(trace.last_event_address, trace.event_count);
            if (!transition.error.empty()) return finish(transition.error);
            auto end = trace.modules.begin() + trace.module_count;
            const auto retired = std::find_if(trace.modules.begin(), end,
                [&](const auto& module) { return module.mapping_id == transition.mapping_id; });
            if (retired == end) return finish("loader_mapping_history_mismatch");
            retired->unload_event_index = trace.event_count;
            trace.active_module_count = mappings.active_count();
            trace.unload_count = mappings.unload_count();
            // Do not read the retired address or refund any lifetime load/byte budget.
        } else if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            if (++trace.thread_count > limits.threads) return finish("loader_thread_limit");
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            trace.exception_code = event.u.Exception.ExceptionRecord.ExceptionCode;
            trace.exception_address = reinterpret_cast<std::uintptr_t>(event.u.Exception.ExceptionRecord.ExceptionAddress);
            trace.last_event_address = trace.exception_address;
            if (entry && trace.initial_breakpoint_continued) {
                CONTEXT context{}; context.ContextFlags = CONTEXT_CONTROL | CONTEXT_DEBUG_REGISTERS;
                if (trace.exception_code != EXCEPTION_SINGLE_STEP || !event.u.Exception.dwFirstChance ||
                    event.dwThreadId != GetThreadId(child.thread_) || trace.exception_address != trace.entry_address)
                    return finish("entry_unexpected_exception");
                if (!GetThreadContext(child.thread_, &context)) {
                    trace.system_error = GetLastError(); return finish("entry_context_read_failed");
                }
                // DR6 B0 only; reject single-step/task-switch/debug-register causes.
                // DR7 L0 enabled, G0/other slots and all RW/LEN bits clear.
                if (context.Eip != trace.entry_address || context.Dr0 != trace.entry_address ||
                    (context.Dr6 & 0xe00fU) != 1 || (context.Dr7 & 0xffff00ffU) != 1)
                    return finish("entry_breakpoint_identity");
                trace.entry_reached = true;
                trace.entry_bytes_read = child.copy(entry->rva, trace.entry_after);
                if (!trace.entry_bytes_read) return finish("entry_bytes_unreadable");
                trace.entry_bytes_match = trace.entry_after == trace.entry_before;
                return finish(trace.entry_bytes_match ? "entry_boundary_reached" : "entry_boundary_modified");
            }
            MEMORY_BASIC_INFORMATION region{};
            if (trace.exception_code == EXCEPTION_BREAKPOINT && event.u.Exception.dwFirstChance &&
                event.dwThreadId == GetThreadId(child.thread_) &&
                VirtualQueryEx(child.process_, event.u.Exception.ExceptionRecord.ExceptionAddress, &region, sizeof(region)) == sizeof(region) &&
                region.State == MEM_COMMIT && region.Type == MEM_IMAGE) {
                for (std::uint32_t i = 0; i < trace.module_count; ++i) {
                    const auto& module = trace.modules[i];
                    if (module.admitted && mappings.active(module.mapping_id, module.base) &&
                        std::string_view(module.file.name.data()) == "ntdll.dll" &&
                        module.base == reinterpret_cast<std::uintptr_t>(region.AllocationBase)) trace.breakpoint_candidate = true;
                }
            }
            if (entry && trace.breakpoint_candidate) {
                std::array<std::byte, 16> before_initialization{};
                if (!child.copy(entry->rva, before_initialization) || before_initialization != trace.entry_before)
                    return finish("entry_preinitialization_drift");
                CONTEXT verified{}; verified.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                if (!GetThreadContext(child.thread_, &verified) || verified.Dr0 != trace.entry_address ||
                    (verified.Dr7 & 0xffff20ffU) != 1) return finish("entry_breakpoint_lost_before_initialization");
                // Set only after the following ContinueDebugEvent actually succeeds.
                continue;
            }
            // Even the expected first loader breakpoint remains pending until kill.
            return finish(trace.breakpoint_candidate ? "loader_breakpoint_candidate" : "loader_exception");
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            child.exit_seen_ = true; return finish("loader_early_exit");
        } else {
            if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT && event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
            return finish("loader_unexpected_event");
        }
    }
}
}
