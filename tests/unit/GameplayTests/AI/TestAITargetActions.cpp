#include <cmath>
#include <algorithm>
#include <gtest/gtest.h>

import core;

using namespace rendern;

namespace
{
    class ForwardBlockedQuery final : public IGameplayObstacleQuery
    {
    public:
        [[nodiscard]] bool Probe(
            const GameplayObstacleProbeRequest& request,
            GameplayObstacleProbeHit& hit) const noexcept override
        {
            requests[callCount < 3u ? callCount : 2u] = request;
            ++callCount;
            if ((callCount % 3u) == 1u)
            {
                hit.distance = 0.2f;
                return true;
            }
            return false;
        }

        mutable GameplayObstacleProbeRequest requests[3]{};
        mutable std::size_t callCount{0u};
    };
    
    class FollowHysteresisQuery final : public IGameplayObstacleQuery
    {
    public:
        [[nodiscard]] bool Probe(
            const GameplayObstacleProbeRequest&,
            GameplayObstacleProbeHit& hit) const noexcept override
        {
            constexpr float clearances[3][3]{
                {0.1f, 0.8f, 0.7f},
                {0.1f, 0.8f, 0.85f},
                {0.1f, 0.8f, 0.85f}};
            const std::size_t evaluation = std::min(callCount / 3u, std::size_t{2u});
            hit.distance = clearances[evaluation][callCount % 3u];
            ++callCount;
            return true;
        }

        mutable std::size_t callCount{0u};
    };
    
    class SlopeAwareFollowQuery final : public IGameplayObstacleQuery
    {
    public:
        [[nodiscard]] bool Probe(
            const GameplayObstacleProbeRequest&,
            GameplayObstacleProbeHit& hit) const noexcept override
        {
            constexpr float clearances[]{0.1f, 0.9f, 0.2f};
            hit.distance = clearances[probeCalls++];
            return true;
        }

        [[nodiscard]] bool ProbeSupport(
            const GameplaySupportProbeRequest& request,
            GameplaySupportProbeHit& hit) const noexcept override
        {
            hit = {
                .distance = 0.0f,
                .position = request.origin,
                .normal = supportCalls++ == 0u
                    ? mathUtils::Vec3{0.70710678f, 0.70710678f, 0.0f}
                : mathUtils::Vec3{0.0f, 1.0f, 0.0f}
            };
            return true;
        }

        mutable std::size_t probeCalls{0u};
        mutable std::size_t supportCalls{0u};
    };
    
    [[nodiscard]] EntityHandle CreateAgent(GameplayWorld& world, const mathUtils::Vec3 position = {})
    {
        const EntityHandle entity = world.CreateEntity();
        world.AddAI(entity);
        world.AddTransform(entity, GameplayTransformComponent{.position = position});
        world.AddCharacterCommand(entity, GameplayCharacterCommandComponent{});
        world.AddCharacterMotor(entity, GameplayCharacterMotorComponent{});
        world.AddCharacterMovementState(entity, GameplayCharacterMovementStateComponent{});
        return entity;
    }

    [[nodiscard]] EntityHandle CreateTarget(GameplayWorld& world, const mathUtils::Vec3 position)
    {
        const EntityHandle entity = world.CreateEntity();
        world.AddTransform(entity, GameplayTransformComponent{.position = position});
        return entity;
    }

    [[nodiscard]] AIActionRuntimeContext FollowContext(const EntityHandle agent)
    {
        return {.agentEntity = agent, .actionId = kAIFollowTargetActionId};
    }

    [[nodiscard]] AIActionRuntimeContext FleeContext(const EntityHandle agent)
    {
        return {.agentEntity = agent, .actionId = kAIFleeTargetActionId};
    }
    
    static_assert(kAIFollowTargetActionId != kAIBuyKeyActionId);
    static_assert(kAIFleeTargetActionId != kAIBuyKeyActionId);
    static_assert(kAIFollowTargetActionId != kAIFleeTargetActionId);

    void ExpectDirection(const GameplayWorld& world, const EntityHandle agent, const float x, const float z)
    {
        const GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(agent);
        ASSERT_NE(command, nullptr);
        EXPECT_NEAR(command->moveWorld.x, x, 0.0001f);
        EXPECT_NEAR(command->moveWorld.z, z, 0.0001f);
    }
}

