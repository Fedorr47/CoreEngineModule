#include <gtest/gtest.h>

#include <cstdint>

import core;

using namespace rendern;

namespace
{
    EntityHandle MakeAgent(GameplayWorld& world, const mathUtils::Vec3 position = {})
    {
        const EntityHandle agent = world.CreateEntity();
        world.AddAI(agent);
        world.AddTransform(agent, GameplayTransformComponent{.position = position});
        world.AddCharacterCommand(agent);
        world.AddCharacterMotor(agent);
        world.AddCharacterMovementState(agent);
        return agent;
    }

    EntityHandle MakeDoor(GameplayWorld& world, const mathUtils::Vec3 position = {}, const bool open = false)
    {
        const EntityHandle door = world.CreateEntity();
        world.AddTransform(door, GameplayTransformComponent{.position = position});
        world.AddInteractionPoint(door, GameplayInteractionPointComponent{.localFacingYawDegrees = 90.0f});
        world.AddDoor(door, GameplayDoorComponent{.isOpen = open});
        return door;
    }

    GameplayTraversalExecutionContext MakeContext(const EntityHandle agent, const EntityHandle door,
        const std::uint64_t link = 1u)
    {
        return {.agentEntity = agent, .traversalLink = GameplayTraversalLinkHandle{link},
            .traversalTypeId = kDoorTraversalTypeId, .targetEntity = door};
    }
}

