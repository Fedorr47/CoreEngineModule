#include <gtest/gtest.h>

#include <vector>

import core;

using namespace rendern;

namespace
{
    class FakeTraversalExecutor final : public IGameplayTraversalExecutor
    {
    public:
        GameplayTraversalExecutionResult startResult{GameplayTraversalExecutionResult::Running};
        std::vector<GameplayTraversalExecutionResult> tickResults{};
        int startCallCount{0};
        int tickCallCount{0};
        int cancelCallCount{0};
        GameplayTraversalExecutionContext lastContext{};

        GameplayTraversalExecutionResult Start(const GameplayTraversalExecutionContext& context) override
        {
            ++startCallCount;
            lastContext = context;
            return startResult;
        }

        GameplayTraversalExecutionResult Tick(
            const GameplayTraversalExecutionContext& context, float) override
        {
            ++tickCallCount;
            lastContext = context;
            if (tickResults.empty())
            {
                return GameplayTraversalExecutionResult::Running;
            }
            const GameplayTraversalExecutionResult result = tickResults.front();
            tickResults.erase(tickResults.begin());
            return result;
        }

        void Cancel(const GameplayTraversalExecutionContext& context) noexcept override
        {
            ++cancelCallCount;
            lastContext = context;
        }
    };

    EntityHandle CreateAgent(GameplayWorld& world)
    {
        const EntityHandle entity = world.CreateEntity();
        world.AddAI(entity);
        world.AddTransform(entity, GameplayTransformComponent{});
        world.AddCharacterCommand(entity, GameplayCharacterCommandComponent{});
        world.AddCharacterMotor(entity, GameplayCharacterMotorComponent{});
        world.AddCharacterMovementState(entity, GameplayCharacterMovementStateComponent{});
        return entity;
    }

    GameplayRoute MakeTraversalRoute(const bool addOrdinarySegment = false)
    {
        GameplayRoute route{
            .points = {
                GameplayRoutePoint{.worldPosition = {0.0f, 0.0f, 0.0f}},
                GameplayRoutePoint{.worldPosition = {1.0f, 0.0f, 0.0f}}
            },
            .segmentAnnotations = {GameplayRouteSegmentAnnotation{
                .traversalLink = GameplayTraversalLinkHandle{7u}}}
        };
        if (addOrdinarySegment)
        {
            route.points.push_back(GameplayRoutePoint{.worldPosition = {3.0f, 0.0f, 0.0f}});
            route.segmentAnnotations.push_back(GameplayRouteSegmentAnnotation{});
        }
        return route;
    }

    AIActionRuntimeContext MakeContext(const EntityHandle agent)
    {
        return {.agentEntity = agent, .actionId = kAIFollowRouteActionId};
    }

    void RegisterLink(GameplayTraversalLinkRegistry& registry, GameplayWorld& world)
    {
        const EntityHandle target = world.CreateEntity();
        ASSERT_TRUE(registry.Register({
            .handle = GameplayTraversalLinkHandle{7u},
            .traversalTypeId = kDoorTraversalTypeId,
            .targetEntity = target}));
    }
}

// Protects runtime traversal resolution so an annotated route cannot execute
// through an unregistered link.
TEST(AIFollowRouteActionRuntime, MissingTraversalLinkFails)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    GameplayTraversalLinkRegistry registry{};
    FakeTraversalExecutor executor{};
    GameplayTraversalExecutorRegistry executorRegistry{};
    ASSERT_TRUE(executorRegistry.Register(kDoorTraversalTypeId, executor));
    AIFollowRouteActionRuntime runtime{world, registry, executorRegistry, MakeTraversalRoute()};

    ASSERT_EQ(runtime.Start(MakeContext(agent)), AIActionRuntimeResult::Running);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Failed);
    EXPECT_EQ(executor.startCallCount, 0);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
}