TEST(AIFollowTargetActionRuntime, ObstacleAvoidanceCorrectsSeekAtExistingCadence)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    world.AddCharacterPhysicalSettings(agent, {.radius = 0.25f, .cylinderHeight = 1.5f});
    const EntityHandle target = CreateTarget(world, {10.0f, 0.0f, 0.0f});
    ForwardBlockedQuery query{};
    world.TryGetCharacterMotor(agent)->velocity = {3.0f, 100.0f, 4.0f};
    AIFollowTargetActionRuntime runtime{
        world, target, {}, &query,
        {.forwardProbeDistance = 1.0f, .forwardProbeTimeHorizonSeconds = 0.5f}};

    ASSERT_EQ(runtime.Start(FollowContext(agent)), AIActionRuntimeResult::Running);
    EXPECT_LT(world.TryGetCharacterCommand(agent)->moveWorld.z, 0.0f);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 1.0f);
    ASSERT_EQ(query.callCount, 3u);
    EXPECT_FLOAT_EQ(query.requests[0].maximumDistance, 2.5f);
    EXPECT_FLOAT_EQ(query.requests[0].origin.y, 1.0f);
    for (const GameplayObstacleProbeRequest& request : query.requests)
    {
        EXPECT_FLOAT_EQ(request.clearanceRadius, 0.27f);
    }
    EXPECT_EQ(runtime.Tick(FollowContext(agent), 0.049f), AIActionRuntimeResult::Running);
    EXPECT_EQ(query.callCount, 3u);
    EXPECT_EQ(runtime.Tick(FollowContext(agent), 0.001f), AIActionRuntimeResult::Running);
    EXPECT_EQ(query.callCount, 6u);
}

TEST(AIFollowTargetActionRuntime, ObstacleAvoidanceHysteresisPersistsAndRestartResets)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    world.AddCharacterPhysicalSettings(agent);
    const EntityHandle target = CreateTarget(world, {10.0f, 0.0f, 0.0f});
    FollowHysteresisQuery query{};
    GameplaySteeringDebugRegistry debugRegistry{};
    debugRegistry.SetEnabled(true);
    AIFollowTargetActionRuntime runtime{
        world, target, {.steeringUpdateIntervalSeconds = 0.0f}, &query, {}, &debugRegistry};

    ASSERT_EQ(runtime.Start(FollowContext(agent)), AIActionRuntimeResult::Running);
    ASSERT_NE(debugRegistry.Find(agent), nullptr);
    EXPECT_EQ(debugRegistry.Find(agent)->avoidance.preferredSide,
        GameplayObstacleAvoidanceSide::Left);
    EXPECT_EQ(debugRegistry.Find(agent)->avoidance.chosenSide,
        GameplayObstacleAvoidanceSide::Left);

    ASSERT_EQ(runtime.Tick(FollowContext(agent), 0.0f), AIActionRuntimeResult::Running);
    ASSERT_NE(debugRegistry.Find(agent), nullptr);
    EXPECT_EQ(debugRegistry.Find(agent)->avoidance.preferredSide,
        GameplayObstacleAvoidanceSide::Right);
    EXPECT_EQ(debugRegistry.Find(agent)->avoidance.chosenSide,
        GameplayObstacleAvoidanceSide::Left);
    EXPECT_TRUE(debugRegistry.Find(agent)->avoidance.sideHeldByHysteresis);

    ASSERT_EQ(runtime.Start(FollowContext(agent)), AIActionRuntimeResult::Running);
    ASSERT_NE(debugRegistry.Find(agent), nullptr);
    EXPECT_EQ(debugRegistry.Find(agent)->avoidance.preferredSide,
        GameplayObstacleAvoidanceSide::Right);
    EXPECT_EQ(debugRegistry.Find(agent)->avoidance.chosenSide,
        GameplayObstacleAvoidanceSide::Right);
    EXPECT_FALSE(debugRegistry.Find(agent)->avoidance.sideHeldByHysteresis);
}

TEST(AIFollowTargetActionRuntime, PhysicalMaximumSlopeReachesAvoidanceWalkability)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    world.AddCharacterPhysicalSettings(agent, {.maximumSlopeAngleDegrees = 30.0f});
    const EntityHandle target = CreateTarget(world, {10.0f, 0.0f, 0.0f});
    SlopeAwareFollowQuery query{};
    AIFollowTargetActionRuntime runtime{world, target, {}, &query};

    ASSERT_EQ(runtime.Start(FollowContext(agent)), AIActionRuntimeResult::Running);
    EXPECT_GT(world.TryGetCharacterCommand(agent)->moveWorld.z, 0.0f);
    EXPECT_EQ(query.supportCalls, 2u);
}

