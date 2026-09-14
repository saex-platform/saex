#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "saex/engine/loader_observation.hpp"
#include "saex/engine/entry_policy.generated.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace saex::engine;
namespace {
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
struct Markers {
    std::wstring path;
    static constexpr const wchar_t* suffixes[]{L".dll-entered", L".proxy-entered", L".main-entered", L".startup-entered",
        L".codec-leaf-entered", L".codec-dll-entered", L".codec-after-return", L".binding-after-stop", L".binding-function-called",
        L".asi-entered", L".asi-after-return", L".asi-end-escaped", L".asi-export-called",
        L".startup-after-call", L".tail-after-asi", L".tail-after-protect",
        L".crt-after-startup", L".crt-io-returned", L".crt-initializer-entered", L".application-initializer-entered", L".application-body-entered", L".platform-api-called", L".instance-window-called", L".event-app-called"};
    explicit Markers(const wchar_t* source) : path(source) {
        for (auto suffix : suffixes) require(!exists(suffix), "preexisting marker");
    }
    bool exists(const wchar_t* suffix) const { return GetFileAttributesW((path + suffix).c_str()) != INVALID_FILE_ATTRIBUTES; }
    ~Markers() { for (auto suffix : suffixes) DeleteFileW((path + suffix).c_str()); }
};
// Disk-only test metadata reader. No DLL is loaded into the test runner.
struct Fixture {
    LoaderFile file;
    std::vector<char> bytes;
    PeLayout layout{};
    std::uint32_t pe{};
    explicit Fixture(const wchar_t* path, bool dll = false, bool allow_no_entry = false) : file(path) {
        require(file.valid(), "fixture pin");
        std::ifstream stream(std::filesystem::path(path), std::ios::binary);
        bytes.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
        pe = raw<DWORD>(60);
        auto parsing = bytes;
        // parse_pe32 intentionally accepts only EXEs. Clear the DLL bit in a private
        // metadata copy for this fixture helper; never change the pinned file.
        if (dll) parsing.at(pe + 23) &= static_cast<char>(~0x20);
        auto parsed = parse_pe32(std::as_bytes(std::span(parsing)));
        // Native system DLLs such as ntdll may have no entry point. Only the
        // explicit disk-metadata mode substitutes one in the private parse copy.
        const auto original_entry = raw<DWORD>(pe + 24 + 16);
        if (dll && allow_no_entry && !original_entry && parsed.error == PeError::entry_point) {
            const auto table = pe + 24 + raw<WORD>(pe + 20);
            for (unsigned i = 0; i < raw<WORD>(pe + 6); ++i) {
                const auto section = raw<IMAGE_SECTION_HEADER>(table + i * 40);
                if (!(section.Characteristics & IMAGE_SCN_MEM_EXECUTE) || !section.SizeOfRawData) continue;
                std::memcpy(parsing.data() + pe + 24 + 16, &section.VirtualAddress, sizeof(DWORD));
                parsed = parse_pe32(std::as_bytes(std::span(parsing))); break;
            }
        }
        if (parsed.error != PeError::none)
            throw std::runtime_error("fixture PE layout: " + std::filesystem::path(path).filename().string() +
                ": " + std::string(pe_error_name(parsed.error)));
        layout = parsed.layout; layout.entry_rva = original_entry;
    }
    template<class T> T raw(std::size_t offset) const {
        require(offset <= bytes.size() && sizeof(T) <= bytes.size() - offset, "fixture raw range");
        T result{}; std::memcpy(&result, bytes.data() + offset, sizeof(T)); return result;
    }
    template<class T> T at(std::uint32_t rva) const {
        const auto offset = raw_offset(layout, rva, sizeof(T)); require(offset.has_value(), "fixture RVA"); return raw<T>(*offset);
    }
    std::string name(std::uint32_t rva) const {
        std::string result;
        for (unsigned i = 0; i < 512; ++i) { const auto c = at<char>(rva + i); if (!c) return result; result += c; }
        throw std::runtime_error("fixture name bound");
    }
    std::uint32_t symbol(std::string_view wanted) const {
        const auto exports = at<IMAGE_EXPORT_DIRECTORY>(raw<DWORD>(pe + 24 + 96));
        require(exports.NumberOfNames <= 64, "fixture export bound");
        for (DWORD i = 0; i < exports.NumberOfNames; ++i)
            if (name(at<DWORD>(exports.AddressOfNames + 4 * i)) == wanted)
                return at<DWORD>(exports.AddressOfFunctions + 4 * at<WORD>(exports.AddressOfNameOrdinals + 2 * i));
        throw std::runtime_error("fixture export missing");
    }
    std::uint32_t startup_slot() const {
        const auto start = raw<DWORD>(pe + 24 + 104);
        for (unsigned i = 0; i < 64; ++i) {
            const auto descriptor = at<IMAGE_IMPORT_DESCRIPTOR>(start + i * 20);
            if (!descriptor.Name) break;
            if (_stricmp(name(descriptor.Name).c_str(), "kernel32.dll")) continue;
            for (unsigned j = 0; j < 4096; ++j) {
                const auto lookup = at<DWORD>(descriptor.OriginalFirstThunk + j * 4);
                if (!lookup) break;
                if (!(lookup & IMAGE_ORDINAL_FLAG32) && name(lookup + 2) == "GetStartupInfoA") return descriptor.FirstThunk + j * 4;
            }
        }
        throw std::runtime_error("fixture startup import missing");
    }
};
}
