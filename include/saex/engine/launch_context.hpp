#pragma once
#include "saex/engine/profile_gate.hpp"
#include <string>
#include <vector>

namespace saex::engine {
inline constexpr std::size_t max_environment_units = 65536;
inline constexpr std::size_t max_environment_entries = 1024;
// Local development input, never a server/resource launch API. Holds the resolved
// directory and a private Unicode environment snapshot; values are not telemetry.
class LaunchContext final {
public:
    explicit LaunchContext(std::wstring_view directory);
    LaunchContext(std::wstring_view directory, std::span<const std::wstring_view> entries);
    ~LaunchContext();
    LaunchContext(const LaunchContext&) = delete;
    LaunchContext& operator=(const LaunchContext&) = delete;
    [[nodiscard]] bool valid() const noexcept { return ready_; }
    [[nodiscard]] bool directory_matches() const noexcept;
    [[nodiscard]] std::string_view error() const noexcept { return error_; }
    [[nodiscard]] const std::wstring& directory() const noexcept { return directory_; }
    [[nodiscard]] std::span<const wchar_t> environment() const noexcept { return environment_; }
    [[nodiscard]] const Sha256& environment_hash() const noexcept { return environment_hash_; }
    [[nodiscard]] std::size_t entry_count() const noexcept { return entry_count_; }
private:
    void prepare(std::wstring_view directory, std::span<const std::wstring_view> entries);
    void* directory_handle_{};
    std::wstring directory_;
    std::vector<wchar_t> environment_;
    Sha256 environment_hash_{};
    std::size_t entry_count_{};
    bool ready_{};
    std::string_view error_{"launch_context_not_prepared"};
};
}
