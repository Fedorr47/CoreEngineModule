#include <array>
#include <iostream>
#include <map>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "AppArguments.h"

namespace
{
    constexpr std::string_view COMMON_ARGUMENTS{"--"};
    constexpr std::array<std::string_view, 1> ValidArgumentNames{MAP_LITERAL};

    bool CheckNamedArgument(std::string_view argumentName)
    {
        return std::ranges::find(ValidArgumentNames, argumentName) != ValidArgumentNames.end();
    }
}

namespace appBootstrap
{
    void ParseArgument(
        const std::string& argument,
        std::map<std::string, std::vector<std::string>>& args)
    {
        std::string_view value = argument;

        if (value.find(COMMON_ARGUMENTS) == 0)
        {
            value = value.substr(COMMON_ARGUMENTS.length());
        }

        const size_t index = value.find('=');
        if (index != std::string::npos)
        {
            const std::string_view key = value.substr(0, index);

            if (!CheckNamedArgument(key))
            {
                std::cerr << "Invalid key: " << key << std::endl;
                return;
            }

            const std::string_view parsedValue = value.substr(index + 1);
            if (parsedValue.empty())
            {
                std::cerr << "Empty value for key: " << key << std::endl;
                return;
            }

            args[std::string(key)].emplace_back(parsedValue);
        }
        else
        {
            std::cerr << "Invalid argument: " << argument << std::endl;
        }
    }

    ParsedBackend ParseAppArguments(
        int argc,
        char** argv,
        std::map<std::string, std::vector<std::string>>& args)
    {
        ParsedBackend backendType = ParsedBackend::Default;

        for (int argIndex = 1; argIndex < argc; ++argIndex)
        {
            const std::string_view argValue = argv[argIndex];

            if (argValue == "--null")
            {
                backendType = ParsedBackend::Null;
                continue;
            }

            ParseArgument(argv[argIndex], args);
        }

        return backendType;
    }
}