TEST(AIFleeTargetActionRuntime, ObstacleAvoidanceCorrectsFleeButSafeStateDoesNotProbe)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    world.AddCharacterPhysicalSettings(agent);
    const EntityHandle threat = CreateTarget(world, {-1.0f, 0.0f, 0.0f});
    ForwardBlockedQuery query{};
    AIFleeTargetActionRuntime runtime{world, threat, {}, &query};

    ASSERT_EQ(runtime.Start(FleeContext(agent)), AIActionRuntimeResult::Running);
    EXPECT_LT(world.TryGetCharacterCommand(agent)->moveWorld.z, 0.0f);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 1.0f);
    EXPECT_EQ(query.callCount, 3u);
    for (const GameplayObstacleProbeRequest& request : query.requests)
    {
        EXPECT_FLOAT_EQ(request.clearanceRadius, 0.32f);
    }

    world.TryGetTransform(threat)->position = {-4.0f, 0.0f, 0.0f};
    EXPECT_EQ(runtime.Tick(FleeContext(agent), 0.05f), AIActionRuntimeResult::Running);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    EXPECT_EQ(query.callCount, 3u);
}

TEST(AIFollowTargetActionRuntime, StartsImmediatelyAndThrottlesThenResamples)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    const EntityHandle target = CreateTarget(world, {10.0f, 0.0f, 0.0f});
    AIFollowTargetActionRuntime runtime{world, target};

    ASSERT_EQ(runtime.Start(FollowContext(agent)), AIActionRuntimeResult::Running);
    ExpectDirection(world, agent, 1.0f, 0.0f);
    world.TryGetTransform(target)->position = {0.0f, 0.0f, 10.0f};
    EXPECT_EQ(runtime.Tick(FollowContext(agent), 0.02f), AIActionRuntimeResult::Running);
    EXPECT_EQ(runtime.Tick(FollowContext(agent), 0.02f), AIActionRuntimeResult::Running);
    ExpectDirection(world, agent, 1.0f, 0.0f);
    EXPECT_EQ(runtime.Tick(FollowContext(agent), 0.011f), AIActionRuntimeResult::Running);
    ExpectDirection(world, agent, 0.0f, 1.0f);
}

TEST(AIFollowTargetActionRuntime, PreservesSteeringCadenceRemainder)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    const EntityHandle target = CreateTarget(world, {10.0f, 0.0f, 0.0f});
    AIFollowTargetActionRuntime runtime{
        world,
        target,
        {.steeringUpdateIntervalSeconds = 0.05f}};
   
    ASSERT_EQ(runtime.Start(FollowContext(agent)), AIActionRuntimeResult::Running);
    ExpectDirection(world, agent, 1.0f, 0.0f);
   
    world.TryGetTransform(target)->position = {0.0f, 0.0f, 10.0f};
    EXPECT_EQ(runtime.Tick(FollowContext(agent), 0.033f), AIActionRuntimeResult::Running);
    ExpectDirection(world, agent, 1.0f, 0.0f);
   
    EXPECT_EQ(runtime.Tick(FollowContext(agent), 0.033f), AIActionRuntimeResult::Running);
    ExpectDirection(world, agent, 0.0f, 1.0f);
   
    world.TryGetTransform(target)->position = {-10.0f, 0.0f, 0.0f};
    EXPECT_EQ(runtime.Tick(FollowContext(agent), 0.033f), AIActionRuntimeResult::Running);
    ExpectDirection(world, agent, 0.0f, 1.0f);
   
    EXPECT_EQ(runtime.Tick(FollowContext(agent), 0.002f), AIActionRuntimeResult::Running);
    ExpectDirection(world, agent, -1.0f, 0.0f);
}

TEST(AIFollowTargetActionRuntime, StopsInsideAcceptanceAndResumesWhenTargetMoves)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    const EntityHandle target = CreateTarget(world, {0.1f, 0.0f, 0.0f});
    AIFollowTargetActionRuntime runtime{world, target};

    ASSERT_EQ(runtime.Start(FollowContext(agent)), AIActionRuntimeResult::Running);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    world.TryGetTransform(target)->position = {5.0f, 0.0f, 0.0f};
    EXPECT_EQ(runtime.Tick(FollowContext(agent), 0.05f), AIActionRuntimeResult::Running);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 1.0f);
}

TEST(AITargetActions, RejectsAgentWithoutCompleteCharacterMovementPipeline)
{
    GameplayWorld world{};
    AISystem aiSystem{};
   
    const EntityHandle agent = world.CreateEntity();
    world.AddAI(agent);
    world.AddTransform(agent, GameplayTransformComponent{});
    world.AddCharacterCommand(agent, GameplayCharacterCommandComponent{});
    world.AddCharacterMotor(agent, GameplayCharacterMotorComponent{});
   
    const EntityHandle target = CreateTarget(world, {5.0f, 0.0f, 0.0f});
   
    EXPECT_EQ(
        AIFollowTargetAction::Start(aiSystem, world, agent, target),
        AIActionExecutionStatus::Failed);
    EXPECT_EQ(
        AIFleeTargetAction::Start(aiSystem, world, agent, target),
        AIActionExecutionStatus::Failed);
}

