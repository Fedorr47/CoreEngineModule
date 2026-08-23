module;

#include <array>
#include <memory>
#include <optional>
#include <string_view>

export module core:gameplay_ai_decision;

export import :gameplay_ai_decision_contracts;
export import :gameplay_goap_decision;
export import :gameplay_ai_access_key_decision;
import :gameplay_traversal_executor_registry;
import :gameplay_traversal_link_registry;
import :level;

export namespace rendern
{
    [[nodiscard]] bool IsGameplayAIDecisionDefinitionRegistered(
        const std::string_view definitionId) noexcept
    {
        return definitionId == kAccessKeyAIDecisionId;
    }

    [[nodiscard]] std::unique_ptr<GameplayAIDecisionInstance> CreateGameplayAIDecision(
        const std::string_view definitionId, const EntityHandle agent, LevelAsset& level,
        GameplayWorld& world,
        const GameplayTraversalLinkRegistry& traversalLinkRegistry,
        const GameplayTraversalExecutorRegistry& traversalExecutorRegistry)
    {
        if (definitionId == kAccessKeyAIDecisionId)
        {
            return CreateAccessKeyAIDecision(agent, level, world,
                traversalLinkRegistry, traversalExecutorRegistry);
        }
        return nullptr;
    }
}