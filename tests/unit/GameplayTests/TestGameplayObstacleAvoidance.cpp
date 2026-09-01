#include <cmath>
#include <cstddef>
#include <gtest/gtest.h>
#include <limits>
#include <iterator>
#include <utility>
#include <vector>

#include "TestSupport/MathTestHelper.h"

import core;

using namespace rendern;

namespace
{
    struct ScriptedResult
    {
        bool hit{false};
        float distance{0.0f};
        mathUtils::Vec3 position{};
        mathUtils::Vec3 normal{};
    };

    class ScriptedObstacleQuery final : public IGameplayObstacleQuery
    {
    public:
        explicit ScriptedObstacleQuery(std::vector<ScriptedResult> results = {})
            : results_(std::move(results))
        {
        }

        [[nodiscard]] bool Probe(
            const GameplayObstacleProbeRequest& request,
            GameplayObstacleProbeHit& hit) const noexcept override
        {
            requests.push_back(request);
            const ScriptedResult result = nextResult_ < results_.size()
                ? results_[nextResult_++]
                : ScriptedResult{};
            hit.distance = result.distance;
            hit.position = result.position;
            hit.normal = result.normal;
            return result.hit;
        }
        
        [[nodiscard]] bool ProbeSupport(
            const GameplaySupportProbeRequest& request,
            GameplaySupportProbeHit& hit) const noexcept override
        {
            supportRequests.push_back(request);
            hit.distance = 0.0f;
            hit.position = request.origin;
            hit.normal = {0.0f, 1.0f, 0.0f};
            return true;
        }

        mutable std::vector<GameplayObstacleProbeRequest> requests{};
        mutable std::vector<GameplaySupportProbeRequest> supportRequests{};

    private:
        std::vector<ScriptedResult> results_{};
        mutable std::size_t nextResult_{0};
    };
    
    class NearWallObstacleQuery final : public IGameplayObstacleQuery
    {
    public:
        [[nodiscard]] bool Probe(
            const GameplayObstacleProbeRequest& request,
            GameplayObstacleProbeHit& hit) const noexcept override
        {
            const bool isForwardProbe = request.direction.x > 0.9f;
            const bool overlapsWallSpan =
                std::abs(request.origin.z) < wallHalfExtent + request.clearanceRadius;
            if (!isForwardProbe || !overlapsWallSpan)
            {
                return false;
            }
            hit.distance = 0.05f;
            hit.position = {0.0f, request.origin.y, request.origin.z};
            hit.normal = {-1.0f, 0.0f, 0.0f};
            return true;
        }

        static constexpr float wallHalfExtent = 0.5f;
    };

    [[nodiscard]] GameplayMovementIntent MovingIntent() noexcept
    {
        return {.moveWorld = {1.0f, 0.0f, 0.0f}, .moveMagnitude = 0.6f, .wantsRun = true};
    }

    void ExpectValidCorrection(
        const GameplayMovementIntent& output,
        const GameplayMovementIntent& input)
    {
        EXPECT_FLOAT_EQ(output.moveWorld.y, 0.0f);
        EXPECT_TRUE(mathUtils::IsFinite(output.moveWorld));
        EXPECT_NEAR(mathUtils::Length(output.moveWorld), 1.0f, MathTestHelper::kTolerance);
        EXPECT_GE(mathUtils::Dot(output.moveWorld, input.moveWorld), 0.0f);
        EXPECT_FLOAT_EQ(output.moveMagnitude, input.moveMagnitude);
        EXPECT_EQ(output.wantsRun, input.wantsRun);
    }
}

