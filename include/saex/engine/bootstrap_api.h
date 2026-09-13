#pragma once
#include <stdint.h>

#if defined(_WIN32)
#define SAEX_BOOTSTRAP_CALL __cdecl
#else
#define SAEX_BOOTSTRAP_CALL
#endif
#ifdef __cplusplus
extern "C" {
#define SAEX_BOOTSTRAP_NOEXCEPT noexcept
#else
#define SAEX_BOOTSTRAP_NOEXCEPT
#endif

#define SAEX_BOOTSTRAP_ABI_MAJOR 1u
enum SaexBootstrapResult {
    SAEX_BOOTSTRAP_OK = 0, SAEX_BOOTSTRAP_INVALID_ARGUMENT = 1,
    SAEX_BOOTSTRAP_ABI_MISMATCH = 2, SAEX_BOOTSTRAP_BUSY = 3
};
enum SaexBootstrapState {
    SAEX_BOOTSTRAP_DISCOVERED = 0, SAEX_BOOTSTRAP_REJECTED = 1,
    SAEX_BOOTSTRAP_OBSERVED_UNVERIFIED = 2, SAEX_BOOTSTRAP_STOPPED = 3
};
enum SaexBootstrapReason {
    SAEX_BOOTSTRAP_NONE = 0, SAEX_BOOTSTRAP_HOST_PATH = 1,
    SAEX_BOOTSTRAP_FILE_REJECTED = 2, SAEX_BOOTSTRAP_IMAGE_BASE = 3,
    SAEX_BOOTSTRAP_IMAGE_REJECTED = 4, SAEX_BOOTSTRAP_RUNTIME_UNVERIFIED = 5,
    SAEX_BOOTSTRAP_INTERNAL_ERROR = 6
};

#pragma pack(push, 4)
typedef struct SaexBootstrapStatus {
    uint32_t abi_major;
    uint32_t struct_bytes;
    uint32_t state;
    uint32_t reason;
    uint32_t observation_attempts;
    uint32_t can_attach;
    uint32_t bindings_loaded;
    uint32_t reserved;
    char observed_profile_id[96]; /* zero-terminated; empty until image observation */
    char profile_source_digest[64]; /* exactly 64 ASCII hex bytes, no terminator */
} SaexBootstrapStatus;
#pragma pack(pop)

// Trusted, same-process caller owns a writable buffer of exactly sizeof(status).
// Call after LoadLibrary returns, outside loader lock. Not a network/worker ABI.
uint32_t SAEX_BOOTSTRAP_CALL SaexBootstrapInitialize(uint32_t abi_major, SaexBootstrapStatus* status, uint32_t bytes) SAEX_BOOTSTRAP_NOEXCEPT;
uint32_t SAEX_BOOTSTRAP_CALL SaexBootstrapQuery(uint32_t abi_major, SaexBootstrapStatus* status, uint32_t bytes) SAEX_BOOTSTRAP_NOEXCEPT;
uint32_t SAEX_BOOTSTRAP_CALL SaexBootstrapStop(uint32_t abi_major, SaexBootstrapStatus* status, uint32_t bytes) SAEX_BOOTSTRAP_NOEXCEPT;
typedef uint32_t (SAEX_BOOTSTRAP_CALL *SaexBootstrapFunction)(uint32_t, SaexBootstrapStatus*, uint32_t);

#ifdef __cplusplus
}
static_assert(sizeof(SaexBootstrapStatus) == 192);
static_assert(alignof(SaexBootstrapStatus) == 4);
#endif
#undef SAEX_BOOTSTRAP_NOEXCEPT
