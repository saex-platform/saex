#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "saex/engine/observed_profile.generated.hpp"
#include "saex/engine/suspended_image.hpp"
#include "saex/engine/windows_file_observation.hpp"
#include <iostream>
#include <string>
#include <stdexcept>

namespace {
std::string hex(const saex::engine::Sha256& hash) {
    constexpr char alphabet[] = "0123456789abcdef";
    std::string result;
    for (const auto byte : hash) {
        const auto value = std::to_integer<unsigned>(byte);
        result += alphabet[value >> 4U]; result += alphabet[value & 15U];
    }
    return result;
}
int reject(std::string_view reason, bool created = false, bool stopped = true, DWORD system_error = 0) {
    std::cout << "{\"scope\":\"created-suspended-process-observation\",\"canAttach\":false,\"childCreated\":"
        << (created ? "true" : "false") << ",\"childExitConfirmed\":" << (created && stopped ? "true" : "false")
        << ",\"systemError\":" << system_error << ",\"reason\":\"" << reason << "\"}\n";
    return 1;
}
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 3 || std::wstring_view(argv[1]) != L"--observe-suspended") {
        std::cerr << "Usage: saex_engine_process_probe --observe-suspended <gta_sa.exe>\n"; return 2;
    }
    using namespace saex::engine;
    try {
        WindowsFileObservation file(argv[2]);
        if (!file.error().empty()) return reject(file.error());
        const auto file_hash = hex(file.hash()); // Allocate before the child exists.
        SuspendedImage child(argv[2], file.layout().image_size);
        auto reason = child.error();
        const auto system_error = child.system_error();
        if (reason.empty() && !child.same_file(file.handle())) reason = "observer_file_identity";
        if (reason.empty() && child.image_base() != file.layout().image_base) reason = "observer_image_base";
        if (reason.empty()) {
            const auto result = check_mapped_observation(child, file.bytes(), file.layout(), observed_profile);
            if (result != ProfileResult::matched_observation) reason = profile_result_name(result);
        }
        const bool stopped = child.stop();
        if (!stopped) return reject("observer_exit_unconfirmed", child.created(), false, system_error);
        if (!reason.empty()) return reject(reason, child.created(), stopped, system_error);
        std::cout << "{\"scope\":\"created-suspended-process-observation\",\"observedProfileId\":\"" << observed_profile.id
            << "\",\"sha256\":\"" << file_hash << "\",\"observerPointerBits\":" << sizeof(void*) * 8
            << ",\"profileSourceDigest\":\"" << observed_profile_source_digest << "\",\"childCreated\":true,\"childPid\":" << child.process_id()
            << ",\"imageBase\":" << child.image_base() << ",\"fileIdentityMatched\":true,\"mappedHeadersAndAnchorsMatched\":true,\"anchorsChecked\":"
            << observed_anchors.size() << ",\"primaryThreadResumed\":false,\"childExitConfirmed\":true,\"canAttach\":false,\"reason\":\"pre_user_code_observation_only\"}\n";
        return 3;
    } catch (const std::exception&) { return reject("observer_exception"); }
}
