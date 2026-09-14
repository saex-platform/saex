#pragma once
#include "saex/engine/loader_observation.hpp"
#include <memory>
#include <string>

namespace saex::engine {
enum class LoaderOrigin { system_x86, game_root };
struct LoaderPinSpec {
    LoaderOrigin origin{};
    std::string_view name;
    std::uint64_t bytes{};
    Sha256 sha256{};
};
// Source-defined observation policy only. No downloaded or runtime JSON policy API.
class PreparedLoaderPolicy final {
public:
    PreparedLoaderPolicy(std::span<const LoaderPinSpec> specs, const Sha256& expected_engine,
        const Sha256& actual_engine, std::wstring_view game_directory, std::wstring_view system_directory,
        bool allow_bootstrap_asi = false);
    [[nodiscard]] std::string_view error() const noexcept { return error_; }
    [[nodiscard]] std::string_view failed_module() const noexcept { return failed_module_; }
    [[nodiscard]] std::span<const LoaderFile* const> pins() const noexcept {
        return error_.empty() ? std::span<const LoaderFile* const>(pointers_.data(), count_) : std::span<const LoaderFile* const>{};
    }
private:
    std::array<std::unique_ptr<LoaderFile>, 64> files_{};
    std::array<const LoaderFile*, 64> pointers_{};
    std::size_t count_{};
    std::string_view error_;
    std::string failed_module_;
};
}
