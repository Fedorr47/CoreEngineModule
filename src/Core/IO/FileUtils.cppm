module;

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

export module core:file_utils;

export namespace fileUtils
{
    namespace fs = std::filesystem;

    struct TextFile
    {
        std::string text;
        std::vector<fs::path> dpendencies;
    };

    struct BinaryFile
    {
        std::vector<std::byte> data;
    };

    [[nodiscard]] inline std::string ReadAllText(const fs::path& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::in);
        if (!file)
        {
            throw std::runtime_error("Filed to open text file:" + path.string());
        }

        std::ostringstream stringStream;
        stringStream << file.rdbuf();
        return stringStream.str();
    }
}

export namespace FILE_UTILS
{
    namespace fs = std::filesystem;

    using fileUtils::BinaryFile;
    using fileUtils::ReadAllText;
    using fileUtils::TextFile;
}