// Protects traversal endpoint semantics so an already-open Door still requires arrival.
TEST(TestDoorTraversalExecutor, AlreadyOpenDoorMovesToInteractionPointBeforeSucceeding)
{
    GameplayWorld world{};
    GameplayObjectReservationSystem reservations{};
    DoorTraversalExecutor executor{world, reservations};
    const EntityHandle agent = MakeAgent(world);
    const EntityHandle door = MakeDoor(world, {2.0f, 0.0f, 0.0f}, true);
    const auto context = MakeContext(agent, door);
    EXPECT_EQ(executor.Start(context), GameplayTraversalExecutionResult::Running);
    EXPECT_TRUE(reservations.IsReservedBy(door, agent));
    EXPECT_EQ(executor.Tick(context, 0.1f), GameplayTraversalExecutionResult::Running);
    EXPECT_GT(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    world.TryGetTransform(agent)->position = {2.0f, 0.0f, 0.0f};
    EXPECT_EQ(executor.Tick(context, 0.1f), GameplayTraversalExecutionResult::Succeeded);
    EXPECT_FALSE(reservations.IsReserved(door));
}

// Protects valid Door startup so reservation and interaction-point resolution produce one active traversal without opening the door early.
TEST(TestDoorTraversalExecutor, ClosedDoorStartsRunning)
{
    GameplayWorld world{};
    GameplayObjectReservationSystem reservations{};
    DoorTraversalExecutor executor{world, reservations};
    const EntityHandle agent = MakeAgent(world);
    const EntityHandle door = MakeDoor(world);
    const auto context = MakeContext(agent, door);
    EXPECT_EQ(executor.Start(context), GameplayTraversalExecutionResult::Running);
    EXPECT_TRUE(reservations.IsReservedBy(door, agent));
    EXPECT_FALSE(world.TryGetDoor(door)->isOpen);
    EXPECT_EQ(executor.Start(context), GameplayTraversalExecutionResult::Failed);
    EXPECT_TRUE(reservations.IsReservedBy(door, agent));
}

// Protects validation-before-mutation when the traversal target is not a Door.
TEST(TestDoorTraversalExecutor, MissingDoorComponentFailsWithoutReservation)
{
    GameplayWorld world{};
    GameplayObjectReservationSystem reservations{};
    DoorTraversalExecutor executor{world, reservations};
    const EntityHandle agent = MakeAgent(world);
    const EntityHandle target = MakeDoor(world);
    world.RemoveDoor(target);
    EXPECT_EQ(executor.Start(MakeContext(agent, target)), GameplayTraversalExecutionResult::Failed);
    EXPECT_FALSE(reservations.IsReserved(target));
}

// Protects startup cleanup when the Door interaction point lacks a transform.
TEST(TestDoorTraversalExecutor, MissingDoorTransformReleasesReservation)
{
    GameplayWorld world{};
    GameplayObjectReservationSystem reservations{};
    DoorTraversalExecutor executor{world, reservations};
    const EntityHandle agent = MakeAgent(world);
    const EntityHandle door = MakeDoor(world);
    world.RemoveTransform(door);
    EXPECT_EQ(executor.Start(MakeContext(agent, door)), GameplayTraversalExecutionResult::Failed);
    EXPECT_FALSE(reservations.IsReserved(door));
}

// Protects validation-before-mutation when the Door has no interaction-point component.
TEST(TestDoorTraversalExecutor, MissingInteractionPointFailsWithoutReservation)
{
    GameplayWorld world{};
    GameplayObjectReservationSystem reservations{};
    DoorTraversalExecutor executor{world, reservations};
    const EntityHandle agent = MakeAgent(world);
    const EntityHandle door = MakeDoor(world);
    world.RemoveInteractionPoint(door);
    EXPECT_EQ(executor.Start(MakeContext(agent, door)), GameplayTraversalExecutionResult::Failed);
    EXPECT_FALSE(reservations.IsReserved(door));
}

// Protects reservation ownership so the executor never adopts a reservation made elsewhere.
TEST(TestDoorTraversalExecutor, SameAgentPreExistingReservationIsNotAdopted)
{
    GameplayWorld world{};
    GameplayObjectReservationSystem reservations{};
    DoorTraversalExecutor executor{world, reservations};
    const EntityHandle agent = MakeAgent(world);
    const EntityHandle door = MakeDoor(world);
    ASSERT_TRUE(reservations.TryReserve(world, door, agent));
    EXPECT_EQ(executor.Start(MakeContext(agent, door)), GameplayTraversalExecutionResult::Failed);
    executor.Cancel(MakeContext(agent, door));
    EXPECT_TRUE(reservations.IsReservedBy(door, agent));
}

// Protects exclusive Door ownership so a competitor cannot steal a reservation.
TEST(TestDoorTraversalExecutor, CompetingAgentCannotStartReservedDoorTraversal)
{
    GameplayWorld world{};
    GameplayObjectReservationSystem reservations{};
    DoorTraversalExecutor executor{world, reservations};
    const EntityHandle owner = MakeAgent(world);
    const EntityHandle competitor = MakeAgent(world);
    const EntityHandle door = MakeDoor(world);
    ASSERT_TRUE(reservations.TryReserve(world, door, owner));
    EXPECT_EQ(executor.Start(MakeContext(competitor, door)), GameplayTraversalExecutionResult::Failed);
    EXPECT_EQ(reservations.GetReservationOwner(door), owner);
}

// Protects local arrival steering so Door approach movement points at the fixed interaction point.
TEST(TestDoorTraversalExecutor, TickMovesAgentTowardInteractionPoint)
{
    GameplayWorld world{};
    GameplayObjectReservationSystem reservations{};
    DoorTraversalExecutor executor{world, reservations};
    const EntityHandle agent = MakeAgent(world);
    const EntityHandle door = MakeDoor(world, {2.0f, 0.0f, 0.0f});
    const auto context = MakeContext(agent, door);
    ASSERT_EQ(executor.Start(context), GameplayTraversalExecutionResult::Running);
    EXPECT_EQ(executor.Tick(context, 0.1f), GameplayTraversalExecutionResult::Running);
    EXPECT_GT(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    EXPECT_GT(world.TryGetCharacterCommand(agent)->moveWorld.x, 0.0f);
    EXPECT_FALSE(world.TryGetDoor(door)->isOpen);
}

// Protects successful Door completion so arrival opens the target, releases ownership, and leaves the agent stationary.
TEST(TestDoorTraversalExecutor, ArrivalOpensDoorAndReleasesReservation)
{
    GameplayWorld world{};
    GameplayObjectReservationSystem reservations{};
    DoorTraversalExecutor executor{world, reservations};
    const EntityHandle agent = MakeAgent(world, {0.1f, 0.0f, 0.0f});
    const EntityHandle door = MakeDoor(world);
    world.TryGetCharacterMotor(agent)->velocity = {3.0f, 4.0f, 2.0f};
    const auto context = MakeContext(agent, door);
    ASSERT_EQ(executor.Start(context), GameplayTraversalExecutionResult::Running);
    EXPECT_EQ(executor.Tick(context, 0.1f), GameplayTraversalExecutionResult::Succeeded);
    EXPECT_TRUE(world.TryGetDoor(door)->isOpen);
    EXPECT_FALSE(reservations.IsReserved(door));
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    EXPECT_FLOAT_EQ(world.TryGetCharacterMotor(agent)->velocity.x, 0.0f);
    EXPECT_FLOAT_EQ(world.TryGetCharacterMotor(agent)->velocity.y, 4.0f);
    EXPECT_FLOAT_EQ(world.TryGetCharacterMovementState(agent)->desiredFacingYawDegrees, 90.0f);
}

// Protects idempotent cancellation so it releases ownership without opening the Door.
TEST(TestDoorTraversalExecutor, CancelReleasesReservationWithoutOpeningDoor)
{
    GameplayWorld world{};
    GameplayObjectReservationSystem reservations{};
    DoorTraversalExecutor executor{world, reservations};
    const EntityHandle agent = MakeAgent(world);
    const EntityHandle door = MakeDoor(world, {2.0f, 0.0f, 0.0f});
    const auto context = MakeContext(agent, door);
    ASSERT_EQ(executor.Start(context), GameplayTraversalExecutionResult::Running);
    world.TryGetCharacterCommand(agent)->moveMagnitude = 1.0f;
    executor.Cancel(context);
    executor.Cancel(context);
    EXPECT_FALSE(reservations.IsReserved(door));
    EXPECT_FALSE(world.TryGetDoor(door)->isOpen);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
}

// Protects terminal cleanup when an active Door traversal loses required agent state.
TEST(TestDoorTraversalExecutor, InvalidAgentFailsAndReleasesReservation)
{
    GameplayWorld world{};
    GameplayObjectReservationSystem reservations{};
    DoorTraversalExecutor executor{world, reservations};
    const EntityHandle agent = MakeAgent(world);
    const EntityHandle door = MakeDoor(world);
    const auto context = MakeContext(agent, door);
    ASSERT_EQ(executor.Start(context), GameplayTraversalExecutionResult::Running);
    world.RemoveAI(agent);
    EXPECT_EQ(executor.Tick(context, 0.1f), GameplayTraversalExecutionResult::Failed);
    EXPECT_FALSE(reservations.IsReserved(door));
}

// Protects terminal cleanup when the active Door entity is invalidated.
TEST(TestDoorTraversalExecutor, InvalidDoorFailsAndClearsActiveState)
{
    GameplayWorld world{};
    GameplayObjectReservationSystem reservations{};
    DoorTraversalExecutor executor{world, reservations};
    const EntityHandle agent = MakeAgent(world);
    const EntityHandle door = MakeDoor(world);
    const auto context = MakeContext(agent, door);
    ASSERT_EQ(executor.Start(context), GameplayTraversalExecutionResult::Running);
    world.DestroyEntity(door);
    EXPECT_EQ(executor.Tick(context, 0.1f), GameplayTraversalExecutionResult::Failed);
    EXPECT_FALSE(reservations.IsReserved(door));
    EXPECT_EQ(executor.Start(context), GameplayTraversalExecutionResult::Failed);
}

// Protects lost-ownership cleanup so the executor fails without releasing a replacement owner.
TEST(TestDoorTraversalExecutor, LostReservationFailsWithoutOpeningDoor)
{
    GameplayWorld world{};
    GameplayObjectReservationSystem reservations{};
    DoorTraversalExecutor executor{world, reservations};
    const EntityHandle agent = MakeAgent(world);
    const EntityHandle replacementOwner = MakeAgent(world);
    const EntityHandle door = MakeDoor(world, {2.0f, 0.0f, 0.0f});
    const auto context = MakeContext(agent, door);
    ASSERT_EQ(executor.Start(context), GameplayTraversalExecutionResult::Running);
    ASSERT_TRUE(reservations.Release(door, agent));
    ASSERT_TRUE(reservations.TryReserve(world, door, replacementOwner));
    world.TryGetCharacterCommand(agent)->moveMagnitude = 1.0f;
    EXPECT_EQ(executor.Tick(context, 0.1f), GameplayTraversalExecutionResult::Failed);
    EXPECT_TRUE(reservations.IsReservedBy(door, replacementOwner));
    EXPECT_FALSE(world.TryGetDoor(door)->isOpen);
    EXPECT_FLOAT_EQ(world.TryGetCharacterCommand(agent)->moveMagnitude, 0.0f);
    EXPECT_EQ(executor.Tick(context, 0.1f), GameplayTraversalExecutionResult::Failed);
}

// Protects executor reuse so one registered Door executor can advance independent traversals for different agents.
TEST(TestDoorTraversalExecutor, SupportsDifferentAgentsOnDifferentDoors)
{
    GameplayWorld world{};
    GameplayObjectReservationSystem reservations{};
    DoorTraversalExecutor executor{world, reservations};
    const EntityHandle firstAgent = MakeAgent(world);
    const EntityHandle secondAgent = MakeAgent(world, {5.0f, 0.0f, 0.0f});
    const EntityHandle firstDoor = MakeDoor(world);
    const EntityHandle secondDoor = MakeDoor(world, {5.0f, 0.0f, 0.0f});
    const auto first = MakeContext(firstAgent, firstDoor, 1u);
    const auto second = MakeContext(secondAgent, secondDoor, 2u);
    ASSERT_EQ(executor.Start(first), GameplayTraversalExecutionResult::Running);
    ASSERT_EQ(executor.Start(second), GameplayTraversalExecutionResult::Running);
    EXPECT_EQ(executor.Tick(first, 0.1f), GameplayTraversalExecutionResult::Succeeded);
    EXPECT_EQ(executor.Tick(second, 0.1f), GameplayTraversalExecutionResult::Succeeded);
    EXPECT_TRUE(world.TryGetDoor(firstDoor)->isOpen);
    EXPECT_TRUE(world.TryGetDoor(secondDoor)->isOpen);
}