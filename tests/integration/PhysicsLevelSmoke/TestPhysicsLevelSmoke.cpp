#include <gtest/gtest.h>

import core;
import std;

#include "App/AppLifecycle.h"
#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/LevelPhysicsRuntime.h"

namespace
{
    constexpr std::string_view PhysicsSmokeMap =
        "levels/physics_falling_body_smoke.level.json";
    constexpr std::string_view FloorNodeName = "PhysicsSmokeFloor";
    constexpr std::string_view SphereNodeName = "PhysicsSmokeSphere";
    constexpr float FixedDeltaSeconds = 1.0f / 60.0f;
    constexpr int SimulationStepCount = 300;
    constexpr float ExpectedRestingHeight = 0.5f;
    constexpr float MinimumAllowedHeight = 0.45f;
    constexpr float RestingHeightTolerance = 0.05f;
    constexpr float HorizontalTolerance = 0.01f;
    constexpr float RuntimeSyncTolerance = 0.001f;

    [[nodiscard]] int FindNodeIndex(
        const rendern::LevelAsset& level,
        const std::string_view nodeName)
    {
        const auto iterator = std::ranges::find(level.nodes, nodeName, &rendern::LevelNode::name);
        return iterator == level.nodes.end()
            ? -1
            : static_cast<int>(std::distance(level.nodes.begin(), iterator));
    }

    [[nodiscard]] bool ArePositionsNear(
        const mathUtils::Vec3& first,
        const mathUtils::Vec3& second,
        const float tolerance) noexcept
    {
        return std::fabs(first.x - second.x) <= tolerance
            && std::fabs(first.y - second.y) <= tolerance
            && std::fabs(first.z - second.z) <= tolerance;
    }

    struct ArgvBuffer
    {
        std::vector<std::string> storage;
        std::vector<char*> argv;

        explicit ArgvBuffer(std::vector<std::string> values)
            : storage(std::move(values))
        {
            argv.reserve(storage.size());
            for (std::string& value : storage)
            {
                argv.push_back(value.data());
            }
        }
    };

    class PhysicsLevelSmokeTest : public testing::Test
    {
    protected:
        void SetUp() override
        {
            arguments = std::make_unique<ArgvBuffer>(std::vector<std::string>{
                "CoreEnginePhysicsLevelSmokeTests.exe",
                "--null",
                "--map=levels/physics_falling_body_smoke.level.json"
            });

            ASSERT_NO_THROW(appLifecycle::InitializeApp(
                app,
                static_cast<int>(arguments->argv.size()),
                arguments->argv.data()));
        }

        void TearDown() override
        {
            if (!app.initialized)
            {
                return;
            }
            if (app.runtimeState.gameplayMode == rendern::GameplayRuntimeMode::Game)
            {
                std::string ignoredError;
                static_cast<void>(appLifecycle::SetGameplayMode(
                    app,
                    rendern::GameplayRuntimeMode::Editor,
                    ignoredError));
            }
            EXPECT_NO_THROW(appLifecycle::ShutdownApp(app));
        }

        appLifecycle::AppState app{};
        std::unique_ptr<ArgvBuffer> arguments;
    };
}

TEST_F(PhysicsLevelSmokeTest, FallingBodyMapRunsThroughApplicationLifecycle)
{
    ASSERT_EQ(app.launchState.currentLevelName, PhysicsSmokeMap);

    rendern::LevelAsset& level = *app.contentState.levelAsset;
    const int floorNodeIndex = FindNodeIndex(level, FloorNodeName);
    const int sphereNodeIndex = FindNodeIndex(level, SphereNodeName);
    ASSERT_GE(floorNodeIndex, 0);
    ASSERT_GE(sphereNodeIndex, 0);
    ASSERT_TRUE(level.nodes[static_cast<std::size_t>(floorNodeIndex)].physicsBody.has_value());
    ASSERT_TRUE(level.nodes[static_cast<std::size_t>(sphereNodeIndex)].physicsBody.has_value());

    const std::size_t expectedBindingCount = static_cast<std::size_t>(std::ranges::count_if(
        level.nodes,
        [](const rendern::LevelNode& node)
        {
            return node.alive && node.physicsBody.has_value();
        }));
    const mathUtils::Vec3 initialPosition =
        level.nodes[static_cast<std::size_t>(sphereNodeIndex)].transform.position;

    std::string error;
    ASSERT_TRUE(appLifecycle::SetGameplayMode(
        app,
        rendern::GameplayRuntimeMode::Game,
        error)) << error;
    EXPECT_EQ(app.runtimeState.gameplayMode, rendern::GameplayRuntimeMode::Game);
    EXPECT_TRUE(app.physicsState.levelPhysicsRuntime->IsActive());
    EXPECT_EQ(app.physicsState.levelPhysicsRuntime->GetBindingCount(), expectedBindingCount);

    for (int stepIndex = 0; stepIndex < SimulationStepCount; ++stepIndex)
    {
        const std::uint32_t completedSteps =
            app.physicsState.joltPhysicsWorld->Update(FixedDeltaSeconds);
        ASSERT_EQ(completedSteps, 1u) << "step index: " << stepIndex;

        error.clear();
        const bool synchronized = app.physicsState.levelPhysicsRuntime->Synchronize(
            level,
            *app.runtimeState.levelInstance,
            app.runtimeState.scene,
            error);
        ASSERT_TRUE(synchronized)
            << "step index: " << stepIndex << ", error: " << error;
    }

    const mathUtils::Vec3 assetFinalPosition =
        level.nodes[static_cast<std::size_t>(sphereNodeIndex)].transform.position;
    const mathUtils::Vec3 runtimeFinalPosition =
        app.runtimeState.levelInstance->GetNodeWorldPosition(sphereNodeIndex);
    EXPECT_LT(assetFinalPosition.y, initialPosition.y);
    EXPECT_TRUE(ArePositionsNear(
        assetFinalPosition, runtimeFinalPosition, RuntimeSyncTolerance));
    EXPECT_LT(runtimeFinalPosition.y, initialPosition.y);
    EXPECT_GT(runtimeFinalPosition.y, MinimumAllowedHeight);
    EXPECT_NEAR(runtimeFinalPosition.y, ExpectedRestingHeight, RestingHeightTolerance);
    EXPECT_NEAR(runtimeFinalPosition.x, 0.0f, HorizontalTolerance);
    EXPECT_NEAR(runtimeFinalPosition.z, 0.0f, HorizontalTolerance);

    error.clear();
    ASSERT_TRUE(appLifecycle::SetGameplayMode(
        app,
        rendern::GameplayRuntimeMode::Editor,
        error)) << error;
    EXPECT_EQ(app.runtimeState.gameplayMode, rendern::GameplayRuntimeMode::Editor);
    EXPECT_FALSE(app.physicsState.levelPhysicsRuntime->IsActive());
    EXPECT_EQ(app.physicsState.levelPhysicsRuntime->GetBindingCount(), 0u);

    const mathUtils::Vec3 restoredAssetPosition =
        level.nodes[static_cast<std::size_t>(sphereNodeIndex)].transform.position;
    const mathUtils::Vec3 restoredRuntimePosition =
        app.runtimeState.levelInstance->GetNodeWorldPosition(sphereNodeIndex);
    EXPECT_TRUE(ArePositionsNear(
        restoredAssetPosition, initialPosition, RuntimeSyncTolerance));
    EXPECT_TRUE(ArePositionsNear(
        restoredRuntimePosition, initialPosition, RuntimeSyncTolerance));
}