#include "TestFixtureLoader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace
{
    [[nodiscard]] std::filesystem::path BuildFixtureRootFromCompileDefinition()
    {
#ifdef CORE_TEST_FIXTURE_ROOT
        return std::filesystem::path(CORE_TEST_FIXTURE_ROOT);
#else
        return std::filesystem::path(__FILE__).parent_path().parent_path() / "assets";
#endif
    }
}

std::filesystem::path GetTestFixtureRoot()
{
    const std::filesystem::path root = BuildFixtureRootFromCompileDefinition();
    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root))
    {
        throw std::runtime_error("Test fixture root is missing or not a directory: " + root.string());
    }
    return root;
}

std::filesystem::path ResolveFixturePath(std::string_view relativePath)
{
    if (relativePath.empty())
    {
        throw std::runtime_error("ResolveJsonFixturePath requires a non-empty relative path.");
    }
    
    const std::filesystem::path inputPath(relativePath);
    if (inputPath.is_absolute())
    {
        throw std::runtime_error("ResolveJsonFixturePath only accepts relative paths, got an absolute path: " + inputPath.string());
    }
    
    const std::filesystem::path candidate = GetTestFixtureRoot() / inputPath;
    const std::filesystem::path normalized = candidate.lexically_normal();
    
    if (!std::filesystem::exists(normalized) || !std::filesystem::is_regular_file(normalized))
    {
        throw std::runtime_error(
            "Fixture filename not found: '" + std::string(relativePath) + "'\n"
            "Resolved path: '" + normalized.string() + "'\n"
            "Fixture root: '" + GetTestFixtureRoot().string() + "'"
            );
    }
    
    return normalized;
}

std::string LoadTextFixture(std::string_view relativePath)
{
    const std::filesystem::path path = ResolveFixturePath(relativePath);
    
    std::ifstream inputFile(path, std::ios::binary);
    if (!inputFile.is_open())
    {
        throw std::runtime_error("Could not open fixture file for reading: " + path.string());
    }
    
    std::ostringstream outputStream;
    outputStream << inputFile.rdbuf();
    if (!inputFile.good() && !inputFile.eof())
    {
        throw std::runtime_error("Failed while reading a fixture file: " + path.string());
    }
    
    return outputStream.str();
}