TEST(AIFollowTargetActionRuntime, RefreshUsesCurrentAgentPosition)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    const EntityHandle target = CreateTarget(world, {10.0f, 0.0f, 0.0f});
    AIFollowTargetActionRuntime runtime{world, target};
    ASSERT_EQ(runtime.Start(FollowContext(agent)), AIActionRuntimeResult::Running);

    world.TryGetTransform(agent)->position = {10.0f, 0.0f, -10.0f};
    EXPECT_EQ(runtime.Tick(FollowContext(agent), 0.05f), AIActionRuntimeResult::Running);
    ExpectDirection(world, agent, 0.0f, 1.0f);
}

TEST(AIFollowTargetActionRuntime, InvalidTargetFailsAndCancelClearsMovement)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    EntityHandle target = CreateTarget(world, {10.0f, 0.0f, 0.0f});
    AIFollowTargetActionRuntime runtime{world, target};
    ASSERT_EQ(runtime.Start(FollowContext(agent)), AIActionRuntimeResult::Running);
    runtime.Cancel(FollowContext(agent));
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);

    ASSERT_EQ(runtime.Start(FollowContext(agent)), AIActionRuntimeResult::Running);
    world.DestroyEntity(target);
    EXPECT_EQ(runtime.Tick(FollowContext(agent), 0.001f), AIActionRuntimeResult::Failed);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
}

TEST(AIFollowTargetActionRuntime, ZeroAndNegativeIntervalsRefreshEveryTick)
{
    for (const float interval : {0.0f, -1.0f})
    {
        GameplayWorld world{};
        const EntityHandle agent = CreateAgent(world);
        const EntityHandle target = CreateTarget(world, {10.0f, 0.0f, 0.0f});
        AIFollowTargetActionRuntime runtime{
            world, target, {.steeringUpdateIntervalSeconds = interval}};
        ASSERT_EQ(runtime.Start(FollowContext(agent)), AIActionRuntimeResult::Running);
        world.TryGetTransform(target)->position = {0.0f, 0.0f, 10.0f};
        EXPECT_EQ(runtime.Tick(FollowContext(agent), 0.0f), AIActionRuntimeResult::Running);
        ExpectDirection(world, agent, 0.0f, 1.0f);
    }
}

TEST(AIFleeTargetActionRuntime, StartsInsideTriggerAndStaysStationaryOutside)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    const EntityHandle threat = CreateTarget(world, {1.0f, 0.0f, 0.0f});
    AIFleeTargetActionRuntime runtime{world, threat};
    ASSERT_EQ(runtime.Start(FleeContext(agent)), AIActionRuntimeResult::Running);
    ExpectDirection(world, agent, -1.0f, 0.0f);

    AIFleeTargetActionRuntime safeRuntime{world, CreateTarget(world, {4.0f, 0.0f, 0.0f})};
    EXPECT_EQ(safeRuntime.Start(FleeContext(agent)), AIActionRuntimeResult::Running);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
}

TEST(AIFleeTargetActionRuntime, SanitizesMalformedPolicySettings)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    const EntityHandle threat = CreateTarget(world, {1.0f, 0.0f, 0.0f});
    
    AIFleeTargetActionRuntime clampedRadii{
        world,
        threat,
        {
            .triggerRadius = 2.0f,
            .safeRadius = 1.0f,
            .steeringUpdateIntervalSeconds = 0.0f
        }};
    
    ASSERT_EQ(clampedRadii.Start(FleeContext(agent)), AIActionRuntimeResult::Running);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 1.0f);
    
    world.TryGetTransform(threat)->position = {2.0f, 0.0f, 0.0f};
    EXPECT_EQ(clampedRadii.Tick(FleeContext(agent), 0.0f), AIActionRuntimeResult::Running);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    
    AIFleeTargetActionRuntime negativeInterval{
        world,
        threat,
        {
            .triggerRadius = 10.0f,
            .safeRadius = 10.0f,
            .steeringUpdateIntervalSeconds = -1.0f
        }};
    
    ASSERT_EQ(negativeInterval.Start(FleeContext(agent)), AIActionRuntimeResult::Running);
    world.TryGetTransform(threat)->position = {0.0f, 0.0f, 2.0f};
    EXPECT_EQ(negativeInterval.Tick(FleeContext(agent), 0.0f), AIActionRuntimeResult::Running);
    ExpectDirection(world, agent, 0.0f, -1.0f);
}