TEST(GameplayObstacleAvoidance, ClearPathDebugSnapshot)
{
    const GameplayMovementIntent input = MovingIntent();
    ScriptedObstacleQuery query{};
    GameplayObstacleAvoidanceDebugSnapshot debug{};
    const auto output = ApplyGameplayObstacleAvoidance(input, {1.0f, 2.0f, 3.0f}, query, {}, &debug);

    EXPECT_TRUE(debug.evaluated);
    EXPECT_FALSE(debug.active);
    EXPECT_EQ(debug.chosenSide, GameplayObstacleAvoidanceSide::None);
    EXPECT_TRUE(debug.forward.queried);
    EXPECT_FALSE(debug.forward.hit);
    MathTestHelper::ExpectVec3Near(debug.baseMovement.moveWorld, output.moveWorld, MathTestHelper::kEpsVec);
    MathTestHelper::ExpectVec3Near(debug.finalMovement.moveWorld, output.moveWorld, MathTestHelper::kEpsVec);
}

TEST(GameplayObstacleAvoidance, BlockedDebugSnapshotsReportActualDecision)
{
    ScriptedObstacleQuery leftQuery{{{true, 0.25f}, {false}, {true, 0.2f}}};
    GameplayObstacleAvoidanceDebugSnapshot left{};
    const auto leftOutput = ApplyGameplayObstacleAvoidance(MovingIntent(), {}, leftQuery, {}, &left);
    EXPECT_TRUE(left.active);
    EXPECT_EQ(left.chosenSide, GameplayObstacleAvoidanceSide::Left);
    EXPECT_FLOAT_EQ(left.forward.clearance, 0.25f);
    EXPECT_FLOAT_EQ(left.left.clearance, 1.0f);
    EXPECT_FLOAT_EQ(left.right.clearance, 0.2f);
    MathTestHelper::ExpectVec3Near(left.finalMovement.moveWorld, leftOutput.moveWorld, MathTestHelper::kEpsVec);

    ScriptedObstacleQuery rightQuery{{{true, 0.25f}, {true, 0.2f}, {false}}};
    GameplayObstacleAvoidanceDebugSnapshot right{};
    const auto rightOutput = ApplyGameplayObstacleAvoidance(MovingIntent(), {}, rightQuery, {}, &right);
    EXPECT_TRUE(right.active);
    EXPECT_EQ(right.chosenSide, GameplayObstacleAvoidanceSide::Right);
    MathTestHelper::ExpectVec3Near(right.finalMovement.moveWorld, rightOutput.moveWorld, MathTestHelper::kEpsVec);
}

TEST(GameplayObstacleAvoidance, HitPositionAndNormalArePropagated)
{
    const mathUtils::Vec3 position{4.0f, 5.0f, 6.0f};
    const mathUtils::Vec3 normal{0.0f, 0.0f, -1.0f};
    ScriptedObstacleQuery query{{{true, 0.5f, position, normal}, {false}, {false}}};
    GameplayObstacleAvoidanceDebugSnapshot debug{};
    ApplyGameplayObstacleAvoidance(MovingIntent(), {}, query, {}, &debug);
    MathTestHelper::ExpectVec3Near(debug.forward.hitPosition, position, MathTestHelper::kEpsVec);
    MathTestHelper::ExpectVec3Near(debug.forward.hitNormal, normal, MathTestHelper::kEpsVec);
}

TEST(GameplayObstacleAvoidance, StationaryDebugSnapshotHasNoStaleProbes)
{
    ScriptedObstacleQuery query{};
    GameplayObstacleAvoidanceDebugSnapshot debug{};
    ApplyGameplayObstacleAvoidance({}, {}, query, {}, &debug);
    EXPECT_TRUE(debug.evaluated);
    EXPECT_FALSE(debug.active);
    EXPECT_FALSE(debug.forward.queried);
    EXPECT_EQ(debug.chosenSide, GameplayObstacleAvoidanceSide::None);
}

