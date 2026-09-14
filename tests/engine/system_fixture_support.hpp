#pragma once
#include "proxy_fixture_support.hpp"
#include <utility>

// Test-only recipes from retained host DLL files. Production reviewed policies
// never discover or approve system exports this way.
namespace {
std::uint32_t system_export_slot(const Fixture& image, std::string_view wanted) {
    const auto directory = image.raw<IMAGE_DATA_DIRECTORY>(image.pe + 24 + 96);
    require(directory.VirtualAddress && directory.Size >= sizeof(IMAGE_EXPORT_DIRECTORY) &&
        raw_offset(image.layout, directory.VirtualAddress, directory.Size).has_value(), "system export directory");
    const auto exports = image.at<IMAGE_EXPORT_DIRECTORY>(directory.VirtualAddress);
    require(exports.NumberOfFunctions && exports.NumberOfFunctions <= 65536 &&
        exports.NumberOfNames && exports.NumberOfNames <= 16384 &&
        exports.NumberOfNames <= exports.NumberOfFunctions, "system export budget");
    require(raw_offset(image.layout, exports.AddressOfFunctions, 4ULL * exports.NumberOfFunctions).has_value() &&
        raw_offset(image.layout, exports.AddressOfNames, 4ULL * exports.NumberOfNames).has_value() &&
        raw_offset(image.layout, exports.AddressOfNameOrdinals, 2ULL * exports.NumberOfNames).has_value(), "system export tables");
    for (DWORD i = 0; i < exports.NumberOfNames; ++i) {
        if (image.name(image.at<DWORD>(exports.AddressOfNames + 4 * i)) != wanted) continue;
        const auto ordinal = image.at<WORD>(exports.AddressOfNameOrdinals + 2 * i);
        require(ordinal < exports.NumberOfFunctions, "system export ordinal");
        return exports.AddressOfFunctions + 4 * ordinal;
    }
    throw std::runtime_error("system export missing");
}

std::uint32_t system_export_rva(const Fixture& image, std::string_view wanted, std::uint32_t size) {
    const auto directory = image.raw<IMAGE_DATA_DIRECTORY>(image.pe + 24 + 96);
    const auto rva = image.at<DWORD>(system_export_slot(image, wanted));
    require(rva && !(rva >= directory.VirtualAddress && rva - directory.VirtualAddress < directory.Size),
        "system export hole or forwarder");
    bool executable{};
    for (std::uint32_t i = 0; i < image.layout.section_count; ++i) {
        const auto& section = image.layout.sections[i];
        if ((section.characteristics & IMAGE_SCN_MEM_EXECUTE) && rva >= section.rva &&
            rva - section.rva <= section.virtual_size && size <= section.virtual_size - (rva - section.rva)) executable = true;
    }
    require(executable, "system export executable range");
    return rva;
}

BootstrapExportSpec system_export(const Fixture& image, std::string_view wanted) {
    const auto rva = system_export_rva(image, wanted, 20);
    BootstrapExportSpec result{rva, image.at<std::array<std::byte,20>>(rva), image.layout.image_base, 0};
    const auto relocations = image.raw<IMAGE_DATA_DIRECTORY>(image.pe + 24 + 96 + 8 * IMAGE_DIRECTORY_ENTRY_BASERELOC);
    require((!relocations.VirtualAddress && !relocations.Size) || (relocations.VirtualAddress && relocations.Size &&
        relocations.Size <= 16U * 1024 * 1024 && raw_offset(image.layout, relocations.VirtualAddress, relocations.Size).has_value()),
        "system relocation directory");
    std::uint32_t cursor{}, occupied{};
    while (cursor < relocations.Size) {
        require(relocations.Size - cursor >= sizeof(IMAGE_BASE_RELOCATION), "system relocation header");
        const auto block = image.at<IMAGE_BASE_RELOCATION>(relocations.VirtualAddress + cursor);
        require(block.SizeOfBlock >= 8 && block.SizeOfBlock <= relocations.Size - cursor && !(block.SizeOfBlock % 4),
            "system relocation block");
        require(!(block.VirtualAddress % 4096), "system relocation page");
        for (std::uint32_t offset = 8; offset < block.SizeOfBlock; offset += 2) {
            const auto entry = image.at<WORD>(relocations.VirtualAddress + cursor + offset);
            const auto type = entry >> 12;
            if (type == IMAGE_REL_BASED_ABSOLUTE) continue;
            require(type == IMAGE_REL_BASED_HIGHLOW, "system relocation type");
            const auto address = static_cast<std::uint64_t>(block.VirtualAddress) + (entry & 4095);
            require(address + 4 <= image.layout.image_size, "system relocation address");
            if (address >= static_cast<std::uint64_t>(rva) + 20 || address + 4 <= rva) continue;
            require(address >= rva && address + 4 <= static_cast<std::uint64_t>(rva) + 20, "system partial relocation");
            const auto start = static_cast<std::uint32_t>(address - rva);
            require(!(occupied & (15U << start)), "system overlapping relocation");
            occupied |= 15U << start;
            result.highlow_mask |= 1U << start;
        }
        cursor += block.SizeOfBlock;
    }
    std::array<std::byte,20> normalized{};
    require(bootstrap_export_prefix(result, result.preferred_base, normalized) && normalized == result.prefix,
        "system export relocation recipe");
    return result;
}

BootstrapExportSpec system_absolute_export(const Fixture& image, const wchar_t* module, std::string_view name) {
    auto result = system_export(image, name);
    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(module));
    require(base && base <= UINT32_MAX, "system absolute module already loaded");
    std::array<std::byte,20> normalized{};
    require(bootstrap_export_prefix(result, static_cast<DWORD>(base), normalized), "system absolute normalization");
    // Exact-prefix observers receive an address-specific fixture recipe. The
    // child's pinned mapping/code checks still reject a different relocated body.
    result.prefix = normalized; result.preferred_base = static_cast<DWORD>(base); result.highlow_mask = 0;
    return result;
}

