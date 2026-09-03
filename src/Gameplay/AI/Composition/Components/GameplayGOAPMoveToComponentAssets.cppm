module;

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

export module core:gameplay_goap_move_to_component_assets;
export import :gameplay_ai_asset_parsing;

export namespace rendern
{
    struct GameplayAIMoveToAsset
    {
        std::string source, target;
        float acceptanceRadius{0.4f}, slowingRadius{1.5f};
        bool wantsRun{};
        float sourceRadius{};
        bool reserveTarget{}, routeCost{};
    };

    GameplayAIMoveToAsset ParseGameplayAIMoveTo(const GameplayAIComponentParseContext& input)
    {
        using namespace ai_asset_detail;
        const auto& object = input.object;
        const auto source = input.source, location = input.location;
        Fields(object, {"type", "context", "source", "target", "acceptanceRadius", "slowingRadius", "wantsRun", "sourceRadius", "reserveTarget", "routeCost"}, source, location);
        return GameplayAIMoveToAsset{
            .source = String(object, "source", source, location), .target = String(object, "target", source, location),
            .acceptanceRadius = Number(object, "acceptanceRadius", 0.4f, source, location),
            .slowingRadius = Number(object, "slowingRadius", 1.5f, source, location),
            .wantsRun = Boolean(object, "wantsRun", false, source, location),
            .sourceRadius = Number(object, "sourceRadius", 0, source, location),
            .reserveTarget = Boolean(object, "reserveTarget", false, source, location),
            .routeCost = Boolean(object, "routeCost", false, source, location)};
    }
}
