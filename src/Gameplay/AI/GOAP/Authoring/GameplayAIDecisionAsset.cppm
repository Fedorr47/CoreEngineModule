module;

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <cstdint>
#include <optional>
#include <variant>
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
    struct GameplayAISpatialObservationAsset
    {
        std::string target, fact;
        float radius{};
        bool requireInteractionPoint{};
        bool requireEntity{true};
        bool latch{};
        std::vector<std::string> requiredFacts;
    };
    struct GameplayAIPickupAvailabilityAsset
    {
        std::string target, fact;
        bool respectReservations{};
    };
    struct GameplayAILocationAsset { std::string target, fact; };
    struct GameplayAINearestLocationAsset
    {
        float radius{};
        std::vector<GameplayAILocationAsset> locations;
    };
    struct GameplayAIResourcePickupAsset
    {
        std::string target, fact;
        std::int32_t amount{1};
    };
    struct GameplayAIResourceReceiptAsset
    {
        std::string id, target, fact;
        std::int32_t price{};
    };
    struct GameplayAIResourceLedgerAsset
    {
        std::string fact;
        std::vector<GameplayAIResourcePickupAsset> pickups;
        std::vector<GameplayAIResourceReceiptAsset> receipts;
    };
    struct GameplayAIObservationAsset
    {
        std::string type;
        std::variant<GameplayAISpatialObservationAsset, GameplayAIPickupAvailabilityAsset,
            GameplayAINearestLocationAsset, GameplayAIResourceLedgerAsset> parameters;
    };
    struct GameplayAIMoveToAsset
    {
        std::string source, target;
        float acceptanceRadius{0.4f}, slowingRadius{1.5f};
        bool wantsRun{};
        float sourceRadius{};
        bool reserveTarget{}, routeCost{};
    };
    struct GameplayAIPurchaseAsset
    {
        std::string receipt;
        float radius{0.6f};
    };
    struct GameplayAICapabilityAsset
    {
        std::string type, context;
        std::variant<GameplayAIMoveToAsset, GameplayAIPurchaseAsset> parameters;
    };
    struct GameplayAIRouteEdgeAsset
    {
        std::string from, to, traversal;
        std::optional<float> cost;
    };
    struct GameplayAIRouteGraphAsset
    {
        std::string source;
        std::vector<std::string> nodes;
        std::vector<GameplayAIRouteEdgeAsset> edges;
    };
    struct GameplayAITraversalBindingAsset
    {
        std::string name, target, type;
        std::uint64_t handle{};
    };

    struct GameplayAIBehaviorAsset
    {
        std::string id{};
        std::string source{};
        std::string model{};
        std::string definition{};
        std::vector<GameplayAIObservationAsset> observations{};
        std::vector<GameplayAICapabilityAsset> capabilities{};
        std::string routeGraph;
        bool inspectPath{};
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
        std::vector<GameplayAITraversalBindingAsset> traversals;
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

    [[nodiscard]] GameplayAIRouteGraphAsset ParseGameplayAIRouteGraphAsset(std::string_view json, std::string_view source);
    [[nodiscard]] GameplayAIRouteGraphAsset LoadGameplayAIRouteGraphAsset(std::string_view path);
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

namespace rendern
{
    GameplayAIBehaviorAsset ParseGameplayAIBehaviorAsset(std::string_view json, std::string_view source)
    {
        using namespace ai_asset_detail;
        const auto value = Root(json, source);
        const auto& root = value.AsObject();
        Fields(root, {"version", "id", "model", "definition", "observations", "capabilities", "routeGraph", "inspectPath"}, source, "root");
        GameplayAIBehaviorAsset result{
            .id = String(root, "id", source, "root"), .source = std::string(source),
            .model = String(root, "model", source, "root"), .definition = String(root, "definition", source, "root")};
        result.routeGraph = OptionalString(root, "routeGraph", source, "root");
        result.inspectPath = Boolean(root, "inspectPath", false, source, "root");
        for (const auto& entry : Array(root, "observations", source, "observations"))
        {
            const auto location = "observations[" + std::to_string(result.observations.size()) + "]";
            const auto& object = Object(entry, source, location);
            GameplayAIObservationAsset observer{.type = String(object, "type", source, location)};
            if (observer.type == "target_available" || observer.type == "within_distance")
            {
                Fields(object, {"type", "target", "fact", "radius", "requireInteractionPoint", "requireEntity", "latch", "requiredFacts"}, source, location);
                GameplayAISpatialObservationAsset parameters{
                    .target = String(object, "target", source, location), .fact = String(object, "fact", source, location),
                    .radius = Number(object, "radius", 0, source, location),
                    .requireInteractionPoint = Boolean(object, "requireInteractionPoint", false, source, location),
                    .requireEntity = Boolean(object, "requireEntity", true, source, location),
                    .latch = Boolean(object, "latch", false, source, location)};
                if (object.contains("requiredFacts"))
                {
                    parameters.requiredFacts = Strings(object, "requiredFacts", source, location);
                }
                observer.parameters = std::move(parameters);
            }
            else if (observer.type == "pickup_available")
            {
                Fields(object, {"type", "target", "fact", "respectReservations"}, source, location);
                observer.parameters = GameplayAIPickupAvailabilityAsset{
                    String(object, "target", source, location), String(object, "fact", source, location),
                    Boolean(object, "respectReservations", false, source, location)};
            }
            else if (observer.type == "nearest_location")
            {
                Fields(object, {"type", "radius", "locations"}, source, location);
                GameplayAINearestLocationAsset parameters{.radius = Number(object, "radius", 0, source, location)};
                for (const auto& item : Array(object, "locations", source, location))
                {
                    const auto& node = Object(item, source, location);
                    Fields(node, {"target", "fact"}, source, location);
                    parameters.locations.push_back({String(node, "target", source, location), String(node, "fact", source, location)});
                }
                observer.parameters = std::move(parameters);
            }
            else if (observer.type == "resource_ledger")
            {
                Fields(object, {"type", "fact", "pickups", "receipts"}, source, location);
                GameplayAIResourceLedgerAsset parameters{.fact = String(object, "fact", source, location)};
                for (const auto& item : Array(object, "pickups", source, location))
                {
                    const auto& pickup = Object(item, source, location);
                    Fields(pickup, {"target", "fact", "amount"}, source, location);
                    parameters.pickups.push_back({String(pickup, "target", source, location),
                        String(pickup, "fact", source, location), static_cast<std::int32_t>(
                            PositiveInteger(pickup, "amount", INT32_MAX, source, location))});
                }
                for (const auto& item : Array(object, "receipts", source, location))
                {
                    const auto& receipt = Object(item, source, location);
                    Fields(receipt, {"id", "target", "fact", "price"}, source, location);
                    parameters.receipts.push_back({String(receipt, "id", source, location),
                        String(receipt, "target", source, location), String(receipt, "fact", source, location),
                        static_cast<std::int32_t>(PositiveInteger(receipt, "price", INT32_MAX, source, location))});
                }
                observer.parameters = std::move(parameters);
            }
            else
            {
                Invalid(source, location, "unknown observation type '" + observer.type + "'");
            }
            result.observations.push_back(std::move(observer));
        }
        std::unordered_set<std::string> contexts;
        for (const auto& entry : Array(root, "capabilities", source, "capabilities"))
        {
            const auto location = "capabilities[" + std::to_string(result.capabilities.size()) + "]";
            const auto& object = Object(entry, source, location);
            GameplayAICapabilityAsset capability{.type = String(object, "type", source, location),
                .context = String(object, "context", source, location)};
            if (capability.type == "move_to")
            {
                Fields(object, {"type", "context", "source", "target", "acceptanceRadius", "slowingRadius", "wantsRun", "sourceRadius", "reserveTarget", "routeCost"}, source, location);
                capability.parameters = GameplayAIMoveToAsset{
                    .source = String(object, "source", source, location), .target = String(object, "target", source, location),
                    .acceptanceRadius = Number(object, "acceptanceRadius", 0.4f, source, location),
                    .slowingRadius = Number(object, "slowingRadius", 1.5f, source, location),
                    .wantsRun = Boolean(object, "wantsRun", false, source, location),
                    .sourceRadius = Number(object, "sourceRadius", 0, source, location),
                    .reserveTarget = Boolean(object, "reserveTarget", false, source, location),
                    .routeCost = Boolean(object, "routeCost", false, source, location)};
            }
            else if (capability.type == "purchase")
            {
                Fields(object, {"type", "context", "receipt", "radius"}, source, location);
                capability.parameters = GameplayAIPurchaseAsset{String(object, "receipt", source, location),
                    Number(object, "radius", 0.6f, source, location)};
            }
            else
            {
                Invalid(source, location, "unknown capability '" + capability.type + "'");
            }
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
        Fields(root, {"version", "roles", "traversals"}, source, "root");
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
        if (root.contains("traversals"))
        {
            std::unordered_set<std::string> names;
            for (const auto& entry : Array(root, "traversals", source, "traversals"))
            {
                const auto& object = Object(entry, source, "traversals");
                Fields(object, {"name", "target", "type", "handle"}, source, "traversals");
                GameplayAITraversalBindingAsset binding{String(object, "name", source, "traversals"),
                    String(object, "target", source, "traversals"), String(object, "type", source, "traversals"),
                    PositiveInteger(object, "handle", 9007199254740991ULL, source, "traversals")};
                if (!names.insert(binding.name).second)
                {
                    Invalid(source, "traversals", "duplicate traversal name");
                }
                result.traversals.push_back(std::move(binding));
            }
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

    GameplayAIRouteGraphAsset ParseGameplayAIRouteGraphAsset(std::string_view json, std::string_view source)
    {
        using namespace ai_asset_detail;
        const auto value = Root(json, source);
        const auto& root = value.AsObject();
        Fields(root, {"version", "nodes", "edges"}, source, "root");
        GameplayAIRouteGraphAsset result{.source = std::string(source), .nodes = Strings(root, "nodes", source, "nodes")};
        for (const auto& value : Array(root, "edges", source, "edges"))
        {
            const auto& edge = Object(value, source, "edges");
            Fields(edge, {"from", "to", "traversal", "cost"}, source, "edges");
            result.edges.push_back({String(edge, "from", source, "edges"), String(edge, "to", source, "edges"),
                OptionalString(edge, "traversal", source, "edges"), edge.contains("cost")
                    ? std::optional{Number(edge, "cost", 0, source, "edges")} : std::nullopt});
        }
        return result;
    }
    GameplayAIRouteGraphAsset LoadGameplayAIRouteGraphAsset(std::string_view path)
    {
        return ParseGameplayAIRouteGraphAsset(FILE_UTILS::ReadAllText(corefs::ResolveAsset(std::filesystem::path(path))), path);
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