TEST(GameplayObstacleAvoidance, DebugOutputDoesNotChangeMovement)
{
    ScriptedObstacleQuery withoutQuery{{{true, 0.2f}, {true, 0.1f}, {false}}};
    ScriptedObstacleQuery withQuery{{{true, 0.2f}, {true, 0.1f}, {false}}};
    GameplayObstacleAvoidanceDebugSnapshot debug{};
    const auto without = ApplyGameplayObstacleAvoidance(MovingIntent(), {}, withoutQuery);
    const auto with = ApplyGameplayObstacleAvoidance(MovingIntent(), {}, withQuery, {}, &debug);
    MathTestHelper::ExpectVec3Near(without.moveWorld, with.moveWorld, MathTestHelper::kEpsVec);
    EXPECT_FLOAT_EQ(without.moveMagnitude, with.moveMagnitude);
    EXPECT_EQ(without.wantsRun, with.wantsRun);
}

TEST(GameplayObstacleAvoidance, ActiveMeansDirectionActuallyChanged)
{
    ScriptedObstacleQuery query{{{true, 0.2f}, {false}, {false}}};
    GameplayObstacleAvoidanceDebugSnapshot debug{};
    const GameplayMovementIntent input = MovingIntent();
    const GameplayMovementIntent output = ApplyGameplayObstacleAvoidance(
        input, {}, query,
        {.forwardProbeDistance = 1.5f,
         .sideProbeDistance = 1.0f,
         .sideProbeAngleDegrees = 0.0f},
        &debug);

    MathTestHelper::ExpectVec3Near(output.moveWorld, input.moveWorld, MathTestHelper::kEpsVec);
    EXPECT_FALSE(debug.active);
    EXPECT_EQ(debug.chosenSide, GameplayObstacleAvoidanceSide::None);
}

TEST(GameplaySteeringDebug, DisabledRegistryDoesNotStoreStates)
{
    GameplaySteeringDebugRegistry registry{};
    registry.Publish(1u, GameplaySteeringDebugMode::Follow, {});
    EXPECT_TRUE(registry.States().empty());
}

TEST(GameplaySteeringDebug, DisablingClearsAndReenablingAllowsFreshPublication)
{
    GameplaySteeringDebugRegistry registry{};
    registry.SetEnabled(true);
    registry.Publish(1u, GameplaySteeringDebugMode::Follow, {});
    ASSERT_NE(registry.Find(1u), nullptr);

    registry.SetEnabled(false);
    EXPECT_TRUE(registry.States().empty());
    registry.Publish(1u, GameplaySteeringDebugMode::Flee, {});
    EXPECT_TRUE(registry.States().empty());

    registry.SetEnabled(true);
    registry.Publish(1u, GameplaySteeringDebugMode::Route, {});
    ASSERT_NE(registry.Find(1u), nullptr);
    EXPECT_EQ(registry.Find(1u)->mode, GameplaySteeringDebugMode::Route);
}


TEST(GameplayObstacleAvoidance, ClearPathPreservesMovement)
{
    const GameplayMovementIntent input = MovingIntent();
    ScriptedObstacleQuery query{};
    const GameplayMovementIntent output = ApplyGameplayObstacleAvoidance(input, {}, query);

    MathTestHelper::ExpectVec3Near(output.moveWorld, input.moveWorld, MathTestHelper::kEpsVec);
    EXPECT_FLOAT_EQ(output.moveMagnitude, 0.6f);
    EXPECT_TRUE(output.wantsRun);
}

TEST(GameplayObstacleAvoidance, StationaryIntentDoesNotQuery)
{
    const GameplayMovementIntent input{};
    ScriptedObstacleQuery query{};
    const GameplayMovementIntent output = ApplyGameplayObstacleAvoidance(input, {}, query);

    EXPECT_TRUE(query.requests.empty());
    MathTestHelper::ExpectVec3Near(output.moveWorld, input.moveWorld, MathTestHelper::kEpsVec);
}

