#include <gtest/gtest.h>

#include <utility>

import core;

using namespace rendern;

namespace
{
    struct Fixture
    {
        GameplayWorld world{};
        GameplayTraversalLinkRegistry links{};
        EntityHandle agent{world.CreateEntity()};
        EntityHandle marker{world.CreateEntity()};
        GameplayTraversalExecutionContext context{
            .agentEntity = agent,
            .traversalLink = GameplayTraversalLinkHandle{47u},
            .traversalTypeId = kJumpTraversalTypeId,
            .targetEntity = marker};

        Fixture()
        {
            world.AddAI(agent);
            world.AddTransform(agent, {.position = {0.0f, 0.0f, 0.0f}});
            world.AddCharacterCommand(agent);
            world.AddCharacterMotor(agent);
            world.AddCharacterMovementState(agent);
            world.AddAction(agent);
            world.AddPhysicsCharacter(agent, {
                .character = physics::PhysicsCharacterHandle{.index = 1u, .generation = 1u}});
            const bool registered = links.Register({
                .handle = context.traversalLink,
                .traversalTypeId = kJumpTraversalTypeId,
                .targetEntity = marker,
                .jump = {
                    .takeoffPosition = {0.0f, 0.0f, 0.0f},
                    .landingPosition = {2.0f, 0.0f, 0.0f},
                    .verticalSpeed = 7.0f,
                    .takeoffTolerance = 0.25f,
                    .landingHorizontalTolerance = 0.5f,
                    .landingVerticalTolerance = 0.3f}});
            EXPECT_TRUE(registered);
        }
    };
}

TEST(JumpTraversalExecutor, ApproachesTakeoffWithoutArrivalSlowdown)
{
    Fixture fixture{};
    fixture.world.TryGetTransform(fixture.agent)->position = {-0.2511f, 0.0f, 0.0f};
    JumpTraversalExecutor executor{fixture.world, fixture.links};
    ASSERT_EQ(executor.Start(fixture.context), GameplayTraversalExecutionResult::Running);
    EXPECT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    EXPECT_GT(fixture.world.TryGetCharacterCommand(fixture.agent)->moveMagnitude, 0.9f);
    EXPECT_EQ(GetGameplayRequestedActionId(*fixture.world.TryGetAction(fixture.agent)),
        GameplayActionId{});
}

TEST(JumpTraversalExecutor, IssuesJumpOnlyAfterEnteringTakeoffToleranceAndOnlyOnce)
{
    Fixture fixture{};
    auto* transform = fixture.world.TryGetTransform(fixture.agent);
    transform->position = {-0.2511f, 0.0f, 0.0f};
    JumpTraversalExecutor executor{fixture.world, fixture.links};
    ASSERT_EQ(executor.Start(fixture.context), GameplayTraversalExecutionResult::Running);

    ASSERT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    EXPECT_EQ(GetGameplayRequestedActionId(*fixture.world.TryGetAction(fixture.agent)),
        GameplayActionId{});
    
    transform->position = {-0.2f, 0.0f, 0.0f};
    ASSERT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    auto* action = fixture.world.TryGetAction(fixture.agent);
    EXPECT_EQ(GetGameplayRequestedActionId(*action), kGameplayActionJump);
    ClearGameplayActionRequest(action->pending);

    ASSERT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    EXPECT_EQ(GetGameplayRequestedActionId(*action), GameplayActionId{});
}

TEST(JumpTraversalExecutor, JumpTraversalDataRequiresExplicitValidConfiguration)
{
    EXPECT_FALSE(GameplayJumpTraversalData{}.IsValid());
    const GameplayJumpTraversalData validData{
        .takeoffPosition = {},
        .landingPosition = {1.0f, 1.0f, 0.0f},
        .verticalSpeed = 5.0f,
        .takeoffTolerance = 0.25f,
        .landingHorizontalTolerance = 0.5f,
        .landingVerticalTolerance = 0.25f
    };

    EXPECT_TRUE(validData.IsValid());
}

TEST(JumpTraversalExecutor, LinkRegistryRejectsMissingJumpTraversalData)
{
    GameplayWorld world{};
    GameplayTraversalLinkRegistry links{};
    EXPECT_FALSE(links.Register({
        .handle = GameplayTraversalLinkHandle{99u},
        .traversalTypeId = kJumpTraversalTypeId,
        .targetEntity = world.CreateEntity()}));
}

