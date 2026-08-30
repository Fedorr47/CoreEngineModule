#include <cmath>
#include <gtest/gtest.h>

#include "TestSupport/MathTestHelper.h"

import core;

using namespace rendern;

using namespace MathTestHelper;

namespace
{
    [[nodiscard]] float PlanarLength(const mathUtils::Vec3& value) noexcept
    {
        return std::sqrt((value.x * value.x) + (value.z * value.z));
    }

    void ExpectZeroVector(const mathUtils::Vec3& value)
    {
        ExpectVec3Near(value, mathUtils::Vec3::ZeroVector(), kEpsVec);
    }

    void ExpectFiniteIntent(const GameplayMovementIntent& intent)
    {
        EXPECT_TRUE(std::isfinite(intent.moveWorld.x));
        EXPECT_TRUE(std::isfinite(intent.moveWorld.y));
        EXPECT_TRUE(std::isfinite(intent.moveWorld.z));
        EXPECT_TRUE(std::isfinite(intent.moveMagnitude));
    }
}

// Protects stationary default construction so a missing steering producer cannot regress into accidental character movement.
TEST(GameplaySteering, DefaultMovementIntentIsStationary)
{
    const GameplayMovementIntent intent{};

    ExpectZeroVector(intent.moveWorld);
    EXPECT_FLOAT_EQ(intent.moveMagnitude, 0.0f);
    EXPECT_FALSE(intent.wantsRun);
    EXPECT_FALSE(intent.IsMoving());
}

// Protects arrival semantics at the acceptance boundary and prevents regressions that would oscillate around a target.
TEST(GameplaySteering, SteeringArrivesInsideAcceptanceRadius)
{
    const GameplayArrivalSteeringSettings settings{ .acceptanceRadius = 0.25f };
    const GameplaySteeringOutput output = BuildGameplayArrivalSteering(
        mathUtils::Vec3(0.0f, 0.0f, 0.0f),
        mathUtils::Vec3(0.1f, 0.0f, 0.0f),
        settings);

    EXPECT_EQ(output.status, GameplaySteeringStatus::Arrived);
    EXPECT_NEAR(output.remainingDistance, 0.1f, kTolerance);
    ExpectZeroVector(output.movement.moveWorld);
    EXPECT_FLOAT_EQ(output.movement.moveMagnitude, 0.0f);
    EXPECT_FALSE(output.movement.wantsRun);
}

// Protects the planar movement model and prevents regressions where vertical-only targets generate horizontal movement.
TEST(GameplaySteering, SteeringUsesPlanarDistance)
{
    const GameplaySteeringOutput output = BuildGameplayArrivalSteering(
        mathUtils::Vec3(2.0f, 0.0f, 3.0f),
        mathUtils::Vec3(2.0f, 100.0f, 3.0f));

    EXPECT_EQ(output.status, GameplaySteeringStatus::Arrived);
    EXPECT_FLOAT_EQ(output.remainingDistance, 0.0f);
    ExpectZeroVector(output.movement.moveWorld);
    EXPECT_FLOAT_EQ(output.movement.moveMagnitude, 0.0f);
}

// Protects normalized world-space steering output and prevents regressions that leak vertical delta into movement direction.
TEST(GameplaySteering, SteeringNormalizesPlanarDirection)
{
    const GameplaySteeringOutput output = BuildGameplayArrivalSteering(
        mathUtils::Vec3(0.0f, 5.0f, 0.0f),
        mathUtils::Vec3(3.0f, 20.0f, 4.0f));

    EXPECT_EQ(output.status, GameplaySteeringStatus::Moving);
    EXPECT_NEAR(output.movement.moveWorld.x, 0.6f, kTolerance);
    EXPECT_FLOAT_EQ(output.movement.moveWorld.y, 0.0f);
    EXPECT_NEAR(output.movement.moveWorld.z, 0.8f, kTolerance);
    EXPECT_NEAR(PlanarLength(output.movement.moveWorld), 1.0f, kTolerance);
    EXPECT_NEAR(output.remainingDistance, 5.0f, kTolerance);
}

// Protects full-speed travel for distant targets and prevents regressions that attenuate movement outside the slowing radius.
TEST(GameplaySteering, SteeringUsesFullMagnitudeOutsideSlowingRadius)
{
    const GameplayArrivalSteeringSettings settings{ .acceptanceRadius = 0.25f, .slowingRadius = 1.0f, .wantsRun = true };
    const GameplaySteeringOutput output = BuildGameplayArrivalSteering(
        mathUtils::Vec3(0.0f, 0.0f, 0.0f),
        mathUtils::Vec3(0.0f, 0.0f, 3.0f),
        settings);

    EXPECT_EQ(output.status, GameplaySteeringStatus::Moving);
    EXPECT_FLOAT_EQ(output.movement.moveMagnitude, 1.0f);
    EXPECT_TRUE(output.movement.wantsRun);
}

