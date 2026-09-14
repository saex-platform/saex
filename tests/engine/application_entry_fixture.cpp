#include "loader_fixture_marker.hpp"
#if defined(SAEX_CWD_COPY_FIXTURE)
#include <cstdio>
#include <cstring>
#endif
extern "C" __declspec(dllimport) void __cdecl proxy_iat_target();
extern "C" __declspec(dllimport) unsigned char application_once;
extern "C" {
#if defined(SAEX_CD_STREAM_TABLES_FIXTURE)
__declspec(align(4)) unsigned char stream_table_window[2192]{};
#endif

#if defined(SAEX_CD_STREAM_ALLOCATION_FIXTURE)
__declspec(dllexport,align(4)) unsigned allocation_heap_handle{};
#endif
#if defined(SAEX_CD_STREAM_CHANNELS_FIXTURE)
__declspec(dllexport,align(4)) FARPROC channels_error_slot{},channels_local_slot{};
__declspec(dllexport,align(4)) unsigned channels_pointer_window[3]{0xaabbccdd,
#if defined(SAEX_CHANNELS_PREEXISTING)
1,
#else
0,
#endif
0xddccbbaa};
#if defined(SAEX_CHANNELS_BAD_NAME)
__declspec(dllexport) char channels_filename[]="XODELS\\GTA3.IMG";
#else
__declspec(dllexport) char channels_filename[]="MODELS\\GTA3.IMG";
#endif
#endif
#define PREFIX __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop \
    __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop
__declspec(dllexport,align(4096)) unsigned char startup_sample[4096]{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
__declspec(dllexport,naked) void frame_target() { PREFIX __asm ret }
__declspec(dllexport,naked) void frame_caller() { __asm call frame_target __asm ret }
void application_canary() { mark_loader_phase(L".application-body-entered"); ExitProcess(96); }
#if defined(SAEX_EVENT_DISPATCH_FIXTURE)
void event_app_canary() { mark_loader_phase(L".event-app-called"); ExitProcess(92); }
#if defined(SAEX_APPLICATION_ROUTING_FIXTURE)
#if defined(SAEX_GAME_PRELUDE_FIXTURE)
#if defined(SAEX_FILE_MANAGER_FIXTURE)
#if defined(SAEX_FILE_MANAGER_READONLY)
extern __declspec(dllexport,align(4096)) const unsigned char file_manager_buffer[4096]{0x11,0x22,0x33,0x44,0x61,0x62,0x63,0};
#else
__declspec(dllexport,align(4096)) unsigned char file_manager_buffer[4096]{0x11,0x22,0x33,0x44,0x61,0x62,0x63,0};
#endif
extern __declspec(dllexport) const unsigned char file_manager_suffix[2]{0x5c,0};
#if defined(SAEX_CWD_SEH_FIXTURE)
__declspec(naked) void cwd_seh_handler() { __asm call event_app_canary PREFIX __asm ret }
#if defined(SAEX_FILE_MANAGER_READY_FIXTURE)
void cwd_seh_cleanup();
#else
__declspec(naked) void cwd_seh_cleanup() { __asm call event_app_canary PREFIX __asm ret }
#endif
struct CwdScope { unsigned previous;void* filter;void (*cleanup)(); };
const CwdScope cwd_seh_scope{0xffffffffU,nullptr,&cwd_seh_cleanup};
// This fixture intentionally reproduces manual x86 registration. The observer
// stops before exception dispatch; this canary is not a SafeSEH handler test.
#pragma warning(push)
#pragma warning(disable:4733)
__declspec(naked) void cwd_seh_prologue() {
    __asm {
        push offset cwd_seh_handler
        mov eax,dword ptr fs:[0]
        push eax
        mov eax,[esp+10h]
        mov [esp+10h],ebp
        lea ebp,[esp+10h]
        sub esp,eax
        push ebx
        push esi
        push edi
        mov eax,[ebp-8]
        mov [ebp-18h],esp
        push eax
        mov eax,[ebp-4]
        mov dword ptr [ebp-4],0ffffffffh
        mov [ebp-8],eax
        lea eax,[ebp-10h]
        mov dword ptr fs:[0],eax
        ret
    }
}
#pragma warning(pop)
#if defined(SAEX_CWD_LOCK_FIXTURE)
// Nonzero is deliberately opaque, not a dereferenceable CRITICAL_SECTION.
__declspec(align(4096)) unsigned cwd_lock_table[1024]{0,1,0,1,0,0,0,1,0,1,0,0,0,1,
#if defined(SAEX_CWD_LOCK_EMPTY)
    0,
#else
    0x12345678,
#endif
    1,0};
#if defined(SAEX_CWD_ACQUIRE_FIXTURE)
FARPROC cwd_acquire_slot{};
#if defined(SAEX_FILE_MANAGER_READY_FIXTURE)
FARPROC cwd_release_slot{};
__declspec(naked) void cwd_unlock() {
    __asm push ebp
    __asm mov ebp,esp
    __asm mov eax,[ebp+8]
    __asm push dword ptr [eax*8+cwd_lock_table]
    __asm call dword ptr [cwd_release_slot]
    __asm pop ebp
    __asm ret
}
__declspec(naked) void cwd_seh_cleanup() {
    __asm push 7
    __asm call cwd_unlock
    __asm pop ecx
    __asm ret
}
#pragma warning(push)
#pragma warning(disable:4733)
__declspec(naked) void cwd_seh_epilogue() {
    __asm mov ecx,[ebp-10h]
    __asm mov dword ptr fs:[0],ecx
    __asm pop ecx
    __asm pop edi
    __asm pop esi
    __asm pop ebx
    __asm leave
    __asm push ecx
    __asm ret
}
#pragma warning(pop)
#endif
#if defined(SAEX_CWD_ACQUIRE_IMAGE)
CRITICAL_SECTION cwd_acquire_static{};
#endif
void cwd_acquire_initialize() {
#if defined(SAEX_CD_STREAM_CHANNELS_FIXTURE)
    channels_error_slot=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"SetLastError");
    channels_local_slot=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LocalAlloc");
    if(!channels_error_slot || !channels_local_slot)ExitProcess(97);
#endif
#if defined(SAEX_CD_STREAM_ALLOCATION_FIXTURE)
    allocation_heap_handle=reinterpret_cast<unsigned>(GetProcessHeap());
#endif
#if defined(SAEX_CWD_ACQUIRE_IMAGE)
    auto* object=&cwd_acquire_static;
#else
    auto* object=static_cast<CRITICAL_SECTION*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
#endif
    if(!object || !InitializeCriticalSectionAndSpinCount(object,4000))ExitProcess(97);
    #if defined(SAEX_CD_STREAM_TABLES_FIXTURE)
    for(auto& v:stream_table_window)v=0xa7;
#endif
    cwd_acquire_slot=GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"RtlEnterCriticalSection");
    if(!cwd_acquire_slot)ExitProcess(97);
#if defined(SAEX_FILE_MANAGER_READY_FIXTURE)
    cwd_release_slot=GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"RtlLeaveCriticalSection");
    if(!cwd_release_slot)ExitProcess(97);