TEST(JumpTraversalExecutor, MovingTakeoffPreservesPlanarMotorVelocityIntoAirborneState)
{
    Fixture fixture{};
    auto* transform = fixture.world.TryGetTransform(fixture.agent);
    transform->position = {-0.2511f, 0.0f, 0.0f};
    JumpTraversalExecutor executor{fixture.world, fixture.links};
    ASSERT_EQ(executor.Start(fixture.context), GameplayTraversalExecutionResult::Running);
    ASSERT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    EXPECT_GT(fixture.world.TryGetCharacterCommand(fixture.agent)->moveMagnitude, 0.0f);
    EXPECT_GT(fixture.world.TryGetCharacterCommand(fixture.agent)->moveWorld.x, 0.0f);
    UpdateGameplayCharacterMovement(fixture.world, {fixture.agent}, 0.1f);
    auto* motor = fixture.world.TryGetCharacterMotor(fixture.agent);
    ASSERT_GT(motor->desiredVelocity.x, 0.0f);
    
    transform->position = {-0.2f, 0.0f, 0.0f};
    ASSERT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    ASSERT_EQ(GetGameplayRequestedActionId(*fixture.world.TryGetAction(fixture.agent)),
        kGameplayActionJump);
    motor->velocity = motor->desiredVelocity;
    UpdateGameplayCharacterMovement(fixture.world, {fixture.agent}, 0.1f);
    auto* movement = fixture.world.TryGetCharacterMovementState(fixture.agent);
    EXPECT_GT(movement->jumpLockedVelocity.x, 0.0f);
    movement->jumpRequestResult = GameplayJumpRequestResult::Accepted;
    movement->jumpPhase = GameplayJumpPhase::Airborne;
    movement->grounded = false;
    UpdateGameplayCharacterMovement(fixture.world, {fixture.agent}, 0.1f);
    ASSERT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    EXPECT_GT(motor->desiredVelocity.x, 0.0f);
}

TEST(JumpTraversalExecutor, AcceptedRequestRemainsRunningBeforeAirborneFeedback)
{
    Fixture fixture{};
    JumpTraversalExecutor executor{fixture.world, fixture.links};
    ASSERT_EQ(executor.Start(fixture.context), GameplayTraversalExecutionResult::Running);
    ASSERT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    auto* action = fixture.world.TryGetAction(fixture.agent);
    ClearGameplayActionRequest(action->pending);
    fixture.world.TryGetCharacterMovementState(fixture.agent)->jumpRequestResult =
        GameplayJumpRequestResult::Accepted;
    EXPECT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
}

TEST(JumpTraversalExecutor, IssuesOneRequestAndSucceedsOnlyAfterValidPhysicalLanding)
{
    Fixture fixture{};
    JumpTraversalExecutor executor{fixture.world, fixture.links};
    ASSERT_EQ(executor.Start(fixture.context), GameplayTraversalExecutionResult::Running);
    ASSERT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    EXPECT_EQ(GetGameplayRequestedActionId(*fixture.world.TryGetAction(fixture.agent)),
        kGameplayActionJump);
    EXPECT_FLOAT_EQ(fixture.world.TryGetCharacterMotor(fixture.agent)->jumpVerticalSpeed, 7.0f);

    fixture.world.TryGetCharacterMovementState(fixture.agent)->jumpPhase = GameplayJumpPhase::Airborne;
    fixture.world.TryGetCharacterMovementState(fixture.agent)->grounded = false;
    EXPECT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    EXPECT_FLOAT_EQ(fixture.world.TryGetCharacterMotor(fixture.agent)->jumpVerticalSpeed, 5.5f);
    EXPECT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);

    fixture.world.TryGetTransform(fixture.agent)->position = {2.2f, 0.0f, 0.0f};
    fixture.world.TryGetCharacterMovementState(fixture.agent)->jumpPhase = GameplayJumpPhase::None;
    fixture.world.TryGetCharacterMovementState(fixture.agent)->grounded = true;
    EXPECT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Succeeded);
}

TEST(JumpTraversalExecutor, RejectedJumpRequestResultFailsWithoutAdvancing)
{
    Fixture fixture{};
    JumpTraversalExecutor executor{fixture.world, fixture.links};
    ASSERT_EQ(executor.Start(fixture.context), GameplayTraversalExecutionResult::Running);
    ASSERT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    fixture.world.TryGetCharacterMovementState(fixture.agent)->jumpRequestResult =
        GameplayJumpRequestResult::Rejected;
    EXPECT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Failed);
    EXPECT_EQ(fixture.world.TryGetCharacterMovementState(fixture.agent)->jumpRequestResult,
        GameplayJumpRequestResult::Rejected);
}

TEST(JumpTraversalExecutor, GroundingOutsideLandingToleranceFails)
{
    Fixture fixture{};
    JumpTraversalExecutor executor{fixture.world, fixture.links};
    ASSERT_EQ(executor.Start(fixture.context), GameplayTraversalExecutionResult::Running);
    ASSERT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    auto* movement = fixture.world.TryGetCharacterMovementState(fixture.agent);
    movement->jumpPhase = GameplayJumpPhase::Airborne;
    movement->grounded = false;
    ASSERT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    movement->jumpPhase = GameplayJumpPhase::None;
    movement->grounded = true;
    fixture.world.TryGetTransform(fixture.agent)->position = {4.0f, 0.0f, 0.0f};
    EXPECT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Failed);
}

TEST(JumpTraversalExecutor, GroundingAtWrongLandingElevationFails)
{
    Fixture fixture{};
    JumpTraversalExecutor executor{fixture.world, fixture.links};
    ASSERT_EQ(executor.Start(fixture.context), GameplayTraversalExecutionResult::Running);
    ASSERT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    auto* movement = fixture.world.TryGetCharacterMovementState(fixture.agent);
    movement->jumpRequestResult = GameplayJumpRequestResult::Accepted;
    movement->jumpPhase = GameplayJumpPhase::Airborne;
    movement->grounded = false;
    ASSERT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    movement->jumpPhase = GameplayJumpPhase::None;
    movement->grounded = true;
    fixture.world.TryGetTransform(fixture.agent)->position = {2.0f, -2.0f, 0.0f};
    EXPECT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Failed);
}