TEST(GameplayObstacleAvoidance, BuildsThreePlanarNormalizedFeelersWithConfiguredLengths)
{
    ScriptedObstacleQuery query{};
    const GameplayObstacleAvoidanceSettings settings{
        .forwardProbeDistance = 2.5f,
        .sideProbeDistance = 1.25f,
        .sideProbeAngleDegrees = 30.0f
    };
    ApplyGameplayObstacleAvoidance(MovingIntent(), {4.0f, 2.0f, 3.0f}, query, settings);

    ASSERT_EQ(query.requests.size(), 3u);
    EXPECT_FLOAT_EQ(query.requests[0].maximumDistance, 2.5f);
    EXPECT_FLOAT_EQ(query.requests[1].maximumDistance, 1.25f);
    EXPECT_FLOAT_EQ(query.requests[2].maximumDistance, 1.25f);
    MathTestHelper::ExpectVec3Near(query.requests[0].direction, {1.0f, 0.0f, 0.0f}, MathTestHelper::kEpsVec);
    for (const GameplayObstacleProbeRequest& request : query.requests)
    {
        EXPECT_FLOAT_EQ(request.direction.y, 0.0f);
        EXPECT_NEAR(mathUtils::Length(request.direction), 1.0f, MathTestHelper::kTolerance);
        MathTestHelper::ExpectVec3Near(request.origin, {4.0f, 2.0f, 3.0f}, MathTestHelper::kEpsVec);
    }
    EXPECT_NEAR(query.requests[1].direction.x, query.requests[2].direction.x, MathTestHelper::kTolerance);
    EXPECT_NEAR(query.requests[1].direction.z, -query.requests[2].direction.z, MathTestHelper::kTolerance);

	// Gameplay movement defines planar right as (-forward.z, 0, forward.x).
    // Therefore for +X forward, left is -Z and right is +Z.
    EXPECT_LT(query.requests[1].direction.z, 0.0f);
    EXPECT_GT(query.requests[2].direction.z, 0.0f);
}

TEST(GameplayObstacleAvoidance, AppliesCharacterClearanceRadiusToEveryProbe)
{
    ScriptedObstacleQuery query{};
    ApplyGameplayObstacleAvoidance(
    {.baseMovement = MovingIntent(), .characterRadius = 0.35f}, query);

    ASSERT_EQ(query.requests.size(), 3u);
    for (const GameplayObstacleProbeRequest& request : query.requests)
    {
        EXPECT_FLOAT_EQ(request.clearanceRadius, 0.37f);
    }
}

TEST(GameplayObstacleAvoidance, SharedSettingsDoNotContaminatePerEvaluationRadius)
{
    const GameplayObstacleAvoidanceSettings settings{.clearanceMargin = 0.1f};
    ScriptedObstacleQuery smallQuery{};
    ScriptedObstacleQuery largeQuery{};

    const GameplayMovementIntent smallOutput = ApplyGameplayObstacleAvoidance(
        {.baseMovement = MovingIntent(), .characterRadius = 0.2f}, smallQuery, settings);
    const GameplayMovementIntent largeOutput = ApplyGameplayObstacleAvoidance(
        {.baseMovement = MovingIntent(), .characterRadius = 0.6f}, largeQuery, settings);

    EXPECT_TRUE(smallOutput.IsMoving());
    EXPECT_TRUE(largeOutput.IsMoving());
    ASSERT_EQ(smallQuery.requests.size(), 3u);
    ASSERT_EQ(largeQuery.requests.size(), 3u);
    EXPECT_FLOAT_EQ(smallQuery.requests.front().clearanceRadius, 0.3f);
    EXPECT_FLOAT_EQ(largeQuery.requests.front().clearanceRadius, 0.7f);
    EXPECT_FLOAT_EQ(settings.clearanceMargin, 0.1f);
}

TEST(GameplayObstacleAvoidance, ZeroRadiusPreservesPointProbesDespiteDefaultMargin)
{
    ScriptedObstacleQuery query{};
    ApplyGameplayObstacleAvoidance(MovingIntent(), {}, query);

    ASSERT_EQ(query.requests.size(), 3u);
    for (const GameplayObstacleProbeRequest& request : query.requests)
    {
        EXPECT_FLOAT_EQ(request.clearanceRadius, 0.0f);
    }
}