#endif
#if defined(SAEX_CWD_QUERY_FIXTURE)
    extern FARPROC cwd_query_slot;
    cwd_query_slot=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"GetCurrentDirectoryA");
    if(!cwd_query_slot)ExitProcess(97);
#endif
    cwd_lock_table[14]=reinterpret_cast<unsigned>(object);
#if defined(SAEX_CWD_ACQUIRE_EMPTY)
    cwd_lock_table[14]=0;
#elif defined(SAEX_CWD_ACQUIRE_HELD)
    EnterCriticalSection(object);
#elif defined(SAEX_CWD_ACQUIRE_READONLY)
    DWORD old{};if(!VirtualProtect(object,4096,PAGE_READONLY,&old))ExitProcess(97);
#elif defined(SAEX_CWD_ACQUIRE_BAD_IAT)
    cwd_acquire_slot=reinterpret_cast<FARPROC>(reinterpret_cast<unsigned>(cwd_acquire_slot)+1);
#elif defined(SAEX_CWD_ACQUIRE_INVALID)
    cwd_lock_table[14]=1;
#endif
}
#endif
__declspec(naked) void cwd_lock_selector() {
    __asm {
        push ebp
        mov ebp,esp
        mov eax,[ebp+8]
        push esi
        lea esi,[eax*8+cwd_lock_table]
        cmp dword ptr [esi],0
        _emit 0x75
        _emit 0x13
        call event_app_canary
    }
#if defined(SAEX_CWD_ACQUIRE_FIXTURE)
    __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop
    __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop
    __asm {
        push dword ptr [esi]
        call dword ptr [cwd_acquire_slot]
        pop esi
        pop ebp
        ret
    }
#else
    PREFIX
    __asm call event_app_canary
    __asm ret
#endif
}
#endif
#if defined(SAEX_CWD_QUERY_FIXTURE)
#if defined(SAEX_CWD_COPY_FIXTURE)
__declspec(naked) char* __cdecl cwd_copy_function(char*,const char*) {
    __asm _emit 0x57
    __asm _emit 0x8b
    __asm _emit 0x7c
    __asm _emit 0x24
    __asm _emit 0x08
    __asm _emit 0xeb
    __asm _emit 0x6e
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm _emit 0x8b
    __asm _emit 0x4c
    __asm _emit 0x24
    __asm _emit 0x0c
    __asm _emit 0xf7
    __asm _emit 0xc1
    __asm _emit 0x03
    __asm _emit 0x00
    __asm _emit 0x00
    __asm _emit 0x00
    __asm _emit 0x74
    __asm _emit 0x1d
    __asm _emit 0x8a
    __asm _emit 0x11
    __asm _emit 0x83
    __asm _emit 0xc1
    __asm _emit 0x01
    __asm _emit 0x84
    __asm _emit 0xd2
    __asm _emit 0x74
    __asm _emit 0x66
    __asm _emit 0x88
    __asm _emit 0x17
    __asm _emit 0x83
    __asm _emit 0xc7
    __asm _emit 0x01
    __asm _emit 0xf7
    __asm _emit 0xc1
    __asm _emit 0x03
    __asm _emit 0x00
    __asm _emit 0x00
    __asm _emit 0x00
    __asm _emit 0x75
    __asm _emit 0xea
    __asm _emit 0xeb
    __asm _emit 0x05
    __asm _emit 0x89
    __asm _emit 0x17
    __asm _emit 0x83
    __asm _emit 0xc7
    __asm _emit 0x04
    __asm _emit 0xba
    __asm _emit 0xff
    __asm _emit 0xfe
    __asm _emit 0xfe
    __asm _emit 0x7e
    __asm _emit 0x8b
    __asm _emit 0x01
    __asm _emit 0x03
    __asm _emit 0xd0
    __asm _emit 0x83
    __asm _emit 0xf0
    __asm _emit 0xff
    __asm _emit 0x33
    __asm _emit 0xc2
    __asm _emit 0x8b
    __asm _emit 0x11
    __asm _emit 0x83
    __asm _emit 0xc1
    __asm _emit 0x04
    __asm _emit 0xa9
    __asm _emit 0x00
    __asm _emit 0x01
    __asm _emit 0x01
    __asm _emit 0x81
    __asm _emit 0x74
    __asm _emit 0xe1
    __asm _emit 0x84
    __asm _emit 0xd2
    __asm _emit 0x74
    __asm _emit 0x34
    __asm _emit 0x84
    __asm _emit 0xf6
    __asm _emit 0x74
    __asm _emit 0x27
    __asm _emit 0xf7
    __asm _emit 0xc2
    __asm _emit 0x00
    __asm _emit 0x00
    __asm _emit 0xff
    __asm _emit 0x00
    __asm _emit 0x74
    __asm _emit 0x12
    __asm _emit 0xf7
    __asm _emit 0xc2
    __asm _emit 0x00
    __asm _emit 0x00
    __asm _emit 0x00
    __asm _emit 0xff
    __asm _emit 0x74
    __asm _emit 0x02
    __asm _emit 0xeb
    __asm _emit 0xc7
    __asm _emit 0x89
    __asm _emit 0x17
    __asm _emit 0x8b
    __asm _emit 0x44
    __asm _emit 0x24
    __asm _emit 0x08
    __asm _emit 0x5f
    __asm _emit 0xc3
    __asm _emit 0x66
    __asm _emit 0x89
    __asm _emit 0x17
    __asm _emit 0x8b
    __asm _emit 0x44
    __asm _emit 0x24
    __asm _emit 0x08
    __asm _emit 0xc6
    __asm _emit 0x47
    __asm _emit 0x02
    __asm _emit 0x00
    __asm _emit 0x5f
    __asm _emit 0xc3
    __asm _emit 0x66
    __asm _emit 0x89
    __asm _emit 0x17
    __asm _emit 0x8b
    __asm _emit 0x44
    __asm _emit 0x24
    __asm _emit 0x08
    __asm _emit 0x5f
    __asm _emit 0xc3
    __asm _emit 0x88
    __asm _emit 0x17
    __asm _emit 0x8b
    __asm _emit 0x44
    __asm _emit 0x24
    __asm _emit 0x08
    __asm _emit 0x5f
    __asm _emit 0xc3
}
int cwd_copy_control() {
    unsigned cases{};
    for(unsigned length=3;length<=126;++length)for(unsigned sa=0;sa<4;++sa)for(unsigned da=0;da<4;++da) {
        alignas(4) char source[268],before[268],destination[140];
        std::memset(source,0x55,sizeof(source));std::memset(destination,0x66,sizeof(destination));
        for(unsigned i=0;i<length;++i)source[4+sa+i]=static_cast<char>('A'+i%23);
        source[4+sa+length]=0;std::memcpy(before,source,sizeof(source));
        const auto result=cwd_copy_function(destination+4+da,source+4+sa);
        if(result!=destination+4+da || std::memcmp(source,before,sizeof(source)))return 110;
        for(unsigned i=0;i<sizeof(destination);++i) {
            const auto expected=i>=4+da && i-4-da<=length?source[4+sa+i-4-da]:char(0x66);
            if(destination[i]!=expected)return 111;
        }
        ++cases;
    }
    std::printf("PASS cwd copier body: %u length/alignment cases; source and destination guards preserved\n",cases);
    return 0;
}
#endif
unsigned cwd_query_cookie=0xaabbccdd;
#if defined(SAEX_CWD_RETURN_FIXTURE)
__declspec(naked) void cwd_cookie_check() {
    __asm cmp ecx,[cwd_query_cookie]
    __asm _emit 0x75
    __asm _emit 0x01
    __asm ret
    __asm jmp event_app_canary
}
#endif
FARPROC cwd_query_slot{};
__declspec(naked) void cwd_query_helper() {
    __asm {
        push ebp
        mov ebp,esp
        sub esp,10ch
        mov eax,[cwd_query_cookie]
        xor eax,[ebp+4]
        push ebx
        mov ebx,[ebp+8]
        test ebx,ebx
        mov [ebp-4],eax
        _emit 0x74
        _emit 0x5f
        call event_app_canary
    }
#if defined(SAEX_CWD_RETURN_FIXTURE)
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm {
        mov ecx,[ebp-4]
        xor ecx,[ebp+4]
        pop ebx
        call cwd_cookie_check
        leave
        ret
    }
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
#else
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
#endif
    __asm {
        lea eax,[ebp-10ch]
        push eax
        push 104h
        call dword ptr [cwd_query_slot]
    }
#if defined(SAEX_CWD_COPY_FIXTURE)
    __asm _emit 0x85
    __asm _emit 0xc0
    __asm _emit 0x74
    __asm _emit 0xac
    __asm _emit 0x40
    __asm _emit 0x3d
    __asm _emit 0x04
    __asm _emit 0x01
    __asm _emit 0x00
    __asm _emit 0x00
    __asm _emit 0x77
    __asm _emit 0xa4
    __asm _emit 0x8b
    __asm _emit 0x4d
    __asm _emit 0x0c
    __asm _emit 0x85
    __asm _emit 0xc9
    __asm _emit 0x75
    __asm _emit 0x25
    __asm call event_app_canary
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm _emit 0x3b
    __asm _emit 0x45
    __asm _emit 0x10
    __asm _emit 0x7e
    __asm _emit 0x10
    __asm call event_app_canary
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm nop
    __asm {
        lea eax,[ebp-10ch]
        push eax
        push ecx
        call cwd_copy_function
    }
#endif
#if defined(SAEX_CWD_RETURN_FIXTURE)
    __asm pop ecx
    __asm pop ecx
    __asm _emit 0xe9
    __asm _emit 0x51
    __asm _emit 0xff
    __asm _emit 0xff
    __asm _emit 0xff
#endif
    __asm call event_app_canary
    PREFIX
    __asm ret
}
#endif
__declspec(dllexport,naked) void file_manager_cwd() {
    __asm push 0ch
    __asm push offset cwd_seh_scope
    __asm call cwd_seh_prologue
#if defined(SAEX_CWD_LOCK_FIXTURE)
    __asm push 7
    __asm call cwd_lock_selector
#endif
#if defined(SAEX_CWD_QUERY_FIXTURE)
    __asm pop ecx
    __asm and dword ptr [ebp-4],0
    __asm push dword ptr [ebp+0ch]
    __asm push dword ptr [ebp+8]
    __asm push 0
    __asm call cwd_query_helper
#endif
#if defined(SAEX_FILE_MANAGER_READY_FIXTURE)
    __asm add esp,0ch
    __asm mov [ebp-1ch],eax
    __asm or dword ptr [ebp-4],-1
    __asm call cwd_seh_cleanup
    __asm mov eax,[ebp-1ch]
    __asm call cwd_seh_epilogue
    __asm _emit 0xc3
#endif
    __asm call event_app_canary
    PREFIX
    __asm ret
}
#else
__declspec(dllexport,naked) void file_manager_cwd() { __asm call event_app_canary PREFIX __asm ret }
#endif
__declspec(dllexport,naked) void file_manager_target() {
    __asm {
        push edi
        push 80h
        push offset file_manager_buffer+4
        call file_manager_cwd
        mov edi,offset file_manager_buffer+4
        add esp,8
        dec edi
        // Preserve the exact seven-byte alignment LEA, rather than a shorter equivalent.
        _emit 0x8d
        _emit 0xa4
        _emit 0x24
        _emit 0
        _emit 0
        _emit 0
        _emit 0
    manager_scan:
        mov al,[edi+1]
        inc edi
        test al,al
        jne manager_scan
        mov ax,word ptr [file_manager_suffix]
        mov word ptr [edi],ax
        pop edi
        ret
    }
}
#endif
__declspec(dllexport,align(4096)) unsigned char prelude_flags[4096]{0x11,0x22,0x33,0x44,7,8,9,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x55};
// Match GTA's exact one-byte RET; MSVC may encode a nonterminal `ret` as C2 00 00.
__declspec(dllexport,naked) void prelude_empty() { __asm _emit 0xc3 PREFIX }
__declspec(dllexport,naked) void prelude_localisation() {
    __asm {
        xor al,al
        mov byte ptr [prelude_flags+4],1
        mov byte ptr [prelude_flags+5],al
        mov byte ptr [prelude_flags+6],al
        ret
    }
}
#if defined(SAEX_CD_STREAM_TABLES_FIXTURE)
#if defined(SAEX_CD_STREAM_DISK_FIXTURE)
decltype(&GetDiskFreeSpaceA) stream_disk_slot=&GetDiskFreeSpaceA;
__declspec(dllexport,align(4)) unsigned long stream_disk_globals[5]{11,22,33,44,55};
#else
void (*stream_disk_slot)()=&event_app_canary;
#endif
#if defined(SAEX_CD_STREAM_ALLOCATION_FIXTURE)
__declspec(dllexport,align(4)) unsigned allocation_new_mode=
#if defined(SAEX_ALLOCATION_NEWH)
    1;
