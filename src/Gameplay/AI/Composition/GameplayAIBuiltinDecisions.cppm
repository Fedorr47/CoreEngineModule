module;

#include <cassert>

export module core:gameplay_ai_builtin_decisions;

import :gameplay_ai_access_key_composition;
import :gameplay_ai_access_key_contracts;
import :gameplay_goap_asset_composition;
import :gameplay_goap_builtin_components;
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
        // AccessKey is the remaining legacy composition until resource/event and
        // reservation adapters are authored. New spatial scenarios come from assets.
        RegisterGameplayAIDecisionAssets(registry,
            LoadGameplayAIDecisionCatalogAsset("ai/decisions/catalog.json"),
            MakeDefaultGameplayGOAPComponents());
        return registry;
    }
}