TEST(GameplayObstacleAvoidance, MalformedClearanceSettingsCannotReachObstacleQuery)
{
    constexpr float infinity = std::numeric_limits<float>::infinity();
    constexpr float nan = std::numeric_limits<float>::quiet_NaN();
    const GameplayObstacleAvoidanceInput malformedInputs[]{
        {.baseMovement = MovingIntent(), .characterRadius = -1.0f},
        {.baseMovement = MovingIntent(), .characterRadius = infinity},
        {.baseMovement = MovingIntent(), .characterRadius = nan},
        {.baseMovement = MovingIntent(), .characterRadius = 0.3f},
        {.baseMovement = MovingIntent(), .characterRadius = 0.3f},
        {.baseMovement = MovingIntent(), .characterRadius = 0.3f}
    };
    const GameplayObstacleAvoidanceSettings malformedSettings[]{
        {.clearanceMargin = 0.1f},
        {.clearanceMargin = 0.1f},
        {.clearanceMargin = 0.1f},
        {.clearanceMargin = -1.0f},
        {.clearanceMargin = infinity},
        {.clearanceMargin = nan}
    };

    for (std::size_t index = 0; index < std::size(malformedSettings); ++index)
    {
        ScriptedObstacleQuery query{};
        ApplyGameplayObstacleAvoidance(
            malformedInputs[index], query, malformedSettings[index]);
        ASSERT_EQ(query.requests.size(), 3u);
        const float expectedRadius = index < 3u ? 0.0f : 0.3f;
        for (const GameplayObstacleProbeRequest& request : query.requests)
        {
            EXPECT_TRUE(std::isfinite(request.clearanceRadius));
            EXPECT_FLOAT_EQ(request.clearanceRadius, expectedRadius);
        }
    }
}

TEST(GameplayObstacleAvoidance, MalformedSupportOriginOffsetUsesZero)
{
    for (const float offset : {-1.0f, std::numeric_limits<float>::infinity(),
             std::numeric_limits<float>::quiet_NaN()})
    {
        ScriptedObstacleQuery query{{{true, 0.1f}, {false}, {false}}};
        const GameplayMovementIntent output = ApplyGameplayObstacleAvoidance(
            {.baseMovement = MovingIntent(),
             .probeOrigin = {0.0f, 2.0f, 0.0f},
             .supportOriginVerticalOffset = offset},
            query);

        EXPECT_TRUE(output.IsMoving());
        ASSERT_EQ(query.supportRequests.size(), 2u);
        for (const GameplaySupportProbeRequest& request : query.supportRequests)
        {
            EXPECT_FLOAT_EQ(request.origin.y, 2.25f);
            EXPECT_TRUE(mathUtils::IsFinite(request.origin));
        }
    }
}

TEST(GameplayObstacleAvoidance, ValidNormalProducesTangentDominantEscapeForBothSides)
{
    for (const GameplayObstacleAvoidanceSide side : {
             GameplayObstacleAvoidanceSide::Left,
             GameplayObstacleAvoidanceSide::Right})
    {
        GameplayObstacleAvoidanceState state{side};
        ScriptedObstacleQuery query{
            {{true, 0.1f, {}, {-1.0f, 0.0f, 0.0f}}, {false}, {false}}};
        const GameplayMovementIntent input = MovingIntent();
        const GameplayMovementIntent output = ApplyGameplayObstacleAvoidance(
            input, {}, query, {}, state);

        EXPECT_EQ(state.committedSide, side);
        EXPECT_TRUE(mathUtils::IsFinite(output.moveWorld));
        EXPECT_NEAR(mathUtils::Length(output.moveWorld), 1.0f, MathTestHelper::kTolerance);
        EXPECT_GT(std::abs(output.moveWorld.z), std::abs(output.moveWorld.x));
        EXPECT_GE(mathUtils::Dot(output.moveWorld, {-1.0f, 0.0f, 0.0f}), 0.0f);
        EXPECT_EQ(output.moveWorld.z < 0.0f,
            side == GameplayObstacleAvoidanceSide::Left);
        EXPECT_FLOAT_EQ(output.moveMagnitude, input.moveMagnitude);
        EXPECT_EQ(output.wantsRun, input.wantsRun);
    }
}

