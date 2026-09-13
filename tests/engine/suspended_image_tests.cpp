#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "saex/engine/suspended_image.hpp"
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

using namespace saex::engine;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct Handle {
    HANDLE value{};
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    explicit Handle(HANDLE h = nullptr) : value(h) {}
};
// Trusted compiler-produced PE32/PE32+ fixture only; production uses the strict PE32 parser.
std::uint32_t image_size(HANDLE file) {
    std::array<std::byte, 4096> bytes{}; DWORD read{};
    require(ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) && read == bytes.size(), "fixture read");
    std::uint32_t pe{}, result{};
    std::memcpy(&pe, bytes.data() + 60, 4);
    require(pe < bytes.size() - 84 && bytes[0] == std::byte{'M'} && bytes[1] == std::byte{'Z'}, "fixture PE bounds");
    std::memcpy(&result, bytes.data() + pe + 24 + 56, 4);
    require(result && result <= max_image_bytes, "fixture image size");
    return result;
}
bool marker_exists(const std::wstring& marker) { return GetFileAttributesW(marker.c_str()) != INVALID_FILE_ATTRIBUTES; }
void exited(HANDLE process) {
    require(WaitForSingleObject(process, 1000) == WAIT_OBJECT_0, "child survived cleanup");
    DWORD code{};
    require(GetExitCodeProcess(process, &code) && code == 0x53414558, "child did not exit through observer termination");
}
void positive_control(const wchar_t* path, const std::wstring& marker) {
    require(!marker_exists(marker), "stale fixture canary requires inspection");
    auto command = L"\"" + std::wstring(path) + L"\"";
    STARTUPINFOW startup{}; startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    require(CreateProcessW(path, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process) != 0, "canary control create");
    Handle p(process.hProcess), t(process.hThread);
    if (WaitForSingleObject(p.value, 5000) != WAIT_OBJECT_0) {
        TerminateProcess(p.value, 74); WaitForSingleObject(p.value, 5000);
        throw std::runtime_error("canary control timeout");
    }
    DWORD code{};
    require(GetExitCodeProcess(p.value, &code) && code == 73 && marker_exists(marker), "canary control failed");
    require(DeleteFileW(marker.c_str()) != 0, "canary control cleanup");
}
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    try {
        const auto marker = std::wstring(argv[1]) + L".executed";
        positive_control(argv[1], marker);
        Handle file(CreateFileW(argv[1], GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        Handle other(CreateFileW(argv[0], GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        require(file.value != INVALID_HANDLE_VALUE && other.value != INVALID_HANDLE_VALUE, "open fixtures");
        const auto size = image_size(file.value);
        for (const auto invalid_size : {std::size_t{0}, max_image_bytes + 1}) {
            SuspendedImage invalid(argv[1], invalid_size);
            require(!invalid.created() && invalid.error() == "invalid_observer_input" && invalid.stop(), "invalid size created child");
        }
        SuspendedImage null_path(nullptr, size);
        require(!null_path.created() && null_path.error() == "invalid_observer_input", "null path created child");
        const auto missing = std::wstring(argv[1]) + L".missing";
        SuspendedImage absent(missing.c_str(), size);
        require(!absent.created() && absent.error() == "observer_create_failed" && absent.stop(), "missing file created child");
        DWORD handles_before{}; require(GetProcessHandleCount(GetCurrentProcess(), &handles_before) != 0, "handle baseline");
        for (unsigned i = 0; i < 12; ++i) {
            SuspendedImage child(argv[1], size);
            if (!child.error().empty()) {
                std::cerr << child.error() << " systemError=" << child.system_error() << '\n';
                throw std::runtime_error("observer creation failed");
            }
            require(child.created() && child.process_id() != GetCurrentProcessId(), "child identity");
            Handle process(OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, child.process_id()));
            require(process.value != nullptr && WaitForSingleObject(process.value, 0) == WAIT_TIMEOUT, "child is not held alive");
            require(child.same_file(file.value) && !child.same_file(other.value) && !child.same_file(nullptr), "file identity mismatch");
            std::array<std::byte, 2> mz{};
            require(child.copy(0, mz) && mz[0] == std::byte{'M'} && mz[1] == std::byte{'Z'}, "mapped header read");
            std::array<std::byte, 1025> excessive{};
            require(!child.copy(0, excessive) && !child.copy(0, {}) && !child.copy(size - 1, mz) &&
                !child.copy(size, mz) && !child.copy(0xffffffffU, mz), "unbounded read accepted");
            bool wrong_thread_read{}, wrong_thread_file{}, wrong_thread_stop{};
            std::thread wrong([&] {
                wrong_thread_read = child.copy(0, mz); wrong_thread_file = child.same_file(file.value); wrong_thread_stop = child.stop();
            });
            wrong.join();
            require(!wrong_thread_read && !wrong_thread_file && !wrong_thread_stop && !child.stopped(), "foreign thread mutated observer");
            require(!marker_exists(marker), "fixture main ran during observation");
            require(child.stop() && child.stopped() && child.stop(), "explicit/idempotent stop");
            require(!child.copy(0, mz) && !child.same_file(file.value), "read after termination");
            exited(process.value);
            require(!marker_exists(marker), "fixture main ran during cleanup");
        }
        DWORD handles_after{}; require(GetProcessHandleCount(GetCurrentProcess(), &handles_after) != 0 && handles_after == handles_before, "observer handle growth");
        Handle unwound;
        bool exception_caught{};
        try {
            SuspendedImage child(argv[1], size);
            require(child.error().empty(), "unwind fixture create");
            unwound.value = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, child.process_id());
            require(unwound.value != nullptr, "unwind child handle");
            throw 42;
        } catch (int value) { require(value == 42, "unexpected exception"); exception_caught = true; }
        require(exception_caught, "exception not raised");
        exited(unwound.value);
        require(!marker_exists(marker), "fixture ran on scope unwind");
        std::cout << "PASS suspended observer: positive canary control, invalid input, 12 lifecycle cycles, identity/bounds/thread/stop, exception cleanup; handles "
            << handles_before << " -> " << handles_after << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