// Protects deterministic arrival slowdown and prevents regressions that force future followers to invent incompatible speed scaling.
TEST(GameplaySteering, SteeringReducesMagnitudeInsideSlowingRadius)
{
    const GameplayArrivalSteeringSettings settings{ .acceptanceRadius = 1.0f, .slowingRadius = 5.0f };
    const GameplaySteeringOutput output = BuildGameplayArrivalSteering(
        mathUtils::Vec3(0.0f, 0.0f, 0.0f),
        mathUtils::Vec3(3.0f, 0.0f, 0.0f),
        settings);

    EXPECT_EQ(output.status, GameplaySteeringStatus::Moving);
    EXPECT_NEAR(output.movement.moveMagnitude, 0.5f, kTolerance);
}


// Protects zero-length steering input and prevents regressions that produce NaN for already-matching positions.
TEST(GameplaySteering, ZeroLengthTargetDeltaDoesNotProduceNaN)
{
    const GameplaySteeringOutput output = BuildGameplayArrivalSteering(
        mathUtils::Vec3(1.0f, 2.0f, 3.0f),
        mathUtils::Vec3(1.0f, 2.0f, 3.0f));

    EXPECT_EQ(output.status, GameplaySteeringStatus::Arrived);
    EXPECT_FLOAT_EQ(output.remainingDistance, 0.0f);
    ExpectZeroVector(output.movement.moveWorld);
    EXPECT_FLOAT_EQ(output.movement.moveMagnitude, 0.0f);
    ExpectFiniteIntent(output.movement);
    EXPECT_TRUE(std::isfinite(output.remainingDistance));
}

// Protects authored settings with no slowdown interval and prevents divide-by-zero regressions in arrival steering.
TEST(GameplaySteering, EqualSlowingAndAcceptanceRadiusDoesNotDivideByZero)
{
    const GameplayArrivalSteeringSettings settings{ .acceptanceRadius = 1.0f, .slowingRadius = 1.0f };
    const GameplaySteeringOutput outside = BuildGameplayArrivalSteering(
        mathUtils::Vec3(0.0f, 0.0f, 0.0f),
        mathUtils::Vec3(2.0f, 0.0f, 0.0f),
        settings);
    const GameplaySteeringOutput inside = BuildGameplayArrivalSteering(
        mathUtils::Vec3(0.0f, 0.0f, 0.0f),
        mathUtils::Vec3(0.5f, 0.0f, 0.0f),
        settings);

    EXPECT_EQ(outside.status, GameplaySteeringStatus::Moving);
    EXPECT_FLOAT_EQ(outside.movement.moveMagnitude, 1.0f);
    EXPECT_EQ(inside.status, GameplaySteeringStatus::Arrived);
    ExpectFiniteIntent(outside.movement);
    ExpectFiniteIntent(inside.movement);
    EXPECT_TRUE(std::isfinite(outside.remainingDistance));
    EXPECT_TRUE(std::isfinite(inside.remainingDistance));
}

// Protects deterministic sanitization of malformed authored radii and prevents regressions that emit invalid floats.
TEST(GameplaySteering, NegativeRadiiAreSanitized)
{
    const GameplayArrivalSteeringSettings settings{ .acceptanceRadius = -2.0f, .slowingRadius = -1.0f };
    const GameplaySteeringOutput output = BuildGameplayArrivalSteering(
        mathUtils::Vec3(0.0f, 0.0f, 0.0f),
        mathUtils::Vec3(1.0f, 0.0f, 0.0f),
        settings);

    EXPECT_EQ(output.status, GameplaySteeringStatus::Moving);
    EXPECT_FLOAT_EQ(output.remainingDistance, 1.0f);
    EXPECT_FLOAT_EQ(output.movement.moveMagnitude, 1.0f);
    EXPECT_NEAR(PlanarLength(output.movement.moveWorld), 1.0f, kTolerance);
    ExpectFiniteIntent(output.movement);
}

TEST(GameplaySteering, SeekNormalizesPlanarDirectionTowardTarget)
{
    const GameplaySteeringOutput output = BuildGameplaySeekSteering(
        {0.0f, 5.0f, 0.0f},
        {3.0f, 20.0f, 4.0f});

    EXPECT_EQ(output.status, GameplaySteeringStatus::Moving);
    ExpectVec3Near(output.movement.moveWorld, {0.6f, 0.0f, 0.8f}, kEpsVec);
    EXPECT_NEAR(PlanarLength(output.movement.moveWorld), 1.0f, kTolerance);
    EXPECT_NEAR(output.remainingDistance, 5.0f, kTolerance);
}

