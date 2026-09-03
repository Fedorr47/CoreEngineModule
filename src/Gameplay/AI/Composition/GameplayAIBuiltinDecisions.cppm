export module core:gameplay_ai_builtin_decisions;

import :gameplay_ai_decision;
import :gameplay_goap_asset_composition;
import :gameplay_goap_builtin_components;

export namespace rendern
{
    [[nodiscard]] GameplayAIDecisionFactoryRegistry MakeDefaultGameplayAIDecisionFactories()
    {
        GameplayAIDecisionFactoryRegistry registry;
        RegisterGameplayAIDecisionAssets(registry,
            LoadGameplayAIDecisionCatalogAsset("ai/decisions/catalog.json"),
            MakeDefaultGameplayGOAPComponents());
        return registry;
    }
}