struct SystemFixtureExports {
    Fixture kernel32, kernelbase, ntdll;
    BootstrapExportSpec protect, startup, last_error, create_event, enter_critical_section;
    std::uint32_t create_thunk_rva{}, create_thunk_slot_rva{};
    explicit SystemFixtureExports(const std::wstring& directory)
        : kernel32((directory + L"\\kernel32.dll").c_str(), true),
          kernelbase((directory + L"\\kernelbase.dll").c_str(), true),
          ntdll((directory + L"\\ntdll.dll").c_str(), true, true),
          protect(system_export(kernel32, "VirtualProtect")), startup(system_export(kernel32, "GetStartupInfoA")),
          last_error(system_absolute_export(kernel32, L"kernel32.dll", "GetLastError")),
          create_event(system_absolute_export(kernelbase, L"kernelbase.dll", "CreateEventA")),
          enter_critical_section(system_absolute_export(ntdll, L"ntdll.dll", "RtlEnterCriticalSection")) {
        create_thunk_rva = system_export_rva(kernel32, "CreateEventA", 6);
        const auto thunk = kernel32.at<std::array<std::byte,6>>(create_thunk_rva);
        require(thunk[0] == std::byte{0xff} && thunk[1] == std::byte{0x25}, "system CreateEventA thunk");
        const auto operand = kernel32.at<DWORD>(create_thunk_rva + 2);
        require(operand >= kernel32.layout.image_base, "system CreateEventA operand");
        create_thunk_slot_rva = operand - kernel32.layout.image_base;
        require(raw_offset(kernel32.layout, create_thunk_slot_rva, 4).has_value(), "system CreateEventA slot");
    }
};

