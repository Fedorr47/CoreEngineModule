module;

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <utility>
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

namespace ProfileUtils
{
    struct SyncInststrumentionAggregate
    {
        std::uint64_t callCount{ 0 };
        std::uint64_t totalProcessedEntityCount{ 0 };
        std::chrono::nanoseconds totalDuration{ 0 };
        std::chrono::nanoseconds maxDuration{ 0 };
    };
}

export namespace rendern
{
    class GameplayRuntime
    {
    public:
        GameplayRuntime() = default;

        void Initialize(LevelAsset& levelAsset, LevelInstance& levelInstance, Scene& scene);
        void Shutdown();

        void BindIntentSource(const EntityHandle entity, GameplayIntentSourceCallback callback);
        void BindKeyboardMouseIntentSource(
            const EntityHandle entity,
            const GameplayKeyboardMouseBindings& bindings);
        void UnbindIntentSource(const EntityHandle entity);

        void BeginFrame();
        void PreAnimationUpdate(const GameplayUpdateContext& ctx);
        void PostAnimationUpdate(const GameplayUpdateContext& ctx);
        void SetSkipDuplicatePostAnimationSyncEnabled( bool enabled) noexcept;
        
        [[nodiscard]] bool IsSkipDuplicatePostAnimationSyncEnabled() const noexcept;
        [[nodiscard]] GameplayWorld& GetWorld() noexcept;
        [[nodiscard]] const GameplayWorld& GetWorld() const noexcept;
        [[nodiscard]] EntityHandle GetControlledEntity() const noexcept;
        [[nodiscard]] GameplayRuntimeMode GetCurrentMode() const noexcept;
        [[nodiscard]] const std::vector<EntityHandle>& GetNodeBoundEntities() const noexcept;

        [[nodiscard]] EntityHandle SpawnNodeBoundEntity(
            const GameplayUpdateContext& ctx,
            const int nodeIndex,
            const bool playerControlled);

    private:
        void ResetEntityFrameState_(const EntityHandle entity);
        void UpdateFollowCamera_(const GameplayUpdateContext& ctx, const bool consumeInput);
        void CompactTrackedState_();
        void RebuildIntentBindingIndex_();
        bool ValidateIntentBindingIndex_() const;
        void EraseIntentBindingAtStableIndex_(std::size_t index);
        void UpsertIntentBinding_(EntityHandle entity, GameplayIntentSourceCallback callback);
        void CreateDefaultGraphInstance_(EntityHandle entity);
        void SyncActionStateToGraphParameters_(EntityHandle entity, GameplayGraphInstance& graph);
        void WriteActionStateToGraphParameters_(GameplayGraphInstance& graph, const GameplayActionComponent* action);
        void ExecuteGameplayGraphs_(const GameplayUpdateContext& ctx);
        void ExecuteGraphLayer_(EntityHandle entity, GameplayGraphInstance& graph,
                                GameplayGraphLayerRuntimeState& runtimeLayer,
                                const GameplayGraphLayerDesc& assetLayer,
                                const GameplayUpdateContext& ctx);
        void ExecuteGraphTasks_(EntityHandle entity, GameplayGraphInstance& graph,
                                const std::vector<GameplayGraphTaskDesc>& tasks);
        void BeginActionState_(EntityHandle entity, GameplayGraphInstance& graph);
        void ResetSimulationState_();
        void HandleRuntimeModeChanged_(const GameplayUpdateContext& ctx);
        // Profiling zone start
        void EnsureBootstrapEntity_(const GameplayUpdateContext& ctx);
        void RecordSyncInstumentationSample_(
            ProfileUtils::SyncInststrumentionAggregate& aggregate,
            const std::chrono::nanoseconds duration,
            std::size_t processedEntityCount);
        void LogSyncInstumentationSample_() const;
        // Profiling zone end
    private:
        GameplayWorld world_{};
        EntityHandle controlledEntity_{ kNullEntity };
        std::vector<GameplayIntentBinding> intentBindings_{};
        std::unordered_map<EntityHandle, std::size_t> intentBindingIndexByEntity_{};
        // TODO: change it to methods like these 
        // ForEachCharacter(...)
        // ForEachAnimatedEntity(...)
        // ForEachPlayerControlled(...)
        std::vector<EntityHandle> nodeBoundEntities_{};
        std::unordered_map<EntityHandle, GameplayGraphInstance> graphInstances_{};
        GameplayGraphAsset defaultGraphAsset_{};
        GameplayRuntimeMode lastMode_{ GameplayRuntimeMode::Editor };
        std::vector<GameplayAnimationNotifyRecord> recentNotifyEvents_{};
        std::vector<GameplayEventRecord> recentGameplayEvents_{};
        GameplayFollowCameraController followCameraController_{};
        // Profiling zone start
        ProfileUtils::SyncInststrumentionAggregate preSyncInstAggregate_{};
        ProfileUtils::SyncInststrumentionAggregate postSyncInstAggregate_{};
        std::uint64_t postSyncSkippedFrameCount_{ 0 };
        std::uint64_t postSyncExecutedFrameCount_{ 0 };
        bool skipDuplicatePostAnimationSyncEnabled_{ false };
        // Profiling zone end
    };

#include "GameplayRuntime_PublicApi.inl"
#include "GameplayRuntime_FrameLifecycle.inl"
#include "GameplayRuntime_GraphExecution.inl"
#include "GameplayRuntime_ModeAndBootstrap.inl"
}