// Protects traversal lifecycle ordering so one follower boundary starts one
// executor invocation and subsequent frames tick it instead of restarting it.
TEST(AIFollowRouteActionRuntime, StartsTraversalExecutorOnlyOnce)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    GameplayTraversalLinkRegistry registry{};
    RegisterLink(registry, world);
    FakeTraversalExecutor executor{};
    GameplayTraversalExecutorRegistry executorRegistry{};
    ASSERT_TRUE(executorRegistry.Register(kDoorTraversalTypeId, executor));
    AIFollowRouteActionRuntime runtime{world, registry, executorRegistry, MakeTraversalRoute()};

    ASSERT_EQ(runtime.Start(MakeContext(agent)), AIActionRuntimeResult::Running);
    ASSERT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    EXPECT_EQ(executor.startCallCount, 1);
    EXPECT_EQ(executor.tickCallCount, 1);
    EXPECT_FLOAT_EQ(world.TryGetCharacterMotor(agent)->velocity.x, 0.0f);
}

// Protects successful traversal completion and bounded next-tick route resumption.
TEST(AIFollowRouteActionRuntime, SuccessfulTraversalResumesRoute)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    GameplayTraversalLinkRegistry registry{};
    RegisterLink(registry, world);
    FakeTraversalExecutor executor{};
    GameplayTraversalExecutorRegistry executorRegistry{};
    ASSERT_TRUE(executorRegistry.Register(kDoorTraversalTypeId, executor));
    executor.startResult = GameplayTraversalExecutionResult::Succeeded;
    AIFollowRouteActionRuntime runtime{world, registry, executorRegistry, MakeTraversalRoute(true)};

    ASSERT_EQ(runtime.Start(MakeContext(agent)), AIActionRuntimeResult::Running);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    EXPECT_GT(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    EXPECT_EQ(executor.tickCallCount, 0);
}

// Protects final-segment synchronous traversal completion without an unnecessary executor tick.
TEST(AIFollowRouteActionRuntime, FinalSynchronousTraversalCompletesAction)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    GameplayTraversalLinkRegistry registry{};
    RegisterLink(registry, world);
    FakeTraversalExecutor executor{};
    GameplayTraversalExecutorRegistry executorRegistry{};
    ASSERT_TRUE(executorRegistry.Register(kDoorTraversalTypeId, executor));
    executor.startResult = GameplayTraversalExecutionResult::Succeeded;
    AIFollowRouteActionRuntime runtime{world, registry, executorRegistry, MakeTraversalRoute()};

    ASSERT_EQ(runtime.Start(MakeContext(agent)), AIActionRuntimeResult::Running);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Succeeded);
    EXPECT_EQ(executor.tickCallCount, 0);
}

// Protects cancellation propagation and ensures a running traversal is cancelled exactly once.
TEST(AIFollowRouteActionRuntime, CancellationPropagatesToActiveTraversal)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    GameplayTraversalLinkRegistry registry{};
    RegisterLink(registry, world);
    FakeTraversalExecutor executor{};
    GameplayTraversalExecutorRegistry executorRegistry{};
    ASSERT_TRUE(executorRegistry.Register(kDoorTraversalTypeId, executor));
    AIFollowRouteActionRuntime runtime{world, registry, executorRegistry, MakeTraversalRoute()};

    ASSERT_EQ(runtime.Start(MakeContext(agent)), AIActionRuntimeResult::Running);
    ASSERT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    runtime.Cancel(MakeContext(agent));
    runtime.Cancel(MakeContext(agent));
    EXPECT_EQ(executor.cancelCallCount, 1);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
}

