#pragma once

// Intentionally does not include STL headers.
// Including translation units must provide std::string/std::map/std::vector/std::string_view.

constexpr std::string_view MAP_LITERAL{"map"};

namespace appBootstrap
{
    enum class ParsedBackend
    {
        Default,
        Null
    };

    void ParseArgument(
        const std::string& argument,
        std::map<std::string, std::vector<std::string>>& args);

    ParsedBackend ParseAppArguments(
        int argc,
        char** argv,
        std::map<std::string, std::vector<std::string>>& args);
}