TEST(GameplaySteering, SeekUsesFullMagnitudeWithoutArrivalSlowdown)
{
    const GameplaySeekSteeringSettings settings{.acceptanceRadius = 0.25f};
    const GameplaySteeringOutput output = BuildGameplaySeekSteering(
        {0.0f, 0.0f, 0.0f},
        {0.3f, 0.0f, 0.0f},
        settings);

    EXPECT_EQ(output.status, GameplaySteeringStatus::Moving);
    EXPECT_FLOAT_EQ(output.movement.moveMagnitude, 1.0f);
}

TEST(GameplaySteering, SeekStopsInsideAcceptanceRadius)
{
    const GameplaySeekSteeringSettings settings{.acceptanceRadius = 0.5f};
    const GameplaySteeringOutput output = BuildGameplaySeekSteering(
        {0.0f, 0.0f, 0.0f},
        {0.25f, 100.0f, 0.0f},
        settings);

    EXPECT_EQ(output.status, GameplaySteeringStatus::Arrived);
    EXPECT_FALSE(output.movement.IsMoving());
    ExpectFiniteIntent(output.movement);
}

TEST(GameplaySteering, SeekSanitizesNegativeAcceptanceRadius)
{
    const GameplaySeekSteeringSettings settings{.acceptanceRadius = -1.0f};
    const GameplaySteeringOutput output = BuildGameplaySeekSteering(
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        settings);

    EXPECT_EQ(output.status, GameplaySteeringStatus::Moving);
    EXPECT_FLOAT_EQ(output.movement.moveMagnitude, 1.0f);
}

TEST(GameplaySteering, SeekPropagatesWantsRun)
{
    const GameplaySeekSteeringSettings settings{.acceptanceRadius = 0.0f, .wantsRun = true};
    const GameplaySteeringOutput output = BuildGameplaySeekSteering(
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        settings);

    EXPECT_TRUE(output.movement.wantsRun);
}

TEST(GameplaySteering, SeekZeroLengthPlanarDeltaDoesNotProduceNaN)
{
    const GameplaySteeringOutput output = BuildGameplaySeekSteering(
        {1.0f, 2.0f, 3.0f},
        {1.0f, 200.0f, 3.0f});

    EXPECT_EQ(output.status, GameplaySteeringStatus::Arrived);
    ExpectZeroVector(output.movement.moveWorld);
    ExpectFiniteIntent(output.movement);
    EXPECT_TRUE(std::isfinite(output.remainingDistance));
}

TEST(GameplaySteering, FleeNormalizesPlanarDirectionAwayFromThreat)
{
    const GameplayMovementIntent movement = BuildGameplayFleeSteering(
        {0.0f, 5.0f, 0.0f},
        {3.0f, 20.0f, 4.0f});

    ExpectVec3Near(movement.moveWorld, {-0.6f, 0.0f, -0.8f}, kEpsVec);
    EXPECT_NEAR(PlanarLength(movement.moveWorld), 1.0f, kTolerance);
    EXPECT_FLOAT_EQ(movement.moveMagnitude, 1.0f);
}

TEST(GameplaySteering, FleeDoesNotStopOrSlowBasedOnThreatDistance)
{
    const GameplayMovementIntent movement = BuildGameplayFleeSteering(
        {0.0f, 0.0f, 0.0f},
        {100.0f, 100.0f, 0.0f});

    EXPECT_TRUE(movement.IsMoving());
    EXPECT_FLOAT_EQ(movement.moveMagnitude, 1.0f);
    ExpectVec3Near(movement.moveWorld, {-1.0f, 0.0f, 0.0f}, kEpsVec);
}

TEST(GameplaySteering, FleePropagatesWantsRun)
{
    const GameplayFleeSteeringSettings settings{.wantsRun = true};
    const GameplayMovementIntent movement = BuildGameplayFleeSteering(
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        settings);

    EXPECT_TRUE(movement.wantsRun);
}

TEST(GameplaySteering, FleeZeroLengthPlanarDeltaDoesNotProduceNaN)
{
    const GameplayMovementIntent movement = BuildGameplayFleeSteering(
        {1.0f, 2.0f, 3.0f},
        {1.0f, 200.0f, 3.0f});

    EXPECT_FALSE(movement.IsMoving());
    ExpectZeroVector(movement.moveWorld);
    ExpectFiniteIntent(movement);
}

