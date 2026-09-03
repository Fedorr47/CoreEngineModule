module;

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

export module core:gameplay_ai_decision_asset;

import :file_system;
import :json_utils;

export namespace rendern
{
    struct GameplayAIObservationAsset
    {
        std::string type{};
        std::string target{};
        std::string fact{};
        float radius{};
        bool requireInteractionPoint{};
    };

    struct GameplayAICapabilityAsset
    {
        std::string type{};
        std::string context{};
        std::string source{};
        std::string target{};
        float acceptanceRadius{0.4f};
        float slowingRadius{1.5f};
        bool wantsRun{};
    };

    struct GameplayAIBehaviorAsset
    {
        std::string id{};
        std::string source{};
        std::string model{};
        std::string definition{};
        std::vector<GameplayAIObservationAsset> observations{};
        std::vector<GameplayAICapabilityAsset> capabilities{};
    };

    struct GameplayAIRoleBindingAsset
    {
        std::string role{};
        std::string node{};
    };

    struct GameplayAILevelBindingsAsset
    {
        std::string source{};
        std::vector<GameplayAIRoleBindingAsset> roles{};
    };

    struct GameplayAIDecisionAssetReference
    {
        std::string id{};
        std::string behavior{};
        std::string bindings{};
    };

    struct GameplayAIDecisionCatalogAsset
    {
        std::string source{};
        std::vector<GameplayAIDecisionAssetReference> decisions{};
    };

    [[nodiscard]] GameplayAIBehaviorAsset ParseGameplayAIBehaviorAsset(
        std::string_view json, std::string_view source);
    [[nodiscard]] GameplayAILevelBindingsAsset ParseGameplayAILevelBindingsAsset(
        std::string_view json, std::string_view source);
    [[nodiscard]] GameplayAIDecisionCatalogAsset ParseGameplayAIDecisionCatalogAsset(
        std::string_view json, std::string_view source);
    [[nodiscard]] GameplayAIBehaviorAsset LoadGameplayAIBehaviorAsset(std::string_view path);
    [[nodiscard]] GameplayAILevelBindingsAsset LoadGameplayAILevelBindingsAsset(std::string_view path);
    [[nodiscard]] GameplayAIDecisionCatalogAsset LoadGameplayAIDecisionCatalogAsset(std::string_view path);
}

namespace rendern::ai_asset_detail
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

namespace rendern
{
    GameplayAIBehaviorAsset ParseGameplayAIBehaviorAsset(
        const std::string_view json, const std::string_view source)
    {
        using namespace ai_asset_detail;
        const JsonValue value = Root(json, source);
        const JsonObject& root = value.AsObject();
        Fields(root, {"version", "id", "model", "definition", "observations", "capabilities"}, source, "root");
        GameplayAIBehaviorAsset result{
            .id = String(root, "id", source, "root"), .source = std::string(source),
            .model = String(root, "model", source, "root"),
            .definition = String(root, "definition", source, "root")};
        std::unordered_set<std::string> facts;
        for (const JsonValue& entry : Array(root, "observations", source, "observations"))
        {
            const std::string location = "observations[" + std::to_string(result.observations.size()) + "]";
            const auto& object = Object(entry, source, location);
            Fields(object, {"type", "target", "fact", "radius", "requireInteractionPoint"}, source, location);
            GameplayAIObservationAsset observation{
                .type = String(object, "type", source, location),
                .target = String(object, "target", source, location),
                .fact = String(object, "fact", source, location),
                .radius = Number(object, "radius", 0.0f, source, location),
                .requireInteractionPoint = Boolean(object, "requireInteractionPoint", false, source, location)};
            if (!facts.insert(observation.fact).second)
            {
                Invalid(source, location, "duplicate observer for fact '" + observation.fact + "'");
            }
            result.observations.push_back(std::move(observation));
        }
        std::unordered_set<std::string> contexts;
        for (const JsonValue& entry : Array(root, "capabilities", source, "capabilities"))
        {
            const std::string location = "capabilities[" + std::to_string(result.capabilities.size()) + "]";
            const auto& object = Object(entry, source, location);
            Fields(object, {"type", "context", "source", "target", "acceptanceRadius", "slowingRadius", "wantsRun"}, source, location);
            GameplayAICapabilityAsset capability{
                .type = String(object, "type", source, location),
                .context = String(object, "context", source, location),
                .source = String(object, "source", source, location),
                .target = String(object, "target", source, location),
                .acceptanceRadius = Number(object, "acceptanceRadius", 0.4f, source, location),
                .slowingRadius = Number(object, "slowingRadius", 1.5f, source, location),
                .wantsRun = Boolean(object, "wantsRun", false, source, location)};
            if (!contexts.insert(capability.context).second)
            {
                Invalid(source, location, "duplicate capability context '" + capability.context + "'");
            }
            result.capabilities.push_back(std::move(capability));
        }
        return result;
    }

