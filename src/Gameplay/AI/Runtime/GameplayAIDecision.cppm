module;

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <map>
#include <utility>

export module core:gameplay_ai_decision;

export import :gameplay_ai_decision_contracts;
import :gameplay;
import :gameplay_traversal_executor_registry;
import :gameplay_traversal_link_registry;
import :gameplay_object_reservation_system;
import :level;

export namespace rendern
{
    // All references are non-owning and are valid only for the duration of Create().
    struct GameplayAIDecisionCreationContext
    {
        EntityHandle agent{kNullEntity};
        LevelAsset& level;
        GameplayWorld& world;
        const GameplayTraversalLinkRegistry& traversalLinkRegistry;
        const GameplayTraversalExecutorRegistry& traversalExecutorRegistry;
        GameplayObjectReservationSystem& reservationSystem;
    };

    using GameplayAIDecisionFactory = std::function<
        std::unique_ptr<GameplayAIDecisionInstance>(const GameplayAIDecisionCreationContext&)>;

    // The registry owns its factory callables. Registration is first-wins: a duplicate
    // identifier or an empty factory is rejected without changing the existing entry.
    class GameplayAIDecisionFactoryRegistry
    {
    public:
        [[nodiscard]] bool Register(
            const std::string_view definitionId, GameplayAIDecisionFactory factory)
        {
            if (definitionId.empty() || !factory)
            {
                return false;
            }
            return factories_.emplace(std::string{definitionId}, std::move(factory)).second;
        }

        [[nodiscard]] bool Contains(const std::string_view definitionId) const noexcept
        {
            return factories_.contains(definitionId);
        }
        
        [[nodiscard]] std::unique_ptr<GameplayAIDecisionInstance> Create(
            const std::string_view definitionId,
            const GameplayAIDecisionCreationContext& context) const
        {
            const auto found = factories_.find(definitionId);
            return found == factories_.end() ? nullptr : found->second(context);
        }

    private:
        std::map<std::string, GameplayAIDecisionFactory, std::less<>> factories_{};
    };
}