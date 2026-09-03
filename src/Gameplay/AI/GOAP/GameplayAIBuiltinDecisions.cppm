module;

#include <cassert>

export module core:gameplay_ai_builtin_decisions;

import :gameplay_ai_access_key_composition;
import :gameplay_ai_access_key_contracts;
import :gameplay_ai_target_recovery_composition;
import :gameplay_ai_decision;

export namespace rendern
{
    // Application composition root: keys select configurations of reusable models.
    [[nodiscard]] GameplayAIDecisionFactoryRegistry MakeDefaultGameplayAIDecisionFactories()
    {
        GameplayAIDecisionFactoryRegistry registry;
        const bool registered = registry.Register(kAccessKeyAIDecisionId,
            [](const GameplayAIDecisionCreationContext& context)
            {
                return CreateAccessKeyAIDecision(context.agent, context.level, context.world,
                    context.traversalLinkRegistry, context.traversalExecutorRegistry,
                    &context.reservationSystem);
            });
        assert(registered && "Built-in AI decision factories must register exactly once.");
        const bool recoveryRegistered = registry.Register(kTargetRecoveryAIDecisionId,
            [](const GameplayAIDecisionCreationContext& context)
            {
                return CreateTargetRecoveryAIDecision(context.agent, context.level, context.world,
                    context.traversalLinkRegistry, context.traversalExecutorRegistry);
            });
        assert(recoveryRegistered && "Built-in AI decision factories must register exactly once.");
        return registry;
    }
}
