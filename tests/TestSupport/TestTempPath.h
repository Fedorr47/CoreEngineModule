#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>

namespace test
{
    class ScopedTempPath final
    {
    public:
        explicit ScopedTempPath(std::filesystem::path path)
            : path_(std::move(path))
        {
        }

        ~ScopedTempPath()
        {
            std::error_code ignored;
            std::filesystem::remove_all(path_, ignored);
        }

        [[nodiscard]] const std::filesystem::path& Path() const noexcept
        {
            return path_;
        }

    private:
        std::filesystem::path path_;
    };

    [[nodiscard]] inline std::filesystem::path MakeUniqueTempPath(const std::string& prefix)
    {
        const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
        return std::filesystem::temp_directory_path()
            / (prefix + "_" + std::to_string(ticks));
    }
}