TEST(JumpTraversalExecutor, CancellationClearsOwnedRequestAndActiveState)
{
    Fixture fixture{};
    JumpTraversalExecutor executor{fixture.world, fixture.links};
    ASSERT_EQ(executor.Start(fixture.context), GameplayTraversalExecutionResult::Running);
    ASSERT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    executor.Cancel(fixture.context);
    EXPECT_EQ(GetGameplayRequestedActionId(*fixture.world.TryGetAction(fixture.agent)),
        GameplayActionId{});
    EXPECT_EQ(fixture.world.TryGetCharacterMovementState(fixture.agent)->jumpRequestResult,
        GameplayJumpRequestResult::None);
    EXPECT_FLOAT_EQ(fixture.world.TryGetCharacterMotor(fixture.agent)->jumpVerticalSpeed, 5.5f);
    EXPECT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Failed);
}

TEST(JumpTraversalExecutor, CancellationBeforeIssuingRequestPreservesUnrelatedScriptJump)
{
    Fixture fixture{};
    fixture.world.TryGetTransform(fixture.agent)->position = {-2.0f, 0.0f, 0.0f};
    JumpTraversalExecutor executor{fixture.world, fixture.links};
    ASSERT_EQ(executor.Start(fixture.context), GameplayTraversalExecutionResult::Running);
    ASSERT_TRUE(QueueGameplayActionRequest(*fixture.world.TryGetAction(fixture.agent), {
        .id = kGameplayActionJump,
        .source = GameplayActionRequestSource::Script,
        .priority = 300}));

    executor.Cancel(fixture.context);

    const GameplayActionRequest& pending = fixture.world.TryGetAction(fixture.agent)->pending;
    EXPECT_EQ(pending.id, kGameplayActionJump);
    EXPECT_EQ(pending.source, GameplayActionRequestSource::Script);
    EXPECT_EQ(pending.priority, 300);
}

TEST(JumpTraversalExecutor, CancellationAfterAcceptancePreservesResolvedResult)
{
    Fixture fixture{};
    JumpTraversalExecutor executor{fixture.world, fixture.links};
    ASSERT_EQ(executor.Start(fixture.context), GameplayTraversalExecutionResult::Running);
    ASSERT_EQ(executor.Tick(fixture.context, 0.1f), GameplayTraversalExecutionResult::Running);
    fixture.world.TryGetCharacterMovementState(fixture.agent)->jumpRequestResult =
        GameplayJumpRequestResult::Accepted;

    executor.Cancel(fixture.context);

    EXPECT_EQ(fixture.world.TryGetCharacterMovementState(fixture.agent)->jumpRequestResult,
        GameplayJumpRequestResult::Accepted);
}

TEST(JumpTraversalExecutor, FollowRouteRuntimeCompletesJumpAndContinuesOrdinaryRoute)
{
    Fixture fixture{};
    JumpTraversalExecutor executor{fixture.world, fixture.links};
    GameplayTraversalExecutorRegistry executors{};
    ASSERT_TRUE(executors.Register(kJumpTraversalTypeId, executor));
    GameplayRoute route{
        .points = {
            {.worldPosition = {0.0f, 0.0f, 0.0f}},
            {.worldPosition = {2.0f, 0.0f, 0.0f}},
            {.worldPosition = {4.0f, 0.0f, 0.0f}}},
        .segmentAnnotations = {
            {.traversalLink = fixture.context.traversalLink}, {}}};
    AIFollowRouteActionRuntime runtime{
        fixture.world, fixture.links, executors, std::move(route)};
    const AIActionRuntimeContext actionContext{
        .agentEntity = fixture.agent, .actionId = kAIFollowRouteActionId};

    ASSERT_EQ(runtime.Start(actionContext), AIActionRuntimeResult::Running);
    ASSERT_EQ(runtime.Tick(actionContext, 0.1f), AIActionRuntimeResult::Running);
    ASSERT_EQ(runtime.Tick(actionContext, 0.1f), AIActionRuntimeResult::Running);
    auto* movement = fixture.world.TryGetCharacterMovementState(fixture.agent);
    movement->jumpPhase = GameplayJumpPhase::Airborne;
    movement->grounded = false;
    ASSERT_EQ(runtime.Tick(actionContext, 0.1f), AIActionRuntimeResult::Running);
    fixture.world.TryGetTransform(fixture.agent)->position = {2.0f, 0.0f, 0.0f};
    movement->jumpPhase = GameplayJumpPhase::None;
    movement->grounded = true;
    ASSERT_EQ(runtime.Tick(actionContext, 0.1f), AIActionRuntimeResult::Running);
    EXPECT_GT(fixture.world.TryGetCharacterCommand(fixture.agent)->moveMagnitude, 0.0f);
}
