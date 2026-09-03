module;

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <map>
#include <utility>

export module core:gameplay_ai_decision;

export import :gameplay_ai_decision_contracts;
export import :gameplay_ai_decision_creation_context;

export namespace rendern
{
    using GameplayAIDecisionFactory = std::function<
        std::unique_ptr<GameplayAIDecisionInstance>(const GameplayAIDecisionCreationContext&)>;

    // Keys identify authored/configured decisions, not C++ decision model types.
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
            if (context.diagnostic != nullptr)
            {
                context.diagnostic->clear();
            }
            if (found == factories_.end())
            {
                if (context.diagnostic != nullptr)
                {
                    *context.diagnostic = "Unknown AI decision '" + std::string(definitionId) + "'";
                }
                return nullptr;
            }
            return found->second(context);
        }

    private:
        std::map<std::string, GameplayAIDecisionFactory, std::less<>> factories_{};
    };
}