// Protects asynchronous traversal completion so a running executor advances
// the follower exactly once after reporting success.
TEST(AIFollowRouteActionRuntime, RunningTraversalResumesRouteAfterTickSuccess)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    GameplayTraversalLinkRegistry registry{};
    RegisterLink(registry, world);
    FakeTraversalExecutor executor{};
    GameplayTraversalExecutorRegistry executorRegistry{};
    ASSERT_TRUE(executorRegistry.Register(kDoorTraversalTypeId, executor));
    executor.tickResults = {
        GameplayTraversalExecutionResult::Running,
        GameplayTraversalExecutionResult::Succeeded};
    AIFollowRouteActionRuntime runtime{world, registry, executorRegistry, MakeTraversalRoute(true)};

    ASSERT_EQ(runtime.Start(MakeContext(agent)), AIActionRuntimeResult::Running);
    ASSERT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    ASSERT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    EXPECT_GT(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    EXPECT_EQ(executor.startCallCount, 1);
    EXPECT_EQ(executor.tickCallCount, 2);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    EXPECT_EQ(executor.tickCallCount, 2);
}

// Protects traversal failure propagation so executor failure terminates the
// FollowRoute runtime and clears ordinary movement intent.
TEST(AIFollowRouteActionRuntime, TraversalExecutorFailureFailsAction)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    GameplayTraversalLinkRegistry registry{};
    RegisterLink(registry, world);
    FakeTraversalExecutor executor{};
    GameplayTraversalExecutorRegistry executorRegistry{};
    ASSERT_TRUE(executorRegistry.Register(kDoorTraversalTypeId, executor));
    executor.startResult = GameplayTraversalExecutionResult::Failed;
    AIFollowRouteActionRuntime runtime{world, registry, executorRegistry, MakeTraversalRoute(true)};

    ASSERT_EQ(runtime.Start(MakeContext(agent)), AIActionRuntimeResult::Running);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Failed);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    EXPECT_EQ(executor.startCallCount, 1);
    EXPECT_EQ(executor.tickCallCount, 0);
}

// Protects asynchronous failure cleanup so a failed executor tick discards the
// active traversal without advancing into ordinary route movement.
TEST(AIFollowRouteActionRuntime, RunningTraversalTickFailureFailsAction)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    GameplayTraversalLinkRegistry registry{};
    RegisterLink(registry, world);
    FakeTraversalExecutor executor{};
    GameplayTraversalExecutorRegistry executorRegistry{};
    ASSERT_TRUE(executorRegistry.Register(kDoorTraversalTypeId, executor));
    executor.tickResults = {GameplayTraversalExecutionResult::Failed};
    AIFollowRouteActionRuntime runtime{world, registry, executorRegistry, MakeTraversalRoute(true)};

    ASSERT_EQ(runtime.Start(MakeContext(agent)), AIActionRuntimeResult::Running);
    ASSERT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Failed);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    EXPECT_EQ(executor.startCallCount, 1);
    EXPECT_EQ(executor.tickCallCount, 1);
}

// Protects consecutive traversal progression without recursion or treating the
// second follower boundary as a terminal failure.
TEST(AIFollowRouteActionRuntime, ConsecutiveSynchronousTraversalsProgressAcrossTicks)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    GameplayTraversalLinkRegistry registry{};
    RegisterLink(registry, world);
    const EntityHandle secondTarget = world.CreateEntity();
    ASSERT_TRUE(registry.Register({
        .handle = GameplayTraversalLinkHandle{8u},
        .traversalTypeId = kJumpTraversalTypeId,
        .targetEntity = secondTarget}));
    FakeTraversalExecutor executor{};
    GameplayTraversalExecutorRegistry executorRegistry{};
    ASSERT_TRUE(executorRegistry.Register(kDoorTraversalTypeId, executor));
    executor.startResult = GameplayTraversalExecutionResult::Succeeded;
    ASSERT_TRUE(executorRegistry.Register(kJumpTraversalTypeId, executor));
    GameplayRoute route{
        .points = {
            GameplayRoutePoint{.worldPosition = {}},
            GameplayRoutePoint{.worldPosition = {1.0f, 0.0f, 0.0f}},
            GameplayRoutePoint{.worldPosition = {2.0f, 0.0f, 0.0f}},
            GameplayRoutePoint{.worldPosition = {4.0f, 0.0f, 0.0f}}},
        .segmentAnnotations = {
            GameplayRouteSegmentAnnotation{.traversalLink = GameplayTraversalLinkHandle{7u}},
            GameplayRouteSegmentAnnotation{.traversalLink = GameplayTraversalLinkHandle{8u}},
            GameplayRouteSegmentAnnotation{}}};
    AIFollowRouteActionRuntime runtime{world, registry, executorRegistry, std::move(route)};

    ASSERT_EQ(runtime.Start(MakeContext(agent)), AIActionRuntimeResult::Running);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    EXPECT_EQ(executor.startCallCount, 1);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    EXPECT_EQ(executor.startCallCount, 2);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    EXPECT_GT(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
}

