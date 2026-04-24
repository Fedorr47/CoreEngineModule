#include <gtest/gtest.h>

#include "MathTestHelper.h"

using namespace MathTestHelper;

namespace
{
    constexpr float kEps = 1e-5f;

    geometry::MovingSphereCcdInput MakeDefaultInput()
    {
        geometry::MovingSphereCcdInput input{};
        input.centerA = Vec3(-3.0f, 0.0f, 0.0f);
        input.centerB = Vec3(0.0f, 0.0f, 0.0f);
        input.velocityA = Vec3(180.0f, 0.0f, 0.0f);
        input.velocityB = Vec3(0.0f, 0.0f, 0.0f);
        input.deltaTime = 1.0f / 60.0f;
        input.radiusA = 0.5f;
        input.radiusB = 0.5f;
        return input;
    }
}

TEST(SphereCcd, StaticTargetDirectHit)
{
    const geometry::MovingSphereCcdResult hit = geometry::SolveMovingSphereSphereCcd(MakeDefaultInput());
    EXPECT_TRUE(hit.hit);
    EXPECT_FALSE(hit.startsOverlapped);
    EXPECT_GE(hit.time01, 0.0f);
    EXPECT_LE(hit.time01, 1.0f);
}

TEST(SphereCcd, ZeroRelativeMotionNoOverlap)
{
    geometry::MovingSphereCcdInput input{};
    input.centerA = Vec3(0.0f, 0.0f, 0.0f);
    input.centerB = Vec3(3.0f, 0.0f, 0.0f);
    input.velocityA = Vec3(10.0f, 0.0f, 0.0f);
    input.velocityB = Vec3(10.0f, 0.0f, 0.0f);
    input.deltaTime = 1.0f / 60.0f;
    input.radiusA = 0.5f;
    input.radiusB = 0.5f;

    const geometry::MovingSphereCcdResult hit = geometry::SolveMovingSphereSphereCcd(input);
    EXPECT_FALSE(hit.hit);
}

TEST(SphereCcd, AlreadyOverlappedAtFrameStart)
{
    geometry::MovingSphereCcdInput input{};
    input.centerA = Vec3(0.2f, 0.0f, 0.0f);
    input.centerB = Vec3(0.0f, 0.0f, 0.0f);
    input.velocityA = Vec3(0.0f, 0.0f, 0.0f);
    input.velocityB = Vec3(0.0f, 0.0f, 0.0f);
    input.deltaTime = 1.0f / 60.0f;
    input.radiusA = 0.5f;
    input.radiusB = 0.5f;

    const geometry::MovingSphereCcdResult hit = geometry::SolveMovingSphereSphereCcd(input);
    EXPECT_TRUE(hit.hit);
    EXPECT_TRUE(hit.startsOverlapped);
    EXPECT_NEAR(hit.time01, 0.0f, kEps);
}

TEST(SphereCcd, ParallelMovementNoHit)
{
    geometry::MovingSphereCcdInput input{};
    input.centerA = Vec3(-1.0f, 0.0f, 0.0f);
    input.centerB = Vec3(-1.0f, 0.0f, 1.2f);
    input.velocityA = Vec3(120.0f, 0.0f, 0.0f);
    input.velocityB = Vec3(120.0f, 0.0f, 0.0f);
    input.deltaTime = 1.0f / 60.0f;
    input.radiusA = 0.5f;
    input.radiusB = 0.5f;

    const geometry::MovingSphereCcdResult hit = geometry::SolveMovingSphereSphereCcd(input);
    EXPECT_FALSE(hit.hit);
}

TEST(SphereCcd, CrossingPathsWrongTimingNoHit)
{
    geometry::MovingSphereCcdInput input{};
    input.centerA = Vec3(-4.0f, 0.0f, 0.0f);
    input.centerB = Vec3(0.0f, 0.0f, -4.0f);
    input.velocityA = Vec3(240.0f, 0.0f, 0.0f);
    input.velocityB = Vec3(0.0f, 0.0f, 30.0f);
    input.deltaTime = 1.0f / 60.0f;
    input.radiusA = 0.5f;
    input.radiusB = 0.5f;

    const geometry::MovingSphereCcdResult hit = geometry::SolveMovingSphereSphereCcd(input);
    EXPECT_FALSE(hit.hit);
}

TEST(SphereCcd, TangentHit)
{
    geometry::MovingSphereCcdInput input{};
    input.centerA = Vec3(-3.0f, 0.0f, 0.0f);
    input.centerB = Vec3(0.0f, 1.0f, 0.0f);
    input.velocityA = Vec3(180.0f, 0.0f, 0.0f);
    input.velocityB = Vec3(0.0f, 0.0f, 0.0f);
    input.deltaTime = 1.0f / 60.0f;
    input.radiusA = 0.5f;
    input.radiusB = 0.5f;

    const geometry::MovingSphereCcdResult hit = geometry::SolveMovingSphereSphereCcd(input);
    EXPECT_TRUE(hit.hit);
    EXPECT_TRUE(hit.tangent);
    EXPECT_NEAR(hit.discriminant, 0.0f, 1e-3f);
    EXPECT_GE(hit.time01, 0.0f);
    EXPECT_LE(hit.time01, 1.0f);
}
