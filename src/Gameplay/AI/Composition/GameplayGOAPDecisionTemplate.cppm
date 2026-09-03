module;

#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module core:gameplay_goap_decision_template;

export import :gameplay_goap_composition_registry;
export import :gameplay_goap_definition_asset;

export namespace rendern
{
    struct GameplayGOAPPreparedCapabilityGroup
    {
        std::string type;
        std::vector<GameplayAICapabilityAsset> assets;
        GameplayGOAPCapabilityRegistration registration;
    };

    // A validated configuration snapshot, independent of any world, level or agent.
    // All access is const; providers and fact state are created separately per instance.
    class GameplayGOAPDecisionTemplate
    {
    public:
        GameplayGOAPDecisionTemplate(GameplayAIBehaviorAsset behavior, GameplayAILevelBindingsAsset bindings,
            const GameplayGOAPDefinitionAsset& definition, GameplayGOAPCompositionRegistry components,
            std::optional<GameplayAIRouteGraphAsset> graph = {})
            : behavior_(std::move(behavior)), bindings_(std::move(bindings)), graph_(std::move(graph)),
              components_(std::move(components)), definitionSource_(definition.source)
        {
            if (behavior_.model != "goap")
            {
                Fail_("unknown decision model '" + behavior_.model + "'");
            }
            if (!behavior_.routeGraph.empty() && !graph_)
            {
                Fail_("missing authored route graph input");
            }
            compiled_ = CompileGameplayGOAPDefinition(definition, components_.SemanticActions());
            for (const auto& observer : behavior_.observations)
            {
                if (components_.Observation(observer.type) == nullptr)
                {
                    Fail_("unknown observation type '" + observer.type + "'");
                }
            }
            for (const auto& reaction : behavior_.reactions)
            {
                if (components_.Reaction(reaction.type) == nullptr)
                {
                    Fail_("unknown reaction type '" + reaction.type + "'");
                }
            }
            std::set<std::string> boundContexts;
            std::map<std::string, std::vector<GameplayAICapabilityAsset>> groups;
            for (const auto& asset : behavior_.capabilities)
            {
                if (components_.Capability(asset.type) == nullptr)
                {
                    Fail_("unknown capability '" + asset.type + "'");
                }
                if (!boundContexts.insert(asset.context).second)
                {
                    Fail_("multiple capability bindings for '" + asset.context + "'");
                }
                const auto authored = std::ranges::find(definition.actions, asset.context,
                    &GameplayGOAPAuthoredAction::context);
                if (authored == definition.actions.end() || authored->action != asset.type)
                {
                    Fail_("capability does not match a definition action/context '" + asset.context + "'");
                }
                groups[asset.type].push_back(asset);
            }
            for (const auto& action : definition.actions)
            {
                if (!boundContexts.contains(action.context))
                {
                    Fail_("action context '" + action.context + "' has no capability binding");
                }
            }
            for (auto& [type, assets] : groups)
            {
                capabilities_.push_back({type, std::move(assets), *components_.Capability(type)});
            }
        }

        [[nodiscard]] const GameplayAIBehaviorAsset& Behavior() const noexcept { return behavior_; }
        [[nodiscard]] const GameplayAILevelBindingsAsset& Bindings() const noexcept { return bindings_; }
        [[nodiscard]] const GameplayAIRouteGraphAsset* RouteGraph() const noexcept { return graph_ ? &*graph_ : nullptr; }
        [[nodiscard]] const GameplayGOAPCompiledDefinition& Compiled() const noexcept { return compiled_; }
        [[nodiscard]] const GameplayGOAPCompositionRegistry& Components() const noexcept { return components_; }
        [[nodiscard]] const std::vector<GameplayGOAPPreparedCapabilityGroup>& Capabilities() const noexcept { return capabilities_; }

        // Route-derived costs belong to the instance's level. Patch a private copy
        // of the compiled definition without repeating parsing or symbol compilation.
        [[nodiscard]] GameplayGOAPDecisionDefinition InstanceDefinition(
            std::span<const GameplayGOAPActionCostOverride> overrides) const
        {
            auto result = compiled_.definition;
            std::set<std::pair<std::string, std::string>> keys;
            for (const auto& value : overrides)
            {
                const auto invalid = [&](std::string_view reason)
                {
                    throw std::runtime_error("GOAP definition '" + definitionSource_ + "', costOverrides '"
                        + value.context + "': " + std::string(reason));
                };
                if (!std::isfinite(value.cost) || value.cost < 0)
                {
                    invalid("cost must be finite and non-negative");
                }
                if (!keys.emplace(value.action, value.context).second)
                {
                    invalid("duplicate override key");
                }
                const auto named = std::ranges::find_if(result.metadata.actions, [&](const auto& action)
                {
                    return action.actionName == value.action && action.contextName == value.context;
                });
                if (named == result.metadata.actions.end())
                {
                    invalid("override does not match an authored action");
                }
                const auto action = std::ranges::find_if(result.actions, [&](const auto& entry)
                {
                    return entry.actionId == named->actionId && entry.contextId == named->contextId;
                });
                action->baseCost = value.cost;
            }
            return result;
        }

    private:
        [[noreturn]] void Fail_(std::string_view reason) const
        {
            throw std::runtime_error("AI behavior '" + behavior_.source + "': " + std::string(reason));
        }
        GameplayAIBehaviorAsset behavior_;
        GameplayAILevelBindingsAsset bindings_;
        std::optional<GameplayAIRouteGraphAsset> graph_;
        GameplayGOAPCompositionRegistry components_;
        GameplayGOAPCompiledDefinition compiled_;
        std::string definitionSource_;
        std::vector<GameplayGOAPPreparedCapabilityGroup> capabilities_;
    };
}
