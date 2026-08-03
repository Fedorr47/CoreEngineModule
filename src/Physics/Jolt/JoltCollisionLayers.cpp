#include "Physics/Jolt/JoltCollisionLayers.h"

#include <array>

namespace
{
    using namespace physics::jolt;
    
    constexpr std::array ObjectToBroadPhase
    {
        BroadPhaseLayers::Static,  // StaticWorld
        BroadPhaseLayers::Moving,  // DynamicWorld
        BroadPhaseLayers::Moving,  // Character
        BroadPhaseLayers::Trigger  // Trigger
    };
    
    constexpr std::array<std::array<bool, ObjectLayers::Count>, ObjectLayers::Count> CollisionMatrix
    {{
        {{false, true,  true,  false}},
        {{true,  true,  true,  true }},
        {{true,  true,  false, true }},
        {{false, true,  true,  false}},
    }};
    
    consteval bool IsCollisionMatrixSymmetric()
    {
        for (JPH::ObjectLayer first = 0; first < ObjectLayers::Count; ++first)
        {
            for (JPH::ObjectLayer second = 0; second < ObjectLayers::Count; ++second)
            {
                if (CollisionMatrix[first][second] != CollisionMatrix[second][first])
                {
                    return false;
                }
            }
        }
        return true;
    }
    
    static_assert(IsCollisionMatrixSymmetric());
}

namespace physics::jolt
{
    JPH::uint JoltBroadPhaseLayerInterface::GetNumBroadPhaseLayers() const { return BroadPhaseLayers::Count; }
    
    JPH::BroadPhaseLayer JoltBroadPhaseLayerInterface::GetBroadPhaseLayer(JPH::ObjectLayer layer) const
    {
        const bool isValidLayer = layer < ObjectLayers::Count;
        JPH_ASSERT(isValidLayer);
        if (!isValidLayer)
        {
            return JPH::cBroadPhaseLayerInvalid;
        }
        
        return ObjectToBroadPhase[layer];
    }
    
    bool JoltObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer first, JPH::ObjectLayer second) const
    {
        JPH_ASSERT(first < ObjectLayers::Count && second < ObjectLayers::Count);
        return first < ObjectLayers::Count 
                && second < ObjectLayers::Count 
                && CollisionMatrix[first][second];
    }
    
    bool JoltObjectVsBroadPhaseLayerFilter::ShouldCollide(
       JPH::ObjectLayer objectLayer, JPH::BroadPhaseLayer broadPhaseLayer) const
    {
        const JPH::uint broadPhaseValue = broadPhaseLayer.GetValue();
        JPH_ASSERT(objectLayer < ObjectLayers::Count && broadPhaseValue < BroadPhaseLayers::Count);
        
        if (objectLayer >= ObjectLayers::Count || broadPhaseValue >= BroadPhaseLayers::Count)
        {
            return false;
        }
        
        for (JPH::ObjectLayer candidate = 0; candidate < ObjectLayers::Count; ++candidate)
        {
            if (ObjectToBroadPhase[candidate] == broadPhaseLayer && CollisionMatrix[objectLayer][candidate])
            {
                return true;
            }
        }
        return false;
    };
}