#include <cmath>
#include <cstddef>
#include <gtest/gtest.h>
#include <limits>
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
            return result.hit;
        }

        mutable std::vector<GameplayObstacleProbeRequest> requests{};

    private:
        std::vector<ScriptedResult> results_{};
        mutable std::size_t nextResult_{0};
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

    EXPECT_LT(output.moveWorld.z, 0.0f);
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