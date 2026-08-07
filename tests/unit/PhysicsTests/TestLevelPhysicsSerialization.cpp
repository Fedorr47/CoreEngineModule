#include <gtest/gtest.h>

import core;

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <stdexcept>

#include "TestSupport/TestTempPath.h"

namespace
{
    class LevelPhysicsBodyJson : public testing::Test
    {
    protected:
        void WriteLevel(const std::string_view fileName, const std::string_view nodeJson)
        {
            std::filesystem::create_directories(tempPath.Path());
            std::ofstream output(Path(fileName), std::ios::binary | std::ios::trunc);
            ASSERT_TRUE(output.is_open());
            output << "{\"name\":\"PhysicsJsonTest\",\"nodes\":[" << nodeJson << "]}";
        }

        [[nodiscard]] std::filesystem::path Path(const std::string_view fileName) const
        {
            return tempPath.Path() / fileName;
        }

        test::ScopedTempPath tempPath{ test::MakeUniqueTempPath("core_level_physics") };
    };
}

TEST_F(LevelPhysicsBodyJson, ParsesStaticBoxAndDynamicSphere)
{
    WriteLevel("valid.level.json", R"json(
        {"name":"Floor","physicsBody":{"motionType":"static","shape":{"type":"box","halfExtents":[10,0.5,10]}}},
        {"name":"Sphere","physicsBody":{"motionType":"dynamic","shape":{"type":"sphere","radius":0.5}}}
    )json");
    const rendern::LevelAsset level = rendern::LoadLevelAssetFromJson(Path("valid.level.json").string());
    ASSERT_EQ(level.nodes.size(), 2u);
    ASSERT_TRUE(level.nodes[0].physicsBody.has_value());
    ASSERT_TRUE(level.nodes[1].physicsBody.has_value());
    EXPECT_EQ(level.nodes[0].physicsBody->motionType, physics::PhysicsMotionType::Static);
    EXPECT_EQ(level.nodes[1].physicsBody->motionType, physics::PhysicsMotionType::Dynamic);
    EXPECT_EQ(std::get<physics::BoxShapeDescriptor>(level.nodes[0].physicsBody->shape).halfExtents.y, 0.5f);
    EXPECT_EQ(std::get<physics::SphereShapeDescriptor>(level.nodes[1].physicsBody->shape).radius, 0.5f);
}

TEST_F(LevelPhysicsBodyJson, RejectsMalformedPhysicsDefinitions)
{
    struct InvalidCase
    {
        std::string_view fileName;
        std::string_view node;
        std::string_view error;
    };
    constexpr InvalidCase Cases[]{
        { "kinematic.level.json", R"json({"name":"Body","physicsBody":{"motionType":"kinematic","shape":{"type":"sphere","radius":1}}})json", "kinematic" },
        { "unknown.level.json", R"json({"name":"Body","physicsBody":{"motionType":"static","shape":{"type":"mesh"}}})json", "unsupported shape" },
        { "invalid.level.json", R"json({"name":"Body","physicsBody":{"motionType":"dynamic","shape":{"type":"sphere","radius":0}}})json", "invalid sphere radius" },
        { "missing_shape.level.json", R"json({"name":"Body","physicsBody":{"motionType":"dynamic"}})json", "missing its shape" },
        { "missing_motion.level.json", R"json({"name":"Body","physicsBody":{"shape":{"type":"sphere","radius":1}}})json", "missing motion type" },
        { "invalid_box.level.json", R"json({"name":"Body","physicsBody":{"motionType":"static","shape":{"type":"box","halfExtents":[1,0,-1]}}})json", "invalid box half extents" },
        { "invalid_capsule.level.json", R"json({"name":"Body","physicsBody":{"motionType":"dynamic","shape":{"type":"capsule","radius":1,"cylinderHeight":0}}})json", "invalid capsule dimensions" }
    };
    for (const InvalidCase& testCase : Cases)
    {
        WriteLevel(testCase.fileName, testCase.node);
        EXPECT_THROW(
            {
                try
                {
                    static_cast<void>(rendern::LoadLevelAssetFromJson(Path(testCase.fileName).string()));
                }
                catch (const std::runtime_error& error)
                {
                    EXPECT_NE(std::string(error.what()).find(testCase.error), std::string::npos);
                    throw;
                }
            },
            std::runtime_error);
    }
}

TEST_F(LevelPhysicsBodyJson, SaveAndReloadPreservesPhysicsDescriptors)
{
    rendern::LevelAsset source{};
    source.name = "RoundTrip";
    source.nodes.push_back({
        .name = "Capsule",
        .physicsBody = rendern::LevelPhysicsBodyDef{
            .shape = physics::CapsuleShapeDescriptor{ .radius = 0.25f, .cylinderHeight = 1.5f },
            .motionType = physics::PhysicsMotionType::Dynamic
        }
    });
    source.nodes.push_back({ .name = "NoPhysics" });

    const std::filesystem::path path = Path("round_trip.level.json");
    rendern::SaveLevelAssetToJson(path.string(), source);
    const rendern::LevelAsset reloaded = rendern::LoadLevelAssetFromJson(path.string());
    ASSERT_EQ(reloaded.nodes.size(), 2u);
    ASSERT_TRUE(reloaded.nodes[0].physicsBody.has_value());
    EXPECT_FALSE(reloaded.nodes[1].physicsBody.has_value());
    EXPECT_EQ(reloaded.nodes[0].physicsBody->motionType, physics::PhysicsMotionType::Dynamic);
    const auto& capsule = std::get<physics::CapsuleShapeDescriptor>(reloaded.nodes[0].physicsBody->shape);
    EXPECT_FLOAT_EQ(capsule.radius, 0.25f);
    EXPECT_FLOAT_EQ(capsule.cylinderHeight, 1.5f);
}

TEST_F(LevelPhysicsBodyJson, SaveRejectsKinematicBody)
{
    rendern::LevelAsset source{};
    source.nodes.push_back({
        .name = "Kinematic",
        .physicsBody = rendern::LevelPhysicsBodyDef{
            .shape = physics::SphereShapeDescriptor{ .radius = 1.0f },
            .motionType = physics::PhysicsMotionType::Kinematic
        }
    });
    EXPECT_THROW(
        rendern::SaveLevelAssetToJson(Path("kinematic_save.level.json").string(), source),
        std::runtime_error);
}