TEST(GameplayObstacleAvoidance, SurfaceTangentEscapesWallEdgeAndReleasesCommitment)
{
    NearWallObstacleQuery query{};
    GameplayObstacleAvoidanceState state{};
    const GameplayObstacleAvoidanceSettings settings{};
    mathUtils::Vec3 position{-0.1f, 0.8f, 0.0f};
    bool clearedEdge = false;
    int releasedSteps = 0;
    float positionAtReleaseX = position.x;

    for (int step = 0; step < 20; ++step)
    {
        const GameplayMovementIntent output = ApplyGameplayObstacleAvoidance(
    {.baseMovement = MovingIntent(), .probeOrigin = position, .characterRadius = 0.3f},
            query, settings, state);
        position = position + output.moveWorld * 0.1f;
        if (state.committedSide == GameplayObstacleAvoidanceSide::None)
        {
            if (!clearedEdge)
            {
                clearedEdge = true;
                positionAtReleaseX = position.x;
            }
            ++releasedSteps;
            EXPECT_GT(output.moveWorld.x, 0.99f);
            EXPECT_NEAR(output.moveWorld.z, 0.0f, MathTestHelper::kTolerance);
            if (releasedSteps == 4)
            {
                break;
            }
            continue;
        }
        EXPECT_EQ(state.committedSide, GameplayObstacleAvoidanceSide::Left);
        EXPECT_GT(std::abs(output.moveWorld.z), std::abs(output.moveWorld.x));
    }

    EXPECT_TRUE(clearedEdge);
    EXPECT_EQ(releasedSteps, 4);
    EXPECT_LT(position.z, -NearWallObstacleQuery::wallHalfExtent);
    EXPECT_GT(position.x, positionAtReleaseX);
}

TEST(GameplayObstacleAvoidance, DegenerateHitNormalFallsBackWithoutNaN)
{
    ScriptedObstacleQuery query{
        {{true, 0.1f, {}, {0.0f, 1.0f, 0.0f}}, {false}, {false}}};
    const GameplayMovementIntent output = ApplyGameplayObstacleAvoidance(
        MovingIntent(), {}, query);

    EXPECT_TRUE(mathUtils::IsFinite(output.moveWorld));
    EXPECT_NEAR(mathUtils::Length(output.moveWorld), 1.0f, MathTestHelper::kTolerance);
}

TEST(GameplayObstacleAvoidance, ForwardBlockedChoosesClearLeft)
{
    ScriptedObstacleQuery query{{{true, 0.2f}, {false, 0.0f}, {true, 0.1f}}};
    const GameplayMovementIntent input = MovingIntent();
    const GameplayMovementIntent output = ApplyGameplayObstacleAvoidance(input, {}, query);

    EXPECT_LT(output.moveWorld.z, 0.0f);
    ExpectValidCorrection(output, input);
}

TEST(GameplayObstacleAvoidance, ForwardBlockedChoosesClearRight)
{
    ScriptedObstacleQuery query{{{true, 0.2f}, {true, 0.1f}, {false, 0.0f}}};
    const GameplayMovementIntent input = MovingIntent();
    const GameplayMovementIntent output = ApplyGameplayObstacleAvoidance(input, {}, query);

    EXPECT_GT(output.moveWorld.z, 0.0f);
    ExpectValidCorrection(output, input);
}

TEST(GameplayObstacleAvoidance, ChoosesSideWithGreaterClearance)
{
    ScriptedObstacleQuery query{{{true, 0.1f}, {true, 0.8f}, {true, 0.2f}}};
    const GameplayMovementIntent output = ApplyGameplayObstacleAvoidance(MovingIntent(), {}, query);

    EXPECT_LT(output.moveWorld.z, 0.0f);
}

