#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

namespace physics::jolt
{
	namespace ObjectLayers
	{
		constexpr JPH::ObjectLayer StaticWorld{0};
		constexpr JPH::ObjectLayer DynamicWorld{1};
		constexpr JPH::ObjectLayer Character{2};
		constexpr JPH::ObjectLayer Trigger{3};
		constexpr JPH::ObjectLayer Count{4};
	}

	namespace BroadPhaseLayers
	{
		constexpr JPH::BroadPhaseLayer Static{0};
		constexpr JPH::BroadPhaseLayer Moving{1};
		constexpr JPH::BroadPhaseLayer Trigger{2};
		constexpr JPH::uint Count{3};
	}

	class JoltBroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface
	{
	public:
		[[nodiscard]] JPH::uint GetNumBroadPhaseLayers() const override;
		[[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override;
	};
	
	class JoltObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
    {
    public:
		[[nodiscard]] bool ShouldCollide(JPH::ObjectLayer first, JPH::ObjectLayer second) const override;
    };
	
	class JoltObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
		[[nodiscard]] bool ShouldCollide(JPH::ObjectLayer objectLayer, JPH::BroadPhaseLayer broadPhaseLayer) const override;
    };
}