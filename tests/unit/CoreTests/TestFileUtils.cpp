#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

import core;

namespace
{
    namespace fs = std::filesystem;
    
    fs::path MakeUniqueTempTestDirectory()
    {
        const auto seed = std::chrono::system_clock::now().time_since_epoch().count();
        fs::path tempDir = std::filesystem::temp_directory_path() / ("core_fileutils_tests_" + std::to_string(seed));
        return tempDir;
    }
    
    class ScopedCurrentPath final
    {
    public:
        explicit ScopedCurrentPath(const fs::path& next) 
            : original_(fs::current_path())
        {
            fs::current_path(next);
        }
        
        ~ScopedCurrentPath()
        {
            std::error_code ignored;
            fs::current_path(original_, ignored);
        }
        
        ScopedCurrentPath(const ScopedCurrentPath&) = delete;
        ScopedCurrentPath& operator=(const ScopedCurrentPath&) = delete;
        
    private:
        fs::path original_;
    };
    
    class ScopedTempDirectory final
    {
    public:
        ScopedTempDirectory()
            : path_(MakeUniqueTempTestDirectory())
        {
            fs::create_directories(path_);
        }
        
        ~ScopedTempDirectory()
        {
            std::error_code ignored;
            fs::remove_all(path_, ignored);
        }
        
        ScopedTempDirectory(const ScopedTempDirectory&) = delete;
        ScopedTempDirectory& operator=(const ScopedTempDirectory&) = delete;
        
        [[nodiscard]] const fs::path& Path() const
        {
            return path_;
        }
        
    private:
        fs::path path_;
    };
}

TEST(FileUtils, ReadAllTextReturnsFileContentsForTempFile)
{
    ScopedTempDirectory tempDir;
    std::string_view testText = "line one\nline two";
    const fs::path filePath = tempDir.Path() / "example.txt";
    {
        std::ofstream outStream(filePath, std::ios::binary);
        ASSERT_TRUE(outStream.is_open());
        outStream << testText;
    }
    
    const std::string text = fileUtils::ReadAllText(filePath);
    EXPECT_EQ(text, testText);
}

TEST(FileUtils, ReadAllTextThrowsRuntimeErrorForMissingFIle)
{
    ScopedTempDirectory tempDir;
    const fs::path missingFile = tempDir.Path() / "missing_file.txt";

    try
    {
        [[maybe_unused]] const std::string text = fileUtils::ReadAllText(missingFile);
        FAIL() << "Expecting missing file read to throw";
    }
    catch (const std::runtime_error& error)
    {
        const std::string message = error.what();
        EXPECT_TRUE(message.starts_with("Failed to open text file:"));
        EXPECT_NE(message.find(missingFile.string()), std::string::npos);
    }
    
}

TEST(FileUtils, ReadAllTextSupportsRelativeAndAbsolutPaths)
{
    ScopedTempDirectory tempDir;
    const fs::path nestedDir = tempDir.Path() / "nested";
    const fs::path absolutePath = nestedDir/ "relative_target.txt";
    std::string_view testText = "resolved from both path modes";
    fs::create_directories(nestedDir);
    
    {
        std::ofstream outStream(absolutePath, std::ios::binary);
        ASSERT_TRUE(outStream.is_open());
        outStream << testText;
    }
    
    const std::string absoluteRead = fileUtils::ReadAllText(absolutePath);
    
    std::string relativeRead;
    {
        const ScopedCurrentPath cwd(tempDir.Path());
        relativeRead = fileUtils::ReadAllText(fs::path("nested") / "relative_target.txt");
    }
    
    EXPECT_EQ(absoluteRead, testText);
    EXPECT_EQ(relativeRead, testText);
}