[[maybe_unused]] void test_system_fixture_exports(SystemFixtureExports& system) {
    const auto absolute = system_absolute_export(system.kernel32, L"kernel32.dll", "VirtualProtect");
    std::array<std::byte,20> normalized{};
    require(bootstrap_export_prefix(system.protect, absolute.preferred_base, normalized) &&
        !absolute.highlow_mask && absolute.prefix == normalized, "system absolute export recipe");
    auto malformed = system.protect; malformed.highlow_mask = 1U << 19;
    require(!bootstrap_export_prefix(malformed, absolute.preferred_base, normalized), "system invalid absolute relocation accepted");
    for (const auto pair : {std::pair{L"kernel32.dll", &system.protect}, {L"kernel32.dll", &system.startup},
            {L"kernel32.dll", &system.last_error}, {L"kernelbase.dll", &system.create_event}, {L"ntdll.dll", &system.enter_critical_section}}) {
        const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(pair.first));
        require(base && base <= UINT32_MAX, "system module already loaded");
        std::array<std::byte,20> expected{}, actual{};
        require(bootstrap_export_prefix(*pair.second, static_cast<DWORD>(base), expected), "system live normalization");
        SIZE_T read{};
        require(ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(base + pair.second->rva), actual.data(), actual.size(), &read) &&
            read == actual.size() && actual == expected, "system disk and loaded prefix differ");
    }
    auto& file = system.kernel32;
    const auto original = file.bytes;
    const auto directory = file.raw<IMAGE_DATA_DIRECTORY>(file.pe + 24 + 96);
    const auto relocations = file.raw<IMAGE_DATA_DIRECTORY>(file.pe + 24 + 96 + 8 * IMAGE_DIRECTORY_ENTRY_BASERELOC);
    const auto slot = system_export_slot(file, "VirtualProtect");
    const auto write = [&]<class T>(std::uint32_t rva, T value) {
        const auto offset = raw_offset(file.layout, rva, sizeof(T)); require(offset.has_value(), "system negative mutation range");
        std::memcpy(file.bytes.data() + *offset, &value, sizeof(T));
    };
    unsigned rejected{};
    const auto rejects = [&](auto mutate, const char* reason) {
        file.bytes = original;
        mutate();
        bool caught{};
        try { (void)system_export(file, "VirtualProtect"); }
        catch (const std::runtime_error& error) { require(std::string_view(error.what()) == reason, "system negative wrong reason"); caught = true; }
        require(caught, "system malformed metadata accepted"); ++rejected;
    };
    rejects([&] { write(directory.VirtualAddress + 20, DWORD{65537}); }, "system export budget");
    rejects([&] { write(directory.VirtualAddress + 28, DWORD{UINT32_MAX}); }, "system export tables");
    rejects([&] { write(slot, DWORD{}); }, "system export hole or forwarder");
    rejects([&] { write(slot, directory.VirtualAddress); }, "system export hole or forwarder");
    rejects([&] { write(slot, DWORD{64}); }, "system export executable range");
    rejects([&] { write(relocations.VirtualAddress + 4, DWORD{7}); }, "system relocation block");
    rejects([&] { write(relocations.VirtualAddress + 8, WORD{0xf000}); }, "system relocation type");
    for (const auto delta : {-1, 17}) {
        rejects([&] {
            const auto rva = static_cast<DWORD>(static_cast<std::int64_t>(system.protect.rva) + delta);
            write(relocations.VirtualAddress, rva & ~4095U);
            write(relocations.VirtualAddress + 4, DWORD{12});
            write(relocations.VirtualAddress + 8, static_cast<WORD>(0x3000U | (rva & 4095U)));
            write(relocations.VirtualAddress + 10, WORD{});
        }, "system partial relocation");
    }
    rejects([&] {
        write(relocations.VirtualAddress, system.protect.rva & ~4095U);
        write(relocations.VirtualAddress + 4, DWORD{12});
        const auto entry = static_cast<WORD>(0x3000U | (system.protect.rva & 4095U));
        write(relocations.VirtualAddress + 8, entry); write(relocations.VirtualAddress + 10, entry);
    }, "system overlapping relocation");
    file.bytes = original;
    require(system_export(file, "VirtualProtect").prefix == system.protect.prefix, "system negative metadata not restored");
    std::cout << "PASS system fixture exports: 5 live prefixes and " << rejected << " malformed metadata rejections\n";
}
}
