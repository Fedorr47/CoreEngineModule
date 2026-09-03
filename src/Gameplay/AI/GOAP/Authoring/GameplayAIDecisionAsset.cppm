module;

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <cstdint>
#include <optional>
#include <any>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

export module core:gameplay_ai_decision_asset;

import :file_system;
export import :gameplay_ai_asset_parsing;

export namespace rendern
{
    struct GameplayAIObservationAsset
    {
        std::string type;
        std::any parameters;
    };
    struct GameplayAICapabilityAsset
    {
        std::string type, context;
        std::any parameters;
    };
    struct GameplayAIReactionAsset
    {
        std::string type;
        std::any parameters;
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
        std::vector<GameplayAIReactionAsset> reactions;
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
        std::string_view json, std::string_view source, const GameplayAIComponentParsers& parsers);
    [[nodiscard]] GameplayAILevelBindingsAsset ParseGameplayAILevelBindingsAsset(
        std::string_view json, std::string_view source);
    [[nodiscard]] GameplayAIDecisionCatalogAsset ParseGameplayAIDecisionCatalogAsset(
        std::string_view json, std::string_view source);
    [[nodiscard]] GameplayAIBehaviorAsset LoadGameplayAIBehaviorAsset(std::string_view path, const GameplayAIComponentParsers& parsers);
    [[nodiscard]] GameplayAILevelBindingsAsset LoadGameplayAILevelBindingsAsset(std::string_view path);
    [[nodiscard]] GameplayAIDecisionCatalogAsset LoadGameplayAIDecisionCatalogAsset(std::string_view path);
}


namespace rendern
{
    GameplayAIBehaviorAsset ParseGameplayAIBehaviorAsset(std::string_view json, std::string_view source,
        const GameplayAIComponentParsers& parsers)
    {
        using namespace ai_asset_detail;
        const auto value = Root(json, source);
        const auto& root = value.AsObject();
        Fields(root, {"version", "id", "model", "definition", "observations", "capabilities", "routeGraph", "inspectPath", "reactions"}, source, "root");
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
            observer.parameters = parsers.Parse(GameplayAIComponentKind::Observation, observer.type,
                {object, source, location});
            result.observations.push_back(std::move(observer));
        }
        if (root.contains("reactions"))
        {
            for (const auto& entry : Array(root, "reactions", source, "reactions"))
            {
                const auto location = "reactions[" + std::to_string(result.reactions.size()) + "]";
                const auto& object = Object(entry, source, location);
                GameplayAIReactionAsset reaction{.type = String(object, "type", source, location)};
                reaction.parameters = parsers.Parse(GameplayAIComponentKind::Reaction, reaction.type,
                    {object, source, location});
                result.reactions.push_back(std::move(reaction));
            }
        }
        std::unordered_set<std::string> contexts;
        for (const auto& entry : Array(root, "capabilities", source, "capabilities"))
        {
            const auto location = "capabilities[" + std::to_string(result.capabilities.size()) + "]";
            const auto& object = Object(entry, source, location);
            GameplayAICapabilityAsset capability{.type = String(object, "type", source, location),
                .context = String(object, "context", source, location)};
            capability.parameters = parsers.Parse(GameplayAIComponentKind::Capability, capability.type,
                {object, source, location});
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

    GameplayAIBehaviorAsset LoadGameplayAIBehaviorAsset(const std::string_view path, const GameplayAIComponentParsers& parsers)
    {
        return ParseGameplayAIBehaviorAsset(FILE_UTILS::ReadAllText(corefs::ResolveAsset(std::filesystem::path(path))), path, parsers);
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
