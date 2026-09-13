#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "saex/engine/launch_context.hpp"
#include "saex/engine/suspended_image.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace saex::engine;
namespace {
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
struct Handle {
    HANDLE value{};
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};
struct Directory {
    std::wstring path;
    Directory() {
        std::array<wchar_t, MAX_PATH> temp{};
        const auto size = GetTempPathW(static_cast<DWORD>(temp.size()), temp.data());
        require(size && size < temp.size(), "temp path");
        path = std::wstring(temp.data()) + L"saex context çığ " + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
        require(CreateDirectoryW(path.c_str(), nullptr) != 0, "unique directory");
    }
    ~Directory() { RemoveDirectoryW(path.c_str()); }
};
struct SavedVariable {
    std::wstring name, old;
    bool present{};
    explicit SavedVariable(const wchar_t* value) : name(value) {
        std::array<wchar_t, 32768> buffer{};
        SetLastError(ERROR_SUCCESS);
        const auto size = GetEnvironmentVariableW(value, buffer.data(), static_cast<DWORD>(buffer.size()));
        require(size < buffer.size(), "environment read bound");
        present = size != 0 || GetLastError() != ERROR_ENVVAR_NOT_FOUND;
        old.assign(buffer.data(), size);
    }
    ~SavedVariable() { SetEnvironmentVariableW(name.c_str(), present ? old.c_str() : nullptr); }
};
bool absent(const std::wstring& directory) { return GetFileAttributesW((directory + L"\\context.received").c_str()) == INVALID_FILE_ATTRIBUTES; }
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    try {
        Directory directory;
        const std::array<std::wstring_view, 3> entries{L"z=", L"PATH=one=two", L"a=çığ"};
        LaunchContext first(directory.path, entries);
        const std::array<std::wstring_view, 3> reversed{entries[2], entries[0], entries[1]};
        LaunchContext second(directory.path, reversed);
        require(first.valid() && second.valid() && first.directory_matches(), "prepared directory");
        require(first.environment_hash() == second.environment_hash() && first.entry_count() == 3, "canonical environment order");
        auto changed = entries; changed[1] = L"PATH=other";
        LaunchContext other(directory.path, changed);
        require(other.valid() && first.environment_hash() != other.environment_hash(), "environment value hash");
        LaunchContext empty(directory.path, std::span<const std::wstring_view>{});
        require(empty.valid() && empty.environment().size() == 2 && empty.environment()[0] == 0 && empty.environment()[1] == 0, "empty double-null block");
        const std::array<std::wstring_view, 1> drive{L"=C:=C:\\local path"};
        LaunchContext drive_context(directory.path, drive);
        require(drive_context.valid(), "per-drive entry");
        const auto reject = [&](std::span<const std::wstring_view> values, std::string_view reason) {
            LaunchContext invalid(directory.path, values);
            require(!invalid.valid() && invalid.error() == reason && invalid.environment().empty(), "invalid environment accepted or partial block published");
            SuspendedImage child(argv[1], 4096, &invalid);
            require(!child.created() && child.error() == "observer_invalid_context" && child.stop(), "invalid context created child");
        };
        const std::array<std::wstring_view, 2> duplicate{L"Path=x", L"PATH=y"};
        reject(duplicate, "launch_environment_duplicate");
        const std::wstring nul(L"x=ok\0hidden=1", 13);
        for (const auto value : {std::wstring_view(L"missing-equal"), std::wstring_view(L"=bad"), std::wstring_view(L"=C:=relative"), std::wstring_view(nul)}) {
            const std::array values{value}; reject(values, "launch_environment_entry");
        }
        std::vector<std::wstring_view> excessive(max_environment_entries + 1, L"x=1");
        reject(excessive, "launch_environment_limit");
        const std::wstring large = L"x=" + std::wstring(32765, L'a');
        const std::wstring large2 = L"y=" + std::wstring(32765, L'a');
        const std::array<std::wstring_view, 2> large_entries{large, large2};
        reject(large_entries, "launch_environment_limit");
        for (const auto value : {L"", L"relative", L"C:relative", L"\\\\server\\share"}) {
            LaunchContext invalid(value, entries);
            require(!invalid.valid() && invalid.error() == "launch_directory_input", "ambiguous directory accepted");
        }
        LaunchContext missing(directory.path + L"\\missing", entries);
        require(!missing.valid() && missing.error() == "launch_directory_unavailable", "missing directory accepted");
        LaunchContext not_directory(argv[1], entries);
        require(!not_directory.valid() && not_directory.error() == "launch_directory_unavailable", "file accepted as directory");
        require(!MoveFileW(directory.path.c_str(), (directory.path + L" renamed").c_str()), "retained directory allowed rename");

        SavedVariable token(L"SAEX_CONTEXT_TOKEN"), expected(L"SAEX_CONTEXT_EXPECTED_DIRECTORY");
        require(SetEnvironmentVariableW(token.name.c_str(), L"sabit-çığ=1") &&
            SetEnvironmentVariableW(expected.name.c_str(), directory.path.c_str()), "set owned fixture environment");
        LaunchContext snapshot(directory.path);
        require(snapshot.valid(), "capture real environment");
        require(SetEnvironmentVariableW(token.name.c_str(), L"changed-after-snapshot") != 0, "change fixture parent environment");
        LaunchContext later(directory.path);
        require(later.valid() && later.environment_hash() != snapshot.environment_hash(), "capture ignored parent mutation");
        auto command = L"\"" + std::wstring(argv[1]) + L"\"";
        std::vector<wchar_t> block(snapshot.environment().begin(), snapshot.environment().end());
        STARTUPINFOW startup{}; startup.cb = sizeof(startup);
        PROCESS_INFORMATION process{};
        require(CreateProcessW(argv[1], command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
            block.data(), snapshot.directory().c_str(), &startup, &process) != 0, "context fixture launch");
        Handle p{process.hProcess}, t{process.hThread};
        if (WaitForSingleObject(p.value, 5000) != WAIT_OBJECT_0) {
            TerminateProcess(p.value, 80); WaitForSingleObject(p.value, 5000); throw std::runtime_error("context fixture timeout");
        }
        DWORD exit{};
        require(GetExitCodeProcess(p.value, &exit) && exit == 73 && !absent(directory.path), "child received wrong cwd or environment");
        require(DeleteFileW((directory.path + L"\\context.received").c_str()) != 0, "owned marker cleanup");
        const auto cycle = [&] {
            LaunchContext iteration(directory.path);
            SuspendedImage held(argv[1], 4096, &iteration);
            require(held.error().empty() && held.created() && held.stop() && held.stopped() && absent(directory.path), "explicit context lost held-start boundary");
        };
        DWORD cold{}; require(GetProcessHandleCount(GetCurrentProcess(), &cold) != 0, "cold context handles");
        cycle(); // First use can initialize process-wide Windows debugger resources.
        DWORD before{}; require(GetProcessHandleCount(GetCurrentProcess(), &before) != 0, "context handle baseline");
        for (unsigned i = 0; i < 12; ++i) cycle();
        DWORD after{};
        require(GetProcessHandleCount(GetCurrentProcess(), &after) && before == after, "context handle growth");
        std::cout << "PASS launch context: canonical hash, Unicode/empty/drive entries, limits/duplicates/NUL/path rejection, snapshot freeze, child cwd/environment, held child, 12 warm cycles; cold/warm/final handles " << cold << '/' << before << '/' << after << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
