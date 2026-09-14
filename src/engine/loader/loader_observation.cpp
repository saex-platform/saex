#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "saex/engine/loader_observation.hpp"
#include "saex/engine/loader_mapping_ledger.hpp"
#include <algorithm>
#include <cstring>
#include <type_traits>

namespace saex::engine {
namespace {
bool same_named_event(HANDLE process,std::uint32_t value,std::string_view name) noexcept {
    if (!value || value>=0xfffffff0U || name.empty() || name.size()>96) return false;
    // SDK installations may not ship Kernelbase.lib. Resolve the documented
    // observer-side export from an already loaded system module; absence rejects.
    const auto module=GetModuleHandleW(L"kernelbase.dll");
    const auto address=module ? GetProcAddress(module,"CompareObjectHandles") : nullptr;
    decltype(&CompareObjectHandles) compare{};
    static_assert(sizeof(compare)==sizeof(address)); std::memcpy(&compare,&address,sizeof(compare));
    if (!compare) return false;
    std::array<char,97> buffer{}; std::copy(name.begin(),name.end(),buffer.begin());
    HANDLE duplicate{};
    if (!DuplicateHandle(process,reinterpret_cast<HANDLE>(value),GetCurrentProcess(),&duplicate,
        SYNCHRONIZE,FALSE,0)) return false;
    const auto named=OpenEventA(SYNCHRONIZE,FALSE,buffer.data());
    const bool same=named && compare(duplicate,named);
    if (named) CloseHandle(named);
    CloseHandle(duplicate);
    return same; // Never waits, sets, resets or closes the child's handle.
}

// Hash an admitted allocation while masking only its four-byte back-pointer.
// VirtualQueryEx describes pages; ownership comes from the observed HeapAlloc call.
bool allocation_hash(HANDLE process,const CdStreamAllocationLayout& block,std::array<std::byte,32>& digest) noexcept {
    if(!block.bytes || block.bytes>67584 || block.raw>UINT32_MAX-block.bytes ||
        block.metadata<block.raw || block.metadata>block.raw+block.bytes-4)return false;
    BCRYPT_HASH_HANDLE hash{};if(BCryptCreateHash(BCRYPT_SHA256_ALG_HANDLE,&hash,nullptr,0,nullptr,0,0)<0)return false;
    std::array<unsigned char,256> bytes{};bool ok=true;
    for(DWORD offset=0;offset<block.bytes && ok;) {
        const auto address=block.raw+offset;MEMORY_BASIC_INFORMATION region{};
        ok=VirtualQueryEx(process,reinterpret_cast<void*>(address),&region,sizeof(region))==sizeof(region) &&
            region.State==MEM_COMMIT && region.Type==MEM_PRIVATE && region.Protect==PAGE_READWRITE &&
            address>=reinterpret_cast<std::uintptr_t>(region.BaseAddress);
        if(!ok)break;
        const auto within=address-reinterpret_cast<std::uintptr_t>(region.BaseAddress);
        if(within>=region.RegionSize){ok=false;break;}
        const auto length=static_cast<DWORD>(std::min<std::size_t>({bytes.size(),block.bytes-offset,region.RegionSize-within}));
        SIZE_T read{};ok=ReadProcessMemory(process,reinterpret_cast<void*>(address),bytes.data(),length,&read) && read==length;
        if(!ok)break;
        for(DWORD i=0;i<length;++i)if(address+i>=block.metadata && address+i<block.metadata+4)bytes[i]=0;
        ok=BCryptHashData(hash,bytes.data(),length,0)>=0;offset+=length;
    }
    if(ok)ok=BCryptFinishHash(hash,reinterpret_cast<PUCHAR>(digest.data()),static_cast<ULONG>(digest.size()),0)>=0;
    BCryptDestroyHash(hash);return ok;
}
class WindowsSuppressionContext final : public SuppressionContextPort {
    HANDLE thread_;
public:
    explicit WindowsSuppressionContext(HANDLE thread) noexcept : thread_(thread) {}
    bool read(SuppressionRegisters& r) noexcept override {
        CONTEXT c{}; c.ContextFlags=CONTEXT_FULL|CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(thread_,&c)) return false;
        // EFLAGS bit 1 is architecturally fixed, but WOW64's context readback can
        // clear it. It carries no application state; restore it on every write.
        r={c.Eip,c.Esp,c.Eax,c.Ebx,c.Ecx,c.Edx,c.Esi,c.Edi,c.Ebp,c.EFlags&~2U,
            {c.SegCs,c.SegSs,c.SegDs,c.SegEs,c.SegFs,c.SegGs},
            {c.Dr0,c.Dr1,c.Dr2,c.Dr3,c.Dr6&0xe00fU,c.Dr7}};
        return true;
    }
    bool write(const SuppressionRegisters& r) noexcept override {
        CONTEXT c{}; c.ContextFlags=CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_DEBUG_REGISTERS;
        c.Eip=r.eip; c.Esp=r.esp; c.Eax=r.eax; c.Ebx=r.ebx; c.Ecx=r.ecx; c.Edx=r.edx;
        c.Esi=r.esi; c.Edi=r.edi; c.Ebp=r.ebp; c.EFlags=r.flags|2U;
        c.SegCs=r.segments[0]; c.SegSs=r.segments[1];
        c.Dr0=r.debug[0]; c.Dr1=r.debug[1]; c.Dr2=r.debug[2]; c.Dr3=r.debug[3]; c.Dr6=r.debug[4]; c.Dr7=r.debug[5];
        return SetThreadContext(thread_,&c)!=0;
    }
};
static_assert(std::is_trivially_destructible_v<WindowsSuppressionContext>);
}
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
LoaderTrace LoaderObservation::run_to_proxy_return(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, LoaderLimits limits) noexcept {
    return run_impl(child, executable, pins, limits, &entry, &proxy);
}
LoaderTrace LoaderObservation::run_to_startup_call(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, LoaderLimits limits) noexcept {
    return run_impl(child, executable, pins, limits, &entry, &proxy, &startup);
}
LoaderTrace LoaderObservation::run_to_codec_return(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup,
    const CodecStopSpec& codec, LoaderLimits limits) noexcept {
    return run_impl(child, executable, pins, limits, &entry, &proxy, &startup, &codec);
}
LoaderTrace LoaderObservation::run_to_codec_bindings(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, LoaderLimits limits) noexcept {
    return run_impl(child, executable, pins, limits, &entry, &proxy, &startup, &codec, &binding);
}
LoaderTrace LoaderObservation::run_to_asi_return(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, LoaderLimits limits) noexcept {
    return run_impl(child, executable, pins, limits, &entry, &proxy, &startup, &codec, &binding, &asi);
}
LoaderTrace LoaderObservation::run_bootstrap_lifecycle(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const BootstrapLifecycleSpec& bootstrap,
    LoaderLimits limits) noexcept {
    return run_impl(child, executable, pins, limits, &entry, &proxy, &startup, &codec, &binding, &asi, &bootstrap);
}
LoaderTrace LoaderObservation::run_frame_target_observation(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const BootstrapLifecycleSpec& bootstrap,
    const FrameTargetSpec& frame, LoaderLimits limits) noexcept {
    return run_impl(child, executable, pins, limits, &entry, &proxy, &startup, &codec, &binding, &asi, &bootstrap, &frame);
}
LoaderTrace LoaderObservation::run_to_startup_return(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail);
}
LoaderTrace LoaderObservation::run_to_crt_startup(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt);
}
LoaderTrace LoaderObservation::run_to_application_entry(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application);
}
LoaderTrace LoaderObservation::run_to_platform_startup(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform);
}
LoaderTrace LoaderObservation::run_platform_suppression(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression);
}
LoaderTrace LoaderObservation::run_instance_startup(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance);
}
LoaderTrace LoaderObservation::run_event_dispatch(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance,&dispatch);
}
LoaderTrace LoaderObservation::run_application_routing(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance,&dispatch,&routing);
}
LoaderTrace LoaderObservation::run_game_prelude(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance,&dispatch,&routing,&prelude);
}
LoaderTrace LoaderObservation::run_file_manager_entry(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance,&dispatch,&routing,&prelude,&manager);
}
LoaderTrace LoaderObservation::run_cwd_seh(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance,&dispatch,&routing,&prelude,&manager,&seh);
}
LoaderTrace LoaderObservation::run_cwd_lock(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance,&dispatch,&routing,&prelude,&manager,&seh,&lock);
}
LoaderTrace LoaderObservation::run_cwd_acquire(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance,&dispatch,&routing,&prelude,&manager,&seh,&lock,&acquire);
}
LoaderTrace LoaderObservation::run_cwd_query(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance,&dispatch,&routing,&prelude,&manager,&seh,&lock,&acquire,&query);
}
LoaderTrace LoaderObservation::run_cwd_copy(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, const CwdCopySpec& copy, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance,&dispatch,&routing,&prelude,&manager,&seh,&lock,&acquire,&query,&copy);
}
LoaderTrace LoaderObservation::run_cwd_return(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, const CwdCopySpec& copy, const CwdReturnSpec& completion, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance,&dispatch,&routing,&prelude,&manager,&seh,&lock,&acquire,&query,&copy,&completion);
}
LoaderTrace LoaderObservation::run_file_manager_ready(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, const CwdCopySpec& copy, const CwdReturnSpec& completion, const FileManagerReadySpec& ready, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance,&dispatch,&routing,&prelude,&manager,&seh,&lock,&acquire,&query,&copy,&completion,&ready);
}
LoaderTrace LoaderObservation::run_cd_stream_tables(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, const CwdCopySpec& copy, const CwdReturnSpec& completion, const FileManagerReadySpec& ready, const CdStreamTablesSpec& tables, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance,&dispatch,&routing,&prelude,&manager,&seh,&lock,&acquire,&query,&copy,&completion,&ready,&tables);
}
LoaderTrace LoaderObservation::run_cd_stream_disk(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, const CwdCopySpec& copy, const CwdReturnSpec& completion, const FileManagerReadySpec& ready, const CdStreamTablesSpec& tables, const CdStreamDiskSpec& disk, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance,&dispatch,&routing,&prelude,&manager,&seh,&lock,&acquire,&query,&copy,&completion,&ready,&tables,&disk);
}
LoaderTrace LoaderObservation::run_cd_stream_allocation(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, const CwdCopySpec& copy, const CwdReturnSpec& completion, const FileManagerReadySpec& ready, const CdStreamTablesSpec& tables, const CdStreamDiskSpec& disk, const CdStreamAllocationSpec& allocation, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance,&dispatch,&routing,&prelude,&manager,&seh,&lock,&acquire,&query,&copy,&completion,&ready,&tables,&disk,&allocation);
}
LoaderTrace LoaderObservation::run_cd_stream_channels(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, const EntryStopSpec& entry,
    const ProxyStopSpec& proxy, const StartupStopSpec& startup, const CodecStopSpec& codec,
    const BindingStopSpec& binding, const AsiStopSpec& asi, const StartupReturnSpec& tail,
    const CrtStartupSpec& crt, const ApplicationEntrySpec& application, const PlatformStartupSpec& platform,
    const PlatformSuppressionSpec& suppression, const InstanceStartupSpec& instance, const EventDispatchSpec& dispatch, const ApplicationRoutingSpec& routing, const GamePreludeSpec& prelude, const FileManagerEntrySpec& manager, const CwdSehSpec& seh, const CwdLockSpec& lock, const CwdAcquireSpec& acquire, const CwdQuerySpec& query, const CwdCopySpec& copy, const CwdReturnSpec& completion, const FileManagerReadySpec& ready, const CdStreamTablesSpec& tables, const CdStreamDiskSpec& disk, const CdStreamAllocationSpec& allocation, const CdStreamChannelsSpec& channels, LoaderLimits limits) noexcept {
    return run_impl(child,executable,pins,limits,&entry,&proxy,&startup,&codec,&binding,&asi,nullptr,nullptr,&tail,&crt,&application,&platform,&suppression,&instance,&dispatch,&routing,&prelude,&manager,&seh,&lock,&acquire,&query,&copy,&completion,&ready,&tables,&disk,&allocation,&channels);
}
LoaderTrace LoaderObservation::run_impl(SuspendedImage& child, void* executable,
    std::span<const LoaderFile* const> pins, LoaderLimits limits, const EntryStopSpec* entry,
    const ProxyStopSpec* proxy, const StartupStopSpec* startup, const CodecStopSpec* codec, const BindingStopSpec* binding,
    const AsiStopSpec* asi, const BootstrapLifecycleSpec* bootstrap, const FrameTargetSpec* frame, const StartupReturnSpec* tail, const CrtStartupSpec* crt, const ApplicationEntrySpec* application, const PlatformStartupSpec* platform, const PlatformSuppressionSpec* suppression, const InstanceStartupSpec* instance, const EventDispatchSpec* dispatch, const ApplicationRoutingSpec* routing, const GamePreludeSpec* prelude, const FileManagerEntrySpec* manager, const CwdSehSpec* seh, const CwdLockSpec* lock, const CwdAcquireSpec* acquire, const CwdQuerySpec* query, const CwdCopySpec* copy, const CwdReturnSpec* completion, const FileManagerReadySpec* ready, const CdStreamTablesSpec* tables, const CdStreamDiskSpec* disk, const CdStreamAllocationSpec* allocation, const CdStreamChannelsSpec* channels) noexcept {
    static_assert(sizeof(void*) == 4, "loader experiment requires a same-bitness x86 observer");
    static_assert(sizeof(STARTUPINFOA) == 68, "startup observation requires the x86 Windows structure layout");
    LoaderTrace trace{};
    // Wrong-thread calls cannot change the owner's pending debug event or cleanup.
    if (GetCurrentThreadId() != child.owner_thread_) { trace.reason = "loader_owner_thread"; return trace; }
    // Return a reference internally: MSVC /Od otherwise reserves one large trace
    // temporary per return site and can exhaust the default x86 stack.
    const auto finish = [&](std::string_view reason) -> const LoaderTrace& {
        trace.reason = reason;
        trace.exit_confirmed = child.created() && child.stop();
        if (child.created() && !trace.exit_confirmed) trace.reason = "loader_exit_unconfirmed";
        return trace;
    };
    if (!limits.events || limits.events > (allocation?160U:128U) || !limits.modules || limits.modules > 64 ||
        !limits.threads || limits.threads > 16 || !limits.milliseconds || limits.milliseconds > 5000 ||
        !limits.bytes || limits.bytes > 256ULL * 1024 * 1024 || pins.size() > 64)
        return finish("loader_invalid_limits");
    if(channels) {
        std::array<std::byte,64> code{};
        if(!allocation || !tables || !manager || !cd_stream_channels_code(*channels,*tables,*manager,code))return finish("cd_stream_channels_invalid_spec");
    }
    for (const auto pin : pins) if (!pin || !pin->valid()) return finish("loader_invalid_pin");
    if (!child.error_.empty() || child.loader_started_ || !child.same_file(executable)) return finish("loader_child_identity_or_state");
    if (platform && (!application || !crt || !valid_platform_startup_spec(*platform,*crt))) return finish("platform_invalid_spec");
    if (suppression && (!platform || !crt || !valid_platform_suppression_spec(*suppression,*platform,*crt))) return finish("suppression_invalid_spec");
    if (instance && (!suppression || !platform || !crt || !valid_instance_startup_spec(*instance,*platform,*crt,*suppression))) return finish("instance_invalid_spec");
    if (dispatch && (!instance || !valid_event_dispatch_spec(*dispatch,*instance))) return finish("event_dispatch_invalid_spec");
    if (routing && (!dispatch || !valid_application_routing_spec(*routing,*dispatch))) return finish("application_routing_invalid_spec");
    if (prelude && (!routing || !valid_game_prelude_spec(*prelude,*routing))) return finish("game_prelude_invalid_spec");
    if (manager && (!prelude || !valid_file_manager_entry_spec(*manager,*prelude,*routing))) return finish("file_manager_invalid_spec");
    if (seh && (!manager || !valid_cwd_seh_spec(*seh,*manager))) return finish("cwd_seh_invalid_spec");
    if (lock && (!seh || !valid_cwd_lock_spec(*lock,*seh))) return finish("cwd_lock_invalid_spec");
    if (acquire && (!lock || !valid_cwd_acquire_spec(*acquire,*lock))) return finish("cwd_acquire_invalid_spec");
    if (query && (!acquire || !valid_cwd_query_spec(*query,*lock) || !cwd_query_directory(query->expected_directory))) return finish("cwd_query_invalid_spec");
    if(copy) {
        CwdCopyCode code{};
        if(!query || !cwd_copy_code(*copy,*query,*lock,code))return finish("cwd_copy_invalid_spec");
    }
    if(ready) {
        FileManagerReadyCode code{};
        if(!completion || !manager || !seh || !lock || !file_manager_ready_code(*ready,*completion,*lock,*seh,*manager,code))return finish("file_manager_ready_invalid_spec");
    }
    if(tables) {
        CdStreamTablesCode code{};
        if(!ready || !manager || !cd_stream_tables_code(*tables,*manager,code))return finish("cd_stream_tables_invalid_spec");
    }
    if(disk) {
        CdStreamDiskCode code{};
        if(!tables || !cd_stream_disk_code(*disk,*tables,*manager,code))return finish("cd_stream_disk_invalid_spec");
    }
    if(allocation) {
        CdStreamAllocationCode code{};
        if(!disk || !cd_stream_allocation_code(*allocation,*disk,*manager,*seh,*ready,code))return finish("cd_stream_allocation_invalid_spec");
    }
    if(completion) {
        CwdReturnCode code{};
        if(!copy || !cwd_return_code(*completion,*copy,*query,*lock,code))return finish("cwd_return_invalid_spec");
    }
    if (application && (!crt || !proxy || !valid_application_entry_spec(*application,*crt) ||
        application->reentry_rva!=proxy->iat_target_rva)) return finish("application_invalid_spec");
    if (crt && (!tail || !valid_crt_startup_spec(*crt) || crt->io.image_base!=child.base_ ||
        crt->io.image_size!=child.image_bytes_)) return finish("crt_invalid_spec");
    if (tail && (!asi || bootstrap || frame || !valid_startup_return_spec(*tail) ||
        tail->frame.image_base != child.base_ || tail->frame.image_size != child.image_bytes_)) return finish("startup_return_invalid_spec");
    if (frame && (!bootstrap || !valid_frame_target_spec(*frame) || frame->image_base != child.base_ ||
        frame->image_size != child.image_bytes_)) return finish("frame_invalid_spec");
    if (bootstrap && (!asi || !valid_bootstrap_lifecycle_spec(*bootstrap))) return finish("bootstrap_invalid_spec");
    if (asi) {
        if (!binding || !asi->module || !asi->module->valid() ||
            std::find(pins.begin(), pins.end(), asi->module) == pins.end() ||
            !asi->call_rva || asi->call_rva > max_image_bytes - 22 ||
            !asi->end_rva || asi->end_rva > max_image_bytes - 16 ||
            (asi->end_rva >= asi->call_rva && asi->end_rva < asi->call_rva + 22)) return finish("asi_invalid_spec");
        const auto path = asi->requested_path;
        if (path.size() < 4 || path.size() >= 260 ||
            !((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) ||
            path[1] != ':' || path[2] != '\\') return finish("asi_invalid_path");
        for (std::size_t i = 0; i < path.size(); ++i)
            if (static_cast<unsigned char>(path[i]) < 32 || static_cast<unsigned char>(path[i]) > 126 ||
                (i != 1 && path[i] == ':') || path[i] == '/' || path[i] == '"') return finish("asi_invalid_path");
    }
    if (binding) {
        if (!codec || !binding->stop_rva || binding->stop_rva > max_image_bytes - 16 ||
            !binding->proc_slot_rva || binding->proc_slot_rva % 4 || binding->proc_slot_rva > max_image_bytes - 4 ||
            binding->slots.empty() || binding->slots.size() > trace.bindings.size()) return finish("binding_invalid_spec");
        for (const auto& slot : binding->slots) {
            if (slot.name.empty() || slot.name.size() >= 32 ||
                slot.name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") != std::string_view::npos ||
                !slot.slot_rva || slot.slot_rva % 4 || slot.slot_rva > max_image_bytes - 4 ||
                !slot.target_rva || slot.target_rva > max_image_bytes - 16) return finish("binding_invalid_slot");
            for (std::uint32_t i = 0; i < trace.binding_count; ++i)
                if (std::string_view(trace.bindings[i].name.data()) == slot.name || trace.bindings[i].slot_rva == slot.slot_rva)
                    return finish("binding_duplicate_slot");
            auto& value = trace.bindings[trace.binding_count++];
            std::copy(slot.name.begin(), slot.name.end(), value.name.begin());
            value.slot_rva = slot.slot_rva; value.target_rva = slot.target_rva;
        }
    }
    if (codec) {
        if (!startup || !proxy || codec->modules.empty() || codec->modules.size() > 3 ||
            codec->requested_name.empty() || codec->requested_name.size() > 15 ||
            codec->requested_name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-") != std::string_view::npos ||
            !codec->sequence_rva || codec->sequence_rva > max_image_bytes - 13 ||
            !codec->name_rva || codec->name_rva > max_image_bytes - 16 ||
            !codec->load_slot_rva || codec->load_slot_rva % 4 || codec->load_slot_rva > max_image_bytes - 4)
            return finish("codec_invalid_spec");
        for (std::size_t i = 0; i < codec->modules.size(); ++i) {
            const auto module = codec->modules[i];
            if (!module || std::find(pins.begin(), pins.end(), module) == pins.end() || module == proxy->module)
                return finish("codec_invalid_pin");
            for (std::size_t j = 0; j < i; ++j)
                if (module->identity().volume == codec->modules[j]->identity().volume &&
                    module->identity().file_id == codec->modules[j]->identity().file_id) return finish("codec_duplicate_pin");
        }
    }
    if (proxy && (!entry || !proxy->module || std::find(pins.begin(), pins.end(), proxy->module) == pins.end() ||
        !proxy->thunk_rva || proxy->thunk_rva > max_image_bytes - 11 ||
        !proxy->call_target_rva || proxy->call_target_rva >= max_image_bytes ||
        !proxy->return_slot_rva || proxy->return_slot_rva > max_image_bytes - 4 ||
        !proxy->iat_target_rva || proxy->iat_target_rva >= max_image_bytes ||
        !proxy->iat_rva || proxy->iat_rva > child.image_bytes_ || child.image_bytes_ - proxy->iat_rva < 4))
        return finish("proxy_invalid_spec");
    if (startup) {
        if (!proxy || proxy->iat_rva % 4 || startup->samples.empty() || startup->samples.size() > trace.startup_samples.size())
            return finish("startup_invalid_spec");
        for (const auto& sample : startup->samples) {
            if (!sample.rva || !sample.length || sample.length > 16) return finish("startup_invalid_sample");
            for (std::uint32_t i = 0; i < trace.startup_sample_count; ++i) {
                const auto& prior = trace.startup_samples[i];
                if (static_cast<std::uint64_t>(sample.rva) < static_cast<std::uint64_t>(prior.rva) + prior.length &&
                    static_cast<std::uint64_t>(prior.rva) < static_cast<std::uint64_t>(sample.rva) + sample.length)
                    return finish("startup_sample_overlap");
            }
            auto& observed = trace.startup_samples[trace.startup_sample_count++];
            observed.rva = sample.rva; observed.length = sample.length;
            auto bytes = std::span(observed.before).first(sample.length);
            if (!child.copy(sample.rva, bytes) || !std::equal(bytes.begin(), bytes.end(), sample.bytes.begin()))
                return finish("startup_sample_precondition");
        }
    }
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
    LoaderMappingLedger mappings(limits.modules);
    // Bounded remote reads in one region of a live admitted image; never follow arbitrary pointers.
    const auto read_image = [&](std::uintptr_t base, std::uint32_t rva, std::span<std::byte> output, bool code) {
        if (!base || output.empty() || output.size() > 16 || rva > UINT32_MAX - base ||
            output.size() > UINT32_MAX - (base + rva)) return false;
        const auto address = base + rva;
        MEMORY_BASIC_INFORMATION region{};
        if (VirtualQueryEx(child.process_, reinterpret_cast<void*>(address), &region, sizeof(region)) != sizeof(region) ||
            region.Type != MEM_IMAGE || region.State != MEM_COMMIT || reinterpret_cast<std::uintptr_t>(region.AllocationBase) != base ||
            (region.Protect & (PAGE_GUARD | PAGE_NOACCESS)) ||
            (code && !(region.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) ||
            address < reinterpret_cast<std::uintptr_t>(region.BaseAddress) ||
            address - reinterpret_cast<std::uintptr_t>(region.BaseAddress) >= region.RegionSize ||
            output.size() > region.RegionSize - (address - reinterpret_cast<std::uintptr_t>(region.BaseAddress))) return false;
        SIZE_T copied{};
        return ReadProcessMemory(child.process_, reinterpret_cast<void*>(address), output.data(), output.size(), &copied) && copied == output.size();
    };
    const auto sample_frame = [&](std::size_t phase) {
        auto& sample = trace.frame_target.samples[phase];
        sample.attempted = true; sample.thread_id = GetThreadId(child.thread_); sample.event_index = trace.event_count;
        sample.call_read = read_image(child.base_, frame->call_rva, sample.call, true);
        sample.target_read = read_image(child.base_, frame->target_rva, sample.target_prefix, true);
        sample.match = matches_frame_target(*frame, sample);
        return sample.match;
    };
    if (frame && !sample_frame(0)) return finish("frame_create_sample_rejected");
    child.loader_started_ = true;
    const auto read_word = [&](std::uintptr_t base, std::uint32_t rva, std::uint32_t& value) {
        return read_image(base, rva, std::as_writable_bytes(std::span(&value, 1)), false);
    };
    const auto proxy_shape = [&] {
        if (!mappings.active(trace.proxy_mapping_id, trace.proxy_base)) return false;
        std::array<std::byte, 11> thunk{};
        std::uint32_t relative{}, operand{}, original{};
        if (!read_image(trace.proxy_base, proxy->thunk_rva, thunk, true) ||
            thunk[0] != std::byte{0xe8} || thunk[5] != std::byte{0xff} || thunk[6] != std::byte{0x25}) return false;
        std::memcpy(&relative, thunk.data() + 1, 4); std::memcpy(&operand, thunk.data() + 7, 4);
        // rel32 arithmetic deliberately follows the 32-bit instruction address space.
        if (static_cast<std::uint32_t>(proxy->thunk_rva + 5 + relative) != proxy->call_target_rva ||
            proxy->return_slot_rva > UINT32_MAX - trace.proxy_base ||
            operand != trace.proxy_base + proxy->return_slot_rva ||
            !read_word(trace.proxy_base, proxy->return_slot_rva, original) || original != trace.entry_address) return false;
        std::array<std::byte, 1> probe{};
        return read_image(trace.proxy_base, proxy->call_target_rva, probe, true) &&
            read_image(trace.proxy_base, proxy->iat_target_rva, probe, true);
    };
    const auto matches_file = [](const LoaderFileIdentity& a, const LoaderFileIdentity& b) {
        return a.volume == b.volume && a.file_id == b.file_id && a.bytes == b.bytes && a.sha256 == b.sha256;
    };
    std::array<std::byte, 13> codec_before{};
    std::uint32_t codec_load_target{};
    // Exact relocated PUSH imm32 / FF15 abs32 / MOV EDI,EAX shape, plus a bounded literal.
    const auto codec_shape = [&](std::array<std::byte, 13>& bytes) {
        if (!proxy_shape() || !read_image(trace.proxy_base, codec->sequence_rva, bytes, true) ||
            bytes[0] != std::byte{0x68} || bytes[5] != std::byte{0xff} || bytes[6] != std::byte{0x15} ||
            bytes[11] != std::byte{0x8b} || bytes[12] != std::byte{0xf8}) return false;
        std::uint32_t name{}, slot{}; std::memcpy(&name, bytes.data() + 1, 4); std::memcpy(&slot, bytes.data() + 7, 4);
        if (codec->name_rva > UINT32_MAX - trace.proxy_base || codec->load_slot_rva > UINT32_MAX - trace.proxy_base ||
            name != trace.proxy_base + codec->name_rva || slot != trace.proxy_base + codec->load_slot_rva) return false;
        std::array<std::byte, 16> literal{};
        if (!read_image(trace.proxy_base, codec->name_rva, std::span(literal).first(codec->requested_name.size() + 1), false) ||
            std::memcmp(literal.data(), codec->requested_name.data(), codec->requested_name.size()) ||
            literal[codec->requested_name.size()] != std::byte{}) return false;
        std::uint32_t target{};
        if (!read_word(trace.proxy_base, codec->load_slot_rva, target) || (codec_load_target && target != codec_load_target)) return false;
        bool system_target{};
        for (std::uint32_t i = 0; i < trace.module_count; ++i) {
            const auto& module = trace.modules[i]; const std::string_view name_view(module.file.name.data());
            std::array<std::byte, 1> byte{};
            if ((name_view == "kernel32.dll" || name_view == "kernelbase.dll") && module.admitted &&
                mappings.active(module.mapping_id, module.base) && target >= module.base &&
                read_image(module.base, target - module.base, byte, true)) system_target = true;
        }
        if (!system_target) return false;
        codec_load_target = target;
        return true;
    };
    std::uint32_t binding_proc_target{};
    const auto binding_shape = [&] {
        std::array<std::byte, 13> codec_bytes{}; std::array<std::byte, 16> stop{};
        std::uint32_t target{}, iat{};
        if (!codec_shape(codec_bytes) || codec_bytes != codec_before ||
            !read_image(trace.proxy_base, binding->stop_rva, stop, true) || stop != binding->expected_stop ||
            !read_word(child.base_, proxy->iat_rva, iat) || iat != trace.startup_address ||
            !read_word(trace.proxy_base, binding->proc_slot_rva, target) || (binding_proc_target && target != binding_proc_target)) return false;
        bool system{};
        for (std::uint32_t i = 0; i < trace.module_count; ++i) {
            const auto& module = trace.modules[i]; std::array<std::byte, 1> byte{};
            const std::string_view name(module.file.name.data());
            if ((name == "kernel32.dll" || name == "kernelbase.dll") && module.admitted &&
                mappings.active(module.mapping_id, module.base) && target >= module.base &&
                read_image(module.base, target - module.base, byte, true)) system = true;
        }
        if (!system) return false;
        binding_proc_target = target;
        return true;
    };
    const auto asi_shape = [&] {
        std::array<std::byte, 6> call{}; std::array<std::byte, 16> after{}, end_bytes{};
        std::uint32_t operand{};
        if (!binding_shape() || !read_image(trace.proxy_base, asi->call_rva, call, true) ||
            call[0] != std::byte{0xff} || call[1] != std::byte{0x15}) return false;
        std::memcpy(&operand, call.data() + 2, 4);
        if (operand != trace.proxy_base + codec->load_slot_rva ||
            !read_image(trace.proxy_base, asi->call_rva + 6, after, true) || after != asi->expected_return ||
            !read_image(trace.proxy_base, asi->end_rva, end_bytes, true) || end_bytes != asi->expected_end) return false;
        for (std::uint32_t i = 0; i < trace.codec_module_count; ++i) {
            const auto id = trace.codec_mapping_ids[i];
            const auto end = trace.modules.begin() + trace.module_count;
            const auto module = std::find_if(trace.modules.begin(), end, [&](const auto& value) { return value.mapping_id == id; });
            if (module == end || !mappings.active(id, module->base)) return false;
        }
        // Preserve the fifth-stop evidence; recheck current values without replacing it.
        for (std::uint32_t i = 0; i < trace.binding_count; ++i) {
            const auto& value = trace.bindings[i]; std::uint32_t pointer{}; std::array<std::byte, 16> target{};
            if (!read_word(trace.proxy_base, value.slot_rva, pointer) || pointer != value.after ||
                !read_image(trace.codec_handle, value.target_rva, target, true) || target != value.target_after) return false;
        }
        return true;
    };
    const auto asi_preloaded = [&] {
        for (std::uint32_t i = 0; i < trace.module_count; ++i) {
            const auto& m = trace.modules[i];
            if (m.admitted && mappings.active(m.mapping_id, m.base) && matches_file(m.file, asi->module->identity())) return true;
        }
        return false;
    };
    const auto read_private = [&](std::uint32_t address, void* output, std::size_t size) {
        MEMORY_BASIC_INFORMATION region{}; SIZE_T copied{};
        if (!address || !size || size > 260 || size > UINT32_MAX - address ||
            VirtualQueryEx(child.process_, reinterpret_cast<void*>(address), &region, sizeof(region)) != sizeof(region) ||
            region.Type != MEM_PRIVATE || region.State != MEM_COMMIT || (region.Protect & (PAGE_GUARD | PAGE_NOACCESS)) ||
            address < reinterpret_cast<std::uintptr_t>(region.BaseAddress)) return false;
        const auto offset = address - reinterpret_cast<std::uintptr_t>(region.BaseAddress);
        return offset < region.RegionSize && size <= region.RegionSize - offset &&
            ReadProcessMemory(child.process_, reinterpret_cast<void*>(address), output, size, &copied) && copied == size;
    };
    const auto arm_asi = [&](std::uintptr_t address, CONTEXT& context) {
        context.ContextFlags = CONTEXT_DEBUG_REGISTERS; context.Dr0 = static_cast<DWORD>(address); context.Dr6 = 0;
        context.Dr2 = static_cast<DWORD>(trace.proxy_base + asi->end_rva); context.Dr7 |= 0x10U;
        if (!SetThreadContext(child.thread_, &context)) return false;
        CONTEXT checked{}; checked.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        return GetThreadContext(child.thread_, &checked) && checked.Dr0 == address &&
            checked.Dr1 == child.base_ + proxy->iat_rva && checked.Dr2 == trace.proxy_base + asi->end_rva &&
            (checked.Dr7 & 0xffff20ffU) == 0x00d00015U;
    };
    CONTEXT startup_saved{};
    const auto tail_sample = [&](std::size_t index) {
        auto& sample=trace.startup_return.frame_samples[index];
        sample.attempted=true; sample.thread_id=GetThreadId(child.thread_); sample.event_index=trace.event_count;
        sample.call_read=read_image(child.base_,tail->frame.call_rva,sample.call,true);
        sample.target_read=read_image(child.base_,tail->frame.target_rva,sample.target_prefix,true);
        sample.match=matches_frame_target(tail->frame,sample); return sample.match;
    };
    const auto system_function = [&](const BootstrapExportSpec& spec,std::uint32_t target) {
        for (std::uint32_t i=0;i<trace.module_count;++i) {
            const auto& module=trace.modules[i];
            if (std::string_view(module.file.name.data())!="kernel32.dll" || !module.admitted ||
                !mappings.active(module.mapping_id,module.base) || target!=module.base+spec.rva) continue;
            std::array<std::byte,20> actual{},expected{};
            return bootstrap_export_prefix(spec,static_cast<std::uint32_t>(module.base),expected) &&
                read_image(module.base,spec.rva,std::span(actual).first(16),true) &&
                read_image(module.base,spec.rva+16,std::span(actual).subspan(16),true) && actual==expected;
        }
        return false;
    };
    const auto tail_shape = [&] {
        std::array<std::byte,6> call{},forward{}; std::array<std::byte,16> after{};
        std::uint32_t operand{},slot{},protect{},startup_target{};
        if (!asi_shape() || !read_image(trace.proxy_base,tail->protect_call_rva,call,true) ||
            call[0]!=std::byte{0xff} || call[1]!=std::byte{0x15} ||
            !read_image(trace.proxy_base,tail->forward_rva,forward,true) ||
            forward[0]!=std::byte{0xff} || forward[1]!=std::byte{0x25}) return false;
        std::memcpy(&operand,call.data()+2,4); std::memcpy(&slot,forward.data()+2,4);
        if (operand!=trace.proxy_base+tail->protect_slot_rva || slot!=trace.proxy_base+tail->startup_slot_rva ||
            !read_image(trace.proxy_base,tail->protect_call_rva+6,after,true) || after!=tail->protect_return_prefix ||
            !read_word(trace.proxy_base,tail->protect_slot_rva,protect) || !system_function(tail->protect_function,protect) ||
            !read_word(trace.proxy_base,tail->startup_slot_rva,startup_target) || !system_function(tail->startup_function,startup_target)) return false;
        trace.startup_return.protect_target=protect; trace.startup_return.startup_target=startup_target; return true;
    };
    const auto image_protection = [&](bool after) {
        auto& state=trace.startup_return; std::uintptr_t cursor=child.base_;
        const auto end=child.base_+child.image_bytes_; std::uint32_t count{};
        while (cursor<end) {
            MEMORY_BASIC_INFORMATION region{};
            if (++count>128 || VirtualQueryEx(child.process_,reinterpret_cast<void*>(cursor),&region,sizeof(region))!=sizeof(region)) return false;
            state.last_region_rva=static_cast<DWORD>(cursor-child.base_); state.last_region_protect=region.Protect;
            state.last_region_state=region.State; state.last_region_type=region.Type;
            if (region.State!=MEM_COMMIT || region.Type!=MEM_IMAGE || reinterpret_cast<std::uintptr_t>(region.AllocationBase)!=child.base_ ||
                (region.Protect & (PAGE_GUARD|PAGE_NOACCESS)) || (after && !valid_startup_image_protection(region.Protect))) return false;
            if (after && region.Protect==PAGE_EXECUTE_WRITECOPY) ++state.copy_on_write_regions;
            if (after && region.Protect==PAGE_EXECUTE_READWRITE) ++state.read_write_regions;
            const auto start=reinterpret_cast<std::uintptr_t>(region.BaseAddress);
            if (cursor<start || cursor-start>=region.RegionSize || region.RegionSize>UINT32_MAX-start) return false;
            if (!after && cursor==child.base_) state.first_page_protect=region.Protect;
            cursor=std::min<std::uintptr_t>(end,start+region.RegionSize);
        }
        state.region_count=count; return true;
    };
    const auto arm_tail = [&](std::uint32_t stage,std::uint32_t address,CONTEXT& context) {
        context.ContextFlags=CONTEXT_DEBUG_REGISTERS; context.Dr0=address; context.Dr6=0;
        // Any second ASI call at the reviewed site is forbidden, including a cached load.
        context.Dr2=static_cast<DWORD>(trace.proxy_base+asi->call_rva);
        if (!SetThreadContext(child.thread_,&context)) return false;
        CONTEXT checked{}; checked.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(child.thread_,&checked) || checked.Dr0!=address || checked.Dr2!=context.Dr2 ||
            checked.Dr1!=child.base_+proxy->iat_rva || (checked.Dr7 & 0xffff20ffU)!=0x00d00015U) return false;
        auto& state=trace.startup_return; state.stage=stage; state.stop_address=address; state.armed=true; return true;
    };
    std::array<std::uint32_t,4> crt_saved{}; // EDI, ESI, EBP, EBX in reviewed I/O prologue order.
    const auto crt_shape = [&] {
        auto& state=trace.crt_startup;
        state.checked_slots=0; state.nonzero_slots=0;
        for (const auto* candidate:{&crt->io,&crt->initialize,&crt->application}) {
            FrameTargetSample sample{}; sample.attempted=true;
            sample.call_read=read_image(child.base_,candidate->call_rva,sample.call,true);
            sample.target_read=read_image(child.base_,candidate->target_rva,sample.target_prefix,true);
            if (!matches_frame_target(*candidate,sample)) return false;
        }
        std::array<std::byte,16> after{};
        if (!read_image(child.base_,crt->io.call_rva+5,after,true) || after!=crt->io_return_prefix) return false;
        for (const auto& table:crt->tables) for (std::size_t i=0;i<table.targets.size();++i) {
            const auto rva=table.rva+static_cast<std::uint32_t>(i*4);
            std::uint32_t value{};
            if (!read_word(child.base_,rva,value) || value!=(table.targets[i] ? child.base_+table.targets[i] : 0)) {
                state.failed_slot_rva=rva; state.actual_target=value; return false;
            }
            ++state.checked_slots;
            if (value) {
                std::array<std::byte,1> byte{};
                if (!read_image(child.base_,table.targets[i],byte,true)) { state.failed_slot_rva=rva; state.actual_target=value; return false; }
                ++state.nonzero_slots;
            }
        }
        ++state.samples_verified; return true;
    };
    const auto arm_crt = [&](std::uint32_t stage,std::uint32_t address,CONTEXT& context) {
        context.ContextFlags=CONTEXT_DEBUG_REGISTERS; context.Dr0=address; context.Dr6=0;
        if (!SetThreadContext(child.thread_,&context)) return false;
        CONTEXT checked{}; checked.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(child.thread_,&checked) || checked.Dr0!=address ||
            checked.Dr1!=child.base_+proxy->iat_rva || checked.Dr2!=trace.proxy_base+asi->call_rva ||
            (checked.Dr7 & 0xffff20ffU)!=0x00d00015U) return false;
        auto& state=trace.crt_startup; state.stage=stage; state.stop_address=address; state.armed=true; return true;
    };
    CONTEXT application_saved{};
    const auto same_nonvolatile = [](const CONTEXT& a,const CONTEXT& b) {
        return a.Ebx==b.Ebx && a.Esi==b.Esi && a.Edi==b.Edi && a.Ebp==b.Ebp && !(a.EFlags&0x500U);
    };
    const auto exact_code = [&](std::uintptr_t base,std::uint32_t rva,std::span<const std::byte> expected) {
        for (std::size_t i=0;i<expected.size();i+=16) {
            std::array<std::byte,16> bytes{}; const auto n=std::min<std::size_t>(16,expected.size()-i);
            if (!read_image(base,rva+static_cast<DWORD>(i),std::span(bytes).first(n),true) ||
                !std::equal(bytes.begin(),bytes.begin()+n,expected.begin()+i)) return false;
        }
        return true;
    };
    const auto application_shape = [&] {
        // Keep the completed CRT checkpoint intact; these are additional samples.
        const auto saved=trace.crt_startup; const auto intact=crt_shape(); trace.crt_startup=saved;
        if (!intact || !tail_shape()) return false;
        FrameTargetSample frame_sample{}; frame_sample.attempted=true;
        frame_sample.call_read=read_image(child.base_,tail->frame.call_rva,frame_sample.call,true);
        frame_sample.target_read=read_image(child.base_,tail->frame.target_rva,frame_sample.target_prefix,true);
        if (!matches_frame_target(tail->frame,frame_sample)) return false;
        std::array<std::byte,31> reentry{};
        std::array<std::byte,1> once{}; std::array<std::byte,6> second{}; std::uint32_t slot{};
        if (!application_reentry_prefix(*application,static_cast<DWORD>(trace.proxy_base),tail->startup_slot_rva,reentry) ||
            !exact_code(trace.proxy_base,application->reentry_rva,reentry) ||
            !read_image(trace.proxy_base,application->once_rva,once,false) || once[0]!=std::byte{1} ||
            !exact_code(child.base_,crt->initialize.target_rva,application->initializer_body) ||
            !exact_code(child.base_,crt->initialize.call_rva+5,application->initializer_return) ||
            !read_image(child.base_,application->second_call_rva,second,true) ||
            !exact_code(child.base_,application->second_call_rva+6,application->second_return)) return false;
        std::memcpy(&slot,second.data()+2,4);
        if (second[0]!=std::byte{0xff} || second[1]!=std::byte{0x15} || slot!=child.base_+proxy->iat_rva) return false;
        trace.application_entry.once_valid=true; return true;
    };
    const auto arm_application = [&](std::uint32_t stage,std::uint32_t address,CONTEXT& context) {
        context.ContextFlags=CONTEXT_DEBUG_REGISTERS; context.Dr0=address; context.Dr6=0;
        if (!SetThreadContext(child.thread_,&context)) return false;
        CONTEXT checked{}; checked.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(child.thread_,&checked) || checked.Dr0!=address ||
            checked.Dr1!=child.base_+proxy->iat_rva || checked.Dr2!=trace.proxy_base+asi->call_rva ||
            (checked.Dr7 & 0xffff20ffU)!=0x00d00015U) return false;
        auto& state=trace.application_entry; state.stage=stage; state.stop_address=address; state.armed=true; return true;
    };
    CONTEXT platform_saved{};
    const auto platform_shape = [&] {
        auto& state=trace.platform_startup;
        if (!application_shape() || !exact_code(child.base_,crt->application.target_rva,platform->prologue) ||
            !exact_code(child.base_,platform->call_rva+6,platform->return_prefix)) return false;
        std::array<std::byte,6> call{}; std::uint32_t slot{},target{};
        if (!read_image(child.base_,platform->call_rva,call,true)) return false;
        std::memcpy(&slot,call.data()+2,4);
        if (call[0]!=std::byte{0xff} || call[1]!=std::byte{0x15} || slot!=child.base_+platform->iat_rva ||
            !read_word(child.base_,platform->iat_rva,target) || (state.function_address && state.function_address!=target)) return false;
        for (std::uint32_t i=0;i<trace.module_count;++i) {
            const auto& m=trace.modules[i];
            if (std::string_view(m.file.name.data())!=platform->module || !m.admitted || !mappings.active(m.mapping_id,m.base) || target!=m.base+platform->function.rva) continue;
            std::array<std::byte,20> expected{};
            if (!bootstrap_export_prefix(platform->function,static_cast<DWORD>(m.base),expected) || !exact_code(m.base,platform->function.rva,expected)) return false;
            state.function_address=target; state.shape_valid=true; return true;
        }
        return false;
    };
    WindowsSuppressionContext suppression_port(child.thread_);
    std::array<std::byte,172> suppression_stack{}; // 152-byte prologue plus original return/four args
    const auto read_last_error = [&](std::uint32_t& value) {
        if (!suppression) return false;
        std::uint32_t target{}; bool identity{};
        if (!read_word(child.base_,suppression->last_error_iat_rva,target)) return false;
        for (std::uint32_t i=0;i<trace.module_count;++i) {
            const auto& m=trace.modules[i];
            if (std::string_view(m.file.name.data())=="kernel32.dll" && m.admitted && mappings.active(m.mapping_id,m.base) &&
                target==m.base+suppression->last_error_function.rva &&
                exact_code(m.base,suppression->last_error_function.rva,suppression->last_error_function.prefix)) identity=true;
        }
        if (!identity) return false;
        SuppressionRegisters r{}; LDT_ENTRY descriptor{};
        if (!suppression_port.read(r) || !GetThreadSelectorEntry(child.thread_,r.segments[4],&descriptor)) return false;
        const auto teb=static_cast<std::uint32_t>(descriptor.BaseLow) |
            (static_cast<std::uint32_t>(descriptor.HighWord.Bytes.BaseMid)<<16) |
            (static_cast<std::uint32_t>(descriptor.HighWord.Bytes.BaseHi)<<24);
        std::uint32_t self{}; auto& state=trace.platform_suppression;
        if (teb<65536 || teb>UINT32_MAX-56 || (state.teb_address && state.teb_address!=teb) ||
            !read_private(teb+offsetof(NT_TIB,Self),&self,4) || self!=teb || !read_private(teb+0x34,&value,4)) return false;
        state.teb_address=teb; return true;
    };
    CONTEXT instance_saved{};
    const auto instance_shape = [&] {
        std::array<std::byte,91> body{}; std::uint32_t name{},target{},forward{};
        if (!platform_shape() || !instance_body(*instance,static_cast<DWORD>(child.base_),body) ||
            !exact_code(child.base_,instance->caller.target_rva,body) ||
            !exact_code(child.base_,instance->caller.call_rva,instance->caller.call) ||
            !read_word(child.base_,instance->body_address_rvas[0],name) || name!=child.base_+instance->name_rva ||
            !read_word(child.base_,instance->body_address_rvas[1],target)) return false;
        for (std::size_t i=0;i<=instance->name.size();++i) {
            std::array<std::byte,1> c{};
            if (!read_image(child.base_,instance->name_rva+static_cast<DWORD>(i),c,false) ||
                c[0]!=std::byte(i<instance->name.size() ? instance->name[i] : 0)) return false;
        }
        bool thunk{},function{};
        for (std::uint32_t i=0;i<trace.module_count;++i) {
            const auto& m=trace.modules[i];
            if (!m.admitted || !mappings.active(m.mapping_id,m.base)) continue;
            if (std::string_view(m.file.name.data())=="kernel32.dll" && target==m.base+instance->create_thunk_rva) {
                std::array<std::byte,6> bytes{std::byte{0xff},std::byte{0x25}};
                const auto address=static_cast<DWORD>(m.base+instance->create_thunk_slot_rva);
                std::memcpy(bytes.data()+2,&address,4);
                thunk=exact_code(m.base,instance->create_thunk_rva,bytes) && read_word(m.base,instance->create_thunk_slot_rva,forward);
            }
        }
        if (!thunk) return false;
        for (std::uint32_t i=0;i<trace.module_count;++i) {
            const auto& m=trace.modules[i];
            if (m.admitted && mappings.active(m.mapping_id,m.base) && std::string_view(m.file.name.data())=="kernelbase.dll" &&
                forward==m.base+instance->create_function.rva && exact_code(m.base,instance->create_function.rva,instance->create_function.prefix)) function=true;
        }
        std::uint32_t ignored{};
        return function && read_last_error(ignored);
    };
    const auto arm_instance = [&](std::uint32_t stage,std::uint32_t address,CONTEXT& context) {
        context.ContextFlags=CONTEXT_DEBUG_REGISTERS; context.Dr0=address;
        context.Dr3=static_cast<DWORD>(child.base_+instance->caller.target_rva+31); context.Dr6=0; context.Dr7=0x00d00055U;
        if (!SetThreadContext(child.thread_,&context)) return false;
        CONTEXT checked{}; checked.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(child.thread_,&checked) || checked.Dr0!=address || checked.Dr3!=context.Dr3 ||
            checked.Dr1!=child.base_+proxy->iat_rva || checked.Dr2!=trace.proxy_base+asi->call_rva ||
            (checked.Dr7&0xffff20ffU)!=0x00d00055U) return false;
        auto& state=trace.instance_startup; state.armed=true; state.stage=stage; state.stop_address=address; return true;
    };
    EventDispatchRegisters dispatch_saved{};
    const auto dispatch_registers=[](const CONTEXT& c) -> EventDispatchRegisters {
        return {c.Esp,c.Eax,c.Ebx,c.Ecx,c.Edx,c.Esi,c.Edi,c.Ebp,c.EFlags};
    };
    const auto dispatch_shape=[&] {
        auto& state=trace.event_dispatch;
        if (!instance_shape() || !exact_code(child.base_,dispatch->dispatcher.call_rva-7,event_dispatch_branch) ||
            !exact_code(child.base_,dispatch->dispatcher.call_rva,dispatch->dispatcher.call) ||
            !exact_code(child.base_,dispatch->dispatcher.target_rva,event_dispatch_prologue) ||
            !exact_code(child.base_,dispatch->application.call_rva,dispatch->application.call)) return false;
        state.shape_valid=read_image(child.base_,dispatch->application.target_rva,std::span(state.application_sample).first(16),true) &&
            read_image(child.base_,dispatch->application.target_rva+16,std::span(state.application_sample).subspan(16),true) &&
            state.application_sample==dispatch->application_prefix;
        return state.shape_valid;
    };
    const auto arm_dispatch=[&](std::uint32_t stage,std::uint32_t address,CONTEXT& c) {
        c.ContextFlags=CONTEXT_DEBUG_REGISTERS; c.Dr0=address; c.Dr6=0;
        if (!SetThreadContext(child.thread_,&c)) return false;
        CONTEXT check{}; check.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(child.thread_,&check) || check.Dr0!=address || check.Dr1!=child.base_+proxy->iat_rva ||
            check.Dr2!=trace.proxy_base+asi->call_rva || check.Dr3!=child.base_+instance->caller.target_rva+31 ||
            (check.Dr7&0xffff20ffU)!=0x00d00055U) return false;
        auto& state=trace.event_dispatch; state.stage=stage; state.stop_address=address; state.armed=true; return true;
    };
    const auto routing_shape=[&] {
        ApplicationRoutingCode code{};
        if (!dispatch_shape() || !application_routing_code(*routing,*dispatch,code) ||
            !exact_code(child.base_,dispatch->application.target_rva,code.entry) ||
            !exact_code(child.base_,routing->detour_rva,code.detour) ||
            !exact_code(child.base_,dispatch->application.target_rva+20,code.indirect) ||
            !exact_code(child.base_,routing->initializer.call_rva,routing->initializer.call) ||
            !exact_code(child.base_,routing->initializer.target_rva,routing->initializer.target_prefix)) return false;
        std::array<std::byte,39> indices{};
        for (std::size_t i=0;i<indices.size();i+=16)
            if (!read_image(child.base_,routing->index_rva+static_cast<DWORD>(i),std::span(indices).subspan(i,std::min<std::size_t>(16,indices.size()-i)),false)) return false;
        if (indices!=routing->indices) return false;
        for (std::size_t i=0;i<code.targets.size();++i) {
            std::uint32_t target{};
            if (!read_word(child.base_,routing->table_rva+static_cast<DWORD>(i*4),target) || target!=code.targets[i]) return false;
        }
        return true;
    };
    const auto arm_routing=[&](std::uint32_t stage,std::uint32_t address,CONTEXT& c) {
        c.ContextFlags=CONTEXT_DEBUG_REGISTERS; c.Dr0=address; c.Dr6=0;
        if (!SetThreadContext(child.thread_,&c)) return false;
        CONTEXT check{};check.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(child.thread_,&check) || check.Dr0!=address || check.Dr1!=child.base_+proxy->iat_rva ||
            check.Dr2!=trace.proxy_base+asi->call_rva || check.Dr3!=child.base_+instance->caller.target_rva+31 ||
            (check.Dr7&0xffff20ffU)!=0x00d00055U) return false;
        auto& state=trace.application_routing;state.stage=stage;state.stop_address=address;state.armed=true;return true;
    };
    EventDispatchRegisters prelude_saved{};
    std::array<std::byte,32> prelude_parent_stack{};
    const auto prelude_shape=[&] {
        std::array<std::byte,20> body{};
        return routing_shape() && game_prelude_body(*prelude,body) &&
            exact_code(child.base_,prelude->empty.call_rva,prelude->empty.call) &&
            exact_code(child.base_,prelude->empty.target_rva,prelude->empty.target_prefix) &&
            exact_code(child.base_,prelude->localisation.call_rva,prelude->localisation.call) &&
            exact_code(child.base_,prelude->localisation.target_rva,body) &&
            exact_code(child.base_,prelude->localisation.call_rva+5,prelude->stop_prefix);
    };
    const auto prelude_read_flags=[&](std::array<std::byte,16>& value) {
        const auto address=child.base_+prelude->flags_rva-4;MEMORY_BASIC_INFORMATION region{};
        return VirtualQueryEx(child.process_,reinterpret_cast<void*>(address),&region,sizeof(region))==sizeof(region) &&
            (region.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) &&
            read_image(child.base_,prelude->flags_rva-4,value,false);
    };
    const auto arm_prelude=[&](std::uint32_t stage,std::uint32_t address,CONTEXT& c) {
        c.ContextFlags=CONTEXT_DEBUG_REGISTERS;c.Dr0=address;c.Dr6=0;
        if (!SetThreadContext(child.thread_,&c)) return false;
        CONTEXT check{};check.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(child.thread_,&check) || check.Dr0!=address || check.Dr1!=child.base_+proxy->iat_rva ||
            check.Dr2!=trace.proxy_base+asi->call_rva || check.Dr3!=child.base_+instance->caller.target_rva+31 ||
            (check.Dr7&0xffff20ffU)!=0x00d00055U) return false;
        auto& state=trace.game_prelude;state.stage=stage;state.stop_address=address;state.armed=true;return true;
    };
    EventDispatchRegisters manager_saved{};
    const auto manager_shape=[&] {
        std::array<std::byte,51> body{};std::array<std::byte,2> suffix{};std::array<std::byte,1> cwd{};
        return prelude_shape() && file_manager_body(*manager,body) &&
            exact_code(child.base_,manager->manager.call_rva,manager->manager.call) &&
            exact_code(child.base_,manager->manager.target_rva,body) &&
            exact_code(child.base_,manager->manager.call_rva+5,manager->return_prefix) &&
            read_image(child.base_,manager->suffix_rva,suffix,false) && suffix[0]==std::byte{0x5c} && suffix[1]==std::byte{} &&
            read_image(child.base_,manager->cwd_rva,cwd,true); // Accessibility only; cwd body is not admitted.
    };
    const auto manager_read_buffer=[&](std::array<std::byte,136>& value) {
        const auto address=child.base_+manager->buffer_rva-4;MEMORY_BASIC_INFORMATION region{};
        if (VirtualQueryEx(child.process_,reinterpret_cast<void*>(address),&region,sizeof(region))!=sizeof(region) ||
            region.State!=MEM_COMMIT || region.Type!=MEM_IMAGE || reinterpret_cast<std::uintptr_t>(region.AllocationBase)!=child.base_ ||
            !(region.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) ||
            region.Protect&(PAGE_GUARD|PAGE_NOACCESS) || address<reinterpret_cast<std::uintptr_t>(region.BaseAddress) ||
            address-reinterpret_cast<std::uintptr_t>(region.BaseAddress)>region.RegionSize ||
            value.size()>region.RegionSize-(address-reinterpret_cast<std::uintptr_t>(region.BaseAddress))) return false;
        for (std::size_t i=0;i<value.size();i+=16) {
            const auto count=std::min<std::size_t>(16,value.size()-i);
            if (!read_image(child.base_,manager->buffer_rva-4+static_cast<DWORD>(i),std::span(value).subspan(i,count),false)) return false;
        }
        return true;
    };
    const auto arm_manager=[&](std::uint32_t stage,std::uint32_t address,CONTEXT& c) {
        c.ContextFlags=CONTEXT_DEBUG_REGISTERS;c.Dr0=address;c.Dr6=0;
        if (!SetThreadContext(child.thread_,&c)) return false;
        CONTEXT check{};check.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(child.thread_,&check) || check.Dr0!=address || check.Dr1!=child.base_+proxy->iat_rva ||
            check.Dr2!=trace.proxy_base+asi->call_rva || check.Dr3!=child.base_+instance->caller.target_rva+31 ||
            (check.Dr7&0xffff20ffU)!=0x00d00055U) return false;
        auto& state=trace.file_manager_entry;state.stage=stage;state.stop_address=address;state.armed=true;return true;
    };
    EventDispatchRegisters seh_saved{};
    std::array<std::uint32_t,2> seh_prior_record{};
    static_assert(offsetof(NT_TIB,ExceptionList)==0 && offsetof(NT_TIB,StackBase)==4 && offsetof(NT_TIB,StackLimit)==8 && offsetof(NT_TIB,Self)==24);
    const auto seh_shape=[&] {
        std::array<std::byte,16> wrapper{};std::array<std::byte,59> body{};std::array<std::byte,12> scope{},actual{};std::array<std::byte,1> byte{};
        return manager_shape() && cwd_seh_code(*seh,wrapper,body,scope) &&
            exact_code(child.base_,seh->wrapper.call_rva,seh->wrapper.call) && exact_code(child.base_,seh->wrapper.target_rva,wrapper) &&
            exact_code(child.base_,seh->prologue.target_rva,body) && exact_code(child.base_,seh->prologue.call_rva+5,seh->stop_prefix) &&
            read_image(child.base_,seh->scope_rva,actual,false) && actual==scope &&
            read_image(child.base_,seh->handler_rva,byte,true) && read_image(child.base_,seh->cleanup_rva,byte,true);
    };
    const auto seh_read_tib=[&](std::array<std::uint32_t,7>& tib) {
        std::uint32_t ignored{};if (!read_last_error(ignored)) return false;
        const auto teb=trace.platform_suppression.teb_address;MEMORY_BASIC_INFORMATION region{};
        return VirtualQueryEx(child.process_,reinterpret_cast<void*>(teb),&region,sizeof(region))==sizeof(region) &&
            (region.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) &&
            read_private(teb,tib.data(),28) && tib[6]==teb;
    };
    const auto arm_seh=[&](std::uint32_t stage,std::uint32_t address,CONTEXT& c) {
        c.ContextFlags=CONTEXT_DEBUG_REGISTERS;c.Dr0=address;c.Dr6=0;
        if (!SetThreadContext(child.thread_,&c)) return false;
        CONTEXT check{};check.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(child.thread_,&check) || check.Dr0!=address || check.Dr1!=child.base_+proxy->iat_rva ||
            check.Dr2!=trace.proxy_base+asi->call_rva || check.Dr3!=child.base_+instance->caller.target_rva+31 ||
            (check.Dr7&0xffff20ffU)!=0x00d00055U) return false;
        auto& state=trace.cwd_seh;state.stage=stage;state.stop_address=address;state.armed=true;return true;
    };
    EventDispatchRegisters lock_saved{};
    const auto lock_shape=[&] {
        std::array<std::byte,17> code{};
        return seh_shape() && cwd_lock_code(*lock,code) && exact_code(child.base_,lock->selector.call_rva,lock->selector.call) &&
            exact_code(child.base_,lock->selector.target_rva,code) && exact_code(child.base_,lock->selector.target_rva+17,lock->stop_prefix);
    };
    const auto lock_read_slot=[&](std::array<std::byte,16>& value) {
        const auto rva=lock->table_rva+52;MEMORY_BASIC_INFORMATION region{};
        return VirtualQueryEx(child.process_,reinterpret_cast<void*>(child.base_+rva),&region,sizeof(region))==sizeof(region) &&
            (region.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) && read_image(child.base_,rva,value,false);
    };
    const auto lock_read_stack=[&](std::array<std::uint32_t,25>& value) {
        MEMORY_BASIC_INFORMATION region{};
        return VirtualQueryEx(child.process_,reinterpret_cast<void*>(lock_saved.esp-32),&region,sizeof(region))==sizeof(region) &&
            (region.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) &&
            read_private(lock_saved.esp-32,value.data(),100);
    };
    const auto arm_lock=[&](std::uint32_t stage,std::uint32_t address,CONTEXT& c) {
        c.ContextFlags=CONTEXT_DEBUG_REGISTERS;c.Dr0=address;c.Dr6=0;
        if (!SetThreadContext(child.thread_,&c)) return false;
        CONTEXT check{};check.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(child.thread_,&check) || check.Dr0!=address || check.Dr1!=child.base_+proxy->iat_rva ||
            check.Dr2!=trace.proxy_base+asi->call_rva || check.Dr3!=child.base_+instance->caller.target_rva+31 ||
            (check.Dr7&0xffff20ffU)!=0x00d00055U) return false;
        auto& state=trace.cwd_lock;state.stage=stage;state.stop_address=address;state.armed=true;return true;
    };
    EventDispatchRegisters acquire_saved{};
    static_assert(sizeof(CRITICAL_SECTION)==24 && offsetof(CRITICAL_SECTION,LockCount)==4 && offsetof(CRITICAL_SECTION,RecursionCount)==8 &&
        offsetof(CRITICAL_SECTION,OwningThread)==12 && offsetof(CRITICAL_SECTION,LockSemaphore)==16 && offsetof(CRITICAL_SECTION,SpinCount)==20);
    const auto acquire_shape=[&] {
        std::array<std::byte,11> code{};std::uint32_t target{};
        if(!lock_shape() || !cwd_acquire_code(*acquire,*lock,code) || !exact_code(child.base_,lock->selector.target_rva+38,code) ||
            !exact_code(child.base_,lock->selector.call_rva+5,acquire->return_prefix) || !read_word(child.base_,acquire->iat_rva,target))return false;
        auto& state=trace.cwd_acquire;
        if(state.function_address && state.function_address!=target)return false;
        for(std::uint32_t i=0;i<trace.module_count;++i){
            const auto& m=trace.modules[i];
            if(m.admitted && mappings.active(m.mapping_id,m.base) && std::string_view(m.file.name.data())=="ntdll.dll" &&
                target==m.base+acquire->function.rva && exact_code(m.base,acquire->function.rva,acquire->function.prefix)){
                state.function_address=target;return true;
            }
        }return false;
    };
    const auto acquire_read_object=[&](std::array<std::uint32_t,6>& value) {
        const auto address=trace.cwd_acquire.object_address;const auto& tib=trace.cwd_seh.tib_after;
        if(address<65536 || address>UINT32_MAX-24 || address%4 || (address<tib[1] && tib[2]<address+24) ||
            (address<tib[6]+56 && tib[6]<address+24))return false;
        MEMORY_BASIC_INFORMATION region{};
        if(VirtualQueryEx(child.process_,reinterpret_cast<void*>(address),&region,sizeof(region))!=sizeof(region))return false;
        if(address<reinterpret_cast<std::uintptr_t>(region.BaseAddress) || address-reinterpret_cast<std::uintptr_t>(region.BaseAddress)>region.RegionSize ||
            24>region.RegionSize-(address-reinterpret_cast<std::uintptr_t>(region.BaseAddress)))return false;
        if(trace.cwd_acquire.object_memory_type && (trace.cwd_acquire.object_memory_type!=region.Type || trace.cwd_acquire.object_protection!=region.Protect))return false;
        trace.cwd_acquire.object_memory_type=region.Type;trace.cwd_acquire.object_protection=region.Protect;
        if(region.Type==MEM_PRIVATE)return (region.Protect&(PAGE_READWRITE|PAGE_WRITECOPY)) && read_private(address,value.data(),24);
        if(region.Type!=MEM_IMAGE || reinterpret_cast<std::uintptr_t>(region.AllocationBase)!=child.base_ ||
            address<child.base_ || address-child.base_>lock->selector.image_size-24 ||
            !(region.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return false;
        const auto rva=static_cast<DWORD>(address-child.base_);
        auto bytes=std::as_writable_bytes(std::span(value));
        return read_image(child.base_,rva,bytes.first(16),false) && read_image(child.base_,rva+16,bytes.subspan(16),false);
    };
    const auto arm_acquire=[&](std::uint32_t stage,std::uint32_t address,CONTEXT& c) {
        c.ContextFlags=CONTEXT_DEBUG_REGISTERS;c.Dr0=address;c.Dr6=0;
        if (!SetThreadContext(child.thread_,&c)) return false;
        CONTEXT check{};check.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(child.thread_,&check) || check.Dr0!=address || check.Dr1!=child.base_+proxy->iat_rva ||
            check.Dr2!=trace.proxy_base+asi->call_rva || check.Dr3!=child.base_+instance->caller.target_rva+31 ||
            (check.Dr7&0xffff20ffU)!=0x00d00055U) return false;
        auto& state=trace.cwd_acquire;state.stage=stage;state.stop_address=address;state.armed=true;return true;
    };
    EventDispatchRegisters query_saved{};
    std::uint32_t query_cookie{};
    std::array<std::uint32_t,2> query_guard{};
    const auto query_shape=[&] {
        std::array<std::byte,18> wrapper{},path{};std::array<std::byte,28> helper{};
        if(!acquire_shape() || !cwd_query_code(*query,*lock,wrapper,helper,path) ||
            !exact_code(child.base_,lock->selector.call_rva+5,wrapper) || !exact_code(child.base_,query->helper_rva,helper) ||
            !exact_code(child.base_,query->helper_rva+123,path) || !exact_code(child.base_,query->helper_rva+141,query->return_prefix))return false;
        std::uint32_t k32{},kb{},target{},implementation{};
        for(std::uint32_t i=0;i<trace.module_count;++i) {
            const auto& m=trace.modules[i];
            if(!m.admitted || !mappings.active(m.mapping_id,m.base))continue;
            if(std::string_view(m.file.name.data())=="kernel32.dll")k32=static_cast<std::uint32_t>(m.base);
            if(std::string_view(m.file.name.data())=="kernelbase.dll")kb=static_cast<std::uint32_t>(m.base);
        }
        std::array<std::byte,20> function{},body{};
        if(!k32 || !kb || !bootstrap_export_prefix(query->function,k32,function) ||
            !bootstrap_export_prefix(query->implementation,kb,body) || !read_word(child.base_,query->iat_rva,target) ||
            target!=k32+query->function.rva || !read_word(k32,query->thunk_slot_rva,implementation) ||
            implementation!=kb+query->implementation.rva || !exact_code(k32,query->function.rva,function) ||
            !exact_code(kb,query->implementation.rva,body))return false;
        auto& state=trace.cwd_query;
        if((state.function_address && state.function_address!=target) ||
            (state.implementation_address && state.implementation_address!=implementation))return false;
        state.function_address=target;state.implementation_address=implementation;return true;
    };
    const auto arm_query=[&](std::uint32_t stage,std::uint32_t address,CONTEXT& c) {
        c.ContextFlags=CONTEXT_DEBUG_REGISTERS;c.Dr0=address;c.Dr6=0;
        if(!SetThreadContext(child.thread_,&c))return false;
        CONTEXT check{};check.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if(!GetThreadContext(child.thread_,&check) || check.Dr0!=address || check.Dr1!=child.base_+proxy->iat_rva ||
            check.Dr2!=trace.proxy_base+asi->call_rva || check.Dr3!=child.base_+instance->caller.target_rva+31 ||
            (check.Dr7&0xffff20ffU)!=0x00d00055U)return false;
        auto& state=trace.cwd_query;state.stage=stage;state.stop_address=address;state.armed=true;return true;
    };
    EventDispatchRegisters copy_api_return{};
    const auto copy_shape=[&] {
        CwdCopyCode code{};
        return query_shape() && cwd_copy_code(*copy,*query,*lock,code) &&
            exact_code(child.base_,query->helper_rva+141,code.checks) &&
            exact_code(child.base_,query->helper_rva+197,code.capacity) &&
            exact_code(child.base_,query->helper_rva+218,code.call) &&
            exact_code(child.base_,copy->copier_rva,code.entry) &&
            exact_code(child.base_,copy->copier_rva+117,code.body) &&
            exact_code(child.base_,query->helper_rva+231,copy->return_prefix);
    };
    const auto arm_copy=[&](std::uint32_t stage,std::uint32_t address,CONTEXT& c) {
        c.ContextFlags=CONTEXT_DEBUG_REGISTERS;c.Dr0=address;c.Dr6=0;
        if(!SetThreadContext(child.thread_,&c))return false;
        CONTEXT check{};check.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if(!GetThreadContext(child.thread_,&check) || check.Dr0!=address || check.Dr1!=child.base_+proxy->iat_rva ||
            check.Dr2!=trace.proxy_base+asi->call_rva || check.Dr3!=child.base_+instance->caller.target_rva+31 ||
            (check.Dr7&0xffff20ffU)!=0x00d00055U)return false;
        auto& state=trace.cwd_copy;state.stage=stage;state.stop_address=address;state.armed=true;return true;
    };
    EventDispatchRegisters return_copy_saved{};
    const auto return_shape=[&] {
        CwdReturnCode code{};
        return copy_shape() && cwd_return_code(*completion,*copy,*query,*lock,code) &&
            exact_code(child.base_,query->helper_rva+231,code.cleanup) &&
            exact_code(child.base_,query->helper_rva+63,code.epilogue) &&
            exact_code(child.base_,completion->checker_rva,code.checker) &&
            exact_code(child.base_,lock->selector.call_rva+23,completion->return_prefix);
    };
    const auto arm_return=[&](std::uint32_t stage,std::uint32_t address,CONTEXT& c) {
        c.ContextFlags=CONTEXT_DEBUG_REGISTERS;c.Dr0=address;c.Dr6=0;
        if(!SetThreadContext(child.thread_,&c))return false;
        CONTEXT check{};check.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if(!GetThreadContext(child.thread_,&check) || check.Dr0!=address || check.Dr1!=child.base_+proxy->iat_rva ||
            check.Dr2!=trace.proxy_base+asi->call_rva || check.Dr3!=child.base_+instance->caller.target_rva+31 ||
            (check.Dr7&0xffff20ffU)!=0x00d00055U)return false;
        auto& state=trace.cwd_return;state.stage=stage;state.stop_address=address;state.armed=true;return true;
    };
    const auto ready_shape=[&] {
        FileManagerReadyCode code{};std::uint32_t target{};
        if(!return_shape() || !file_manager_ready_code(*ready,*completion,*lock,*seh,*manager,code) ||
            !exact_code(child.base_,lock->selector.call_rva+23,code.wrapper) ||
            !exact_code(child.base_,seh->cleanup_rva,code.cleanup) || !exact_code(child.base_,ready->unlock_rva,code.unlock) ||
            !exact_code(child.base_,ready->epilogue_rva,code.epilogue) || !read_word(child.base_,ready->iat_rva,target))return false;
        auto& state=trace.file_manager_ready;
        if(state.function_address && state.function_address!=target)return false;
        for(std::uint32_t i=0;i<trace.module_count;++i) {
            const auto& m=trace.modules[i];
            if(m.admitted && mappings.active(m.mapping_id,m.base) && std::string_view(m.file.name.data())=="ntdll.dll" &&
                target==m.base+ready->function.rva && exact_code(m.base,ready->function.rva,ready->function.prefix)) {
                state.function_address=target;return true;
            }
        }return false;
    };
    const auto arm_ready=[&](std::uint32_t stage,std::uint32_t address,CONTEXT& c) {
        c.ContextFlags=CONTEXT_DEBUG_REGISTERS;c.Dr0=address;c.Dr6=0;
        if(!SetThreadContext(child.thread_,&c))return false;
        CONTEXT check{};check.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if(!GetThreadContext(child.thread_,&check) || check.Dr0!=address || check.Dr1!=child.base_+proxy->iat_rva ||
            check.Dr2!=trace.proxy_base+asi->call_rva || check.Dr3!=child.base_+instance->caller.target_rva+31 ||
            (check.Dr7&0xffff20ffU)!=0x00d00055U)return false;
        auto& state=trace.file_manager_ready;state.stage=stage;state.stop_address=address;state.armed=true;return true;
    };
    EventDispatchRegisters tables_saved{};
    const auto tables_shape=[&] {
        CdStreamTablesCode code{};
        return ready_shape() && cd_stream_tables_code(*tables,*manager,code) &&
            exact_code(child.base_,manager->manager.call_rva+5,code.caller) && exact_code(child.base_,tables->target_rva,code.body);
    };
    const auto tables_read=[&](CdStreamTableWindow& window) {
        MEMORY_BASIC_INFORMATION region{};const auto address=child.base_+tables->handles_rva-4;
        if(VirtualQueryEx(child.process_,reinterpret_cast<void*>(address),&region,sizeof(region))!=sizeof(region) ||
            region.Type!=MEM_IMAGE || region.State!=MEM_COMMIT || reinterpret_cast<std::uintptr_t>(region.AllocationBase)!=child.base_ ||
            !(region.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) ||
            (region.Protect&(PAGE_GUARD|PAGE_NOACCESS)) || address<reinterpret_cast<std::uintptr_t>(region.BaseAddress) ||
            address-reinterpret_cast<std::uintptr_t>(region.BaseAddress)>region.RegionSize ||
            window.size()>region.RegionSize-(address-reinterpret_cast<std::uintptr_t>(region.BaseAddress)))return false;
        for(std::size_t i=0;i<window.size();i+=16)if(!read_image(child.base_,tables->handles_rva-4+static_cast<DWORD>(i),
            std::span(window).subspan(i,std::min<std::size_t>(16,window.size()-i)),false))return false;
        return true;
    };

    EventDispatchRegisters disk_return_saved{};
    const auto disk_shape=[&] {
        CdStreamDiskCode code{};
        if(!tables_shape() || !cd_stream_disk_code(*disk,*tables,*manager,code) ||
            !exact_code(child.base_,tables->target_rva+68,code.body))return false;
        std::uint32_t k32{},kb{},target{},implementation{};
        for(std::uint32_t i=0;i<trace.module_count;++i) {
            const auto& m=trace.modules[i];if(!m.admitted || !mappings.active(m.mapping_id,m.base))continue;
            if(std::string_view(m.file.name.data())=="kernel32.dll")k32=static_cast<DWORD>(m.base);
            if(std::string_view(m.file.name.data())=="kernelbase.dll")kb=static_cast<DWORD>(m.base);
        }
        std::array<std::byte,6> thunk{std::byte{0xff},std::byte{0x25}};
        if(!k32 || !kb || k32>UINT32_MAX-16*1024*1024 || kb>UINT32_MAX-16*1024*1024)return false;
        const auto slot=k32+disk->thunk_slot_rva;std::memcpy(thunk.data()+2,&slot,4);
        std::array<std::byte,20> body{};
        if(!bootstrap_export_prefix(disk->implementation,kb,body) || !exact_code(k32,disk->thunk_rva,thunk) ||
            !read_word(child.base_,tables->disk_iat_rva,target) || target!=k32+disk->thunk_rva ||
            !read_word(k32,disk->thunk_slot_rva,implementation) || implementation!=kb+disk->implementation.rva ||
            !exact_code(kb,disk->implementation.rva,body))return false;
        auto& state=trace.cd_stream_disk;
        if((state.function_address && state.function_address!=target) ||
            (state.implementation_address && state.implementation_address!=implementation))return false;
        state.function_address=target;state.implementation_address=implementation;return true;
    };
    const auto disk_read_globals=[&](std::array<std::uint32_t,5>& values) {
        const auto address=child.base_+disk->flags_rva-4;MEMORY_BASIC_INFORMATION region{};
        if(VirtualQueryEx(child.process_,reinterpret_cast<void*>(address),&region,sizeof(region))!=sizeof(region) ||
            region.State!=MEM_COMMIT || region.Type!=MEM_IMAGE || reinterpret_cast<std::uintptr_t>(region.AllocationBase)!=child.base_ ||
            !(region.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) ||
            (region.Protect&(PAGE_GUARD|PAGE_NOACCESS)) || address<reinterpret_cast<std::uintptr_t>(region.BaseAddress) ||
            address-reinterpret_cast<std::uintptr_t>(region.BaseAddress)>region.RegionSize ||
            20>region.RegionSize-(address-reinterpret_cast<std::uintptr_t>(region.BaseAddress)))return false;
        auto bytes=std::as_writable_bytes(std::span(values));
        return read_image(child.base_,disk->flags_rva-4,bytes.first(16),false) &&
            read_image(child.base_,disk->flags_rva+12,bytes.subspan(16),false);
    };
    const auto arm_disk=[&](std::uint32_t stage,CONTEXT& c) {
        auto& state=trace.cd_stream_disk;
        const auto address=stage==1?state.function_address:stage==2?state.implementation_address:
            static_cast<DWORD>(child.base_+tables->target_rva+(stage==3?74:132));
        c.ContextFlags=CONTEXT_DEBUG_REGISTERS;c.Dr0=address;c.Dr6=0;
        if(!SetThreadContext(child.thread_,&c))return false;
        CONTEXT check{};check.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if(!GetThreadContext(child.thread_,&check) || check.Dr0!=address || check.Dr1!=child.base_+proxy->iat_rva ||
            check.Dr2!=trace.proxy_base+asi->call_rva || check.Dr3!=child.base_+instance->caller.target_rva+31 ||
            (check.Dr7&0xffff20ffU)!=0x00d00055U)return false;
        state.stage=stage;state.stop_address=address;state.armed=true;return true;
    };

    EventDispatchRegisters allocation_saved{},allocation_heap_return{};
    std::array<std::byte,44> allocation_caller_stack{};
    const auto allocation_shape=[&] {
        CdStreamAllocationCode code{};std::uint32_t target{};
        auto& state=trace.cd_stream_allocation;state.shape_failure=0;
        const auto checked=[&](bool valid,std::uint32_t step){if(!valid)state.shape_failure=step;return valid;};
        if(!checked(disk_shape(),1) || !checked(cd_stream_allocation_code(*allocation,*disk,*manager,*seh,*ready,code),2) ||
            !checked(exact_code(child.base_,disk->allocator_rva,code.aligned),3) || !checked(exact_code(child.base_,allocation->malloc_rva,code.malloc),4) ||
            !checked(exact_code(child.base_,allocation->nh_rva,code.nh),5) ||
            !checked(exact_code(child.base_,allocation->nh_rva+43,std::array<std::byte,1>{std::byte{0xc3}}),6) ||
            !checked(exact_code(child.base_,allocation->heap_rva,code.heap_entry),7) || !checked(exact_code(child.base_,allocation->heap_rva+70,code.heap_tail),8) ||
            !checked(read_word(child.base_,allocation->iat_rva,target),9))return false;
        // Cleanup scope is recorded but no exception handler execution is permitted.
        std::array<std::byte,12> scope{};
        if(!checked(read_image(child.base_,allocation->scope_rva,scope,false),10))return false;
        std::array<DWORD,3> words{};std::memcpy(words.data(),scope.data(),12);
        if(!checked(words[0]==UINT32_MAX && !words[1] && words[2]==child.base_+allocation->scope_cleanup_rva,11))return false;
        if(!checked(!state.function_address || state.function_address==target,12))return false;
        for(std::uint32_t i=0;i<trace.module_count;++i) {
            const auto& m=trace.modules[i];std::array<std::byte,20> prefix{};
            if(m.admitted && mappings.active(m.mapping_id,m.base) && std::string_view(m.file.name.data())=="ntdll.dll" &&
                target==m.base+allocation->function.rva && bootstrap_export_prefix(allocation->function,static_cast<DWORD>(m.base),prefix) &&
                exact_code(m.base,allocation->function.rva,prefix)){state.function_address=target;return true;}
        }state.shape_failure=13;return false;
    };
    const auto allocation_globals=[&](std::array<std::uint32_t,4>& values) {
        return read_word(child.base_,allocation->new_mode_rva,values[0]) && read_word(child.base_,allocation->threshold_rva,values[1]) &&
            read_word(child.base_,allocation->heap_handle_rva,values[2]) && read_word(child.base_,allocation->heap_mode_rva,values[3]);
    };
    const auto arm_allocation=[&](std::uint32_t stage,CONTEXT& c) {
        auto& state=trace.cd_stream_allocation;
        const std::array<DWORD,11> rvas{0,disk->allocator_rva,allocation->heap_rva,allocation->heap_rva+22,
            allocation->heap_rva+30,allocation->heap_rva+99,0,allocation->heap_rva+105,disk->allocator_rva+17,disk->allocator_rva+30,tables->target_rva+137};
        const auto address=stage==6?state.function_address:static_cast<DWORD>(child.base_+rvas[stage]);
        c.ContextFlags=CONTEXT_DEBUG_REGISTERS;c.Dr0=address;c.Dr6=0;
        if(!SetThreadContext(child.thread_,&c))return false;
        CONTEXT check{};check.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if(!GetThreadContext(child.thread_,&check) || check.Dr0!=address || check.Dr1!=child.base_+proxy->iat_rva ||
            check.Dr2!=trace.proxy_base+asi->call_rva || check.Dr3!=child.base_+instance->caller.target_rva+31 ||
            (check.Dr7&0xffff20ffU)!=0x00d00055U)return false;
        state.stage=stage;state.stop_address=address;state.armed=true;return true;
    };
    EventDispatchRegisters channels_saved{},channels_error_return{},channels_local_return{};
    std::array<std::byte,32> channels_caller_stack{};
    const auto channels_shape=[&] {
        auto& state=trace.cd_stream_channels;state.shape_failure=0;
        const auto checked=[&](bool valid,std::uint32_t step){if(!valid)state.shape_failure=step;return valid;};
        std::array<std::byte,64> code{};
        if(!checked(allocation_shape(),1) || !checked(cd_stream_channels_code(*channels,*tables,*manager,code),2) ||
            !checked(exact_code(child.base_,tables->target_rva+137,code),3))return false;
        std::array<std::byte,16> name{};constexpr char expected[]="MODELS\\GTA3.IMG";
        if(!checked(read_image(child.base_,channels->filename_rva,name,false) && std::memcmp(name.data(),expected,16)==0,4))return false;
        const std::array<const BootstrapExportSpec*,4> specs{&channels->error_thunk,&channels->error_implementation,&channels->local_thunk,&channels->local_implementation};
        const std::array<std::string_view,4> modules{"kernel32.dll","ntdll.dll","kernel32.dll","kernelbase.dll"};
        std::array<std::uint32_t,4> functions{};std::uint32_t kernel{};
        for(unsigned n=0;n<4;++n)for(std::uint32_t i=0;i<trace.module_count;++i) {
            const auto& item=trace.modules[i];std::array<std::byte,20> prefix{};
            if(item.admitted && mappings.active(item.mapping_id,item.base) && std::string_view(item.file.name.data())==modules[n] &&
                bootstrap_export_prefix(*specs[n],static_cast<DWORD>(item.base),prefix) && exact_code(item.base,specs[n]->rva,prefix)) {
                functions[n]=static_cast<DWORD>(item.base+specs[n]->rva);if(n==0)kernel=static_cast<DWORD>(item.base);
            }
        }
        if(!checked(std::all_of(functions.begin(),functions.end(),[](auto x){return x!=0;}),5))return false;
        std::uint32_t a{},b{},c{},d{};
        if(!checked(read_word(child.base_,channels->error_iat_rva,a) && a==functions[0] && read_word(child.base_,channels->local_iat_rva,b) && b==functions[2] &&
            read_word(kernel,channels->error_thunk_slot_rva,c) && c==functions[1] && read_word(kernel,channels->local_thunk_slot_rva,d) && d==functions[3],6))return false;
        if(!checked(!state.functions[0] || state.functions==functions,7))return false;
        state.functions=functions;return true;
    };
    const auto arm_channels=[&](std::uint32_t stage,CONTEXT& c) {
        auto& state=trace.cd_stream_channels;
        const auto start=static_cast<DWORD>(child.base_+tables->target_rva+137);
        const std::array<DWORD,10> addresses{0,start+7,state.functions[0],state.functions[1],start+13,start+41,state.functions[2],state.functions[3],start+47,start+59};
        c.ContextFlags=CONTEXT_DEBUG_REGISTERS;c.Dr0=addresses[stage];c.Dr6=0;
        if(!SetThreadContext(child.thread_,&c))return false;
        CONTEXT check{};check.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if(!GetThreadContext(child.thread_,&check) || check.Dr0!=addresses[stage] || check.Dr1!=child.base_+proxy->iat_rva ||
            check.Dr2!=trace.proxy_base+asi->call_rva || check.Dr3!=child.base_+instance->caller.target_rva+31 ||
            (check.Dr7&0xffff20ffU)!=0x00d00055U)return false;
        state.stage=stage;state.stop_address=addresses[stage];state.armed=true;return true;
    };
    const auto arm_tables=[&](std::uint32_t stage,CONTEXT& c) {
        const auto address=static_cast<DWORD>(child.base_+tables->target_rva+(stage==2?68:0));
        c.ContextFlags=CONTEXT_DEBUG_REGISTERS;c.Dr0=address;c.Dr6=0;
        if(!SetThreadContext(child.thread_,&c))return false;
        CONTEXT check{};check.ContextFlags=CONTEXT_DEBUG_REGISTERS;
        if(!GetThreadContext(child.thread_,&check) || check.Dr0!=address || check.Dr1!=child.base_+proxy->iat_rva ||
            check.Dr2!=trace.proxy_base+asi->call_rva || check.Dr3!=child.base_+instance->caller.target_rva+31 ||
            (check.Dr7&0xffff20ffU)!=0x00d00055U)return false;
        auto& state=trace.cd_stream_tables;state.stage=stage;state.stop_address=address;state.armed=true;return true;
    };
    CONTEXT bootstrap_saved{};
    std::array<std::byte, 224> bootstrap_envelope{};
    const auto bootstrap_shape = [&] {
        if (!mappings.active(trace.asi_mapping_id, trace.asi_handle) || !asi_shape()) return false;
        for (const auto& function : bootstrap->exports) {
            std::array<std::byte, 20> actual{}, expected{};
            if (!bootstrap_export_prefix(function, static_cast<std::uint32_t>(trace.asi_handle), expected) ||
                !read_image(trace.asi_handle, function.rva, std::span(actual).first(16), true) ||
                !read_image(trace.asi_handle, function.rva + 16, std::span(actual).subspan(16), true) || actual != expected) return false;
        }
        return true;
    };
    const auto write_stack = [&](std::uint32_t address, const void* bytes, std::size_t size) {
        MEMORY_BASIC_INFORMATION region{}, original{}; SIZE_T written{};
        if (!size || size > 224 || address > UINT32_MAX - size ||
            VirtualQueryEx(child.process_, reinterpret_cast<void*>(bootstrap_saved.Esp), &original, sizeof(original)) != sizeof(original) ||
            VirtualQueryEx(child.process_, reinterpret_cast<void*>(address), &region, sizeof(region)) != sizeof(region) ||
            region.AllocationBase != original.AllocationBase || region.Type != MEM_PRIVATE || region.State != MEM_COMMIT ||
            (region.Protect & 0xffU) != PAGE_READWRITE || (region.Protect & PAGE_GUARD) ||
            address < reinterpret_cast<std::uintptr_t>(region.BaseAddress) || address + size > bootstrap_saved.Esp) return false;
        const auto offset = address - reinterpret_cast<std::uintptr_t>(region.BaseAddress);
        if (offset >= region.RegionSize || size > region.RegionSize - offset) return false;
        trace.bootstrap.stack_write_attempted = true;
        const auto ok = WriteProcessMemory(child.process_, reinterpret_cast<void*>(address), bytes, size, &written);
        if (written) trace.bootstrap.stack_written = true;
        if (!ok || written != size) return false;
        std::array<std::byte, 224> checked{};
        return read_private(address, checked.data(), size) && std::memcmp(checked.data(), bytes, size) == 0;
    };
    const auto arm_bootstrap = [&](CONTEXT& context) {
        auto& state = trace.bootstrap;
        if (state.calls_armed >= state.calls.size() || !bootstrap_shape()) return false;
        if (!state.calls_armed) {
            bootstrap_saved = context;
            if (context.Esp < 512 || context.Esp % 4 || (context.EFlags & 0x500U)) return false; // TF/DF
            const auto base = (context.Esp - 256U) & ~15U;
            state.frame_address = base - 20U; state.output_address = base + 16U;
        }
        bootstrap_envelope.fill(std::byte{0xa5});
        const std::array<std::uint32_t, 4> frame{static_cast<std::uint32_t>(trace.asi_return_address),
            SAEX_BOOTSTRAP_ABI_MAJOR, state.output_address, sizeof(SaexBootstrapStatus)};
        if (!write_stack(state.output_address - 16, bootstrap_envelope.data(), bootstrap_envelope.size()) ||
            !write_stack(state.frame_address, frame.data(), sizeof(frame))) return false;
        auto& call = state.calls[state.calls_armed]; call.export_index = bootstrap_call_sequence[state.calls_armed];
        call.target = static_cast<std::uint32_t>(trace.asi_handle + bootstrap->exports[call.export_index].rva);
        context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_DEBUG_REGISTERS;
        context.Eip = call.target; context.Esp = state.frame_address; context.Dr6 = 0;
        // Clear resume flag from the preceding execution breakpoint; DR0 remains at return.
        context.EFlags &= ~0x10000U;
        if (!SetThreadContext(child.thread_, &context)) return false;
        CONTEXT checked{}; checked.ContextFlags = context.ContextFlags;
        if (!GetThreadContext(child.thread_, &checked) || checked.Eip != context.Eip || checked.Esp != context.Esp ||
            checked.Ebx != bootstrap_saved.Ebx || checked.Esi != bootstrap_saved.Esi ||
            checked.Edi != bootstrap_saved.Edi || checked.Ebp != bootstrap_saved.Ebp ||
            checked.Dr0 != trace.asi_return_address || checked.Dr1 != child.base_ + proxy->iat_rva ||
            checked.Dr2 != trace.proxy_base + asi->end_rva || (checked.Dr7 & 0xffff20ffU) != 0x00d00015U) return false;
        call.armed = true; ++state.calls_armed;
        return true;
    };
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
        if (trace.cd_stream_channels.armed) trace.cd_stream_channels.continued=true;
        if (trace.cd_stream_allocation.armed) trace.cd_stream_allocation.continued=true;
        if (trace.cd_stream_disk.armed) trace.cd_stream_disk.continued=true;
        if (trace.cd_stream_tables.armed) trace.cd_stream_tables.continued=true;
        if (trace.file_manager_ready.armed) trace.file_manager_ready.continued=true;
        if (trace.cwd_return.armed) trace.cwd_return.continued=true;
        if (trace.cwd_copy.armed) trace.cwd_copy.continued=true;
        if (trace.cwd_query.armed) trace.cwd_query.continued=true;
        if (trace.cwd_acquire.armed) trace.cwd_acquire.continued=true;
        if (trace.cwd_lock.armed) trace.cwd_lock.continued=true;
        if (trace.cwd_seh.armed) trace.cwd_seh.continued=true;
        if (trace.file_manager_entry.armed) trace.file_manager_entry.continued=true;
        if (trace.game_prelude.armed) trace.game_prelude.continued=true;
        if (trace.application_routing.armed) trace.application_routing.continued=true;
        if (trace.event_dispatch.armed) trace.event_dispatch.continued=true;
        if (trace.instance_startup.armed) trace.instance_startup.continued=true;
        if (trace.platform_suppression.transaction.applied) trace.platform_suppression.continued=true;
        if (trace.platform_startup.armed) trace.platform_startup.continued=true;
        if (trace.application_entry.armed) trace.application_entry.continued=true;
        if (trace.crt_startup.armed) trace.crt_startup.continued=true;
        if (trace.startup_return.armed) {
            trace.startup_return.continued=true;
            if (trace.startup_return.stage==2) trace.startup_return.protect_continued=true;
        }
        if (trace.bootstrap.calls_armed > trace.bootstrap.calls_returned)
            trace.bootstrap.calls[trace.bootstrap.calls_armed - 1].continued = true;
        if (trace.asi_return_armed) trace.asi_load_continued = true;
        if (trace.asi_call_armed) trace.asi_scan_continued = true;
        if (trace.binding_breakpoint_armed) trace.binding_continued = true;
        if (trace.codec_breakpoint_armed) trace.codec_continued = true;
        if (trace.startup_breakpoint_armed) trace.startup_continued = true;
        if (trace.proxy_breakpoint_armed) trace.proxy_continued = true;
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
            if (trace.binding_continued && !trace.asi_load_continued) return finish("binding_unexpected_load");
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
            if (trace.proxy_validated && transition.mapping_id == trace.proxy_mapping_id)
                return finish("proxy_mapping_retired");
            if (trace.binding_continued && std::find(trace.codec_mapping_ids.begin(), trace.codec_mapping_ids.end(), transition.mapping_id) != trace.codec_mapping_ids.end())
                return finish("binding_mapping_retired");
            if (asi && matches_file(retired->file, asi->module->identity())) return finish("asi_mapping_retired");
            // Do not read the retired address or refund any lifetime load/byte budget.
        } else if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            if (++trace.thread_count > limits.threads) return finish("loader_thread_limit");
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            trace.exception_code = event.u.Exception.ExceptionRecord.ExceptionCode;
            trace.exception_address = reinterpret_cast<std::uintptr_t>(event.u.Exception.ExceptionRecord.ExceptionAddress);
            trace.last_event_address = trace.exception_address;
            if (entry && trace.initial_breakpoint_continued) {
                CONTEXT context{}; context.ContextFlags = CONTEXT_CONTROL | CONTEXT_DEBUG_REGISTERS | CONTEXT_INTEGER;
                const auto expected_address = trace.cd_stream_channels.armed ? trace.cd_stream_channels.stop_address : trace.cd_stream_allocation.armed ? trace.cd_stream_allocation.stop_address : trace.cd_stream_disk.armed ? trace.cd_stream_disk.stop_address : trace.cd_stream_tables.armed ? trace.cd_stream_tables.stop_address : trace.file_manager_ready.armed ? trace.file_manager_ready.stop_address : trace.cwd_return.armed ? trace.cwd_return.stop_address : trace.cwd_copy.armed ? trace.cwd_copy.stop_address : trace.cwd_query.armed ? trace.cwd_query.stop_address : trace.cwd_acquire.armed ? trace.cwd_acquire.stop_address : trace.cwd_lock.armed ? trace.cwd_lock.stop_address : trace.cwd_seh.armed ? trace.cwd_seh.stop_address : trace.file_manager_entry.armed ? trace.file_manager_entry.stop_address : trace.game_prelude.armed ? trace.game_prelude.stop_address : trace.application_routing.armed ? trace.application_routing.stop_address : trace.event_dispatch.armed ? trace.event_dispatch.stop_address : trace.instance_startup.armed ? trace.instance_startup.stop_address : trace.platform_suppression.transaction.applied ? trace.platform_suppression.return_address : trace.platform_startup.armed ? trace.platform_startup.stop_address : trace.application_entry.armed ? trace.application_entry.stop_address : trace.crt_startup.armed ? trace.crt_startup.stop_address : trace.startup_return.armed ? trace.startup_return.stop_address : trace.asi_load_continued ? trace.asi_return_address : trace.asi_scan_continued ? trace.asi_call_address : trace.binding_continued ? trace.binding_stop_address : trace.codec_continued ? trace.codec_return_address : trace.startup_continued ? trace.startup_address : trace.proxy_continued ? trace.proxy_return_address : trace.entry_address;
                if (trace.exception_code != EXCEPTION_SINGLE_STEP || !event.u.Exception.dwFirstChance ||
                    event.dwThreadId != GetThreadId(child.thread_))
                    return finish("entry_unexpected_exception");
                if (!GetThreadContext(child.thread_, &context)) {
                    trace.system_error = GetLastError(); return finish("entry_context_read_failed");
                }
                const auto watched_dr7 = trace.instance_startup.armed ? 0x00d00055U : trace.asi_scan_continued ? 0x00d00015U : 0x00d00005U;
                if (trace.startup_continued && (context.Dr0 != expected_address ||
                    context.Dr1 != child.base_ + proxy->iat_rva || (context.Dr7 & 0xffff20ffU) != watched_dr7 ||
                    (trace.asi_scan_continued && context.Dr2 != trace.proxy_base + (trace.startup_return.armed ? asi->call_rva : asi->end_rva))))
                    return finish("entry_breakpoint_identity");
                if (trace.instance_startup.armed) {
                    if (context.Dr3!=child.base_+instance->caller.target_rva+31) return finish("instance_guard_identity");
                    if (context.Dr6&8U) return finish("instance_forbidden_branch");
                }
                if (trace.asi_scan_continued && (context.Dr6 & 4U)) {
                    if ((context.Dr6 & 0xe00fU) != 4U || context.Eip != context.Dr2 || trace.exception_address != context.Dr2)
                        return finish("entry_breakpoint_identity");
                    return finish(trace.startup_return.armed ? "startup_return_extra_asi_call" : trace.asi_load_continued ? "asi_return_skipped" : "asi_candidate_missing");
                }
                // The main-thread IAT write watchpoint traps AFTER a write; it is not prevention.
                if (trace.startup_continued && (context.Dr6 & 2U)) {
                    if ((context.Dr6 & 0xe00fU) != 2U) return finish("entry_breakpoint_identity");
                    trace.startup_iat_write_observed = true;
                    return finish("startup_iat_written");
                }
                if (trace.exception_address != expected_address) return finish("entry_unexpected_exception");
                // B0 only for the execution fault. Startup also retains a 4-byte write watch in DR1.
                if (context.Eip != expected_address || context.Dr0 != expected_address ||
                    (context.Dr6 & 0xe00fU) != 1 ||
                    (context.Dr7 & (trace.startup_continued ? 0xffff20ffU : 0xffff00ffU)) != (trace.startup_continued ? watched_dr7 : 1U) ||
                    (trace.startup_continued && context.Dr1 != child.base_ + proxy->iat_rva))
                    return finish("entry_breakpoint_identity");


                if(trace.cd_stream_channels.armed) {
                    auto& state=trace.cd_stream_channels;const auto stage=state.stage,A=channels_saved.esp;
                    state.shape_valid=channels_shape();if(!state.shape_valid)return finish("cd_stream_channels_shape_drift");
                    state.frame_valid=cd_stream_channels_frame(stage,channels_saved,dispatch_registers(context),channels_error_return,channels_local_return);
                    if(!state.frame_valid)return finish("cd_stream_channels_frame");
                    std::array<std::byte,32> caller{};std::array<std::uint32_t,3> args{};
                    state.arguments_valid=read_private(A+12,caller.data(),32) && caller==channels_caller_stack;
                    if(stage<=3) {
                        const auto count=stage==1?1U:2U;
                        state.arguments_valid=state.arguments_valid && read_private(context.Esp,args.data(),count*4) && args[count-1]==0;
                        if(stage!=1)state.arguments_valid=state.arguments_valid && args[0]==child.base_+tables->target_rva+150;
                    } else if(stage>=5 && stage<=7) {
                        const auto offset=stage==5?0U:1U;
                        state.arguments_valid=state.arguments_valid && read_private(context.Esp,args.data(),(2+offset)*4) && args[offset]==0x40 && args[offset+1]==240;
                        if(offset)state.arguments_valid=state.arguments_valid && args[0]==child.base_+tables->target_rva+184;
                    } else if(stage==9)state.arguments_valid=state.arguments_valid && read_private(context.Esp,args.data(),8) && args[0]==child.base_+channels->filename_rva && !args[1];
                    if(!state.arguments_valid)return finish("cd_stream_channels_arguments");
                    CdStreamTableWindow table{};state.tables_valid=tables_read(table) && cd_stream_channels_tables(trace.cd_stream_tables.after,table,stage>=5);
                    if(!state.tables_valid)return finish("cd_stream_channels_tables_drift");
                    if(!read_image(child.base_,channels->pointer_rva-4,state.pointer_after,false))return finish("cd_stream_channels_pointer_unreadable");
                    for(unsigned i=0;i<12;++i) {
                        const auto expected=stage==9 && i>=4 && i<8?std::byte((state.pointer>>((i-4)*8))&255U):state.pointer_before[i];
                        if(state.pointer_after[i]!=expected)return finish("cd_stream_channels_pointer_drift");
                    }
                    std::array<std::uint32_t,7> tib{};std::array<std::uint32_t,2> prior{};
                    state.seh_preserved=seh_read_tib(tib) && tib==trace.cwd_seh.tib_before &&
                        (tib[0]==UINT32_MAX || (read_private(tib[0],prior.data(),8) && prior==seh_prior_record));
                    if(!state.seh_preserved)return finish("cd_stream_channels_seh_drift");
                    std::array<std::byte,136> directory{};std::array<std::byte,32> parent{};std::array<std::byte,156> application_caller{};
                    std::array<std::byte,16> flags{};std::array<std::uint32_t,6> object{};std::array<std::uint32_t,5> disk_globals{};
                    state.parent_preserved=disk_read_globals(disk_globals) && disk_globals==trace.cd_stream_disk.globals_after &&
                        manager_read_buffer(directory) && directory==trace.file_manager_ready.directory_after &&
                        read_private(prelude_saved.esp,parent.data(),32) && parent==prelude_parent_stack &&
                        read_private(dispatch_saved.esp,application_caller.data(),156) && std::equal(application_caller.begin(),application_caller.end(),suppression_stack.begin()+16) &&
                        prelude_read_flags(flags) && flags==trace.game_prelude.flags_after && acquire_read_object(object) &&
                        cwd_acquire_object(trace.cwd_acquire.object_address,trace.cwd_acquire.object_before,object,trace.cwd_acquire.thread_id,false) &&
                        same_named_event(child.process_,trace.instance_startup.event_handle,instance->name);
                    if(!state.parent_preserved)return finish("cd_stream_channels_parent_drift");
                    std::array<std::uint32_t,4> heap_globals{};std::array<std::byte,32> hash{};std::uint32_t metadata{};
                    const auto& block=trace.cd_stream_allocation.block;
                    state.allocation_preserved=allocation_globals(heap_globals) && heap_globals==trace.cd_stream_allocation.globals &&
                        allocation_hash(child.process_,block,hash) && hash==trace.cd_stream_allocation.block_after_hash &&
                        read_private(block.metadata,&metadata,4) && metadata==block.raw;
                    if(!state.allocation_preserved)return finish("cd_stream_channels_allocation_drift");
                    if(stage==4) {
                        channels_error_return=dispatch_registers(context);state.error_reset=read_last_error(state.error_after_reset) && state.error_after_reset==0;
                        if(!state.error_reset)return finish("cd_stream_channels_error_not_reset");
                    }
                    if(stage==8) {
                        channels_local_return=dispatch_registers(context);state.local_returned=true;state.pointer=context.Eax;
                        if(!read_last_error(state.error_after_local))return finish("cd_stream_channels_error_unreadable");
                        state.allocation_succeeded=state.pointer!=0;if(!state.allocation_succeeded)return finish("cd_stream_channels_allocation_failed");
                        state.block_valid=cd_stream_channels_range(state.pointer,block) && !(state.pointer<tib[1] && tib[2]<state.pointer+240) &&
                            !(state.pointer<tib[6]+56 && tib[6]<state.pointer+240);
                        if(!state.block_valid)return finish("cd_stream_channels_block_rejected");
                    }
                    if(stage>=8) {
                        MEMORY_BASIC_INFORMATION region{};
                        if(VirtualQueryEx(child.process_,reinterpret_cast<void*>(state.pointer),&region,sizeof(region))!=sizeof(region) ||
                            region.State!=MEM_COMMIT || region.Type!=MEM_PRIVATE || region.Protect!=PAGE_READWRITE ||
                            !read_private(state.pointer,state.channels.data(),240))return finish("cd_stream_channels_block_unreadable");
                        state.zero_initialized=cd_stream_channels_memory(state.channels);
                        if(!state.zero_initialized)return finish("cd_stream_channels_nonzero_block");
                    }
                    if(stage==9){state.pointer_published=true;state.verified=true;return finish("cd_stream_channels_verified");}
                    if(!arm_channels(stage+1,context))return finish("cd_stream_channels_arm_failed");
                    continue;
                }
                if(trace.cd_stream_allocation.armed) {
                    auto& state=trace.cd_stream_allocation;const auto stage=state.stage,a=trace.cd_stream_disk.geometry.bytes_per_sector,A=allocation_saved.esp;
                    state.shape_valid=allocation_shape();if(!state.shape_valid)return finish("cd_stream_allocation_shape_drift");
                    std::array<std::uint32_t,4> globals{};state.globals_preserved=allocation_globals(globals) && globals==state.globals;
                    if(!state.globals_preserved)return finish("cd_stream_allocation_globals_drift");
                    state.frame_valid=cd_stream_allocation_frame(stage,allocation_saved,dispatch_registers(context),a,state.block,allocation_heap_return);
                    if(!state.frame_valid)return finish("cd_stream_allocation_frame");
                    std::array<std::byte,44> caller_stack{};
                    state.arguments_valid=read_private(A,caller_stack.data(),44) && caller_stack==allocation_caller_stack;
                    DWORD ret{};state.arguments_valid=state.arguments_valid && read_private(A-4,&ret,4) && ret==child.base_+tables->target_rva+137;
                    if(stage>=2 && stage<=7) {
                        std::array<DWORD,8> nested{};
                        state.arguments_valid=state.arguments_valid && read_private(A-36,nested.data(),32) &&
                            nested==std::array<DWORD,8>{static_cast<DWORD>(child.base_+allocation->nh_rva+16),2048+a,
                                static_cast<DWORD>(child.base_+allocation->malloc_rva+15),2048+a,0,
                                static_cast<DWORD>(child.base_+disk->allocator_rva+17),2048+a,allocation_saved.esi};
                    }
                    if(stage==5 || stage==6) {
                        std::array<DWORD,4> args{};const auto count=stage==6?4U:3U;
                        state.arguments_valid=state.arguments_valid && read_private(context.Esp,args.data(),count*4);
                        if(stage==6)state.arguments_valid=state.arguments_valid && args[0]==child.base_+allocation->heap_rva+105;
                        const auto offset=stage==6?1U:0U;
                        state.arguments_valid=state.arguments_valid && args[offset]==state.globals[2] && !args[offset+1] && args[offset+2]==2048+a;
                    }
                    if(!state.arguments_valid)return finish("cd_stream_allocation_arguments");
                    if(stage==3) {
                        if(((context.EFlags&0x40U)!=0)!=(state.globals[3]==3))return finish("cd_stream_allocation_heap_branch");
                        state.branch_verified=state.globals[3]==1;
                    }
                    if(stage==4) {
                        if(state.globals[3]!=3 || (context.EFlags&0x41U))return finish("cd_stream_allocation_threshold_branch");
                        state.branch_verified=true;
                    }
                    std::array<std::uint32_t,7> tib{},expected_tib=trace.cwd_seh.tib_before;
                    const bool active=stage>=3 && stage<=7;
                    if(active)expected_tib[0]=A-56;
                    state.seh_preserved=seh_read_tib(tib) && tib==expected_tib;
                    if(active) {
                        std::array<DWORD,11> heap_frame{};
                        state.seh_preserved=state.seh_preserved && read_private(A-80,heap_frame.data(),44) &&
                            heap_frame[0]==allocation_saved.edi && heap_frame[1]==a && heap_frame[2]==allocation_saved.ebx && heap_frame[4]==A-80 &&
                            heap_frame[6]==trace.cwd_seh.tib_before[0] && heap_frame[7]==child.base_+seh->handler_rva &&
                            heap_frame[8]==child.base_+allocation->scope_rva && heap_frame[9]==UINT32_MAX && heap_frame[10]==allocation_saved.ebp;
                    }
                    std::array<std::uint32_t,2> prior{};state.prior_record_preserved=trace.cwd_seh.tib_before[0]==UINT32_MAX ||
                        (read_private(trace.cwd_seh.tib_before[0],prior.data(),8) && prior==seh_prior_record);
                    if(!state.seh_preserved || !state.prior_record_preserved)return finish("cd_stream_allocation_seh_drift");
                    CdStreamTableWindow table{};std::array<std::uint32_t,5> disk_globals{};std::array<std::byte,136> directory{};
                    std::array<std::byte,32> parent{};std::array<std::byte,156> caller{};std::array<std::byte,16> flags{};std::array<std::uint32_t,6> object{};
                    state.parent_preserved=tables_read(table) && table==trace.cd_stream_tables.after &&
                        disk_read_globals(disk_globals) && disk_globals==trace.cd_stream_disk.globals_after &&
                        manager_read_buffer(directory) && directory==trace.file_manager_ready.directory_after &&
                        read_private(prelude_saved.esp,parent.data(),32) && parent==prelude_parent_stack &&
                        read_private(dispatch_saved.esp,caller.data(),156) && std::equal(caller.begin(),caller.end(),suppression_stack.begin()+16) &&
                        prelude_read_flags(flags) && flags==trace.game_prelude.flags_after && acquire_read_object(object) &&
                        cwd_acquire_object(trace.cwd_acquire.object_address,trace.cwd_acquire.object_before,object,trace.cwd_acquire.thread_id,false) &&
                        same_named_event(child.process_,trace.instance_startup.event_handle,instance->name);
                    if(!state.parent_preserved)return finish("cd_stream_allocation_parent_drift");
                    if(stage==7) {
                        state.api_returned=true;state.allocation_succeeded=context.Eax!=0;
                        if(!state.allocation_succeeded)return finish("cd_stream_allocation_failed");
                        allocation_heap_return=dispatch_registers(context);
                        state.block_valid=cd_stream_allocation_layout(context.Eax,a,state.block);
                        if(!state.block_valid || (state.block.raw<tib[1] && tib[2]<state.block.raw+state.block.bytes) ||
                            (state.block.raw<tib[6]+56 && tib[6]<state.block.raw+state.block.bytes))return finish("cd_stream_allocation_block_rejected");
                        state.block_valid=allocation_hash(child.process_,state.block,state.block_before_hash) &&
                            read_private(state.block.metadata,&state.metadata_before,4);
                        if(!state.block_valid)return finish("cd_stream_allocation_block_unreadable");
                        // HeapAlloc does not define GetLastError on failure; retained only as a diagnostic on success.
                        if(!read_last_error(state.last_error))return finish("cd_stream_allocation_error_read");
                    }
                    if(stage>=8) {
                        state.content_preserved=allocation_hash(child.process_,state.block,state.block_after_hash) && state.block_before_hash==state.block_after_hash;
                        state.metadata_valid=read_private(state.block.metadata,&state.metadata_after,4) &&
                            state.metadata_after==(stage==10?state.block.raw:state.metadata_before);
                        if(!state.content_preserved || !state.metadata_valid)return finish("cd_stream_allocation_content_drift");
                    }
                    if(stage==10) {
                        state.verified=true;if(!channels)return finish("cd_stream_allocation_verified");
                        auto& next=trace.cd_stream_channels;next.shape_valid=channels_shape();
                        if(!next.shape_valid)return finish("cd_stream_channels_precondition_shape");
                        channels_saved=dispatch_registers(context);std::uint32_t count{},pointer{};
                        if(!read_private(context.Esp+12,channels_caller_stack.data(),32) || !read_private(context.Esp+40,&count,4) || count!=5 ||
                            !read_image(child.base_,channels->pointer_rva-4,next.pointer_before,false) || !read_word(child.base_,channels->pointer_rva,pointer) || pointer ||
                            !read_last_error(next.error_before))return finish("cd_stream_channels_precondition_state");
                        if(!arm_channels(1,context))return finish("cd_stream_channels_arm_failed");
                        continue;
                    }
                    const auto next=stage==3 && state.globals[3]==1?5U:stage+1;
                    if(!arm_allocation(next,context))return finish("cd_stream_allocation_arm_failed");
                    continue;
                }
                if(trace.cd_stream_disk.armed) {
                    auto& state=trace.cd_stream_disk;const auto stage=state.stage;
                    state.shape_valid=disk_shape();if(!state.shape_valid)return finish("cd_stream_disk_shape_drift");
                    state.frame_valid=cd_stream_disk_frame(stage,tables_saved,dispatch_registers(context),
                        static_cast<DWORD>(child.base_+tables->handles_rva),state.geometry,&disk_return_saved);
                    if(!state.frame_valid)return finish("cd_stream_disk_frame");
                    std::array<std::uint32_t,7> locals{};
                    state.arguments_valid=read_private(tables_saved.esp-28,locals.data(),28) && locals[0]==tables_saved.edi &&
                        locals[5]==child.base_+manager->manager.call_rva+12 && locals[6]==5;
                    if(stage<=2) {
                        std::array<std::uint32_t,6> args{};
                        state.arguments_valid=state.arguments_valid && read_private(context.Esp,args.data(),24) &&
                            args==std::array<std::uint32_t,6>{static_cast<DWORD>(child.base_+tables->target_rva+74),0,
                                tables_saved.esp-12,tables_saved.esp-24,tables_saved.esp-16,tables_saved.esp-20};
                    }
                    if(stage==4) {
                        std::array<std::uint32_t,4> args{};
                        state.arguments_valid=state.arguments_valid && read_private(context.Esp,args.data(),16) &&
                            args==std::array<std::uint32_t,4>{2048,state.geometry.bytes_per_sector,0,tables_saved.esi} &&
                            locals[1]==state.geometry.bytes_per_sector && locals[2]==state.geometry.total_clusters &&
                            locals[3]==state.geometry.free_clusters && locals[4]==state.geometry.sectors_per_cluster;
                    }
                    if(!state.arguments_valid)return finish("cd_stream_disk_arguments");
                    state.globals_valid=disk_read_globals(state.globals_after) &&
                        cd_stream_disk_memory(state.globals_before,state.globals_after,cd_stream_disk_flags(state.geometry.bytes_per_sector),stage==4);
                    if(!state.globals_valid)return finish("cd_stream_disk_globals_drift");
                    CdStreamTableWindow current_tables{};
                    state.tables_preserved=tables_read(current_tables) && current_tables==trace.cd_stream_tables.after;
                    std::array<std::byte,32> parent{};std::array<std::byte,156> caller{};std::uint32_t outer{};
                    state.caller_preserved=read_private(tables_saved.esp,&outer,4) && outer==child.base_+routing->initializer.call_rva+5 &&
                        read_private(prelude_saved.esp,parent.data(),parent.size()) && parent==prelude_parent_stack &&
                        read_private(dispatch_saved.esp,caller.data(),caller.size()) && std::equal(caller.begin(),caller.end(),suppression_stack.begin()+16);
                    std::array<std::byte,136> directory{};state.manager_preserved=manager_read_buffer(directory) && directory==trace.file_manager_ready.directory_after;
                    std::array<std::uint32_t,7> tib{};state.seh_preserved=seh_read_tib(tib) && tib==trace.cwd_seh.tib_before;
                    std::array<std::uint32_t,6> object{};state.lock_preserved=acquire_read_object(object) &&
                        cwd_acquire_object(trace.cwd_acquire.object_address,trace.cwd_acquire.object_before,object,trace.cwd_acquire.thread_id,false);
                    std::array<std::byte,16> flags{};state.localisation_preserved=prelude_read_flags(flags) && flags==trace.game_prelude.flags_after;
                    std::uint32_t error{};state.last_error_read=read_last_error(error);
                    if(!state.tables_preserved || !state.caller_preserved || !state.manager_preserved || !state.seh_preserved ||
                        !state.lock_preserved || !state.localisation_preserved || !state.last_error_read ||
                        (stage<=2 && error!=trace.cwd_query.last_error_after) || (stage==4 && error!=state.last_error) ||
                        !same_named_event(child.process_,trace.instance_startup.event_handle,instance->name))return finish("cd_stream_disk_parent_drift");
                    if(stage==3) {
                        disk_return_saved=dispatch_registers(context);
                        state.api_returned=true;state.api_result=context.Eax;state.last_error=error;
                        state.query_succeeded=context.Eax!=0;
                        // Failed output parameters are unspecified: never decode or consume them.
                        const auto verdict=cd_stream_disk_query_result(context.Eax,{locals[1],locals[2],locals[3],locals[4]},state.geometry);
                        if(verdict==CdStreamDiskQueryResult::failed)return finish("cd_stream_disk_query_failed");
                        state.geometry_valid=verdict==CdStreamDiskQueryResult::accepted;
                        if(!state.geometry_valid)return finish("cd_stream_disk_geometry_rejected");
                    }
                    if(stage==4){
                        state.verified=true;if(!allocation)return finish("cd_stream_disk_verified");
                        auto& next=trace.cd_stream_allocation;next.shape_valid=allocation_shape();
                        if(!next.shape_valid)return finish("cd_stream_allocation_precondition_shape");
                        next.request_valid=allocation_globals(next.globals) && cd_stream_allocation_request(state.geometry.bytes_per_sector,next.globals);
                        if(!next.request_valid)return finish("cd_stream_allocation_request_rejected");
                        allocation_saved=dispatch_registers(context);
                        if(!read_private(context.Esp,allocation_caller_stack.data(),44))return finish("cd_stream_allocation_precondition_stack");
                        if(!arm_allocation(1,context))return finish("cd_stream_allocation_arm_failed");
                        continue;
                    }
                    if(!arm_disk(stage+1,context))return finish("cd_stream_disk_arm_failed");
                    continue;
                }
                if(trace.cd_stream_tables.armed) {
                    auto& state=trace.cd_stream_tables;const auto stage=state.stage;
                    state.shape_valid=tables_shape();if(!state.shape_valid)return finish("cd_stream_tables_shape_drift");
                    state.frame_valid=cd_stream_tables_frame(stage,tables_saved,dispatch_registers(context),static_cast<DWORD>(child.base_+tables->handles_rva));
                    if(!state.frame_valid)return finish("cd_stream_tables_frame");
                    state.memory_read=tables_read(state.after);
                    state.tables_valid=state.memory_read && cd_stream_tables_memory(state.before,state.after,stage==2);
                    if(!state.tables_valid)return finish("cd_stream_tables_memory_drift");
                    std::array<std::uint32_t,2> call{};
                    state.arguments_valid=read_private(tables_saved.esp-8,call.data(),8) &&
                        call[0]==child.base_+manager->manager.call_rva+12 && call[1]==5;
                    if(stage==2) {
                        std::array<std::uint32_t,6> args{};
                        state.arguments_valid=state.arguments_valid && read_private(context.Esp,args.data(),24) &&
                            args==std::array<std::uint32_t,6>{0,tables_saved.esp-12,tables_saved.esp-24,tables_saved.esp-16,tables_saved.esp-20,tables_saved.edi};
                    }
                    if(!state.arguments_valid)return finish("cd_stream_tables_arguments");
                    std::array<std::byte,32> parent{};std::array<std::byte,156> caller{};
                    std::uint32_t outer_return{};
                    state.caller_preserved=read_private(tables_saved.esp,&outer_return,4) && outer_return==child.base_+routing->initializer.call_rva+5 &&
                        read_private(prelude_saved.esp,parent.data(),parent.size()) && parent==prelude_parent_stack &&
                        read_private(dispatch_saved.esp,caller.data(),caller.size()) && std::equal(caller.begin(),caller.end(),suppression_stack.begin()+16);
                    if(!state.caller_preserved)return finish("cd_stream_tables_caller_drift");
                    std::array<std::byte,136> directory{};state.manager_preserved=manager_read_buffer(directory) && directory==trace.file_manager_ready.directory_after;
                    std::array<std::uint32_t,7> tib{};state.seh_preserved=seh_read_tib(tib) && tib==trace.cwd_seh.tib_before;
                    std::array<std::uint32_t,6> object{};state.lock_preserved=acquire_read_object(object) &&
                        cwd_acquire_object(trace.cwd_acquire.object_address,trace.cwd_acquire.object_before,object,trace.cwd_acquire.thread_id,false);
                    std::array<std::byte,16> flags{};state.localisation_preserved=prelude_read_flags(flags) && flags==trace.game_prelude.flags_after;
                    std::uint32_t error{};state.last_error_preserved=read_last_error(error) && error==trace.cwd_query.last_error_after;
                    if(!state.manager_preserved || !state.seh_preserved || !state.lock_preserved || !state.localisation_preserved || !state.last_error_preserved ||
                        !same_named_event(child.process_,trace.instance_startup.event_handle,instance->name))return finish("cd_stream_tables_parent_drift");
                    if(stage==2){
                        state.verified=true;
                        if(!disk)return finish("cd_stream_tables_verified");
                        auto& next=trace.cd_stream_disk;next.shape_valid=disk_shape();
                        if(!next.shape_valid)return finish("cd_stream_disk_precondition_shape");
                        if(!disk_read_globals(next.globals_before))return finish("cd_stream_disk_precondition_memory");
                        if(!arm_disk(1,context))return finish("cd_stream_disk_arm_failed");
                        continue;
                    }
                    if(!arm_tables(2,context))return finish("cd_stream_tables_arm_failed");
                    continue;
                }
                if(trace.file_manager_ready.armed) {
                    auto& state=trace.file_manager_ready;const auto stage=state.stage;
                    const std::uint32_t base=static_cast<std::uint32_t>(child.base_),t=query_saved.esp,root=base+manager->buffer_rva;
                    const auto length=trace.cwd_query.returned_length;
                    state.shape_valid=ready_shape();if(!state.shape_valid)return finish("file_manager_ready_shape_drift");
                    state.frame_valid=file_manager_ready_frame(stage,query_saved,manager_saved,dispatch_registers(context),root,length);
                    if(!state.frame_valid)return finish("file_manager_ready_frame");
                    std::array<std::byte,16> slot{};std::array<std::uint32_t,6> object{};
                    if(!lock_read_slot(slot) || slot!=trace.cwd_lock.slot_after || !acquire_read_object(object) ||
                        !cwd_acquire_object(trace.cwd_acquire.object_address,trace.cwd_acquire.object_before,object,trace.cwd_acquire.thread_id,stage<=2))
                        return finish("file_manager_ready_lock_drift");
                    if(stage>=3)state.lock_released=true;
                    std::array<std::uint32_t,7> tib{};std::array<std::uint32_t,2> prior{};
                    if(!seh_read_tib(tib) || tib!=(stage<5?trace.cwd_seh.tib_after:trace.cwd_seh.tib_before))return finish("file_manager_ready_seh_drift");
                    state.prior_record_preserved=trace.cwd_seh.tib_before[0]==UINT32_MAX ||
                        (read_private(trace.cwd_seh.tib_before[0],prior.data(),8) && prior==seh_prior_record);
                    if(!state.prior_record_preserved)return finish("file_manager_ready_prior_record_drift");
                    if(stage<5) {
                        std::array<std::uint32_t,4> record{};std::uint32_t result{};
                        if(!read_private(t+28,record.data(),16) || record[0]!=trace.cwd_seh.tib_before[0] ||
                            record[1]!=base+seh->handler_rva || record[2]!=base+seh->scope_rva || record[3]!=UINT32_MAX ||
                            !read_private(t+16,&result,4) || result!=root)return finish("file_manager_ready_wrapper_state");
                    } else state.seh_removed=true;
                    std::array<std::uint32_t,6> stack{};
                    state.arguments_valid=false;
                    if(stage<=2) {
                        const auto offset=stage==1?0U:1U;
                        state.arguments_valid=read_private(context.Esp,stack.data(),(5+offset)*4) &&
                            (!offset || stack[0]==base+ready->unlock_rva+19) && stack[offset]==trace.cwd_acquire.object_address &&
                            stack[offset+1]==query_saved.ebp && stack[offset+2]==base+seh->cleanup_rva+7 &&
                            stack[offset+3]==7 && stack[offset+4]==base+lock->selector.call_rva+38;
                    } else if(stage==3) {
                        state.arguments_valid=read_private(context.Esp,stack.data(),16) && stack[0]==query_saved.ebp &&
                            stack[1]==base+seh->cleanup_rva+7 && stack[2]==7 && stack[3]==base+lock->selector.call_rva+38;
                    } else if(stage==4 || stage==5) {
                        state.arguments_valid=read_private(context.Esp,stack.data(),4) &&
                            stack[0]==(stage==4?base+lock->selector.call_rva+46:base+manager->manager.target_rva+16);
                    } else if(stage==6) {
                        state.arguments_valid=read_private(context.Esp,stack.data(),16) && stack[0]==root && stack[1]==128 &&
                            stack[2]==manager_saved.edi && stack[3]==state.return_address;
                    } else if(stage<=8) {
                        state.arguments_valid=read_private(context.Esp,stack.data(),8) && stack[0]==manager_saved.edi && stack[1]==state.return_address;
                    } else state.arguments_valid=true; // Full caller envelope below includes the next return.
                    if(!state.arguments_valid)return finish("file_manager_ready_arguments");
                    std::array<std::byte,32> parent_stack{};std::array<std::byte,156> caller{};
                    state.caller_preserved=read_private(prelude_saved.esp,parent_stack.data(),parent_stack.size()) && parent_stack==prelude_parent_stack &&
                        read_private(dispatch_saved.esp,caller.data(),caller.size()) && std::equal(caller.begin(),caller.end(),suppression_stack.begin()+16);
                    if(!state.caller_preserved)return finish("file_manager_ready_caller_drift");
                    state.buffer_valid=manager_read_buffer(state.directory_after) &&
                        file_manager_ready_buffer(trace.cwd_copy.destination_after,state.directory_after,length,stage>=8);
                    if(!state.buffer_valid)return finish("file_manager_ready_buffer_drift");
                    if(stage>=8)state.suffix_written=true;
                    std::uint32_t error{};state.last_error_preserved=read_last_error(error) && error==trace.cwd_query.last_error_after;
                    if(!state.last_error_preserved)return finish("file_manager_ready_last_error_drift");
                    std::array<std::byte,16> flags{};state.localisation_preserved=prelude_read_flags(flags) && flags==trace.game_prelude.flags_after;
                    if(!state.localisation_preserved)return finish("file_manager_ready_localisation_drift");
                    if(!same_named_event(child.process_,trace.instance_startup.event_handle,instance->name))return finish("file_manager_ready_instance_identity");
                    if(stage>=6)state.wrapper_returned=true;
                    if(stage==9){
                        state.manager_returned=true;state.verified=true;
                        if(!tables)return finish("file_manager_ready_verified");
                        auto& next_state=trace.cd_stream_tables;
                        next_state.shape_valid=tables_shape();if(!next_state.shape_valid)return finish("cd_stream_tables_precondition_shape");
                        next_state.memory_read=tables_read(next_state.before);if(!next_state.memory_read)return finish("cd_stream_tables_precondition_memory");
                        tables_saved=dispatch_registers(context);
                        if(tables_saved.esp<65600 || (tables_saved.flags&0x500U))return finish("cd_stream_tables_precondition_frame");
                        if(!arm_tables(1,context))return finish("cd_stream_tables_arm_failed");
                        continue;
                    }
                    const std::array<DWORD,8> next{state.function_address,base+ready->unlock_rva+19,base+ready->epilogue_rva,
                        base+lock->selector.call_rva+46,base+manager->manager.target_rva+16,base+manager->manager.target_rva+40,
                        base+manager->manager.target_rva+49,state.return_address};
                    if(!arm_ready(stage+1,next[stage-1],context))return finish("file_manager_ready_arm_failed");
                    continue;
                }
                if(trace.cwd_return.armed) {
                    auto& state=trace.cwd_return;const auto& query_state=trace.cwd_query;
                    state.shape_valid=return_shape();if(!state.shape_valid)return finish("cwd_return_shape_drift");
                    state.frame_valid=cwd_return_frame(state.stage,query_saved,return_copy_saved,dispatch_registers(context),query_cookie,trace.cwd_copy.destination_address);
                    if(!state.frame_valid)return finish("cwd_return_frame");
                    if(state.stage==1)state.arguments_cleaned=true;
                    else if(state.stage==2)state.cookie_call_reached=true;
                    else if(state.stage==3)state.checker_entered=true;
                    else if(state.stage==4)state.cookie_matched=true;
                    else if(state.stage==5)state.checker_returned=true;
                    else if(state.stage==6)state.helper_returned=true;
                    const auto helper_return=static_cast<DWORD>(child.base_+lock->selector.call_rva+23);
                    std::array<std::uint32_t,21> stack{};
                    state.stack_preserved=read_private(acquire_saved.esp,stack.data(),84) &&
                        cwd_query_stack(7,trace.cwd_acquire.stack_after,stack,trace.cwd_copy.destination_address,helper_return);
                    if(!state.stack_preserved)return finish("cwd_return_stack_drift");
                    std::array<std::uint32_t,7> tib{};std::array<std::uint32_t,2> prior{};
                    state.seh_preserved=seh_read_tib(tib) && tib==trace.cwd_seh.tib_after && query_saved.esp-304>=tib[2];
                    state.prior_record_preserved=trace.cwd_seh.tib_before[0]==UINT32_MAX ||
                        (read_private(trace.cwd_seh.tib_before[0],prior.data(),8) && prior==seh_prior_record);
                    if(!state.seh_preserved || !state.prior_record_preserved)return finish("cwd_return_seh_drift");
                    std::array<std::byte,16> slot{};std::array<std::uint32_t,6> object{};
                    state.lock_preserved=lock_read_slot(slot) && slot==trace.cwd_lock.slot_after &&
                        acquire_read_object(object) && object==trace.cwd_acquire.object_after;
                    if(!state.lock_preserved)return finish("cwd_return_lock_drift");
                    std::uint32_t cookie{},encoded{},ebx{},ebp{};
                    state.cookie_preserved=read_word(child.base_,query->cookie_rva,cookie) && cookie==query_cookie &&
                        read_private(query_saved.esp-20,&encoded,4) && encoded==(query_cookie^helper_return) &&
                        read_private(query_saved.esp-288,&ebx,4) && ebx==(state.stage<=2?0U:static_cast<DWORD>(child.base_+query->helper_rva+75)) &&
                        read_private(query_saved.esp-16,&ebp,4) && ebp==query_saved.ebp;
                    if(!state.cookie_preserved)return finish("cwd_return_cookie_drift");
                    std::array<std::byte,32> parent_stack{};std::array<std::byte,156> caller{};
                    state.caller_preserved=read_private(prelude_saved.esp,parent_stack.data(),parent_stack.size()) && parent_stack==prelude_parent_stack &&
                        read_private(dispatch_saved.esp,caller.data(),caller.size()) && std::equal(caller.begin(),caller.end(),suppression_stack.begin()+16);
                    if(!state.caller_preserved)return finish("cwd_return_caller_drift");
                    std::uint32_t error{};state.last_error_preserved=read_last_error(error) && error==query_state.last_error_after;
                    if(!state.last_error_preserved)return finish("cwd_return_last_error_drift");
                    std::array<std::byte,260> source{};std::array<std::uint32_t,2> guard{};
                    state.source_snapshot_preserved=read_private(trace.cwd_copy.source_address,source.data(),source.size()) && source==query_state.directory;
                    if(!state.source_snapshot_preserved)return finish("cwd_return_source_drift");
                    state.guard_preserved=read_private(trace.cwd_copy.source_address+260,guard.data(),8) && guard==query_guard;
                    if(!state.guard_preserved)return finish("cwd_return_guard_drift");
                    std::array<std::byte,136> destination{};
                    state.destination_preserved=manager_read_buffer(destination) && destination==trace.cwd_copy.destination_after;
                    if(!state.destination_preserved)return finish("cwd_return_destination_drift");
                    std::array<std::byte,16> flags{};
                    state.localisation_preserved=prelude_read_flags(flags) && flags==trace.game_prelude.flags_after;
                    if(!state.localisation_preserved)return finish("cwd_return_localisation_drift");
                    if(!same_named_event(child.process_,trace.instance_startup.event_handle,instance->name))return finish("cwd_return_instance_identity");
                    std::array<std::uint32_t,4> arguments{};
                    if(!read_private(query_saved.esp-12,arguments.data(),16) || arguments[0]!=state.return_address || arguments[1]!=0 ||
                        arguments[2]!=trace.cwd_copy.destination_address || arguments[3]!=128)return finish("cwd_return_helper_arguments");
                    if(state.stage==6) {
                        state.source_retired=true;state.verified=true;
                        if(!ready)return finish("cwd_return_verified");
                        auto& next=trace.file_manager_ready;next.shape_valid=ready_shape();
                        if(!next.shape_valid)return finish("file_manager_ready_precondition_shape");
                        next.return_address=static_cast<DWORD>(child.base_+manager->manager.call_rva+5);
                        if(!arm_ready(1,static_cast<DWORD>(child.base_+ready->unlock_rva+13),context))return finish("file_manager_ready_arm_failed");
                        continue;
                    }
                    const auto address=state.stage==1?static_cast<DWORD>(child.base_+query->helper_rva+70):
                        state.stage==2?state.checker_address:state.stage==3?state.checker_address+6:
                        state.stage==4?static_cast<DWORD>(child.base_+query->helper_rva+75):state.return_address;
                    if(!arm_return(state.stage+1,address,context))return finish("cwd_return_arm_failed");
                    continue;
                }
                if(trace.cwd_copy.armed) {
                    auto& state=trace.cwd_copy;const auto& query_state=trace.cwd_query;
                    if(state.stage==1)state.pointer_branch_reached=true;
                    else if(state.stage==2)state.capacity_branch_reached=true;
                    else if(state.stage==3)state.call_reached=true;
                    else if(state.stage==4)state.function_entered=true;
                    else if(state.stage==5)state.function_returned=true;
                    else return finish("cwd_copy_stage");
                    state.shape_valid=copy_shape();if(!state.shape_valid)return finish("cwd_copy_shape_drift");
                    state.frame_valid=cwd_copy_frame(state.stage,query_saved,copy_api_return,dispatch_registers(context),
                        state.source_address,state.destination_address,query_state.returned_length);
                    if(!state.frame_valid)return finish("cwd_copy_frame");
                    std::array<std::uint32_t,3> args{};
                    state.arguments_valid=state.stage<=2;
                    if(state.stage>=3) {
                        const auto words=state.stage==4?3U:2U;
                        state.arguments_valid=read_private(context.Esp,args.data(),words*4) &&
                            args[words-2]==state.destination_address && args[words-1]==state.source_address &&
                            (state.stage!=4 || args[0]==child.base_+query->helper_rva+231);
                    }
                    if(!state.arguments_valid)return finish("cwd_copy_arguments");
                    const auto helper_return=static_cast<DWORD>(child.base_+lock->selector.call_rva+23);
                    std::array<std::uint32_t,21> stack{};
                    state.stack_preserved=read_private(acquire_saved.esp,stack.data(),84) &&
                        cwd_query_stack(7,trace.cwd_acquire.stack_after,stack,state.destination_address,helper_return);
                    if(!state.stack_preserved)return finish("cwd_copy_stack_drift");
                    std::array<std::uint32_t,7> tib{};std::array<std::uint32_t,2> prior{};
                    state.seh_preserved=seh_read_tib(tib) && tib==trace.cwd_seh.tib_after && query_saved.esp-304>=tib[2];
                    state.prior_record_preserved=trace.cwd_seh.tib_before[0]==UINT32_MAX ||
                        (read_private(trace.cwd_seh.tib_before[0],prior.data(),8) && prior==seh_prior_record);
                    if(!state.seh_preserved || !state.prior_record_preserved)return finish("cwd_copy_seh_drift");
                    std::array<std::byte,16> slot{};std::array<std::uint32_t,6> object{};
                    state.lock_preserved=lock_read_slot(slot) && slot==trace.cwd_lock.slot_after &&
                        acquire_read_object(object) && object==trace.cwd_acquire.object_after;
                    if(!state.lock_preserved)return finish("cwd_copy_lock_drift");
                    std::uint32_t cookie{},encoded{},ebx{},ebp{};
                    state.cookie_preserved=read_word(child.base_,query->cookie_rva,cookie) && cookie==query_cookie &&
                        read_private(query_saved.esp-20,&encoded,4) && encoded==(query_cookie^helper_return) &&
                        read_private(query_saved.esp-288,&ebx,4) && !ebx &&
                        read_private(query_saved.esp-16,&ebp,4) && ebp==query_saved.ebp;
                    if(!state.cookie_preserved)return finish("cwd_copy_cookie_drift");
                    std::array<std::byte,32> parent_stack{};std::array<std::byte,156> caller{};
                    state.caller_preserved=read_private(prelude_saved.esp,parent_stack.data(),parent_stack.size()) && parent_stack==prelude_parent_stack &&
                        read_private(dispatch_saved.esp,caller.data(),caller.size()) && std::equal(caller.begin(),caller.end(),suppression_stack.begin()+16);
                    if(!state.caller_preserved)return finish("cwd_copy_caller_drift");
                    std::uint32_t error{};state.last_error_preserved=read_last_error(error) && error==query_state.last_error_after;
                    if(!state.last_error_preserved)return finish("cwd_copy_last_error_drift");
                    std::array<std::byte,260> source{};std::array<std::uint32_t,2> guard{};
                    state.source_preserved=read_private(state.source_address,source.data(),source.size()) && source==query_state.directory;
                    if(!state.source_preserved)return finish("cwd_copy_source_drift");
                    state.guard_preserved=read_private(state.source_address+260,guard.data(),8) && guard==query_guard;
                    if(!state.guard_preserved)return finish("cwd_copy_guard_drift");
                    state.destination_valid=manager_read_buffer(state.destination_after) &&
                        cwd_copy_buffer(state.destination_before,state.destination_after,source,query_state.returned_length,state.stage==5);
                    if(!state.destination_valid)return finish("cwd_copy_destination_drift");
                    std::array<std::byte,16> flags{};
                    state.localisation_preserved=prelude_read_flags(flags) && flags==trace.game_prelude.flags_after;
                    if(!state.localisation_preserved)return finish("cwd_copy_localisation_drift");
                    if(!same_named_event(child.process_,trace.instance_startup.event_handle,instance->name))return finish("cwd_copy_instance_identity");
                    if(state.stage==5) {
                        state.copied_bytes=query_state.returned_length+1;state.copied=true;state.verified=true;
                        if(completion) {
                            auto& next=trace.cwd_return;next.shape_valid=return_shape();
                            if(!next.shape_valid)return finish("cwd_return_precondition_shape");
                            return_copy_saved=dispatch_registers(context);
                            next.checker_address=static_cast<DWORD>(child.base_+completion->checker_rva);
                            next.return_address=static_cast<DWORD>(child.base_+lock->selector.call_rva+23);
                            if(!arm_return(1,static_cast<DWORD>(child.base_+query->helper_rva+63),context))return finish("cwd_return_arm_failed");
                            continue;
                        }
                        return finish("cwd_copy_verified");
                    }
                    const auto address=state.stage==1?static_cast<DWORD>(child.base_+query->helper_rva+200):
                        state.stage==2?static_cast<DWORD>(child.base_+query->helper_rva+226):
                        state.stage==3?state.function_address:static_cast<DWORD>(child.base_+query->helper_rva+231);
                    if(!arm_copy(state.stage+1,address,context))return finish("cwd_copy_arm_failed");
                    continue;
                }
                if(trace.cwd_query.armed) {
                    auto& state=trace.cwd_query;
                    if(state.stage==1)state.wrapper_call_reached=true;
                    else if(state.stage==2)state.helper_entered=true;
                    else if(state.stage==3)state.branch_reached=true;
                    else if(state.stage==4)state.query_path_reached=true;
                    else if(state.stage==5)state.call_reached=true;
                    else if(state.stage==6)state.function_entered=true;
                    else if(state.stage==7)state.function_returned=true;
                    else return finish("cwd_query_stage");
                    state.shape_valid=query_shape();if(!state.shape_valid)return finish("cwd_query_shape_drift");
                    const auto helper_return=static_cast<DWORD>(child.base_+lock->selector.call_rva+23);
                    state.frame_valid=cwd_query_frame(state.stage,query_saved,dispatch_registers(context),query_cookie,helper_return);
                    if(!state.frame_valid)return finish("cwd_query_frame");
                    std::array<std::uint32_t,21> stack{};
                    state.stack_valid=read_private(acquire_saved.esp,stack.data(),84) &&
                        cwd_query_stack(state.stage,trace.cwd_acquire.stack_after,stack,static_cast<DWORD>(child.base_+manager->buffer_rva),helper_return);
                    if(!state.stack_valid)return finish("cwd_query_stack_drift");
                    std::array<std::uint32_t,7> tib{};std::array<std::uint32_t,2> prior{};
                    state.seh_preserved=seh_read_tib(tib) && tib==trace.cwd_seh.tib_after;
                    state.prior_record_preserved=trace.cwd_seh.tib_before[0]==UINT32_MAX ||
                        (read_private(trace.cwd_seh.tib_before[0],prior.data(),8) && prior==seh_prior_record);
                    if(!state.seh_preserved || !state.prior_record_preserved)return finish("cwd_query_seh_drift");
                    std::array<std::byte,16> slot{};std::array<std::uint32_t,6> object{};
                    state.lock_preserved=lock_read_slot(slot) && slot==trace.cwd_lock.slot_after && acquire_read_object(object) && object==trace.cwd_acquire.object_after;
                    if(!state.lock_preserved)return finish("cwd_query_lock_drift");
                    std::uint32_t cookie{};
                    state.cookie_preserved=read_word(child.base_,query->cookie_rva,cookie) && cookie==query_cookie;
                    if(state.stage>=3) {
                        std::uint32_t encoded{},saved_ebx{},saved_ebp{};
                        state.cookie_preserved=state.cookie_preserved && read_private(query_saved.esp-20,&encoded,4) && encoded==(query_cookie^helper_return);
                        if(!read_private(query_saved.esp-288,&saved_ebx,4) || saved_ebx ||
                            !read_private(query_saved.esp-16,&saved_ebp,4) || saved_ebp!=query_saved.ebp)return finish("cwd_query_helper_stack");
                    }
                    if(!state.cookie_preserved)return finish("cwd_query_cookie_drift");
                    std::array<std::byte,32> parent_stack{};std::array<std::byte,156> caller{};std::uint32_t error{};
                    state.caller_preserved=read_private(prelude_saved.esp,parent_stack.data(),parent_stack.size()) && parent_stack==prelude_parent_stack &&
                        read_private(dispatch_saved.esp,caller.data(),caller.size()) && std::equal(caller.begin(),caller.end(),suppression_stack.begin()+16);
                    if(!state.caller_preserved || !read_last_error(error))return finish("cwd_query_caller_drift");
                    state.last_error_after=error;
                    if(state.stage<7 && error!=state.last_error_before)return finish("cwd_query_last_error_drift");
                    std::array<std::byte,136> root{};std::array<std::byte,16> flags{};
                    state.root_preserved=manager_read_buffer(root) && root==trace.file_manager_entry.buffer_after;
                    state.localisation_preserved=prelude_read_flags(flags) && flags==trace.game_prelude.flags_after;
                    if(!state.root_preserved || !state.localisation_preserved)return finish("cwd_query_outer_buffer_drift");
                    if(!same_named_event(child.process_,trace.instance_startup.event_handle,instance->name))return finish("cwd_query_instance_identity");
                    if(state.stage==5 || state.stage==6) {
                        state.buffer_address=query_saved.esp-284;
                        if(state.buffer_address<tib[2] || state.buffer_address>tib[1] || 268>tib[1]-state.buffer_address)return finish("cwd_query_buffer_bounds");
                        std::array<std::uint32_t,3> args{};const auto words=state.stage==5?2U:3U;
                        if(!read_private(context.Esp,args.data(),words*4) || args[words-2]!=260 || args[words-1]!=state.buffer_address ||
                            (state.stage==6 && args[0]!=child.base_+query->helper_rva+141))return finish("cwd_query_arguments");
                        std::array<std::uint32_t,2> guard{};
                        if(!read_private(state.buffer_address+260,guard.data(),8))return finish("cwd_query_guard_read");
                        if(state.stage==5)query_guard=guard;
                        else if(guard!=query_guard)return finish("cwd_query_guard_drift");
                    }
                    if(state.stage==7) {
                        std::array<std::uint32_t,2> guard{};
                        state.guard_preserved=read_private(state.buffer_address+260,guard.data(),8) && guard==query_guard;
                        if(!state.guard_preserved)return finish("cwd_query_guard_drift");
                        state.returned_length=context.Eax;
                        state.buffer_read=read_private(state.buffer_address,state.directory.data(),state.directory.size());
                        if(!state.buffer_read)return finish("cwd_query_buffer_read");
                        const auto reason=cwd_query_result(context.Eax,state.directory,query->expected_directory);
                        if(!reason.empty())return finish(reason);
                        state.path_verified=true;state.verified=true;
                        if(copy) {
                            auto& next=trace.cwd_copy;next.shape_valid=copy_shape();
                            if(!next.shape_valid)return finish("cwd_copy_precondition_shape");
                            copy_api_return=dispatch_registers(context);next.source_address=state.buffer_address;
                            next.destination_address=static_cast<DWORD>(child.base_+manager->buffer_rva);
                            next.function_address=static_cast<DWORD>(child.base_+copy->copier_rva);
                            next.destination_before=trace.file_manager_entry.buffer_after;
                            if(!arm_copy(1,static_cast<DWORD>(child.base_+query->helper_rva+158),context))return finish("cwd_copy_arm_failed");
                            continue;
                        }
                        return finish("cwd_query_verified");
                    }
                    const auto address=state.stage==1?static_cast<DWORD>(child.base_+query->helper_rva):
                        state.stage==2?static_cast<DWORD>(child.base_+query->helper_rva+26):
                        state.stage==3?static_cast<DWORD>(child.base_+query->helper_rva+123):
                        state.stage==4?static_cast<DWORD>(child.base_+query->helper_rva+135):
                        state.stage==5?state.function_address:static_cast<DWORD>(child.base_+query->helper_rva+141);
                    if(!arm_query(state.stage+1,address,context))return finish("cwd_query_arm_failed");
                    continue;
                }
                if(trace.cwd_acquire.armed){
                    auto& state=trace.cwd_acquire;
                    if(state.stage==1)state.branch_reached=true;
                    else if(state.stage==2)state.call_reached=true;
                    else if(state.stage==3)state.function_entered=true;
                    else if(state.stage==4)state.function_returned=true;
                    else if(state.stage==5)state.selector_returned=true;
                    else return finish("cwd_acquire_stage");
                    state.shape_valid=acquire_shape();if(!state.shape_valid)return finish("cwd_acquire_shape_drift");
                    std::array<std::byte,16> slot{};
                    state.slot_preserved=lock_read_slot(slot) && slot==trace.cwd_lock.slot_after;
                    if(!state.slot_preserved)return finish("cwd_acquire_slot_drift");
                    state.frame_valid=cwd_acquire_frame(state.stage,acquire_saved,dispatch_registers(context),lock_saved.ebp);
                    if(!state.frame_valid)return finish("cwd_acquire_frame");
                    if(state.stage==2 || state.stage==3){
                        std::array<std::uint32_t,2> args{};const auto words=state.stage==2?1U:2U;
                        if(!read_private(context.Esp,args.data(),words*4) || args[words-1]!=state.object_address ||
                            (state.stage==3 && args[0]!=child.base_+lock->selector.target_rva+46))return finish("cwd_acquire_arguments");
                    }
                    state.object_read=acquire_read_object(state.object_after);
                    state.object_valid=state.object_read && cwd_acquire_object(state.object_address,state.object_before,state.object_after,state.thread_id,state.stage>=4);
                    if(!state.object_valid)return finish("cwd_acquire_object_drift");
                    state.acquired=state.stage>=4;
                    state.stack_preserved=read_private(acquire_saved.esp,state.stack_after.data(),84) && state.stack_after==state.stack_before;
                    if(!state.stack_preserved)return finish("cwd_acquire_stack_drift");
                    std::array<std::uint32_t,7> tib{};
                    state.seh_preserved=seh_read_tib(tib) && tib==trace.cwd_seh.tib_after;
                    if(!state.seh_preserved)return finish("cwd_acquire_seh_drift");
                    std::array<std::uint32_t,2> prior{};
                    state.prior_record_preserved=trace.cwd_seh.tib_before[0]==UINT32_MAX ||
                        (read_private(trace.cwd_seh.tib_before[0],prior.data(),8) && prior==seh_prior_record);
                    if (!state.prior_record_preserved) return finish("cwd_acquire_prior_record_drift");
                    std::array<std::byte,32> parent_stack{};std::array<std::byte,156> caller{};std::uint32_t error{};
                    state.caller_preserved=read_private(prelude_saved.esp,parent_stack.data(),parent_stack.size()) && parent_stack==prelude_parent_stack &&
                        read_private(dispatch_saved.esp,caller.data(),caller.size()) && std::equal(caller.begin(),caller.end(),suppression_stack.begin()+16);
                    if (!state.caller_preserved || !read_last_error(error) || error!=trace.instance_startup.last_error) return finish("cwd_acquire_caller_drift");
                    std::array<std::byte,136> buffer{};state.buffer_preserved=manager_read_buffer(buffer) && buffer==trace.file_manager_entry.buffer_after;
                    if (!state.buffer_preserved) return finish("cwd_acquire_buffer_drift");
                    std::array<std::byte,16> flags{};state.localisation_preserved=prelude_read_flags(flags) && flags==trace.game_prelude.flags_after;
                    if (!state.localisation_preserved) return finish("cwd_acquire_localisation_drift");
                    if (!same_named_event(child.process_,trace.instance_startup.event_handle,instance->name)) return finish("cwd_acquire_instance_identity");
                    if(state.stage==5) {
                        state.verified=true;
                        if(!query)return finish("cwd_acquire_verified");
                        trace.cwd_query.shape_valid=query_shape();
                        if(!trace.cwd_query.shape_valid)return finish("cwd_query_precondition_shape");
                        query_saved=dispatch_registers(context);
                        if(!read_word(child.base_,query->cookie_rva,query_cookie) || !read_last_error(trace.cwd_query.last_error_before))return finish("cwd_query_precondition_memory");
                        if(!arm_query(1,static_cast<DWORD>(child.base_+lock->selector.call_rva+18),context))return finish("cwd_query_arm_failed");
                        continue;
                    }
                    const auto address=state.stage==1?static_cast<DWORD>(child.base_+lock->selector.target_rva+40):
                        state.stage==2?state.function_address:state.stage==3?static_cast<DWORD>(child.base_+lock->selector.target_rva+46):static_cast<DWORD>(child.base_+lock->selector.call_rva+5);
                    if(!arm_acquire(state.stage+1,address,context))return finish("cwd_acquire_arm_failed");
                    continue;
                }
                if (trace.cwd_lock.armed) {
                    auto& state=trace.cwd_lock;
                    if(state.stage==1)state.entry_reached=true;
                    else if(state.stage==2)state.comparison_reached=true;
                    else return finish("cwd_lock_stage");
                    state.shape_valid=lock_shape();if(!state.shape_valid)return finish("cwd_lock_shape_drift");
                    state.slot_read=lock_read_slot(state.slot_after);
                    state.slot_preserved=state.slot_read && state.slot_after==state.slot_before;
                    if(!state.slot_preserved)return finish("cwd_lock_slot_drift");
                    state.frame_valid=cwd_lock_frame(*lock,state.stage,lock_saved,dispatch_registers(context),state.slot_value);
                    if(!state.frame_valid)return finish("cwd_lock_frame");
                    state.memory_valid=lock_read_stack(state.stack_after) && cwd_lock_memory(*lock,state.stage,lock_saved,state.stack_before,state.stack_after);
                    if(!state.memory_valid)return finish("cwd_lock_stack_drift");
                    std::array<std::uint32_t,7> tib{};
                    state.seh_preserved=seh_read_tib(tib) && tib==trace.cwd_seh.tib_after;
                    if(!state.seh_preserved)return finish("cwd_lock_seh_drift");
                    std::array<std::uint32_t,2> prior{};
                    state.prior_record_preserved=trace.cwd_seh.tib_before[0]==UINT32_MAX ||
                        (read_private(trace.cwd_seh.tib_before[0],prior.data(),8) && prior==seh_prior_record);
                    if (!state.prior_record_preserved) return finish("cwd_lock_prior_record_drift");
                    std::array<std::byte,32> parent_stack{};std::array<std::byte,156> caller{};std::uint32_t error{};
                    state.caller_preserved=read_private(prelude_saved.esp,parent_stack.data(),parent_stack.size()) && parent_stack==prelude_parent_stack &&
                        read_private(dispatch_saved.esp,caller.data(),caller.size()) && std::equal(caller.begin(),caller.end(),suppression_stack.begin()+16);
                    if (!state.caller_preserved || !read_last_error(error) || error!=trace.instance_startup.last_error) return finish("cwd_lock_caller_drift");
                    std::array<std::byte,136> buffer{};state.buffer_preserved=manager_read_buffer(buffer) && buffer==trace.file_manager_entry.buffer_after;
                    if (!state.buffer_preserved) return finish("cwd_lock_buffer_drift");
                    std::array<std::byte,16> flags{};state.localisation_preserved=prelude_read_flags(flags) && flags==trace.game_prelude.flags_after;
                    if (!state.localisation_preserved) return finish("cwd_lock_localisation_drift");
                    if (!same_named_event(child.process_,trace.instance_startup.event_handle,instance->name)) return finish("cwd_lock_instance_identity");
                    if(state.stage==2){
                        state.verified=true;if(!acquire)return finish("cwd_lock_verified");
                        auto& next=trace.cwd_acquire;
                        if(!state.slot_value)return finish("cwd_acquire_missing_slot");
                        next.shape_valid=acquire_shape();if(!next.shape_valid)return finish("cwd_acquire_precondition_shape");
                        acquire_saved=dispatch_registers(context);next.object_address=state.slot_value;next.thread_id=GetThreadId(child.thread_);
                        next.object_read=acquire_read_object(next.object_before);next.object_after=next.object_before;
                        next.object_valid=next.object_read && cwd_acquire_object(next.object_address,next.object_before,next.object_after,next.thread_id,false);
                        if(!next.object_valid)return finish("cwd_acquire_precondition_object");
                        if(!read_private(acquire_saved.esp,next.stack_before.data(),84))return finish("cwd_acquire_precondition_stack");
                        next.stack_after=next.stack_before;
                        if(!arm_acquire(1,static_cast<DWORD>(child.base_+lock->selector.target_rva+38),context))return finish("cwd_acquire_arm_failed");
                        continue;
                    }
                    if(!arm_lock(2,static_cast<DWORD>(child.base_+lock->selector.target_rva+17),context))return finish("cwd_lock_arm_failed");
                    continue;
                }
                if (trace.cwd_seh.armed) {
                    auto& state=trace.cwd_seh;
                    if (state.stage==1) state.wrapper_entry_reached=true;
                    else if (state.stage==2) state.prologue_entry_reached=true;
                    else if (state.stage==3) state.prologue_returned=true;
                    else return finish("cwd_seh_stage");
                    state.shape_valid=seh_shape();if (!state.shape_valid) return finish("cwd_seh_shape_drift");
                    state.frame_valid=cwd_seh_frame(*seh,state.stage,seh_saved,dispatch_registers(context));
                    if (!state.frame_valid) return finish("cwd_seh_frame");
                    state.memory_read=seh_read_tib(state.tib_after) && read_private(seh_saved.esp-60,state.stack_after.data(),80);
                    state.memory_valid=state.memory_read && cwd_seh_memory(*seh,state.stage,seh_saved,state.tib_before,state.tib_after,state.stack_before,state.stack_after);
                    if (!state.memory_valid) return finish("cwd_seh_memory_drift");
                    std::array<std::uint32_t,2> prior{};
                    state.prior_record_preserved=state.tib_before[0]==UINT32_MAX ||
                        (read_private(state.tib_before[0],prior.data(),8) && prior==seh_prior_record);
                    if (!state.prior_record_preserved) return finish("cwd_seh_prior_record_drift");
                    std::array<std::byte,32> parent_stack{};std::array<std::byte,156> caller{};std::uint32_t error{};
                    state.caller_preserved=read_private(prelude_saved.esp,parent_stack.data(),parent_stack.size()) && parent_stack==prelude_parent_stack &&
                        read_private(dispatch_saved.esp,caller.data(),caller.size()) && std::equal(caller.begin(),caller.end(),suppression_stack.begin()+16);
                    if (!state.caller_preserved || !read_last_error(error) || error!=trace.instance_startup.last_error) return finish("cwd_seh_caller_drift");
                    std::array<std::byte,136> buffer{};state.buffer_preserved=manager_read_buffer(buffer) && buffer==trace.file_manager_entry.buffer_after;
                    if (!state.buffer_preserved) return finish("cwd_seh_buffer_drift");
                    std::array<std::byte,16> flags{};state.localisation_preserved=prelude_read_flags(flags) && flags==trace.game_prelude.flags_after;
                    if (!state.localisation_preserved) return finish("cwd_seh_localisation_drift");
                    if (!same_named_event(child.process_,trace.instance_startup.event_handle,instance->name)) return finish("cwd_seh_instance_identity");
                    if (state.stage==3) {
                        state.verified=true;if(!lock)return finish("cwd_seh_verified");
                        auto& next=trace.cwd_lock;next.shape_valid=lock_shape();
                        if(!next.shape_valid)return finish("cwd_lock_precondition_shape");
                        lock_saved=dispatch_registers(context);
                        if(!valid_cwd_lock_origin(lock_saved,state.tib_after))return finish("cwd_lock_precondition_tib");
                        if(!lock_read_stack(next.stack_before))return finish("cwd_lock_precondition_stack");
                        next.slot_read=lock_read_slot(next.slot_before);
                        if(!next.slot_read)return finish("cwd_lock_precondition_slot");
                        next.slot_after=next.slot_before;next.stack_after=next.stack_before;
                        next.slot_address=static_cast<DWORD>(child.base_+lock->table_rva+56);
                        std::memcpy(&next.slot_value,next.slot_before.data()+4,4);
                        if(!arm_lock(1,static_cast<DWORD>(child.base_+lock->selector.target_rva),context))return finish("cwd_lock_arm_failed");
                        continue;
                    }
                    const auto address=state.stage==1?seh->prologue.target_rva:seh->prologue.call_rva+5;
                    if (!arm_seh(state.stage+1,static_cast<DWORD>(child.base_+address),context)) return finish("cwd_seh_arm_failed");
                    continue;
                }
                if (trace.file_manager_entry.armed) {
                    auto& state=trace.file_manager_entry;
                    if (state.stage==1) state.entry_reached=true;
                    else if (state.stage==2) state.cwd_call_reached=true;
                    else return finish("file_manager_stage");
                    state.shape_valid=manager_shape();if (!state.shape_valid) return finish("file_manager_shape_drift");
                    std::array<std::uint32_t,5> stack{};const auto words=state.stage==1 ? 2U : 5U;
                    state.frame_valid=read_private(context.Esp,stack.data(),words*4) &&
                        file_manager_entry_frame(*manager,*prelude,*routing,state.stage,manager_saved,dispatch_registers(context),std::span(stack).first(words));
                    if (!state.frame_valid) return finish("file_manager_frame");
                    std::array<std::byte,32> parent_stack{};std::array<std::byte,156> caller{};std::uint32_t error{};
                    state.stack_preserved=read_private(prelude_saved.esp,parent_stack.data(),parent_stack.size()) && parent_stack==prelude_parent_stack &&
                        read_private(dispatch_saved.esp,caller.data(),caller.size()) && std::equal(caller.begin(),caller.end(),suppression_stack.begin()+16);
                    if (!state.stack_preserved || !read_last_error(error) || error!=trace.instance_startup.last_error) return finish("file_manager_caller_drift");
                    state.buffer_read=manager_read_buffer(state.buffer_after);
                    state.buffer_unchanged=state.buffer_read && state.buffer_before==state.buffer_after;
                    if (!state.buffer_unchanged) return finish("file_manager_buffer_drift");
                    std::array<std::byte,16> flags{};
                    state.localisation_preserved=prelude_read_flags(flags) && flags==trace.game_prelude.flags_after;
                    if (!state.localisation_preserved) return finish("file_manager_localisation_drift");
                    if (!same_named_event(child.process_,trace.instance_startup.event_handle,instance->name)) return finish("file_manager_instance_identity");
                    if (state.stage==2) {
                        state.verified=true;
                        if (!seh) return finish("file_manager_entry_verified");
                        auto& next=trace.cwd_seh;next.shape_valid=seh_shape();
                        if (!next.shape_valid) return finish("cwd_seh_precondition_shape");
                        seh_saved=dispatch_registers(context);
                        next.memory_read=seh_read_tib(next.tib_before);
                        if (!next.memory_read || !valid_cwd_seh_origin(seh_saved,next.tib_before)) return finish("cwd_seh_precondition_tib");
                        if (!read_private(seh_saved.esp-60,next.stack_before.data(),80) ||
                            (next.tib_before[0]!=UINT32_MAX && !read_private(next.tib_before[0],seh_prior_record.data(),8))) return finish("cwd_seh_precondition_stack");
                        next.tib_after=next.tib_before;next.stack_after=next.stack_before;next.record_address=seh_saved.esp-24;
                        if (!arm_seh(1,static_cast<DWORD>(child.base_+seh->wrapper.target_rva),context)) return finish("cwd_seh_arm_failed");
                        continue;
                    }
                    if (!arm_manager(2,static_cast<DWORD>(child.base_+manager->manager.target_rva+11),context)) return finish("file_manager_arm_failed");
                    continue;
                }
                if (trace.game_prelude.armed) {
                    auto& state=trace.game_prelude;
                    if (state.stage==1) state.initializer_entry_reached=true;
                    else if (state.stage==2) state.empty_entry_reached=true;
                    else if (state.stage==3) state.empty_returned=true;
                    else if (state.stage==4) state.localisation_entry_reached=true;
                    else if (state.stage==5) state.localisation_returned=true;
                    else return finish("game_prelude_stage");
                    state.shape_valid=prelude_shape();if (!state.shape_valid) return finish("game_prelude_shape_drift");
                    std::array<std::uint32_t,2> stack{};const auto words=state.stage==2 || state.stage==4 ? 2U : 1U;
                    state.frame_valid=read_private(context.Esp,stack.data(),words*4) &&
                        game_prelude_frame(*prelude,*routing,state.stage,prelude_saved,dispatch_registers(context),std::span(stack).first(words));
                    if (!state.frame_valid) return finish("game_prelude_frame");
                    std::array<std::byte,32> parent_stack{};std::array<std::byte,156> caller{};std::uint32_t error{};
                    state.stack_preserved=read_private(prelude_saved.esp,parent_stack.data(),parent_stack.size()) && parent_stack==prelude_parent_stack &&
                        read_private(dispatch_saved.esp,caller.data(),caller.size()) && std::equal(caller.begin(),caller.end(),suppression_stack.begin()+16);
                    if (!state.stack_preserved || !read_last_error(error) || error!=trace.instance_startup.last_error) return finish("game_prelude_caller_drift");
                    state.flags_read=prelude_read_flags(state.flags_after);
                    state.flags_valid=state.flags_read && game_prelude_flags(state.stage,state.flags_before,state.flags_after);
                    if (!state.flags_valid) return finish("game_prelude_flags_drift");
                    if (!same_named_event(child.process_,trace.instance_startup.event_handle,instance->name)) return finish("game_prelude_instance_identity");
                    if (state.stage==5) {
                        state.verified=true;
                        if (!manager) return finish("game_prelude_verified");
                        auto& next=trace.file_manager_entry;next.shape_valid=manager_shape();
                        if (!next.shape_valid) return finish("file_manager_precondition_shape");
                        manager_saved=dispatch_registers(context);
                        next.buffer_address=static_cast<DWORD>(child.base_+manager->buffer_rva);
                        next.cwd_address=static_cast<DWORD>(child.base_+manager->cwd_rva);
                        next.buffer_read=manager_read_buffer(next.buffer_before);
                        if (!next.buffer_read) return finish("file_manager_precondition_buffer");
                        next.buffer_after=next.buffer_before;
                        if (!arm_manager(1,static_cast<DWORD>(child.base_+manager->manager.target_rva),context)) return finish("file_manager_arm_failed");
                        continue;
                    }
                    const auto next=state.stage==1 ? prelude->empty.target_rva : state.stage==2 ? prelude->localisation.call_rva :
                        state.stage==3 ? prelude->localisation.target_rva : prelude->localisation.call_rva+5;
                    if (!arm_prelude(state.stage+1,static_cast<DWORD>(child.base_+next),context)) return finish("game_prelude_arm_failed");
                    continue;
                }
                if (trace.application_routing.armed) {
                    auto& state=trace.application_routing;
                    if (state.stage==1) state.entry_reached=true;
                    else if (state.stage==2) state.detour_reached=true;
                    else if (state.stage==3) state.indirect_reached=true;
                    else if (state.stage==4) state.initializer_call_reached=true;
                    else return finish("application_routing_stage");
                    state.shape_valid=routing_shape();
                    if (!state.shape_valid) return finish("application_routing_shape_drift");
                    std::array<std::uint32_t,8> stack{};
                    state.frame_valid=read_private(context.Esp,stack.data(),sizeof(stack)) &&
                        application_routing_frame(*routing,*dispatch,state.stage,dispatch_saved,dispatch_registers(context),stack);
                    if (!state.frame_valid) return finish("application_routing_frame");
                    std::array<std::byte,156> caller{};std::uint32_t error{};
                    state.stack_preserved=read_private(dispatch_saved.esp,caller.data(),caller.size()) &&
                        std::equal(caller.begin(),caller.end(),suppression_stack.begin()+16);
                    if (!state.stack_preserved || !read_last_error(error) || error!=trace.instance_startup.last_error)
                        return finish("application_routing_caller_drift");
                    if (!same_named_event(child.process_,trace.instance_startup.event_handle,instance->name)) return finish("application_routing_instance_identity");
                    if (state.stage>=3) {
                        state.selected_index=context.Eax;
                        state.selected_target=static_cast<DWORD>(child.base_+routing->target_rvas[context.Eax]);
                    }
                    if (state.stage==4) {
                        state.verified=true;
                        if (!prelude) return finish("application_routing_boundary_verified");
                        auto& next=trace.game_prelude;next.shape_valid=prelude_shape();
                        if (!next.shape_valid) return finish("game_prelude_precondition_shape");
                        prelude_saved=dispatch_registers(context);
                        if (!read_private(context.Esp,prelude_parent_stack.data(),prelude_parent_stack.size())) return finish("game_prelude_precondition_stack");
                        next.flags_address=static_cast<DWORD>(child.base_+prelude->flags_rva);
                        next.flags_read=prelude_read_flags(next.flags_before);
                        if (!next.flags_read) return finish("game_prelude_precondition_flags");
                        next.flags_after=next.flags_before;
                        if (!arm_prelude(1,static_cast<DWORD>(child.base_+routing->initializer.target_rva),context)) return finish("game_prelude_arm_failed");
                        continue;
                    }
                    const auto next=state.stage==1 ? routing->detour_rva : state.stage==2 ? dispatch->application.target_rva+20 : routing->initializer.call_rva;
                    if (!arm_routing(state.stage+1,static_cast<DWORD>(child.base_+next),context)) return finish("application_routing_arm_failed");
                    continue;
                }
                if (trace.event_dispatch.armed) {
                    auto& state=trace.event_dispatch;
                    if (state.stage==1) state.call_reached=true;
                    else if (state.stage==2) state.entry_reached=true;
                    else if (state.stage==3) state.application_call_reached=true;
                    else return finish("event_dispatch_stage");
                    if (!dispatch_shape()) return finish("event_dispatch_shape_drift");
                    std::array<std::uint32_t,7> stack{};
                    const std::size_t words=state.stage==1 ? 2 : state.stage==2 ? 3 : 7;
                    state.frame_valid=read_private(context.Esp,stack.data(),words*4) &&
                        event_dispatch_frame(*dispatch,state.stage,dispatch_saved,dispatch_registers(context),std::span(stack).first(words));
                    if (!state.frame_valid) return finish("event_dispatch_frame");
                    std::array<std::byte,156> caller{}; std::uint32_t error{};
                    state.stack_preserved=read_private(dispatch_saved.esp,caller.data(),caller.size()) &&
                        std::equal(caller.begin(),caller.end(),suppression_stack.begin()+16);
                    if (!state.stack_preserved || !read_last_error(error) || error!=trace.instance_startup.last_error)
                        return finish("event_dispatch_caller_drift");
                    if (!same_named_event(child.process_,trace.instance_startup.event_handle,instance->name)) return finish("event_dispatch_instance_identity");
                    if (state.stage==3) {
                        state.verified=true;
                        if (!routing) return finish("event_dispatch_boundary_verified");
                        trace.application_routing.shape_valid=routing_shape();
                        if (!trace.application_routing.shape_valid) return finish("application_routing_precondition_shape");
                        if (!arm_routing(1,static_cast<DWORD>(child.base_+dispatch->application.target_rva),context)) return finish("application_routing_arm_failed");
                        continue;
                    }
                    const auto next=state.stage==1 ? dispatch->dispatcher.target_rva : dispatch->application.call_rva;
                    if (!arm_dispatch(state.stage+1,static_cast<DWORD>(child.base_+next),context)) return finish("event_dispatch_arm_failed");
                    continue;
                }
                if (trace.instance_startup.armed) {
                    auto& state=trace.instance_startup;
                    if (!instance_shape()) return finish("instance_shape_drift");
                    if (!same_nonvolatile(context,instance_saved)) return finish("instance_registers");
                    const auto body=static_cast<DWORD>(child.base_+instance->caller.target_rva);
                    if (state.stage==1) {
                        state.create_call_reached=true; state.create_stack=context.Esp;
                        std::array<std::uint32_t,5> args{};
                        state.arguments_valid=state.caller_stack>=20 && context.Esp==state.caller_stack-20 &&
                            read_private(context.Esp,args.data(),sizeof(args)) &&
                            args==std::array<std::uint32_t,5>{0,0,1,static_cast<DWORD>(child.base_+instance->name_rva),
                                static_cast<DWORD>(child.base_+instance->caller.call_rva+5)};
                        if (!state.arguments_valid) return finish("instance_arguments");
                        if (!arm_instance(2,body+18,context)) return finish("instance_arm_failed");
                    } else if (state.stage==2) {
                        state.create_returned=true; state.event_handle=context.Eax;
                        if (context.Esp!=state.caller_stack-4 || !read_last_error(state.last_error)) return finish("instance_create_abi");
                        if (!state.event_handle) return finish("instance_create_failed");
                        state.event_identity_valid=same_named_event(child.process_,state.event_handle,instance->name);
                        if (!state.event_identity_valid) return finish("instance_event_identity");
                        if (!arm_instance(3,body+24,context)) return finish("instance_arm_failed");
                    } else if (state.stage==3) {
                        state.getter_returned=true;
                        if (context.Esp!=state.caller_stack-4 || context.Eax!=state.last_error) return finish("instance_getter_abi");
                        if (state.last_error==ERROR_ALREADY_EXISTS) {
                            state.existing_detected=true; return finish("instance_existing_detected");
                        }
                        if (state.last_error!=ERROR_SUCCESS) return finish("instance_ambiguous_last_error");
                        if (!arm_instance(4,static_cast<DWORD>(child.base_+instance->caller.call_rva+5),context)) return finish("instance_arm_failed");
                    } else if (state.stage==4) {
                        state.caller_returned=true;
                        if (context.Esp!=state.caller_stack || context.Eax) return finish("instance_return_abi");
                        std::array<std::byte,156> stack{};
                        state.stack_preserved=read_private(context.Esp,stack.data(),stack.size()) &&
                            std::equal(stack.begin(),stack.end(),suppression_stack.begin()+16);
                        if (!state.stack_preserved) return finish("instance_stack_drift");
                        if (!same_named_event(child.process_,state.event_handle,instance->name)) return finish("instance_event_identity");
                        state.verified=true;
                        if (!dispatch) return finish("instance_startup_verified");
                        if (!dispatch_shape()) return finish("event_dispatch_precondition_shape");
                        dispatch_saved=dispatch_registers(context); trace.event_dispatch.caller_stack=context.Esp;
                        if (!arm_dispatch(1,static_cast<DWORD>(child.base_+dispatch->dispatcher.call_rva),context)) return finish("event_dispatch_arm_failed");
                        continue;
                    } else return finish("instance_stage");
                    continue;
                }
                if (trace.platform_suppression.transaction.applied) {
                    auto& s=trace.platform_suppression; s.return_reached=true;
                    SuppressionRegisters actual{}; auto expected=s.transaction.replacement;
                    expected.debug[4]=1; // execution fault, before the first return-site instruction
                    if (!suppression_port.read(actual)) return finish("suppression_return_read_failed");
                    actual.flags&=~0x10000U; // CPU-generated RF is not an application flag change
                    if (actual!=expected || !platform_shape()) return finish("suppression_return_drift");
                    std::array<std::byte,172> stack{};
                    s.stack_preserved=read_private(s.transaction.original.esp,stack.data(),stack.size()) && stack==suppression_stack;
                    s.last_error_preserved=read_last_error(s.last_error_after) && s.last_error_after==s.last_error_before;
                    if (!s.stack_preserved || !s.last_error_preserved) return finish("suppression_state_drift");
                    if (instance) {
                        if (!instance_shape()) return finish("instance_precondition_shape");
                        instance_saved=context;
                        trace.instance_startup.suppression_boundary_validated=true;
                        trace.instance_startup.caller_stack=context.Esp;
                        if (!arm_instance(1,static_cast<DWORD>(child.base_+instance->caller.target_rva+12),context)) return finish("instance_arm_failed");
                        continue; // Natural API/stack effects prohibit restoring an obsolete CALL frame later.
                    }
                    if (!s.transaction.restore(suppression_port)) return finish("suppression_restore_failed");
                    s.verified=true; return finish("platform_suppression_verified");
                }
                if (trace.platform_startup.armed) {
                    auto& state=trace.platform_startup; state.call_reached=true;
                    if (!platform_shape()) return finish("platform_shape_drift");
                    std::uint32_t saved_ebx{}; std::array<std::uint32_t,5> original{}; std::byte local_flag{};
                    state.stack_valid=state.entry_stack>=platform->stack_bytes && context.Esp==state.entry_stack-platform->stack_bytes &&
                        read_private(context.Esp+16,&saved_ebx,4) && saved_ebx==platform_saved.Ebx &&
                        read_private(context.Esp+23,&local_flag,1) && local_flag==std::byte{1} &&
                        read_private(state.entry_stack,original.data(),sizeof(original)) &&
                        original[0]==child.base_+crt->application.call_rva+5 &&
                        std::equal(trace.application_entry.arguments.begin(),trace.application_entry.arguments.end(),original.begin()+1);
                    state.registers_valid=context.Ebx==0 && context.Esi==platform_saved.Esi && context.Edi==platform_saved.Edi &&
                        context.Ebp==platform_saved.Ebp && !(context.EFlags&0x500U);
                    if (!state.stack_valid || !state.registers_valid) return finish("platform_stack_or_registers");
                    if (!read_private(context.Esp,state.arguments.data(),sizeof(state.arguments))) return finish("platform_argument_read");
                    state.arguments_valid=state.arguments==platform_startup_arguments;
                    if (!state.arguments_valid) return finish("platform_arguments");
                    state.verified=true;
                    if (!suppression) return finish("platform_startup_boundary_verified");
                    auto& s=trace.platform_suppression;
                    SuppressionRegisters call_context{};
                    if (!suppression_port.read(call_context) || call_context.eip!=state.stop_address ||
                        !read_private(call_context.esp,suppression_stack.data(),suppression_stack.size())) return finish("suppression_snapshot_failed");
                    s.last_error_read=read_last_error(s.last_error_before);
                    if (!s.last_error_read) return finish("suppression_last_error_identity");
                    s.return_address=state.stop_address+6;
                    if (!s.transaction.apply(suppression_port,call_context,state.stop_address)) return finish("suppression_context_failed");
                    continue;
                }
                if (trace.application_entry.armed) {
                    auto& state=trace.application_entry;
                    if (!application_shape()) return finish("application_shape_drift");
                    if (state.stage==1) {
                        state.initializer_returned=true; state.initializer_result=context.Eax;
                        state.initializer_abi_valid=context.Esp==state.initializer_stack && same_nonvolatile(context,application_saved);
                        if (!state.initializer_abi_valid) return finish("application_initializer_abi");
                        if (context.Eax) return finish("application_initializer_failed");
                        if (!arm_application(2,static_cast<DWORD>(child.base_+application->second_call_rva),context)) return finish("application_arm_failed");
                    } else if (state.stage==2) {
                        state.second_call_reached=true; state.second_stack=context.Esp;
                        MEMORY_BASIC_INFORMATION stack{},output{}; std::array<std::byte,68> info{};
                        if (context.Esp%4 || !read_private(context.Esp,&state.second_argument,4) || !state.second_argument || state.second_argument%4 ||
                            VirtualQueryEx(child.process_,reinterpret_cast<void*>(context.Esp),&stack,sizeof(stack))!=sizeof(stack) ||
                            VirtualQueryEx(child.process_,reinterpret_cast<void*>(state.second_argument),&output,sizeof(output))!=sizeof(output) ||
                            output.AllocationBase!=stack.AllocationBase || output.Protect!=PAGE_READWRITE ||
                            !read_private(state.second_argument,info.data(),info.size())) return finish("application_second_argument");
                        application_saved=context;
                        if (!arm_application(3,static_cast<DWORD>(child.base_+application->second_call_rva+6),context)) return finish("application_arm_failed");
                    } else if (state.stage==3) {
                        state.second_returned=true;
                        state.second_abi_valid=state.second_stack<=UINT32_MAX-4 && context.Esp==state.second_stack+4 && same_nonvolatile(context,application_saved);
                        if (!state.second_abi_valid) return finish("application_second_abi");
                        STARTUPINFOA info{};
                        if (!read_private(state.second_argument,&info,sizeof(info)) || info.cb!=68) return finish("application_second_info");
                        state.show_command=(info.dwFlags & STARTF_USESHOWWINDOW) ? info.wShowWindow : SW_SHOWDEFAULT;
                        if (!arm_application(4,static_cast<DWORD>(child.base_+crt->application.call_rva),context)) return finish("application_arm_failed");
                    } else if (state.stage==4) {
                        state.call_stack=context.Esp;
                        if (state.initializer_stack<16 || context.Esp!=state.initializer_stack-16 || context.Esp%4 ||
                            !read_private(context.Esp,state.arguments.data(),sizeof(state.arguments)) ||
                            state.arguments[0]!=child.base_ || state.arguments[1] || state.arguments[3]!=state.show_command ||
                            !state.arguments[2] || state.arguments[2]>UINT32_MAX-32768) return finish("application_arguments");
                        // Private command-line storage is bounded and never copied into diagnostics.
                        bool terminated{};
                        for (std::uint32_t i=0;i<32768;++i) {
                            char ch{};
                            if (!read_private(state.arguments[2]+i,&ch,1)) return finish("application_command_line");
                            if (!ch) { state.command_line_bytes=i; terminated=true; break; }
                        }
                        if (!terminated) return finish("application_command_line");
                        state.arguments_valid=true; application_saved=context;
                        if (!arm_application(5,static_cast<DWORD>(child.base_+crt->application.target_rva),context)) return finish("application_arm_failed");
                    } else if (state.stage==5) {
                        state.entry_reached=true; std::array<std::uint32_t,5> stack{};
                        state.entry_stack_valid=state.call_stack>=4 && context.Esp==state.call_stack-4 && same_nonvolatile(context,application_saved) &&
                            read_private(context.Esp,stack.data(),sizeof(stack)) && stack[0]==child.base_+crt->application.call_rva+5 &&
                            std::equal(state.arguments.begin(),state.arguments.end(),stack.begin()+1);
                        if (!state.entry_stack_valid) return finish("application_entry_stack");
                        state.verified=true;
                        if (platform) {
                            if (!platform_shape()) return finish("platform_precondition_shape");
                            platform_saved=context; auto& next=trace.platform_startup; next.entry_stack=context.Esp;
                            next.stop_address=static_cast<DWORD>(child.base_+platform->call_rva);
                            context.ContextFlags=CONTEXT_DEBUG_REGISTERS; context.Dr0=next.stop_address; context.Dr6=0;
                            if (!SetThreadContext(child.thread_,&context)) return finish("platform_arm_failed");
                            CONTEXT checked{}; checked.ContextFlags=CONTEXT_DEBUG_REGISTERS;
                            if (!GetThreadContext(child.thread_,&checked) || checked.Dr0!=next.stop_address ||
                                checked.Dr1!=child.base_+proxy->iat_rva || checked.Dr2!=trace.proxy_base+asi->call_rva ||
                                (checked.Dr7 & 0xffff20ffU)!=0x00d00015U) return finish("platform_arm_failed");
                            next.armed=true; continue;
                        }
                        return finish("application_entry_verified");
                    } else return finish("application_stage");
                    continue;
                }
                if (trace.crt_startup.armed) {
                    auto& state=trace.crt_startup;
                    if (!tail_shape()) return finish("crt_loader_shape");
                    if (!crt_shape()) return finish("crt_shape_drift");
                    if (state.stage==1) {
                        state.io_return_reached=true; state.return_code=context.Eax;
                        state.stack_valid=state.io_return_stack<=UINT32_MAX-4 && context.Esp==state.io_return_stack+4;
                        state.registers_valid=context.Edi==crt_saved[0] && context.Esi==crt_saved[1] &&
                            context.Ebp==crt_saved[2] && context.Ebx==crt_saved[3] && !(context.EFlags&0x500U);
                        if (!state.stack_valid || !state.registers_valid) return finish("crt_io_calling_convention");
                        if (context.Eax!=0) return finish("crt_io_failed");
                        if (!arm_crt(2,static_cast<DWORD>(child.base_+crt->initialize.call_rva),context)) return finish("crt_arm_failed");
                        continue;
                    }
                    state.initializer_call_reached=true;
                    // Entry has no arguments; balanced intervening cdecl calls must preserve the outer frame.
                    if (context.Esp!=state.io_return_stack+4) return finish("crt_initializer_stack");
                    FrameTargetSample sample{}; sample.attempted=true;
                    sample.call_read=read_image(child.base_,tail->frame.call_rva,sample.call,true);
                    sample.target_read=read_image(child.base_,tail->frame.target_rva,sample.target_prefix,true);
                    if (!matches_frame_target(tail->frame,sample)) return finish("crt_frame_drift");
                    state.verified=true;
                    if (application) {
                        if (!application_shape()) return finish("application_precondition_shape");
                        application_saved=context; trace.application_entry.initializer_stack=context.Esp;
                        if (!arm_application(1,static_cast<DWORD>(child.base_+crt->initialize.call_rva+5),context)) return finish("application_arm_failed");
                        continue;
                    }
                    return finish("crt_initializer_boundary_verified");
                }
                if (trace.startup_return.armed) {
                    auto& state=trace.startup_return;
                    if (!tail_shape()) return finish("startup_return_shape");
                    if (state.stage==1) {
                        state.protect_call_reached=true; state.protect_stack=context.Esp;
                        std::array<std::uint32_t,4> args{};
                        if (context.Esp%4 || !read_private(context.Esp,args.data(),sizeof(args))) return finish("startup_return_arguments_read");
                        state.old_protect_address=args[3];
                        MEMORY_BASIC_INFORMATION stack{},output{};
                        if (args[0]!=child.base_ || args[1]!=child.image_bytes_ || args[2]!=PAGE_EXECUTE_READWRITE || !args[3] || args[3]%4 ||
                            VirtualQueryEx(child.process_,reinterpret_cast<void*>(startup_saved.Esp),&stack,sizeof(stack))!=sizeof(stack) ||
                            VirtualQueryEx(child.process_,reinterpret_cast<void*>(args[3]),&output,sizeof(output))!=sizeof(output) ||
                            output.State!=MEM_COMMIT || output.Type!=MEM_PRIVATE || output.Protect!=PAGE_READWRITE ||
                            output.AllocationBase!=stack.AllocationBase || !read_private(args[3],&state.old_protect,4)) return finish("startup_return_arguments");
                        state.arguments_valid=true;
                        if (!image_protection(false)) return finish("startup_return_image_precondition");
                        if (!arm_tail(2,static_cast<DWORD>(trace.proxy_base+tail->protect_call_rva+6),context)) return finish("startup_return_arm_failed");
                        continue;
                    }
                    if (state.stage==2) {
                        state.protect_return_reached=true; state.return_code=context.Eax;
                        if (!context.Eax) return finish("startup_return_protect_failed");
                        if (state.protect_stack>UINT32_MAX-16 || context.Esp!=state.protect_stack+16 ||
                            !read_private(state.old_protect_address,&state.old_protect,4) || state.old_protect!=state.first_page_protect)
                            return finish("startup_return_protect_result");
                        state.protections_verified=image_protection(true);
                        if (!state.protections_verified) return finish("startup_return_protection_mismatch");
                        if (!arm_tail(3,static_cast<DWORD>(trace.startup_return_address),context)) return finish("startup_return_arm_failed");
                        continue;
                    }
                    state.startup_return_reached=true;
                    state.stack_valid=startup_saved.Esp<=UINT32_MAX-8 && context.Esp==startup_saved.Esp+8;
                    state.registers_valid=context.Ebx==startup_saved.Ebx && context.Esi==startup_saved.Esi &&
                        context.Edi==startup_saved.Edi && context.Ebp==startup_saved.Ebp && !(context.EFlags & 0x500U);
                    if (!state.stack_valid || !state.registers_valid) return finish("startup_return_calling_convention");
                    std::array<std::byte,6> call{}; std::uint32_t slot{};
                    if (!read_image(child.base_,static_cast<DWORD>(trace.startup_return_address-child.base_-6),call,true)) return finish("startup_return_callsite_read");
                    std::memcpy(&slot,call.data()+2,4);
                    if (call[0]!=std::byte{0xff} || call[1]!=std::byte{0x15} || slot!=child.base_+proxy->iat_rva) return finish("startup_return_callsite");
                    if (!read_private(static_cast<DWORD>(trace.startup_argument_address),&state.startup_info_bytes,4) || state.startup_info_bytes!=68)
                        return finish("startup_return_info");
                    if (!tail_sample(1)) return finish("startup_return_frame_drift");
                    state.verified=true;
                    if (crt) {
                        if (!crt_shape()) return finish("crt_precondition_shape");
                        auto& next=trace.crt_startup;
                        std::uint32_t returned{};
                        if (context.Esp>UINT32_MAX-crt->io_return_stack_offset) return finish("crt_stack_precondition");
                        next.io_return_stack=context.Esp+crt->io_return_stack_offset;
                        if (!read_private(next.io_return_stack,&returned,4) || returned!=child.base_+crt->io.call_rva+5 ||
                            !read_private(context.Esp,crt_saved.data(),sizeof(crt_saved))) return finish("crt_stack_precondition");
                        if (!arm_crt(1,returned,context)) return finish("crt_arm_failed");
                        continue;
                    }
                    return finish("startup_return_verified");
                }
                if (trace.bootstrap.calls_armed) {
                    auto& state = trace.bootstrap;
                    auto& call = state.calls[state.calls_armed - 1];
                    call.returned = true; call.return_code = context.Eax; call.stack_after = context.Esp;
                    ++state.calls_returned;
                    if (!bootstrap_shape()) return finish("bootstrap_return_shape");
                    call.stack_valid = context.Esp == state.frame_address + 4 && context.Ebx == bootstrap_saved.Ebx &&
                        context.Esi == bootstrap_saved.Esi && context.Edi == bootstrap_saved.Edi &&
                        context.Ebp == bootstrap_saved.Ebp && !(context.EFlags & 0x500U);
                    if (!call.stack_valid) return finish("bootstrap_calling_convention");
                    if (!read_private(state.output_address - 16, bootstrap_envelope.data(), bootstrap_envelope.size()))
                        return finish("bootstrap_output_unreadable");
                    std::memcpy(&call.status, bootstrap_envelope.data() + 16, sizeof(call.status));
                    const auto guard = [](std::byte b) { return b == std::byte{0xa5}; };
                    call.guards_valid = std::all_of(bootstrap_envelope.begin(), bootstrap_envelope.begin() + 16, guard) &&
                        std::all_of(bootstrap_envelope.end() - 16, bootstrap_envelope.end(), guard);
                    if (!call.guards_valid) return finish("bootstrap_output_overrun");
                    if (call.return_code != SAEX_BOOTSTRAP_OK) return finish("bootstrap_call_failed");
                    call.status_valid = valid_bootstrap_status(*bootstrap, state.calls_armed - 1, call.status,
                        state.calls_armed > 2 ? &state.calls[1].status : nullptr);
                    if (!call.status_valid) return finish("bootstrap_status_mismatch");
                    if (state.calls_returned == state.calls.size()) {
                        state.verified = true;
                        state.observed_unverified = state.calls[1].status.state == SAEX_BOOTSTRAP_OBSERVED_UNVERIFIED;
                        if (frame) {
                            if (!sample_frame(2)) return finish("frame_terminal_sample_rejected");
                            trace.frame_target.verified = true;
                        }
                        return finish(!state.observed_unverified ? "bootstrap_lifecycle_host_rejected" :
                            frame ? "frame_target_samples_verified" : "bootstrap_lifecycle_verified");
                    }
                    if (!arm_bootstrap(context)) return finish("bootstrap_call_arm_failed");
                    continue;
                }
                if (trace.asi_scan_continued) {
                    if (trace.asi_load_continued) {
                        trace.asi_return_reached = true; trace.asi_handle = context.Eax; trace.asi_stack_after = context.Esp;
                        if (!asi_shape()) return finish("asi_return_shape");
                        if (trace.asi_stack_before > UINT32_MAX - 4 || context.Esp != trace.asi_stack_before + 4)
                            return finish("asi_stack_mismatch");
                        if (!context.Eax) return finish("asi_load_failed");
                        for (std::uint32_t i = 0; i < trace.module_count; ++i) {
                            const auto& m = trace.modules[i];
                            if (!m.admitted || !mappings.active(m.mapping_id, m.base) || !matches_file(m.file, asi->module->identity())) continue;
                            if (trace.asi_mapping_id) return finish("asi_mapping_ambiguous");
                            if (m.event_index <= trace.asi_arm_event) return finish("asi_mapping_not_fresh");
                            trace.asi_mapping_id = m.mapping_id;
                            if (m.base != context.Eax) return finish("asi_handle_mismatch");
                        }
                        if (!trace.asi_mapping_id) return finish("asi_mapping_missing");
                        trace.asi_verified = true;
                        if (tail) {
                            if (!tail_shape()) return finish("startup_return_precondition_shape");
                            if (!tail_sample(0)) return finish("startup_return_frame_precondition");
                            if (!arm_tail(1,static_cast<DWORD>(trace.proxy_base+tail->protect_call_rva),context)) return finish("startup_return_arm_failed");
                            continue;
                        }
                        if (frame && !sample_frame(1)) return finish("frame_asi_sample_rejected");
                        if (bootstrap) {
                            if (!bootstrap_shape()) return finish("bootstrap_export_shape");
                            if (!arm_bootstrap(context)) return finish("bootstrap_call_arm_failed");
                            continue;
                        }
                        return finish("asi_return_verified");
                    }
                    trace.asi_call_reached = true; trace.asi_stack_before = context.Esp;
                    if (!asi_shape()) return finish("asi_call_shape");
                    if (asi_preloaded()) return finish("asi_module_preloaded");
                    std::uint32_t pointer{}; std::array<char, 260> path{};
                    if (context.Esp % 4 || !read_private(context.Esp, &pointer, 4) ||
                        !read_private(pointer, path.data(), asi->requested_path.size() + 1)) return finish("asi_path_unreadable");
                    if (path[asi->requested_path.size()] != '\0') return finish("asi_path_mismatch");
                    const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 'a' - 'A') : c; };
                    for (std::size_t i = 0; i < asi->requested_path.size(); ++i)
                        if (lower(path[i]) != lower(asi->requested_path[i])) return finish("asi_path_mismatch");
                    trace.asi_path_verified = true; trace.asi_arm_event = trace.event_count;
                    if (!arm_asi(trace.asi_return_address, context)) return finish("asi_breakpoint_arm_failed");
                    trace.asi_return_armed = true;
                    continue;
                }
                if (trace.binding_continued) {
                    trace.binding_reached = true;
                    if (!binding_shape()) return finish("binding_return_shape");
                    for (std::uint32_t i = 0; i < trace.codec_module_count; ++i) {
                        const auto id = trace.codec_mapping_ids[i];
                        const auto end = trace.modules.begin() + trace.module_count;
                        const auto module = std::find_if(trace.modules.begin(), end, [&](const auto& value) { return value.mapping_id == id; });
                        if (module == end || !mappings.active(id, module->base)) return finish("binding_mapping_retired");
                    }
                    bool all_match = true;
                    for (std::uint32_t i = 0; i < trace.binding_count; ++i) {
                        auto& value = trace.bindings[i];
                        if (!read_word(trace.proxy_base, value.slot_rva, value.after) ||
                            !read_image(trace.codec_handle, value.target_rva, value.target_after, true)) return finish("binding_slot_unreadable");
                        value.target_stable = value.target_before == value.target_after;
                        value.match = value.after == trace.codec_handle + value.target_rva && value.target_stable;
                        all_match = all_match && value.match;
                    }
                    if (!all_match) return finish("binding_slot_mismatch");
                    trace.binding_verified = true;
                    if (!asi) return finish("codec_bindings_verified");
                    if (!asi_shape()) return finish("asi_precondition_shape");
                    if (asi_preloaded()) return finish("asi_module_preloaded");
                    trace.asi_call_address = trace.proxy_base + asi->call_rva;
                    trace.asi_return_address = trace.asi_call_address + 6;
                    if (!arm_asi(trace.asi_call_address, context)) return finish("asi_breakpoint_arm_failed");
                    trace.asi_call_armed = true;
                    continue;
                }
                if (trace.codec_continued) {
                    trace.codec_reached = true; trace.codec_handle = context.Eax;
                    std::array<std::byte, 13> bytes{};
                    if (!codec_shape(bytes) || bytes != codec_before) return finish("codec_return_shape");
                    std::uint32_t iat{};
                    if (!read_word(child.base_, proxy->iat_rva, iat) || iat != trace.startup_address)
                        return finish("startup_iat_drift");
                    if (!context.Eax) return finish("codec_load_failed");
                    for (std::size_t n = 0; n < codec->modules.size(); ++n) {
                        for (std::uint32_t i = 0; i < trace.module_count; ++i) {
                            const auto& module = trace.modules[i];
                            if (module.admitted && mappings.active(module.mapping_id, module.base) &&
                                matches_file(module.file, codec->modules[n]->identity())) {
                                if (trace.codec_mapping_ids[n]) return finish("codec_mapping_ambiguous");
                                if (module.event_index <= trace.codec_arm_event) return finish("codec_mapping_not_fresh");
                                trace.codec_mapping_ids[n] = module.mapping_id;
                                if (n == 0 && module.base != context.Eax) return finish("codec_handle_mismatch");
                            }
                        }
                        if (!trace.codec_mapping_ids[n]) return finish("codec_mapping_missing");
                        ++trace.codec_module_count;
                    }
                    trace.codec_modules_verified = true;
                    if (!binding) return finish("codec_return_verified");
                    if (!binding_shape()) return finish("binding_precondition_shape");
                    for (std::uint32_t i = 0; i < trace.binding_count; ++i) {
                        auto& value = trace.bindings[i];
                        if (!read_word(trace.proxy_base, value.slot_rva, value.before) ||
                            !read_image(trace.codec_handle, value.target_rva, value.target_before, true)) return finish("binding_precondition_read");
                        if (value.before) return finish("binding_preexisting_slot");
                    }
                    trace.binding_stop_address = trace.proxy_base + binding->stop_rva;
                    context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                    context.Dr0 = static_cast<DWORD>(trace.binding_stop_address); context.Dr6 = 0;
                    if (!SetThreadContext(child.thread_, &context)) return finish("binding_breakpoint_write_failed");
                    CONTEXT verified{}; verified.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                    if (!GetThreadContext(child.thread_, &verified) || verified.Dr0 != trace.binding_stop_address ||
                        verified.Dr1 != child.base_ + proxy->iat_rva || (verified.Dr7 & 0xffff20ffU) != 0x00d00005U)
                        return finish("binding_breakpoint_arm_failed");
                    trace.binding_breakpoint_armed = true;
                    continue;
                }
                if (trace.startup_continued) {
                    trace.startup_reached = true;
                    if (tail) startup_saved=context;
                    if (!proxy_shape()) return finish("startup_proxy_shape");
                    std::uint32_t iat{};
                    if (!read_word(child.base_, proxy->iat_rva, iat) || iat != trace.startup_address)
                        return finish("startup_iat_drift");
                    if (!read_image(trace.proxy_base, proxy->iat_target_rva, trace.startup_target_after, true))
                        return finish("startup_target_unreadable");
                    trace.startup_target_stable = trace.startup_target_after == trace.startup_target_before;
                    if (!trace.startup_target_stable) return finish("startup_target_drift");
                    MEMORY_BASIC_INFORMATION stack{};
                    const auto stack_address = static_cast<std::uintptr_t>(context.Esp);
                    if (VirtualQueryEx(child.process_, reinterpret_cast<void*>(stack_address), &stack, sizeof(stack)) != sizeof(stack) ||
                        stack.Type != MEM_PRIVATE || stack.State != MEM_COMMIT || (stack.Protect & (PAGE_GUARD | PAGE_NOACCESS)) ||
                        stack_address < reinterpret_cast<std::uintptr_t>(stack.BaseAddress) ||
                        stack_address - reinterpret_cast<std::uintptr_t>(stack.BaseAddress) >= stack.RegionSize ||
                        8 > stack.RegionSize - (stack_address - reinterpret_cast<std::uintptr_t>(stack.BaseAddress)))
                        return finish("startup_stack_region");
                    std::array<std::uint32_t, 2> words{}; SIZE_T copied{};
                    if (!ReadProcessMemory(child.process_, reinterpret_cast<void*>(stack_address), words.data(), sizeof(words), &copied) || copied != sizeof(words))
                        return finish("startup_stack_read");
                    trace.startup_return_address = words[0]; trace.startup_argument_address = words[1];
                    if (words[0] < child.base_ || words[0] - child.base_ < 6 || words[0] - child.base_ >= child.image_bytes_)
                        return finish("startup_callsite_range");
                    std::array<std::byte, 6> call{}; std::uint32_t operand{};
                    if (!read_image(child.base_, words[0] - child.base_ - 6, call, true)) return finish("startup_callsite_read");
                    std::memcpy(&operand, call.data() + 2, 4);
                    trace.startup_callsite_verified = call[0] == std::byte{0xff} && call[1] == std::byte{0x15} && operand == child.base_ + proxy->iat_rva;
                    if (!trace.startup_callsite_verified) return finish("startup_callsite_shape");
                    MEMORY_BASIC_INFORMATION argument{};
                    const auto address = static_cast<std::uintptr_t>(words[1]);
                    // Admit a complete x86 STARTUPINFOA in the same allocation as the held stack.
                    trace.startup_argument_valid = address && !(address % 4) &&
                        VirtualQueryEx(child.process_, reinterpret_cast<void*>(address), &argument, sizeof(argument)) == sizeof(argument) &&
                        argument.Type == MEM_PRIVATE && argument.State == MEM_COMMIT && argument.AllocationBase == stack.AllocationBase &&
                        !(argument.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (argument.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)) &&
                        address >= reinterpret_cast<std::uintptr_t>(argument.BaseAddress) &&
                        address - reinterpret_cast<std::uintptr_t>(argument.BaseAddress) < argument.RegionSize &&
                        68 <= argument.RegionSize - (address - reinterpret_cast<std::uintptr_t>(argument.BaseAddress));
                    if (!trace.startup_argument_valid) return finish("startup_argument_region");
                    for (std::uint32_t i = 0; i < trace.startup_sample_count; ++i) {
                        auto& sample = trace.startup_samples[i];
                        auto bytes = std::span(sample.after).first(sample.length);
                        sample.read = child.copy(sample.rva, bytes);
                        if (!sample.read) return finish("startup_sample_unreadable");
                        sample.match = std::equal(bytes.begin(), bytes.end(), sample.before.begin());
                    }
                    if (!codec) return finish("startup_call_verified");
                    for (std::uint32_t i = 0; i < trace.module_count; ++i) {
                        const auto& module = trace.modules[i];
                        if (module.admitted && mappings.active(module.mapping_id, module.base))
                            for (const auto pin : codec->modules)
                                if (matches_file(module.file, pin->identity())) return finish("codec_module_preloaded");
                    }
                    if (!codec_shape(codec_before)) return finish("codec_call_shape");
                    trace.codec_shape_verified = true;
                    trace.codec_return_address = trace.proxy_base + codec->sequence_rva + 11;
                    context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                    context.Dr0 = static_cast<DWORD>(trace.codec_return_address); context.Dr6 = 0;
                    if (!SetThreadContext(child.thread_, &context)) return finish("codec_breakpoint_write_failed");
                    CONTEXT verified{}; verified.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                    if (!GetThreadContext(child.thread_, &verified) || verified.Dr0 != trace.codec_return_address ||
                        verified.Dr1 != child.base_ + proxy->iat_rva || (verified.Dr7 & 0xffff20ffU) != 0x00d00005U)
                        return finish("codec_breakpoint_arm_failed");
                    trace.codec_arm_event = trace.event_count; trace.codec_breakpoint_armed = true;
                    continue;
                }
                if (trace.proxy_continued) {
                    trace.proxy_return_reached = true;
                    if (!proxy_shape()) return finish("proxy_return_shape");
                    if (!child.copy(entry->rva, trace.proxy_entry_after)) return finish("proxy_return_entry_unreadable");
                    trace.proxy_entry_restored = trace.proxy_entry_after == trace.entry_before;
                    if (!trace.proxy_entry_restored) return finish("proxy_entry_not_restored");
                    if (!read_word(child.base_, proxy->iat_rva, trace.proxy_iat_after)) return finish("proxy_iat_unreadable");
                    trace.proxy_iat_verified = trace.proxy_iat_after == trace.proxy_base + proxy->iat_target_rva;
                    if (!trace.proxy_iat_verified) return finish("proxy_iat_mismatch");
                    if (!startup) return finish("proxy_return_verified");
                    trace.startup_address = trace.proxy_iat_after;
                    if (!read_image(trace.proxy_base, proxy->iat_target_rva, trace.startup_target_before, true))
                        return finish("startup_target_unreadable");
                    context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                    context.Dr0 = static_cast<DWORD>(trace.startup_address); context.Dr1 = static_cast<DWORD>(child.base_ + proxy->iat_rva);
                    context.Dr6 = 0; context.Dr7 = (context.Dr7 & ~0xffff20ffU) | 0x00d00005U;
                    if (!SetThreadContext(child.thread_, &context)) return finish("startup_breakpoint_write_failed");
                    CONTEXT verified{}; verified.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                    if (!GetThreadContext(child.thread_, &verified) || verified.Dr0 != trace.startup_address ||
                        verified.Dr1 != child.base_ + proxy->iat_rva || (verified.Dr7 & 0xffff20ffU) != 0x00d00005U)
                        return finish("startup_breakpoint_arm_failed");
                    trace.startup_breakpoint_armed = true;
                    continue;
                }
                trace.entry_reached = true;
                trace.entry_bytes_read = child.copy(entry->rva, trace.entry_after);
                if (!trace.entry_bytes_read) return finish("entry_bytes_unreadable");
                trace.entry_bytes_match = trace.entry_after == trace.entry_before;
                if (proxy) {
                    const auto& identity = proxy->module->identity();
                    for (std::uint32_t i = 0; i < trace.module_count; ++i) {
                        const auto& module = trace.modules[i];
                        if (module.admitted && mappings.active(module.mapping_id, module.base) &&
                            module.file.volume == identity.volume && module.file.file_id == identity.file_id &&
                            module.file.bytes == identity.bytes && module.file.sha256 == identity.sha256) {
                            if (trace.proxy_mapping_id) return finish("proxy_mapping_ambiguous");
                            trace.proxy_mapping_id = module.mapping_id; trace.proxy_base = module.base;
                        }
                    }
                    if (!trace.proxy_mapping_id) return finish("proxy_mapping_missing");
                    if (!proxy_shape()) return finish("proxy_entry_shape");
                    std::uint32_t relative{}; std::memcpy(&relative, trace.entry_after.data() + 1, 4);
                    if (trace.entry_after[0] != std::byte{0xe9} ||
                        static_cast<std::uint32_t>(trace.entry_address + 5 + relative) != trace.proxy_base + proxy->thunk_rva ||
                        !std::equal(trace.entry_after.begin() + 5, trace.entry_after.end(), trace.entry_before.begin() + 5))
                        return finish("proxy_entry_redirect");
                    if (!read_word(child.base_, proxy->iat_rva, trace.proxy_iat_before)) return finish("proxy_iat_unreadable");
                    bool system_target{};
                    for (std::uint32_t i = 0; i < trace.module_count; ++i) {
                        const auto& module = trace.modules[i];
                        const std::string_view name(module.file.name.data());
                        std::array<std::byte, 1> byte{};
                        if ((name == "kernel32.dll" || name == "kernelbase.dll") && module.admitted &&
                            mappings.active(module.mapping_id, module.base) && trace.proxy_iat_before >= module.base &&
                            read_image(module.base, trace.proxy_iat_before - module.base, byte, true)) system_target = true;
                    }
                    if (!system_target) return finish("proxy_iat_before_not_system");
                    trace.proxy_validated = true;
                    trace.proxy_return_address = trace.proxy_base + proxy->thunk_rva + 5;
                    context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                    context.Dr0 = static_cast<DWORD>(trace.proxy_return_address); context.Dr6 = 0;
                    if (!SetThreadContext(child.thread_, &context)) return finish("proxy_breakpoint_write_failed");
                    CONTEXT verified{}; verified.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                    if (!GetThreadContext(child.thread_, &verified) || verified.Dr0 != trace.proxy_return_address ||
                        (verified.Dr7 & 0xffff20ffU) != 1) return finish("proxy_breakpoint_arm_failed");
                    trace.proxy_breakpoint_armed = true;
                    continue;
                }
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
