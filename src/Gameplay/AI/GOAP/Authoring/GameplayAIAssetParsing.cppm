module;

#include <algorithm>
#include <any>
#include <cmath>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module core:gameplay_ai_asset_parsing;
export import :json_utils;

export namespace rendern::ai_asset_detail
{
    using jsonUtils::JsonObject;
    using jsonUtils::JsonValue;

    [[noreturn]] void Invalid(const std::string_view source,
        const std::string_view location, const std::string_view reason)
    {
        throw std::runtime_error("AI asset '" + std::string(source) + "', " +
            std::string(location) + ": " + std::string(reason));
    }

    const JsonObject& Object(const JsonValue& value,
        const std::string_view source, const std::string_view location)
    {
        if (!value.IsObject())
        {
            Invalid(source, location, "expected an object");
        }
        return value.AsObject();
    }

    void Fields(const JsonObject& object, const std::initializer_list<std::string_view> allowed,
        const std::string_view source, const std::string_view location)
    {
        for (const auto& [name, value] : object)
        {
            if (std::ranges::find(allowed, name) == allowed.end())
            {
                Invalid(source, location, "unknown field '" + name + "'");
            }
        }
    }

    const JsonValue& Required(const JsonObject& object, const char* key,
        const std::string_view source, const std::string_view location)
    {
        const auto found = object.find(key);
        if (found == object.end())
        {
            Invalid(source, location, "missing field '" + std::string(key) + "'");
        }
        return found->second;
    }

    std::string String(const JsonObject& object, const char* key,
        const std::string_view source, const std::string_view location)
    {
        const JsonValue& value = Required(object, key, source, location);
        if (!value.IsString() || value.AsString().empty())
        {
            Invalid(source, location, "field '" + std::string(key) + "' must be a non-empty string");
        }
        return value.AsString();
    }

    const jsonUtils::JsonArray& Array(const JsonObject& object, const char* key,
        const std::string_view source, const std::string_view location)
    {
        const JsonValue& value = Required(object, key, source, location);
        if (!value.IsArray())
        {
            Invalid(source, location, "field '" + std::string(key) + "' must be an array");
        }
        return value.AsArray();
    }

    bool Boolean(const JsonObject& object, const char* key, const bool fallback,
        const std::string_view source, const std::string_view location)
    {
        const auto found = object.find(key);
        if (found == object.end())
        {
            return fallback;
        }
        if (!found->second.IsBool())
        {
            Invalid(source, location, "field '" + std::string(key) + "' must be boolean");
        }
        return found->second.AsBool();
    }

    float Number(const JsonObject& object, const char* key, const float fallback,
        const std::string_view source, const std::string_view location)
    {
        const auto found = object.find(key);
        if (found == object.end())
        {
            return fallback;
        }
        const JsonValue& value = found->second;
        if (!value.IsNumber() || !std::isfinite(value.AsNumber()) || value.AsNumber() < 0.0 ||
            value.AsNumber() > std::numeric_limits<float>::max())
        {
            Invalid(source, location, "field '" + std::string(key) + "' must be a finite non-negative float");
        }
        return static_cast<float>(value.AsNumber());
    }

    std::string OptionalString(const JsonObject& object, const char* key,
        std::string_view source, std::string_view location)
    {
        return object.contains(key) ? String(object, key, source, location) : std::string{};
    }
    std::uint64_t PositiveInteger(const JsonObject& object, const char* key,
        std::uint64_t maximum, std::string_view source, std::string_view location)
    {
        const auto& value = Required(object, key, source, location);
        if (!value.IsNumber() || !std::isfinite(value.AsNumber()) || value.AsNumber() < 1.0
            || value.AsNumber() > static_cast<double>(maximum)
            || std::floor(value.AsNumber()) != value.AsNumber())
        {
            Invalid(source, location, "field '" + std::string(key) + "' must be a positive integer in range");
        }
        return static_cast<std::uint64_t>(value.AsNumber());
    }
    std::vector<std::string> Strings(const JsonObject& object, const char* key,
        std::string_view source, std::string_view location)
    {
        std::vector<std::string> result;
        for (const auto& entry : Array(object, key, source, location))
        {
            if (!entry.IsString() || entry.AsString().empty())
            {
                Invalid(source, location, "expected non-empty strings");
            }
            result.push_back(entry.AsString());
        }
        return result;
    }

    JsonValue Root(const std::string_view json, const std::string_view source)
    {
        JsonValue root;
        try
        {
            root = jsonUtils::JsonParser(json).Parse();
        }
        catch (const std::exception& error)
        {
            Invalid(source, "root", error.what());
        }
        const auto& object = Object(root, source, "root");
        const JsonValue& version = Required(object, "version", source, "root");
        if (!version.IsNumber() || version.AsNumber() != 1.0)
        {
            Invalid(source, "root", "unsupported version; expected 1");
        }
        return root;
    }
}

export namespace rendern
{
    enum class GameplayAIComponentKind { Observation, Capability, Reaction };

    struct GameplayAIComponentParseContext
    {
        const jsonUtils::JsonObject& object;
        std::string_view source, location;
    };

    // JSON is transient. Stored parameters are owning, typed C++ values, never a property bag.
    class GameplayAIComponentParsers
    {
    public:
        template<class T>
        [[nodiscard]] bool Register(GameplayAIComponentKind kind, std::string_view type,
            std::function<T(const GameplayAIComponentParseContext&)> parse)
        {
            if (type.empty() || !parse)
            {
                return false;
            }
            return parsers_[kind].emplace(std::string(type),
                [parse = std::move(parse)](const GameplayAIComponentParseContext& context) -> std::any
                {
                    return parse(context);
                }).second;
        }

        [[nodiscard]] std::any Parse(GameplayAIComponentKind kind, std::string_view type,
            const GameplayAIComponentParseContext& context) const
        {
            const auto category = parsers_.find(kind);
            if (category != parsers_.end())
            {
                const auto found = category->second.find(type);
                if (found != category->second.end())
                {
                    return found->second(context);
                }
            }
            const std::string categoryName = kind == GameplayAIComponentKind::Observation ? "observation"
                : kind == GameplayAIComponentKind::Capability ? "capability" : "reaction";
            ai_asset_detail::Invalid(context.source, context.location,
                "unknown " + categoryName + " type '" + std::string(type) + "'");
        }

    private:
        using Parser = std::function<std::any(const GameplayAIComponentParseContext&)>;
        std::map<GameplayAIComponentKind, std::map<std::string, Parser, std::less<>>> parsers_;
    };
}
