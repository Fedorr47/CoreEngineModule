module;

#include <algorithm>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

export module core:gameplay_runtime;

import :gameplay;
export import :gameplay_runtime_common;
import :gameplay_graph;
import :gameplay_graph_assets;
import :gameplay_input_system;
import :gameplay_bootstrap;
import :gameplay_scene_sync;
import :gameplay_follow_camera;
import :character_controller;
import :character_movement;
import :combat_system;
import :interaction_system;
import :gameplay_animation_bridge;
import :gameplay_animation_bridge_system;

export namespace rendern
{
    class GameplayRuntime
    {
    public:
#include "GameplayRuntime_PublicApi.inl"

    private:
#include "GameplayRuntime_FrameLifecycle.inl"
#include "GameplayRuntime_GraphExecution.inl"
#include "GameplayRuntime_ModeAndBootstrap.inl"

    private:
        GameplayWorld world_{};
        EntityHandle controlledEntity_{ kNullEntity };
        std::vector<GameplayIntentBinding> intentBindings_{};
        std::vector<EntityHandle> nodeBoundEntities_{};
        std::unordered_map<EntityHandle, GameplayGraphInstance> graphInstances_{};
        GameplayGraphAsset defaultGraphAsset_{};
        GameplayRuntimeMode lastMode_{ GameplayRuntimeMode::Editor };
        std::vector<GameplayAnimationNotifyRecord> recentNotifyEvents_{};
        std::vector<GameplayEventRecord> recentGameplayEvents_{};
        GameplayFollowCameraController followCameraController_{};
    };
}
