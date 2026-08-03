#include <gtest/gtest.h>

#include "Physics/Jolt/JoltCollisionLayers.h"

#include <array>

namespace
{
    using namespace physics::jolt;
    constexpr std::array Objects{
		ObjectLayers::StaticWorld, 
		ObjectLayers::DynamicWorld,
        ObjectLayers::Character, 
		ObjectLayers::Trigger};

    constexpr bool Expected[4][4] = {
        {false, true, true, false}, 
		{true, true, true, true},
        {true, true, false, true}, 
		{false, true, true, false}};
}

TEST(JoltCollisionLayersTests, MapsEveryObjectLayerToExpectedBroadPhaseLayer)
{
    JoltBroadPhaseLayerInterface interface;
    EXPECT_EQ(interface.GetNumBroadPhaseLayers(), BroadPhaseLayers::Count);
    EXPECT_EQ(interface.GetBroadPhaseLayer(ObjectLayers::StaticWorld), BroadPhaseLayers::Static);
    EXPECT_EQ(interface.GetBroadPhaseLayer(ObjectLayers::DynamicWorld), BroadPhaseLayers::Moving);
    EXPECT_EQ(interface.GetBroadPhaseLayer(ObjectLayers::Character), BroadPhaseLayers::Moving);
    EXPECT_EQ(interface.GetBroadPhaseLayer(ObjectLayers::Trigger), BroadPhaseLayers::Trigger);
}

TEST(JoltCollisionLayersTests, BroadPhaseFilterMatchesExpectedRules)
{
    JoltObjectVsBroadPhaseLayerFilter filter;
    constexpr bool expected[4][3] = {
        {false, true, false}, 
        {true, true, true},
        {true, true, true}, 
        {false, true, false}};
    
    constexpr std::array broadPhases{
        BroadPhaseLayers::Static, 
        BroadPhaseLayers::Moving,
        BroadPhaseLayers::Trigger};
    
    for (std::size_t object = 0; object < Objects.size(); ++object)
    {
        for (std::size_t broadPhase = 0; broadPhase < broadPhases.size(); ++broadPhase)
        {
            EXPECT_EQ(filter.ShouldCollide(Objects[object], broadPhases[broadPhase]), expected[object][broadPhase]);
        }
    }
}

TEST(JoltCollisionLayersTests, CollisionMatrixMatchesEveryDeclaredPair)
{
    JoltObjectLayerPairFilter filter;
    for (std::size_t first = 0; first < Objects.size(); ++first)
    {
        for (std::size_t second = 0; second < Objects.size(); ++second)
        {
            EXPECT_EQ(filter.ShouldCollide(Objects[first], Objects[second]), Expected[first][second]);
        }
    }
}

TEST(JoltCollisionLayersTests, CollisionMatrixIsSymmetric)
{
    JoltObjectLayerPairFilter filter;
    // Symmetry prevents collision behavior from depending on body insertion/query order.
    for (const auto first : Objects)
    {
        for (const auto second : Objects)
        {
            EXPECT_EQ(filter.ShouldCollide(first, second), filter.ShouldCollide(second, first));
        }
    }
}