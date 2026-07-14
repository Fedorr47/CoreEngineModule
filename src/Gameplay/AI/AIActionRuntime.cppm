module;


export module core:ai_action_runtime;

import :EnTTHelpers;
export import :ai_decision_contracts;
#include <cstdint>

export namespace rendern
{
    using EnTT_helpers::EntityHandle;
    using EnTT_helpers::kNullEntity;

    struct AIActionRuntimeContext
    {
        EntityHandle agentEntity{kNullEntity};
        AIActionId actionId{};
        
        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            const bool bHasAgentEntity = agentEntity != kNullEntity;
            const bool bHasActionId = actionId.IsValid();
            
            return bHasAgentEntity && bHasActionId;
        }
        
        friend constexpr bool operator==(
            const AIActionRuntimeContext&, 
            const AIActionRuntimeContext&) = default;
    };
    
    enum class AIActionRuntimeResult : std::uint8_t
    {
        Running,
        Succeeded,
        Failed
    };
    
    class IAIActionRuntime
    {
    public:
        virtual ~IAIActionRuntime() = default;
        
        // Starts one action invocation for the supplied agent/action pair. The
        // caller owns lifecycle ordering and must observe only Running,
        // Succeeded, or Failed as valid start results; NotStarted is reserved
        // for future task-side state before Start is called.
        [[nodiscard]] virtual AIActionRuntimeResult Start(
            const AIActionRuntimeContext& context) = 0;
        
        // Advances an invocation that previously returned Running from Start.
        // The caller, not this interface, decides when Tick is allowed and how
        // terminal states are handled; valid tick results are Running,
        // Succeeded, or Failed.
        [[nodiscard]] virtual AIActionRuntimeResult Tick(
            const AIActionRuntimeContext& context,
            float deltaSeconds) = 0;
        
        // Provides an explicit interruption and cleanup boundary for a running
        // invocation. Cancellation does not apply planner effects; future task
        // state remains responsible for deciding when cancellation is legal.
        [[nodiscard]] virtual void Cancel(
            const AIActionRuntimeContext& context) noexcept = 0;
    };
}