    GameplayAILevelBindingsAsset ParseGameplayAILevelBindingsAsset(
        const std::string_view json, const std::string_view source)
    {
        using namespace ai_asset_detail;
        const JsonValue value = Root(json, source);
        const JsonObject& root = value.AsObject();
        Fields(root, {"version", "roles"}, source, "root");
        GameplayAILevelBindingsAsset result{.source = std::string(source)};
        std::unordered_set<std::string> roles;
        for (const JsonValue& entry : Array(root, "roles", source, "roles"))
        {
            const auto& object = Object(entry, source, "roles");
            Fields(object, {"role", "node"}, source, "roles");
            GameplayAIRoleBindingAsset binding{
                String(object, "role", source, "roles"), String(object, "node", source, "roles")};
            if (!roles.insert(binding.role).second)
            {
                Invalid(source, "roles", "duplicate role '" + binding.role + "'");
            }
            result.roles.push_back(std::move(binding));
        }
        return result;
    }

    GameplayAIDecisionCatalogAsset ParseGameplayAIDecisionCatalogAsset(
        const std::string_view json, const std::string_view source)
    {
        using namespace ai_asset_detail;
        const JsonValue value = Root(json, source);
        const JsonObject& root = value.AsObject();
        Fields(root, {"version", "decisions"}, source, "root");
        GameplayAIDecisionCatalogAsset result{.source = std::string(source)};
        std::unordered_set<std::string> ids;
        for (const JsonValue& entry : Array(root, "decisions", source, "decisions"))
        {
            const auto& object = Object(entry, source, "decisions");
            Fields(object, {"id", "behavior", "bindings"}, source, "decisions");
            GameplayAIDecisionAssetReference reference{
                String(object, "id", source, "decisions"), String(object, "behavior", source, "decisions"),
                String(object, "bindings", source, "decisions")};
            if (!ids.insert(reference.id).second)
            {
                Invalid(source, "decisions", "duplicate decision id '" + reference.id + "'");
            }
            result.decisions.push_back(std::move(reference));
        }
        return result;
    }

    GameplayAIBehaviorAsset LoadGameplayAIBehaviorAsset(const std::string_view path)
    {
        return ParseGameplayAIBehaviorAsset(FILE_UTILS::ReadAllText(corefs::ResolveAsset(std::filesystem::path(path))), path);
    }
    GameplayAILevelBindingsAsset LoadGameplayAILevelBindingsAsset(const std::string_view path)
    {
        return ParseGameplayAILevelBindingsAsset(FILE_UTILS::ReadAllText(corefs::ResolveAsset(std::filesystem::path(path))), path);
    }
    GameplayAIDecisionCatalogAsset LoadGameplayAIDecisionCatalogAsset(const std::string_view path)
    {
        return ParseGameplayAIDecisionCatalogAsset(FILE_UTILS::ReadAllText(corefs::ResolveAsset(std::filesystem::path(path))), path);
    }
}
