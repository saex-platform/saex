#include "saex/engine/loader_policy.hpp"
#include <algorithm>

namespace saex::engine {
namespace {
bool valid_name(std::string_view name) noexcept {
    if (name.size() < 5 || name.size() > 127 || !name.ends_with(".dll") || name.find("..") != name.npos) return false;
    return std::all_of(name.begin(), name.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
    });
}
}
PreparedLoaderPolicy::PreparedLoaderPolicy(std::span<const LoaderPinSpec> specs, const Sha256& expected_engine,
    const Sha256& actual_engine, std::wstring_view game_directory, std::wstring_view system_directory, bool allow_bootstrap_asi) {
    if (expected_engine != actual_engine || std::all_of(expected_engine.begin(), expected_engine.end(), [](std::byte b) { return b == std::byte{}; })) {
        error_ = "loader_policy_engine_mismatch"; return;
    }
    if (specs.empty() || specs.size() > files_.size() || game_directory.empty() || system_directory.empty()) {
        error_ = "loader_policy_input"; return;
    }
    std::uint64_t total{};
    // Validate the ENTIRE recipe before file IO; basename uniqueness includes both origins.
    for (std::size_t i = 0; i < specs.size(); ++i) {
        const auto& spec = specs[i];
        const bool bootstrap_name = allow_bootstrap_asi && spec.origin == LoaderOrigin::game_root && spec.name == "saex_bootstrap.asi";
        if ((!valid_name(spec.name) && !bootstrap_name) || (spec.origin != LoaderOrigin::system_x86 && spec.origin != LoaderOrigin::game_root) ||
            !spec.bytes || spec.bytes > max_loader_file_bytes ||
            std::all_of(spec.sha256.begin(), spec.sha256.end(), [](std::byte b) { return b == std::byte{}; })) {
            error_ = "loader_policy_spec"; return;
        }
        for (std::size_t j = 0; j < i; ++j) if (specs[j].name == spec.name) { error_ = "loader_policy_duplicate"; return; }
        total += spec.bytes;
        if (total > 256ULL * 1024 * 1024) { error_ = "loader_policy_byte_limit"; return; }
    }
    std::uint64_t charged{};
    for (const auto& spec : specs) {
        const auto root = spec.origin == LoaderOrigin::system_x86 ? system_directory : game_directory;
        auto path = std::wstring(root) + L"\\" + std::wstring(spec.name.begin(), spec.name.end());
        auto file = std::make_unique<LoaderFile>(path.c_str(), 256ULL * 1024 * 1024 - charged);
        if (!file->valid()) { error_ = "loader_policy_file_unavailable"; failed_module_ = spec.name; return; }
        const auto& actual = file->identity();
        charged += actual.bytes;
        if (spec.name != actual.name.data() || spec.bytes != actual.bytes || spec.sha256 != actual.sha256) {
            error_ = "loader_policy_file_mismatch"; failed_module_ = spec.name; return;
        }
        pointers_[count_] = file.get(); files_[count_++] = std::move(file);
    }
}
}