#else
    0;
#endif
__declspec(dllexport,align(4)) unsigned allocation_threshold=
#if defined(SAEX_ALLOCATION_BLOCKED)
    131072;
#else
    1016;
#endif
__declspec(dllexport,align(4)) unsigned allocation_heap_mode=
#if defined(SAEX_ALLOCATION_SBH) || defined(SAEX_ALLOCATION_BLOCKED)
    3;
#else
    1;
#endif
__declspec(dllexport) decltype(&HeapAlloc) allocation_api_slot=&HeapAlloc;
__declspec(dllexport,naked) void allocation_cleanup() { __asm call event_app_canary __asm ret }
extern __declspec(dllexport) const CwdScope allocation_scope{0xffffffffU,nullptr,&allocation_cleanup};
// The rejected SBH/new-handler branches end in a canary. Their bytes do not
// grant permission; the observer checks branch flags before either path.
__declspec(dllexport,naked) void allocation_heap() {
    __asm {
        push 0ch
        push offset allocation_scope
        call cwd_seh_prologue
        mov esi,[ebp+8]
        cmp dword ptr [allocation_heap_mode],3
        jne allocation_heap_tail
        cmp esi,[allocation_threshold]
        ja allocation_heap_tail
        call event_app_canary
    }
    __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop
    __asm {
    allocation_heap_tail:
        test esi,esi
        jne allocation_heap_size
        inc esi
    allocation_heap_size:
        cmp dword ptr [allocation_heap_mode],1
        je allocation_heap_args
        add esi,0fh
        and esi,0fffffff0h
    allocation_heap_args:
        push esi
        push 0
        push dword ptr [allocation_heap_handle]
        call dword ptr [allocation_api_slot]
        call cwd_seh_epilogue
        ret
    }
}
__declspec(dllexport,naked) void allocation_nh() {
    __asm {
        cmp dword ptr [esp+4],0ffffffe0h
        ja allocation_nh_null
        push dword ptr [esp+4]
        call allocation_heap
        test eax,eax
        pop ecx
        jne allocation_nh_return
        call event_app_canary
    }
    __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop
    __asm {
    allocation_nh_null:
        xor eax,eax
    allocation_nh_return:
        ret
    }
}
__declspec(dllexport,naked) void allocation_malloc() {
    __asm {
        push dword ptr [allocation_new_mode]
        push dword ptr [esp+8]
        call allocation_nh
        pop ecx
        pop ecx
        ret
    }
}
__declspec(dllexport,naked) void allocation_aligned() {
    __asm {
        mov eax,[esp+4]
        push esi
        mov esi,[esp+0ch]
        add eax,esi
        push eax
        call allocation_malloc
        mov ecx,eax
        lea eax,[ecx+esi]
        add esp,4
        dec esi
        not esi
        and eax,esi
        mov [eax-4],ecx
        pop esi
        ret
    }
}
#endif
__declspec(naked) void stream_tables_target() {
    __asm {
        sub esp,10h
        push edi
        mov ecx,20h
        xor eax,eax
        mov edi,offset stream_table_window+4
        mov edx,offset stream_table_window+140
        rep stosd
        jmp stream_names_loop
        _emit 0x8d
        _emit 0xa4
        _emit 0x24
        _emit 0
        _emit 0
        _emit 0
        _emit 0
    stream_names_loop:
        mov byte ptr [edx],0
        add edx,40h
        cmp edx,offset stream_table_window+2188
        jl stream_names_loop
        lea eax,[esp+8]
        push eax
        lea ecx,[esp+10h]
        push ecx
        lea edx,[esp+0ch]
        push edx
        lea eax,[esp+1ch]
        push eax
        push 0
        call dword ptr [stream_disk_slot]
    }
#if defined(SAEX_CD_STREAM_DISK_FIXTURE)
    __asm {
        mov ecx,[esp+4]
        xor eax,eax
        cmp ecx,800h
        ja stream_large_sector
        mov eax,20000000h
    stream_large_sector:
        push esi
        push 0
        push ecx
        or eax,40000000h
        push 800h
        mov dword ptr [stream_disk_globals+12],1
        mov dword ptr [stream_disk_globals+4],eax
        mov dword ptr [stream_disk_globals+8],0
    }
#endif
#if defined(SAEX_CD_STREAM_ALLOCATION_FIXTURE)
    __asm call allocation_aligned
#endif
#if defined(SAEX_CD_STREAM_CHANNELS_FIXTURE)
    __asm {
        add esp,0ch
        push 0
        mov edi,eax
        call dword ptr [channels_error_slot]
        mov eax,[esp+1ch]
        lea ecx,[eax+eax*2]
        shl ecx,4
        push ecx
        push 40h
        mov dword ptr [stream_table_window+136],0
        mov dword ptr [stream_table_window+132],eax
        call dword ptr [channels_local_slot]
        push 0
        push offset channels_filename
        mov dword ptr [channels_pointer_window+4],eax
    }
#endif
    __asm call event_app_canary
}
#endif
__declspec(dllexport,naked) void routing_initializer() {
    __asm call prelude_empty
    __asm call prelude_localisation
#if defined(SAEX_FILE_MANAGER_FIXTURE)
    __asm call file_manager_target
#else
    __asm call event_app_canary
#endif
#if defined(SAEX_CD_STREAM_TABLES_FIXTURE)
    __asm push 5
    __asm call stream_tables_target
#endif
    PREFIX
#if defined(SAEX_FILE_MANAGER_READY_FIXTURE)
    __asm call event_app_canary
#endif
    __asm ret
}
#else
__declspec(dllexport,naked) void routing_initializer() { __asm call event_app_canary PREFIX __asm ret }
#endif
__declspec(dllexport,naked) void routing_initialize_call() { __asm call routing_initializer __asm ret }
__declspec(dllexport,naked) void routing_default() { __asm mov eax,2 __asm ret }
__declspec(dllexport) unsigned char routing_indices[39]{
    0,10,10,10,1,10,10,10,10,2,10,10,10,10,10,10,10,10,10,10,10,3,4,10,5,6,7,8,10,10,10,10,10,10,10,10,10,10,9};