TEST(GameplayObstacleAvoidance, EqualClearanceUsesDeterministicLeftTieBreak)
{
    for (int iteration = 0; iteration < 8; ++iteration)
    {
        ScriptedObstacleQuery query{{{true, 0.1f}, {true, 0.5f}, {true, 0.5f}}};
        const GameplayMovementIntent output = ApplyGameplayObstacleAvoidance(MovingIntent(), {}, query);
        EXPECT_LT(output.moveWorld.z, 0.0f);
    }
}

TEST(GameplayObstacleAvoidance, StatefulInitialDecisionCommitsGreaterClearanceAndLeftTie)
{
    GameplayObstacleAvoidanceState state{};
    ScriptedObstacleQuery rightQuery{{{true, 0.1f}, {true, 0.2f}, {true, 0.8f}}};
    const GameplayMovementIntent right =
        ApplyGameplayObstacleAvoidance(MovingIntent(), {}, rightQuery, {}, state);
    EXPECT_GT(right.moveWorld.z, 0.0f);
    EXPECT_EQ(state.committedSide, GameplayObstacleAvoidanceSide::Right);

    state = {};
    ScriptedObstacleQuery tieQuery{{{true, 0.1f}, {true, 0.5f}, {true, 0.5f}}};
    const GameplayMovementIntent tie =
        ApplyGameplayObstacleAvoidance(MovingIntent(), {}, tieQuery, {}, state);
    EXPECT_LT(tie.moveWorld.z, 0.0f);
    EXPECT_EQ(state.committedSide, GameplayObstacleAvoidanceSide::Left);
}

TEST(GameplayObstacleAvoidance, HysteresisHoldsUntilOppositeSideIsStrictlyBetter)
{
    const GameplayObstacleAvoidanceSettings settings{
        .sideSwitchClearanceAdvantage = 0.15f};
    GameplayObstacleAvoidanceState state{GameplayObstacleAvoidanceSide::Left};
    GameplayObstacleAvoidanceDebugSnapshot debug{};
    ScriptedObstacleQuery smallAdvantage{
        {{true, 0.1f}, {true, 0.8f}, {true, 0.85f}}};

    const GameplayMovementIntent held = ApplyGameplayObstacleAvoidance(
        MovingIntent(), {}, smallAdvantage, settings, state, &debug);

    EXPECT_LT(held.moveWorld.z, 0.0f);
    EXPECT_EQ(state.committedSide, GameplayObstacleAvoidanceSide::Left);
    EXPECT_EQ(debug.preferredSide, GameplayObstacleAvoidanceSide::Right);
    EXPECT_EQ(debug.chosenSide, GameplayObstacleAvoidanceSide::Left);
    EXPECT_TRUE(debug.sideHeldByHysteresis);

    ScriptedObstacleQuery exactThreshold{
        {{true, 0.1f}, {true, 0.4f}, {true, 0.55f}}};
    const GameplayMovementIntent threshold = ApplyGameplayObstacleAvoidance(
        MovingIntent(), {}, exactThreshold, settings, state);
    EXPECT_LT(threshold.moveWorld.z, 0.0f);
    EXPECT_EQ(state.committedSide, GameplayObstacleAvoidanceSide::Left);

    ScriptedObstacleQuery largeAdvantage{
        {{true, 0.1f}, {true, 0.4f}, {true, 0.8f}}};
    const GameplayMovementIntent switched = ApplyGameplayObstacleAvoidance(
        MovingIntent(), {}, largeAdvantage, settings, state);
    EXPECT_GT(switched.moveWorld.z, 0.0f);
    EXPECT_EQ(state.committedSide, GameplayObstacleAvoidanceSide::Right);
}

