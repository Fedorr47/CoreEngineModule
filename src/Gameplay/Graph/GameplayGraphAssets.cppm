module;

#include <utility>

export module core:gameplay_graph_assets;

import :gameplay_graph;

export namespace rendern
{
    [[nodiscard]] inline GameplayGraphAsset MakeDefaultHumanoidGameplayGraphAsset()
    {
        GameplayGraphAsset asset{};
        asset.id = "default_humanoid_graph";

        GameplayGraphLayerDesc baseLayer{};
        baseLayer.name = "Base";
        baseLayer.defaultState = "Grounded";

        GameplayGraphStateDesc grounded{};
        grounded.name = "Grounded";
        grounded.transitions = {
            GameplayGraphTransitionDesc{
                .toState = "Action",
                .conditions = { GameplayGraphConditionDesc{ .name = "BoolTrue", .parameter = "hasActionRequest", .boolValue = true } }
            }
        };

        GameplayGraphStateDesc action{};
        action.name = "Action";
        action.onEnter = {
            GameplayGraphTaskDesc{ .name = "BeginActionState" }
        };
        action.transitions = {
            GameplayGraphTransitionDesc{
                .toState = "Grounded",
                .conditions = { GameplayGraphConditionDesc{ .name = "BoolFalse", .parameter = "actionBusy", .boolValue = false } }
            }
        };

        baseLayer.states.push_back(std::move(grounded));
        baseLayer.states.push_back(std::move(action));
        asset.layers.push_back(std::move(baseLayer));
        return asset;
    }
}