TEST(AIFleeTargetActionRuntime, HysteresisStopsAtSafeAndReactivatesAtTrigger)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    const EntityHandle threat = CreateTarget(world, {1.0f, 0.0f, 0.0f});
    AIFleeTargetActionRuntime runtime{
        world, threat, {.triggerRadius = 2.0f, .safeRadius = 4.0f, .steeringUpdateIntervalSeconds = 0.0f}};
    ASSERT_EQ(runtime.Start(FleeContext(agent)), AIActionRuntimeResult::Running);

    world.TryGetTransform(threat)->position = {3.0f, 0.0f, 0.0f};
    EXPECT_EQ(runtime.Tick(FleeContext(agent), 0.0f), AIActionRuntimeResult::Running);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 1.0f);
    world.TryGetTransform(threat)->position = {4.0f, 0.0f, 0.0f};
    EXPECT_EQ(runtime.Tick(FleeContext(agent), 0.0f), AIActionRuntimeResult::Running);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    world.TryGetTransform(threat)->position = {3.0f, 0.0f, 0.0f};
    EXPECT_EQ(runtime.Tick(FleeContext(agent), 0.0f), AIActionRuntimeResult::Running);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    world.TryGetTransform(threat)->position = {2.0f, 0.0f, 0.0f};
    EXPECT_EQ(runtime.Tick(FleeContext(agent), 0.0f), AIActionRuntimeResult::Running);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 1.0f);
}

TEST(AIFleeTargetActionRuntime, ThrottlesRefreshAndExactOverlapIsFinite)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    const EntityHandle threat = CreateTarget(world, {1.0f, 0.0f, 0.0f});
    AIFleeTargetActionRuntime runtime{world, threat, {.triggerRadius = 20.0f, .safeRadius = 20.0f}};
    ASSERT_EQ(runtime.Start(FleeContext(agent)), AIActionRuntimeResult::Running);
    world.TryGetTransform(threat)->position = {0.0f, 0.0f, 1.0f};
    EXPECT_EQ(runtime.Tick(FleeContext(agent), 0.04f), AIActionRuntimeResult::Running);
    ExpectDirection(world, agent, -1.0f, 0.0f);
    EXPECT_EQ(runtime.Tick(FleeContext(agent), 0.011f), AIActionRuntimeResult::Running);
    ExpectDirection(world, agent, 0.0f, -1.0f);

    world.TryGetTransform(threat)->position = world.TryGetTransform(agent)->position;
    EXPECT_EQ(runtime.Tick(FleeContext(agent), 0.05f), AIActionRuntimeResult::Running);
    const GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(agent);
    EXPECT_FLOAT_EQ(command->moveMagnitude, 0.0f);
    EXPECT_TRUE(std::isfinite(command->moveWorld.x));
    EXPECT_TRUE(std::isfinite(command->moveWorld.z));
}

TEST(AIFleeTargetActionRuntime, InvalidTargetFailsAndCancelClearsMovement)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    const EntityHandle threat = CreateTarget(world, {1.0f, 0.0f, 0.0f});
    AIFleeTargetActionRuntime runtime{world, threat};
    ASSERT_EQ(runtime.Start(FleeContext(agent)), AIActionRuntimeResult::Running);
    runtime.Cancel(FleeContext(agent));
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    ASSERT_EQ(runtime.Start(FleeContext(agent)), AIActionRuntimeResult::Running);
    world.DestroyEntity(threat);
    EXPECT_EQ(runtime.Tick(FleeContext(agent), 0.001f), AIActionRuntimeResult::Failed);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
}

TEST(AITargetActions, AISystemOwnsRuntimeAndPassesDeterministicDelta)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    const EntityHandle agent = CreateAgent(world);
    const EntityHandle target = CreateTarget(world, {10.0f, 0.0f, 0.0f});
    ASSERT_EQ(
        AIFollowTargetAction::Start(aiSystem, world, agent, target),
        AIActionExecutionStatus::Running);
    world.TryGetTransform(target)->position = {0.0f, 0.0f, 10.0f};
    EXPECT_EQ(aiSystem.Update(world, 0.02f), 1u);
    EXPECT_EQ(aiSystem.Update(world, 0.02f), 1u);
    ExpectDirection(world, agent, 1.0f, 0.0f);
    EXPECT_EQ(aiSystem.Update(world, 0.011f), 1u);
    ExpectDirection(world, agent, 0.0f, 1.0f);
    EXPECT_EQ(aiSystem.GetActionStatus(agent, kAIFollowTargetActionId), AIActionExecutionStatus::Running);
}