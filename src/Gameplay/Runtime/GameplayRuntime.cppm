module;

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cassert>

#include "Core/ThreadAffinity/ThreadAffinityAssertions.h"

export module core:gameplay_runtime;

import :gameplay;
import :ai_action_contracts;
export import :gameplay_runtime_common;
import :gameplay_graph;
import :gameplay_graph_assets;
import :gameplay_input_system;
import :gameplay_bootstrap;
import :ai_system;
import :ai_follow_route_action;
import :ai_move_to_action;
import :ai_move_to_action_binding;
import :ai_decision_runtime;
import :gameplay_ai_decision;
import :gameplay_goap_inspection;
import :gameplay_object_reservation_system;
import :gameplay_route;
import :gameplay_route_search;
import :gameplay_steering;
import :gameplay_traversal_link;
import :gameplay_traversal_link_registry;
import :gameplay_traversal_executor;
import :gameplay_traversal_executor_registry;
import :gameplay_scene_sync;
import :gameplay_pickup_system;
import :gameplay_follow_camera;
import :character_controller;
import :character_movement;
import :combat_system;
import :interaction_system;
import :gameplay_animation_bridge;
import :gameplay_animation_bridge_system;
import :thread_affinity;
import :door_traversal_executor;
import :jump_traversal_executor;

namespace ProfileUtils
{
    struct SyncInstrumentationAggregate
    {
        std::uint64_t callCount{ 0 };
        std::uint64_t totalProcessedEntityCount{ 0 };
        std::chrono::nanoseconds totalDuration{ 0 };
        std::chrono::nanoseconds maxDuration{ 0 };
    };
}

export namespace rendern
{
    struct GameplayAIDebugAgentView
    {
        EntityHandle agent{kNullEntity};
        AIDebugViewModel snapshot{};
    };
    
    // TODO: maybe transform to a facade pattern
    class GameplayRuntime
    {
    public:
        GameplayRuntime()
        {
            const bool bRegisteredDoor = traversalExecutorRegistry_.RegisterRuntimeOwned(
                DoorTraversalExecutor::kTypeId, doorTraversalExecutor_);
            const bool bRegisteredJump = traversalExecutorRegistry_.RegisterRuntimeOwned(
                JumpTraversalExecutor::kTypeId, jumpTraversalExecutor_);
            assert(bRegisteredDoor && bRegisteredJump &&
                "Built-in traversal executors must register exactly once.");
        }

        void Initialize(LevelAsset& levelAsset, LevelInstance& levelInstance, Scene& scene);
        void Shutdown();

        void BindIntentSource(const EntityHandle entity, GameplayIntentSourceCallback callback);
        void BindKeyboardMouseIntentSource(
            const EntityHandle entity,
            const GameplayKeyboardMouseBindings& bindings);
        void UnbindIntentSource(const EntityHandle entity);
        [[nodiscard]] bool ApplyKeyboardMouseBindings(const GameplayKeyboardMouseBindings& bindings);
        [[nodiscard]] const GameplayKeyboardMouseBindings& GetKeyboardMouseBindings() const noexcept;
        [[nodiscard]] const GameplayActionDefinitions& GetGameplayActionDefinitions() const noexcept;
        [[nodiscard]] const GameplayActionAnimationBindings& GetGameplayActionAnimationBindings() const noexcept;
        [[nodiscard]] bool ApplyGameplayActionConfiguration(
            const GameplayActionDefinitions& definitions,
            const GameplayActionAnimationBindings& animationBindings,
            std::string& diagnostic);

        void BeginFrame();
        void PrePhysicsUpdate(const GameplayUpdateContext& ctx);
        void PostPhysicsUpdate(const GameplayUpdateContext& ctx);
        void PostAnimationUpdate(const GameplayUpdateContext& ctx);
        void SetSkipDuplicatePostAnimationSyncEnabled( bool enabled) noexcept;
        
        [[nodiscard]] bool IsSkipDuplicatePostAnimationSyncEnabled() const noexcept;
        [[nodiscard]] GameplayWorld& GetWorld() noexcept;
        [[nodiscard]] const GameplayWorld& GetWorld() const noexcept;
        [[nodiscard]] EntityHandle GetControlledEntity() const noexcept;
        [[nodiscard]] GameplayRuntimeMode GetCurrentMode() const noexcept;
        [[nodiscard]] bool IsCurrentLevelAsset(const LevelAsset& levelAsset) const noexcept;
        [[nodiscard]] bool IsCurrentLevelContext(const GameplayUpdateContext& ctx) const noexcept;
        [[nodiscard]] const std::vector<EntityHandle>& GetNodeBoundEntities() const noexcept;
        [[nodiscard]] std::span<const GameplayWorldEvent> GetCurrentWorldEvents() const noexcept;
        void ClearCurrentWorldEvents() noexcept;

