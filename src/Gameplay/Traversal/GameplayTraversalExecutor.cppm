module;

#include <cstdint>

export module core:gameplay_traversal_executor;

import :gameplay;
import :gameplay_route;
import :gameplay_traversal_link;

export namespace rendern
{
    enum class GameplayTraversalExecutionResult : std::uint8_t
    {
        Running,
        Succeeded,
        Failed
    };

    struct GameplayTraversalExecutionContext
    {
        EntityHandle agentEntity{kNullEntity};
        GameplayTraversalLinkHandle traversalLink{};
        GameplayTraversalTypeId traversalTypeId{};
        EntityHandle targetEntity{kNullEntity};

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return agentEntity != kNullEntity 
            && traversalLink.IsValid() 
            && traversalTypeId.IsValid() 
            && targetEntity != kNullEntity;
        }
    };

    class IGameplayTraversalExecutor
    {
    public:
        virtual ~IGameplayTraversalExecutor() = default;
        [[nodiscard]] virtual GameplayTraversalExecutionResult Start(
            const GameplayTraversalExecutionContext& context) = 0;
        [[nodiscard]] virtual GameplayTraversalExecutionResult Tick(
            const GameplayTraversalExecutionContext& context, float deltaSeconds) = 0;
        virtual void Cancel(const GameplayTraversalExecutionContext& context) noexcept = 0;
    };

    class GameplayUnsupportedTraversalExecutor final : public IGameplayTraversalExecutor
    {
    public:
        [[nodiscard]] GameplayTraversalExecutionResult Start(const GameplayTraversalExecutionContext&) override
        {
            return GameplayTraversalExecutionResult::Failed;
        }
        [[nodiscard]] GameplayTraversalExecutionResult Tick(const GameplayTraversalExecutionContext&, float) override
        {
            return GameplayTraversalExecutionResult::Failed;
        }
        void Cancel(const GameplayTraversalExecutionContext&) noexcept override {}
    };
}