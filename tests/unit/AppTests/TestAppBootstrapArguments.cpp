#include <gtest/gtest.h>

#include <array>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "App/AppArguments.h"

namespace appBootstrap
{
    void ParseArgument(const std::string& argument, std::map<std::string, std::vector<std::string>>& args);
}

struct ArgvBuffer
{
    std::vector<std::string> storage;
    std::vector<char*> argv;
        
    explicit ArgvBuffer(std::vector<std::string> values)
        : storage(std::move(values))
    {
        argv.reserve(storage.size());
        for (std::string& value : storage)
        {
            argv.push_back(value.data());
        }
    }
};

TEST(AppBootstrapArguments, ParseArgument_MapOverrideFromEqualsSyntax)
{
    std::map<std::string, std::vector<std::string>> parsedArgs;

    appBootstrap::ParseArgument("--map=level_start", parsedArgs);

    ASSERT_EQ(parsedArgs.size(), 1u);
    const auto it = parsedArgs.find(std::string(MAP_LITERAL));
    ASSERT_NE(it, parsedArgs.end());
    ASSERT_EQ(it->second.size(), 1u);
    EXPECT_EQ(it->second.front(), "level_start");
}

TEST(AppBootstrapArguments, ParseArgument_RejectsInvalidKeysAndMalformedMapForms)
{
    const std::array<std::string_view, 5> invalidArguments{
        "--unknown=value",
        "--map=",
        "--map",
        "map",
        "--map level_start"
    };

    for (const std::string_view argument : invalidArguments)
    {
        std::map<std::string, std::vector<std::string>> parsedArgs;
        appBootstrap::ParseArgument(std::string(argument), parsedArgs);
        EXPECT_TRUE(parsedArgs.empty()) << "argument: " << argument;
    }
}

TEST(AppBootstrapArguments, ParseAppArguments_ParsesMapOverridesAndIgnoresInvalidArgs)
{
    ArgvBuffer argvData({
        "CoreEngineModule.exe",
        "--map=first",
        "--bad=value",
        "--map=second",
        "--map",
        "--map=third"
    });

    std::map<std::string, std::vector<std::string>> parsedArgs;

    const appBootstrap::ParsedBackend backend = appBootstrap::ParseAppArguments(
    static_cast<int>(argvData.argv.size()),
    argvData.argv.data(),
    parsedArgs);

    EXPECT_EQ(backend, appBootstrap::ParsedBackend::Default);
    const auto it = parsedArgs.find(std::string(MAP_LITERAL));
    ASSERT_NE(it, parsedArgs.end());
    EXPECT_EQ(it->second, (std::vector<std::string>{"first", "second", "third"}));
}

TEST(AppBootstrapArguments, ParseAppArguments_NullBackendAndEmptyArgumentListBehavior)
{
    {
        ArgvBuffer argvData({
            "CoreEngineModule.exe",
            "--null",
            "--map=arena"
        });

        std::map<std::string, std::vector<std::string>> parsedArgs;
        const appBootstrap::ParsedBackend backend = appBootstrap::ParseAppArguments(
        static_cast<int>(argvData.argv.size()),
    argvData.argv.data(),
        parsedArgs);

        EXPECT_EQ(backend, appBootstrap::ParsedBackend::Null);
        const auto it = parsedArgs.find(std::string(MAP_LITERAL));
        ASSERT_NE(it, parsedArgs.end());
        EXPECT_EQ(it->second, (std::vector<std::string>{"arena"}));
    }

    {
        std::map<std::string, std::vector<std::string>> parsedArgs;
        char** argv = nullptr;
        const appBootstrap::ParsedBackend backend =
            appBootstrap::ParseAppArguments(0, argv, parsedArgs);

        EXPECT_EQ(backend, appBootstrap::ParsedBackend::Default);
        EXPECT_TRUE(parsedArgs.empty());
    }
}