__declspec(dllexport) void (*routing_table[11])(){&routing_default,&routing_default,&routing_default,&routing_default,&routing_default,
    &routing_initialize_call,&routing_default,&routing_default,&routing_default,&routing_default,&routing_default};
__declspec(dllexport) void routing_detour();
// MSVC widens this cross-function conditional branch to the required six-byte JA.
// The fixture's exact opcode/rel32 shape is independently checked before execution.
#pragma warning(push)
#pragma warning(disable:4414)
__declspec(dllexport,naked) void event_app_target() {
    __asm {
        mov eax,[esp+4]
        cmp eax,26h
        ja routing_default
        nop
        nop
        jmp routing_detour
        jmp dword ptr [eax*4+routing_table]
    }
}
#pragma warning(pop)
__declspec(dllexport,naked) void routing_detour() {
    __asm {
        movzx eax,byte ptr [eax+routing_indices]
        jmp event_app_target+20
    }
}
#else
__declspec(dllexport,naked) void event_app_target() { __asm call event_app_canary PREFIX __asm ret }
#endif
__declspec(dllexport,naked) void event_dispatch_target() {
    __asm {
        push esi
        mov esi,[esp+0ch]
        push edi
        mov edi,[esp+0ch]
        push esi
        push edi
        call event_app_target
        add esp,8
        pop edi
        pop esi
        ret
    }
}
#endif
#if defined(SAEX_INSTANCE_FIXTURE)
#if defined(SAEX_CD_STREAM_CHANNELS_FIXTURE)
__declspec(dllexport) char instance_name[]="Local\\SAEX.CdStreamChannelsFixture.v1";
#elif defined(SAEX_CD_STREAM_ALLOCATION_FIXTURE)
__declspec(dllexport) char instance_name[]="Local\\SAEX.CdStreamAllocationFixture.v1";
#elif defined(SAEX_CD_STREAM_DISK_FIXTURE)
__declspec(dllexport) char instance_name[]="Local\\SAEX.CdStreamDiskFixture.v1";
#elif defined(SAEX_CD_STREAM_TABLES_FIXTURE)
__declspec(dllexport) char instance_name[]="Local\\SAEX.CdStreamTablesFixture.v1";
#elif defined(SAEX_FILE_MANAGER_READY_FIXTURE)
__declspec(dllexport) char instance_name[]="Local\\SAEX.FileManagerReadyFixture.v1";
#elif defined(SAEX_CWD_RETURN_FIXTURE)
__declspec(dllexport) char instance_name[]="Local\\SAEX.CwdReturnFixture.v1";
#elif defined(SAEX_CWD_COPY_FIXTURE)
__declspec(dllexport) char instance_name[]="Local\\SAEX.CwdCopyFixture.v1";
#elif defined(SAEX_CWD_QUERY_FIXTURE)
__declspec(dllexport) char instance_name[]="Local\\SAEX.CwdQueryFixture.v1";
#elif defined(SAEX_CWD_ACQUIRE_FIXTURE)
__declspec(dllexport) char instance_name[]="Local\\SAEX.CwdAcquireFixture.v1";
#elif defined(SAEX_CWD_LOCK_FIXTURE)
__declspec(dllexport) char instance_name[]="Local\\SAEX.CwdLockFixture.v1";
#elif defined(SAEX_CWD_SEH_FIXTURE)
__declspec(dllexport) char instance_name[]="Local\\SAEX.CwdSehFixture.v1";
#elif defined(SAEX_FILE_MANAGER_FIXTURE)
__declspec(dllexport) char instance_name[]="Local\\SAEX.FileManagerFixture.v1";
#elif defined(SAEX_GAME_PRELUDE_FIXTURE)
__declspec(dllexport) char instance_name[]="Local\\SAEX.GamePreludeFixture.v1";
#elif defined(SAEX_APPLICATION_ROUTING_FIXTURE)
__declspec(dllexport) char instance_name[]="Local\\SAEX.ApplicationRoutingFixture.v1";
#elif defined(SAEX_EVENT_DISPATCH_FIXTURE)
__declspec(dllexport) char instance_name[]="Local\\SAEX.EventDispatchFixture.v1";
#else
__declspec(dllexport) char instance_name[]="Local\\SAEX.InstanceFixture.v1";
#endif
__declspec(dllexport) char* instance_name_pointer=instance_name;
char* instance_window_title{};
DWORD* instance_window_object{};
void instance_window_canary() { mark_loader_phase(L".instance-window-called"); ExitProcess(94); }
FARPROC instance_find_window=reinterpret_cast<FARPROC>(&instance_window_canary);
FARPROC instance_foreground=reinterpret_cast<FARPROC>(&instance_window_canary);
__declspec(dllexport,naked) void instance_target() {
    __asm {
        mov eax,instance_name_pointer
        push eax
        push 1
        push 0
        push 0
        call CreateEventA
        call GetLastError
        cmp eax,0b7h
        jne instance_new
        mov ecx,instance_window_title
        mov edx,instance_name_pointer
        push ecx
        push edx
        call dword ptr [instance_find_window]
        test eax,eax
        jz instance_fallback
        push eax
        call dword ptr [instance_foreground]
        mov eax,1
        ret
    instance_fallback:
        mov eax,instance_window_object
        mov ecx,[eax]
        push ecx
        call dword ptr [instance_foreground]
        mov eax,1
        ret
    instance_new:
        xor eax,eax
        ret
    }
}
#endif
#if defined(SAEX_PLATFORM_FIXTURE)
__declspec(dllexport) FARPROC platform_call_slot{};
#if defined(SAEX_SUPPRESSION_FIXTURE)
__declspec(dllexport,naked) void suppression_last_error_call() { __asm call GetLastError __asm ret }
#endif
__declspec(dllexport,naked) void crt_application_target() {
#if defined(SAEX_PLATFORM_bad_stack)
    __asm sub esp,80h
#else
    __asm sub esp,84h
#endif
    __asm push ebx
#if defined(SAEX_PLATFORM_bad_args)
    __asm push 3
#else
    __asm push 2
#endif
    __asm xor ebx,ebx
    __asm push ebx
    __asm push ebx
    __asm push 2001h
#if defined(SAEX_PLATFORM_bad_flag)
    __asm mov byte ptr [esp+17h],0
#else
    __asm mov byte ptr [esp+17h],1
#endif
#if defined(SAEX_PLATFORM_bad_saved)
    __asm inc dword ptr [esp+10h]
#elif defined(SAEX_PLATFORM_bad_register)
    __asm inc esi
#elif defined(SAEX_PLATFORM_fault)
    __asm int 3
#endif
    __asm call dword ptr [platform_call_slot]
#if defined(SAEX_INSTANCE_FIXTURE)
    __asm call instance_target
#endif
#if defined(SAEX_EVENT_DISPATCH_FIXTURE)
    __asm {
        test eax,eax
        jne event_exit
        push ebx
        push 18h
        call event_dispatch_target
        add esp,8
        test eax,eax
        jne event_after
    event_exit:
        xor eax,eax
        pop ebx
        add esp,84h
        ret 10h
    event_after:
    }
#endif
    PREFIX
    __asm int 3
}
#else
__declspec(dllexport,naked) void crt_application_target() { __asm call application_canary PREFIX __asm int 3 }
#endif
__declspec(dllexport,naked) void crt_application_call() { __asm call crt_application_target __asm int 3 }
__declspec(dllexport) void (*crt_table[3])(){nullptr,&frame_target,&crt_application_target};
DWORD application_show{},application_instance{}; char* application_command{};
void application_arguments(STARTUPINFOA* info) {
    application_show=(info->dwFlags & STARTF_USESHOWWINDOW) ? info->wShowWindow : SW_SHOWDEFAULT;
    application_instance=reinterpret_cast<DWORD>(GetModuleHandleW(nullptr));
    application_command=GetCommandLineA();
#if defined(SAEX_APPLICATION_bad_instance)
    ++application_instance;
#elif defined(SAEX_APPLICATION_bad_show)
    ++application_show;
#elif defined(SAEX_APPLICATION_bad_command)
    application_command=reinterpret_cast<char*>(1);
#elif defined(SAEX_APPLICATION_long_command)
    application_command=static_cast<char*>(VirtualAlloc(nullptr,32768,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    if (!application_command) ExitProcess(97);
    for (unsigned i=0;i<32768;++i) application_command[i]='x';
#endif
}
__declspec(dllexport,naked) void application_second_call() {
    __asm call GetStartupInfoA
    PREFIX
    __asm {
        mov eax,esp
        push eax
        call application_arguments
        add esp,4
        add esp,44h
    }
#if defined(SAEX_SUPPRESSION_FIXTURE)
    __asm push 1234ABCDh
    __asm call SetLastError
#endif
    __asm {
        push application_show
        push application_command
        push 0
        push application_instance
        jmp crt_application_call
    }
}
__declspec(naked) void application_second_setup() {
    __asm sub esp,44h
    __asm mov eax,esp
#if defined(SAEX_APPLICATION_bad_second)
    __asm xor eax,eax
#endif
    __asm push eax
    __asm jmp application_second_call
}
unsigned application_initialize_work() {
    mark_loader_phase(L".application-initializer-entered");
#if defined(SAEX_CWD_ACQUIRE_FIXTURE)
    cwd_acquire_initialize();
#endif
#if defined(SAEX_CWD_LOCK_READONLY)
    DWORD lock_old_protection{};
    if(!VirtualProtect(cwd_lock_table,4096,PAGE_READONLY,&lock_old_protection))ExitProcess(97);
#endif
#if defined(SAEX_FILE_MANAGER_READONLY)
    // The startup proxy has made the image writable. Inject a real runtime
    // protection failure after that boundary, on this fixture's dedicated page.
    DWORD old_protection{};
    if (!VirtualProtect(const_cast<unsigned char*>(file_manager_buffer),4096,PAGE_READONLY,&old_protection)) ExitProcess(97);
#endif
#if defined(SAEX_PLATFORM_FIXTURE)
    platform_call_slot=GetProcAddress(GetModuleHandleW(L"saex_application_proxy.dll"),"platform_system_canary");
#if defined(SAEX_PLATFORM_bad_iat)
    platform_call_slot=reinterpret_cast<FARPROC>(reinterpret_cast<DWORD>(platform_call_slot)+1);
#endif
#endif
#if defined(SAEX_APPLICATION_crash)
    DebugBreak();
#elif defined(SAEX_APPLICATION_hang)
    Sleep(INFINITE);
#elif defined(SAEX_APPLICATION_table_drift)
    crt_table[0]=&frame_target;
#elif defined(SAEX_APPLICATION_once_drift)
    application_once=0;
#elif defined(SAEX_APPLICATION_candidate_drift)
    auto p=reinterpret_cast<unsigned char*>(&frame_target); p[15]^=1; FlushInstructionCache(GetCurrentProcess(),p,16);
#elif defined(SAEX_APPLICATION_unpinned)
    LoadLibraryW(L"saex_loader_fixture_dll.dll");
#endif
#if defined(SAEX_APPLICATION_failure)
    return 1;
#else
    return 0;
#endif
}
__declspec(dllexport,naked) void crt_initialize_target() {
    PREFIX
    __asm call application_initialize_work
#if defined(SAEX_APPLICATION_register_drift)
    __asm inc esi
#endif
    __asm ret
}
__declspec(dllexport,naked) void crt_initialize_call() {
    __asm call crt_initialize_target
    PREFIX
    __asm jmp application_second_setup
}
__declspec(dllexport,naked) void crt_io_target() {
    __asm {
        sub esp,48h
        push ebx
        push ebp
        push esi
        push edi
        lea eax,[esp+10h]
        push eax
        call GetStartupInfoA
        xor eax,eax
        pop edi
        pop esi
        pop ebp
        pop ebx
        add esp,48h
        ret
    }
}
__declspec(dllexport,naked) void crt_io_call() {
    __asm call crt_io_target
    PREFIX
    __asm jmp crt_initialize_call
}
#undef PREFIX
}
int main(int argc,char**) {
#if defined(SAEX_CWD_COPY_FIXTURE)
    if(argc==4)return cwd_copy_control();
#endif
#if defined(SAEX_APPLICATION_ROUTING_FIXTURE)
    if (argc==3) {
#if defined(SAEX_CWD_ACQUIRE_FIXTURE)
        cwd_acquire_initialize();
#endif
        __asm push 0
        __asm push 24
        __asm call event_dispatch_target
        __asm add esp,8
        return 91;
    }
#elif defined(SAEX_EVENT_DISPATCH_FIXTURE)
    if (argc==3) { event_dispatch_target(); return 91; }
#endif
#if defined(SAEX_INSTANCE_FIXTURE)
    if (argc==2) { instance_target(); return 93; } // Canary sensitivity control only.
#else
    (void)argc;
#endif
    crt_io_call(); proxy_iat_target(); return 95;
}