TEST(GameplayObstacleAvoidance, ClearAndStationaryEvaluationsResetCommitment)
{
    const GameplayMovementIntent input = MovingIntent();
    GameplayObstacleAvoidanceState state{GameplayObstacleAvoidanceSide::Right};
    ScriptedObstacleQuery clearQuery{};
    const GameplayMovementIntent clear = ApplyGameplayObstacleAvoidance(
        input, {}, clearQuery, {}, state);
    EXPECT_EQ(state.committedSide, GameplayObstacleAvoidanceSide::None);
    MathTestHelper::ExpectVec3Near(clear.moveWorld, input.moveWorld, MathTestHelper::kEpsVec);

    state.committedSide = GameplayObstacleAvoidanceSide::Left;
    ScriptedObstacleQuery stationaryQuery{};
    const GameplayMovementIntent stationary = ApplyGameplayObstacleAvoidance(
        {}, {}, stationaryQuery, {}, state);
    EXPECT_EQ(state.committedSide, GameplayObstacleAvoidanceSide::None);
    EXPECT_TRUE(stationaryQuery.requests.empty());
    EXPECT_FALSE(stationary.IsMoving());
}

TEST(GameplayObstacleAvoidance, MalformedHysteresisAdvantageUsesDeterministicZero)
{
    for (const float advantage : {-1.0f, std::numeric_limits<float>::infinity(),
             std::numeric_limits<float>::quiet_NaN()})
    {
        GameplayObstacleAvoidanceState state{GameplayObstacleAvoidanceSide::Left};
        ScriptedObstacleQuery query{{{true, 0.1f}, {true, 0.4f}, {true, 0.41f}}};
        const GameplayMovementIntent output = ApplyGameplayObstacleAvoidance(
            MovingIntent(), {}, query,
            {.sideSwitchClearanceAdvantage = advantage}, state);
        EXPECT_GT(output.moveWorld.z, 0.0f);
        EXPECT_EQ(state.committedSide, GameplayObstacleAvoidanceSide::Right);
    }
}

TEST(GameplayObstacleAvoidance, SideHitDoesNotModifyClearForwardMovement)
{
    ScriptedObstacleQuery query{{{false, 0.0f}, {true, 0.0f}, {false, 0.0f}}};
    const GameplayMovementIntent input = MovingIntent();
    const GameplayMovementIntent output = ApplyGameplayObstacleAvoidance(input, {}, query);

    MathTestHelper::ExpectVec3Near(output.moveWorld, input.moveWorld, MathTestHelper::kEpsVec);
}

TEST(GameplayObstacleAvoidance, NegativeAndNonFiniteProbeSettingsAreSkipped)
{
    ScriptedObstacleQuery query{};
    const GameplayObstacleAvoidanceSettings settings{
        .forwardProbeDistance = -1.0f,
        .sideProbeDistance = std::numeric_limits<float>::infinity(),
        .sideProbeAngleDegrees = std::numeric_limits<float>::quiet_NaN()
    };
    const GameplayMovementIntent output = ApplyGameplayObstacleAvoidance(MovingIntent(), {}, query, settings);

    EXPECT_TRUE(query.requests.empty());
    EXPECT_TRUE(mathUtils::IsFinite(output.moveWorld));
}

TEST(GameplayObstacleAvoidance, ClampsMalformedHitDistances)
{
    ScriptedObstacleQuery negative{{{true, 0.1f}, {true, -5.0f}, {true, 0.25f}}};
    ScriptedObstacleQuery excessive{{{true, 0.1f}, {true, 50.0f}, {true, 0.25f}}};

    const GameplayMovementIntent negativeOutput = ApplyGameplayObstacleAvoidance(MovingIntent(), {}, negative);
    const GameplayMovementIntent excessiveOutput = ApplyGameplayObstacleAvoidance(MovingIntent(), {}, excessive);

    EXPECT_GT(negativeOutput.moveWorld.z, 0.0f);
    EXPECT_LT(excessiveOutput.moveWorld.z, 0.0f);
    ExpectValidCorrection(negativeOutput, MovingIntent());
    ExpectValidCorrection(excessiveOutput, MovingIntent());
}