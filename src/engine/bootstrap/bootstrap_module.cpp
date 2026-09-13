#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "saex/engine/bootstrap_session.hpp"
#include "saex/engine/observed_profile.generated.hpp"
#include "saex/engine/windows_file_observation.hpp"
#include "saex/engine/windows_image_reader.hpp"
#include <array>

namespace {
static_assert(sizeof(void*) == 4, "GTA bootstrap must be x86");
constinit saex::engine::BootstrapSession session;

class CurrentProcessObservation final : public saex::engine::BootstrapObservation {
public:
    std::uint32_t inspect() noexcept override {
        using namespace saex::engine;
        try {
            // Caller cannot supply a different executable path or arbitrary image base.
            std::array<wchar_t, 32768> path{};
            const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
            if (!length || length >= path.size()) return SAEX_BOOTSTRAP_HOST_PATH;
            WindowsFileObservation file(path.data());
            if (!file.error().empty()) return SAEX_BOOTSTRAP_FILE_REJECTED;
            const auto base = GetModuleHandleW(nullptr);
            if (reinterpret_cast<std::uintptr_t>(base) != file.layout().image_base) return SAEX_BOOTSTRAP_IMAGE_BASE;
            const WindowsImageReader reader(base, file.layout().image_size);
            // No relocation/unpack relaxation: this observed profile has no phase evidence.
            if (check_mapped_observation(reader, file.bytes(), file.layout(), observed_profile) != ProfileResult::matched_observation)
                return SAEX_BOOTSTRAP_IMAGE_REJECTED;
            return SAEX_BOOTSTRAP_RUNTIME_UNVERIFIED;
        } catch (...) { return SAEX_BOOTSTRAP_INTERNAL_ERROR; }
    }
};
std::uint32_t invoke(saex::engine::BootstrapOperation operation, std::uint32_t abi,
    SaexBootstrapStatus* output, std::uint32_t bytes) noexcept {
    CurrentProcessObservation observation;
    return session.invoke(operation, observation, abi, output, bytes);
}
}

// CRT initialization exists; SAEX performs no work in the loader-lock callback.
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
extern "C" std::uint32_t SAEX_BOOTSTRAP_CALL SaexBootstrapInitialize(std::uint32_t abi, SaexBootstrapStatus* out, std::uint32_t bytes) noexcept {
    return invoke(saex::engine::BootstrapOperation::initialize, abi, out, bytes);
}
extern "C" std::uint32_t SAEX_BOOTSTRAP_CALL SaexBootstrapQuery(std::uint32_t abi, SaexBootstrapStatus* out, std::uint32_t bytes) noexcept {
    return invoke(saex::engine::BootstrapOperation::query, abi, out, bytes);
}
extern "C" std::uint32_t SAEX_BOOTSTRAP_CALL SaexBootstrapStop(std::uint32_t abi, SaexBootstrapStatus* out, std::uint32_t bytes) noexcept {
    return invoke(saex::engine::BootstrapOperation::stop, abi, out, bytes);
}