        [[nodiscard]] EntityHandle SpawnNodeBoundEntity(
            const GameplayUpdateContext& ctx,
            const int nodeIndex,
            const bool playerControlled);
        [[nodiscard]] bool DestroyNodeBoundEntity(EntityHandle entity);
        [[nodiscard]] AIActionExecutionStatus StartAIFollowRoute(
            EntityHandle agentEntity,
            GameplayRoute route,
            const GameplayArrivalSteeringSettings& steeringSettings = {});
        [[nodiscard]] AIActionExecutionStatus StartAIMoveTo(
            EntityHandle agentEntity,
            const GameplayRouteGraph& routeGraph,
            GameplayRouteNodeId startNodeId,
            GameplayRouteNodeId goalNodeId,
            const GameplayArrivalSteeringSettings& steeringSettings = {});
        [[nodiscard]] std::unique_ptr<AIMoveToActionBinding> CreateAIMoveToActionBinding(
           IAIMoveToActionRequestProvider& requestProvider);
        [[nodiscard]] AIPlanExecutionStatus UpdateAIDecision(
            AIDecisionRuntime& decision,
            const AIAgentWorldState& observedState,
            std::span<const AIGoalSelectionCandidate> candidates,
            std::span<const AIActionDefinition> actions,
            const AIActionBindingRegistry& bindings);
        void CancelAIDecision(AIDecisionRuntime& decision) noexcept;
        [[nodiscard]] bool StartAIDecision(EntityHandle agentEntity, std::string_view definitionId);
        void CancelAIDecision(EntityHandle agentEntity) noexcept;
        [[nodiscard]] AIPlanExecutionStatus GetAIDecisionStatus(EntityHandle agentEntity) const noexcept;
        [[nodiscard]] const AIAgentWorldState* GetAIDecisionObservedState(EntityHandle agentEntity) const noexcept;
        [[nodiscard]] std::vector<GameplayAIDebugAgentView> BuildAIDebugAgentViews() const;
        [[nodiscard]] bool HasAIDecisionDefinition(std::string_view definitionId) const noexcept;
        [[nodiscard]] bool RegisterGameplayTraversalLink(GameplayTraversalLink link);
        [[nodiscard]] bool RemoveGameplayTraversalLink(GameplayTraversalLinkHandle handle) noexcept;
        [[nodiscard]] std::optional<GameplayTraversalLink> FindGameplayTraversalLink(
            GameplayTraversalLinkHandle handle) const noexcept;
        [[nodiscard]] bool RegisterGameplayTraversalExecutor(
            GameplayTraversalTypeId typeId,
            IGameplayTraversalExecutor& executor);
        [[nodiscard]] bool RemoveGameplayTraversalExecutor(
            GameplayTraversalTypeId typeId) noexcept;
        [[nodiscard]] bool HasGameplayTraversalExecutor(
            GameplayTraversalTypeId typeId) const noexcept;
        void CancelAIAction(EntityHandle agentEntity);
        void ClearAIAction(EntityHandle agentEntity);
        // Clears one node-bound entity's transient action, movement and graph state.
        void ResetNodeBoundEntitySimulationState(EntityHandle entity);
        [[nodiscard]] AIActionExecutionStatus GetAIActionStatus(EntityHandle agentEntity) const noexcept;
        [[nodiscard]] bool TryReserveGameplayObject(EntityHandle objectEntity, EntityHandle agentEntity);
        [[nodiscard]] bool ReleaseGameplayObject(EntityHandle objectEntity, EntityHandle agentEntity) noexcept;
        [[nodiscard]] bool IsGameplayObjectReserved(EntityHandle objectEntity) const noexcept;
        [[nodiscard]] bool IsGameplayObjectReservedBy(EntityHandle objectEntity, EntityHandle agentEntity) const noexcept;
        [[nodiscard]] EntityHandle GetGameplayObjectReservationOwner(EntityHandle objectEntity) const noexcept;

    private:
        void ResetEntityFrameState_(const EntityHandle entity);
        void UpdateActiveAIDecisions_();
        void CancelAllAIDecisions_() noexcept;
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
        void ResetEntitySimulationState_(EntityHandle entity);
        void RemoveDeadNodeBoundEntities_(const GameplayUpdateContext& ctx);
        void HandleRuntimeModeChanged_(const GameplayUpdateContext& ctx);
        // Profiling zone start
        void EnsureBootstrapEntity_(const GameplayUpdateContext& ctx);
        void RecordSyncInstrumentationSample_(
            ProfileUtils::SyncInstrumentationAggregate& aggregate,
            const std::chrono::nanoseconds duration,
            std::size_t processedEntityCount);
        void LogSyncInstrumentationSample_() const;
        // Profiling zone end
    private:
        GameplayWorld world_{};
        EntityHandle controlledEntity_{ kNullEntity };
        GameplayKeyboardMouseBindings keyboardMouseBindings_{};
        GameplayActionDefinitions actionDefinitions_{ MakeDefaultGameplayActionDefinitions() };
        GameplayActionAnimationBindings actionAnimationBindings_{ MakeDefaultGameplayActionAnimationBindings() };
        LevelAsset* currentLevelAsset_{ nullptr };
        LevelInstance* currentLevelInstance_{ nullptr };
        Scene* currentScene_{ nullptr };
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
        AISystem aiSystem_{};
        std::unordered_map<EntityHandle, std::unique_ptr<GameplayAIDecisionInstance>> activeAIDecisions_{};
        GameplayTraversalLinkRegistry traversalLinkRegistry_{};
        GameplayTraversalExecutorRegistry traversalExecutorRegistry_{};
        GameplayUnsupportedTraversalExecutor unsupportedTraversalExecutor_{};
        GameplayObjectReservationSystem objectReservationSystem_{};
        GameplayPickupSystem pickupSystem_{};
        DoorTraversalExecutor doorTraversalExecutor_{world_, objectReservationSystem_};
        JumpTraversalExecutor jumpTraversalExecutor_{world_, traversalLinkRegistry_};
        std::vector<GameplayAnimationNotifyRecord> recentNotifyEvents_{};
        std::vector<GameplayEventRecord> recentGameplayEvents_{};
        std::vector<GameplayWorldEvent> currentWorldEvents_{};
        GameplayFollowCameraController followCameraController_{};
        // Profiling zone start
        ProfileUtils::SyncInstrumentationAggregate preSyncInstAggregate_{};
        ProfileUtils::SyncInstrumentationAggregate postSyncInstAggregate_{};
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
