#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "saex/engine/observed_profile.generated.hpp"
#include "saex/engine/windows_image_reader.hpp"
#include "saex/engine/windows_file_observation.hpp"
#include <iostream>
#include <stdexcept>

namespace {
class Handle {
public:
    explicit Handle(HANDLE value) : value_(value) {}
    ~Handle() { if (value_ && value_ != INVALID_HANDLE_VALUE) CloseHandle(value_); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    HANDLE get() const { return value_; }
private:
    HANDLE value_;
};
class View {
public:
    explicit View(void* value) : value_(value) {}
    ~View() { if (value_) UnmapViewOfFile(value_); }
    View(const View&) = delete;
    View& operator=(const View&) = delete;
    void* get() const { return value_; }
private:
    void* value_;
};
std::string hex(const saex::engine::Sha256& hash) {
    constexpr char alphabet[] = "0123456789abcdef";
    std::string result;
    for (const auto b : hash) {
        const auto value = std::to_integer<unsigned>(b);
        result += alphabet[value >> 4U]; result += alphabet[value & 15U];
    }
    return result;
}
int reject(std::string_view reason) {
    std::cout << "{\"scope\":\"non-executing-image-observation\",\"canAttach\":false,\"reason\":\"" << reason << "\"}\n";
    return 1;
}
}

int wmain(int argc, wchar_t** argv) {
    using namespace saex::engine;
    if (argc != 2) { std::cerr << "Usage: saex_engine_image_probe <gta_sa.exe>\n"; return 2; }
    try {
        WindowsFileObservation file(argv[1]);
        // Unknown bytes never reach the OS image mapping path, much less SDK/native calls.
        if (!file.error().empty()) return reject(file.error());
        Handle mapping(CreateFileMappingW(file.handle(), nullptr, PAGE_READONLY | SEC_IMAGE_NO_EXECUTE, 0, 0, nullptr));
        if (!mapping.get()) return reject("non_executing_mapping_failed");
        View view(MapViewOfFile(mapping.get(), FILE_MAP_READ, 0, 0, 0));
        if (!view.get()) return reject("image_view_failed");
        const WindowsImageReader reader(view.get(), file.layout().image_size);
        const auto mapped = check_mapped_observation(reader, file.bytes(), file.layout(), observed_profile);
        if (mapped != ProfileResult::matched_observation) return reject(profile_result_name(mapped));
        std::cout << "{\"scope\":\"non-executing-image-observation\",\"observedProfileId\":\"" << observed_profile.id
            << "\",\"sha256\":\"" << hex(file.hash()) << "\",\"profileSourceDigest\":\"" << observed_profile_source_digest
            << "\",\"fileProfileMatched\":true,\"mappedHeadersAndAnchorsMatched\":true,\"anchorsChecked\":" << observed_anchors.size()
            << ",\"processPointerBits\":" << sizeof(void*) * 8 << ",\"canAttach\":false,\"reason\":\"observed_profile_runtime_unverified\"}\n";
        return 3; // Observation succeeded; runtime support remains closed.
    }
    catch (const std::exception&) { return reject("inspection_failed"); }
}