// Protects the explicit command integration boundary and prevents regressions that pass non-canonical movement to the motor.
TEST(GameplaySteering, AdapterWritesCanonicalCharacterMovementFields)
{
    GameplayMovementIntent intent{};
    intent.moveWorld = mathUtils::Vec3(3.0f, 7.0f, 4.0f);
    intent.moveMagnitude = 2.0f;
    intent.wantsRun = true;
    GameplayCharacterCommandComponent command{};
    command.moveInputX = 0.75f;
    command.moveInputY = -0.25f;

    ApplyGameplayMovementIntent(intent, command);

    EXPECT_NEAR(command.moveWorld.x, 0.6f, kTolerance);
    EXPECT_FLOAT_EQ(command.moveWorld.y, 0.0f);
    EXPECT_NEAR(command.moveWorld.z, 0.8f, kTolerance);
    EXPECT_NEAR(PlanarLength(command.moveWorld), 1.0f, kTolerance);
    EXPECT_FLOAT_EQ(command.moveMagnitude, 1.0f);
    EXPECT_TRUE(command.wantsRun);
    EXPECT_FLOAT_EQ(command.moveInputX, 0.0f);
    EXPECT_FLOAT_EQ(command.moveInputY, 0.0f);
}

// Protects separation between movement and action requests and prevents regressions that erase pending action intent state.
TEST(GameplaySteering, AdapterPreservesSemanticActionIntents)
{
    GameplayMovementIntent intent{};
    intent.moveWorld = mathUtils::Vec3(1.0f, 0.0f, 0.0f);
    intent.moveMagnitude = 1.0f;
    GameplayCharacterCommandComponent command{};
    command.actionIntents = { GameplayActionId{ "Test.One" }, GameplayActionId{ "Test.Two" } };

    ApplyGameplayMovementIntent(intent, command);

    EXPECT_EQ(command.actionIntents.size(), 2u);
}

// Protects against invalid manual movement intents and prevents regressions that retain run state for stationary commands.
TEST(GameplaySteering, AdapterClearsDegenerateMovement)
{
    GameplayMovementIntent degenerateDirection{};
    degenerateDirection.moveWorld = mathUtils::Vec3(0.0f, 3.0f, 0.0f);
    degenerateDirection.moveMagnitude = 1.0f;
    degenerateDirection.wantsRun = true;
    GameplayCharacterCommandComponent command{};

    ApplyGameplayMovementIntent(degenerateDirection, command);

    ExpectZeroVector(command.moveWorld);
    EXPECT_FLOAT_EQ(command.moveMagnitude, 0.0f);
    EXPECT_FALSE(command.wantsRun);
    EXPECT_TRUE(std::isfinite(command.moveWorld.x));
    EXPECT_TRUE(std::isfinite(command.moveWorld.y));
    EXPECT_TRUE(std::isfinite(command.moveWorld.z));

    GameplayMovementIntent negativeMagnitude{};
    negativeMagnitude.moveWorld = mathUtils::Vec3(1.0f, 0.0f, 0.0f);
    negativeMagnitude.moveMagnitude = -1.0f;
    negativeMagnitude.wantsRun = true;

    ApplyGameplayMovementIntent(negativeMagnitude, command);

    ExpectZeroVector(command.moveWorld);
    EXPECT_FLOAT_EQ(command.moveMagnitude, 0.0f);
    EXPECT_FALSE(command.wantsRun);
}

// Protects the floating-point arrival boundary so an agent negligibly outside
// the authored radius is accepted instead of returning Moving with no progress.
TEST(GameplaySteering, PositionWithinArrivalToleranceIsAccepted)
{
    GameplayArrivalSteeringSettings settings{};
    settings.acceptanceRadius = 0.2f;
    settings.slowingRadius = 1.0f;

    const GameplaySteeringOutput output =
        BuildGameplayArrivalSteering(
            {9.79994583f, 0.0f, 0.0f},
            {10.0f, 0.0f, 0.0f},
            settings);

    EXPECT_EQ(output.status, GameplaySteeringStatus::Arrived);
    EXPECT_FALSE(output.movement.IsMoving());
    EXPECT_NEAR(output.remainingDistance, 0.20005417f, 0.000001f);
}

// Protects the steering contract so a Moving result immediately outside the
// arrival tolerance always provides an executable non-zero movement intent.
TEST(GameplaySteering, MovingOutsideArrivalToleranceProvidesMovementIntent)
{
    GameplayArrivalSteeringSettings settings{};
    settings.acceptanceRadius = 0.2f;
    settings.slowingRadius = 1.0f;

    const GameplaySteeringOutput output =
        BuildGameplayArrivalSteering(
            {9.798f, 0.0f, 0.0f},
            {10.0f, 0.0f, 0.0f},
            settings);

    ASSERT_EQ(output.status, GameplaySteeringStatus::Moving);
    EXPECT_TRUE(output.movement.IsMoving());
    EXPECT_GT(output.movement.moveMagnitude, 0.0f);
    EXPECT_NEAR(output.movement.moveWorld.x, 1.0f, 0.000001f);
    EXPECT_NEAR(output.movement.moveWorld.z,0.0f,0.000001f);
}
        