// Protects open traversal extension so independently registered type IDs
// dispatch to their own executors without a central type switch.
TEST(AIFollowRouteActionRuntime, SelectsExecutorByTraversalTypeId)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    const EntityHandle target = world.CreateEntity();
    GameplayTraversalLinkRegistry linkRegistry{};
    ASSERT_TRUE(linkRegistry.Register({
        .handle = GameplayTraversalLinkHandle{7u},
        .traversalTypeId = kJumpTraversalTypeId,
        .targetEntity = target}));
    FakeTraversalExecutor doorExecutor{};
    FakeTraversalExecutor jumpExecutor{};
    GameplayTraversalExecutorRegistry executorRegistry{};
    ASSERT_TRUE(executorRegistry.Register(kDoorTraversalTypeId, doorExecutor));
    ASSERT_TRUE(executorRegistry.Register(kJumpTraversalTypeId, jumpExecutor));
    AIFollowRouteActionRuntime runtime{
        world, linkRegistry, executorRegistry, MakeTraversalRoute()};

    ASSERT_EQ(runtime.Start(MakeContext(agent)), AIActionRuntimeResult::Running);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    EXPECT_EQ(doorExecutor.startCallCount, 0);
    EXPECT_EQ(jumpExecutor.startCallCount, 1);
}

// Protects unsupported traversal handling so a structurally valid custom type
// fails safely when no executor has been registered for it.
TEST(AIFollowRouteActionRuntime, UnregisteredTraversalTypeFails)
{
    constexpr GameplayTraversalTypeId kCustomTraversalTypeId{99u};
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    const EntityHandle target = world.CreateEntity();
    GameplayTraversalLinkRegistry linkRegistry{};
    ASSERT_TRUE(linkRegistry.Register({
        .handle = GameplayTraversalLinkHandle{7u},
        .traversalTypeId = kCustomTraversalTypeId,
        .targetEntity = target}));
    GameplayTraversalExecutorRegistry executorRegistry{};
    AIFollowRouteActionRuntime runtime{
        world, linkRegistry, executorRegistry, MakeTraversalRoute()};

    ASSERT_EQ(runtime.Start(MakeContext(agent)), AIActionRuntimeResult::Running);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Failed);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
}

// Protects executor pinning so an active traversal continues through the
// executor that accepted Start even after its registry entry is removed.
TEST(AIFollowRouteActionRuntime, ActiveTraversalPinsSelectedExecutor)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateAgent(world);
    GameplayTraversalLinkRegistry linkRegistry{};
    RegisterLink(linkRegistry, world);
    FakeTraversalExecutor executor{};
    GameplayTraversalExecutorRegistry executorRegistry{};
    ASSERT_TRUE(executorRegistry.Register(kDoorTraversalTypeId, executor));
    AIFollowRouteActionRuntime runtime{
        world, linkRegistry, executorRegistry, MakeTraversalRoute()};

    ASSERT_EQ(runtime.Start(MakeContext(agent)), AIActionRuntimeResult::Running);
    ASSERT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    ASSERT_TRUE(executorRegistry.Remove(kDoorTraversalTypeId));
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 0.016f), AIActionRuntimeResult::Running);
    EXPECT_EQ(executor.tickCallCount, 1);
    runtime.Cancel(MakeContext(agent));
    EXPECT_EQ(executor.cancelCallCount, 1);
}