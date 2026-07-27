module;

#include <cmath>
#include <optional>

export module core:gameplay_interaction_point;

import :gameplay;
import :math_utils;

export namespace rendern
{
	struct GameplayResolvedInteractionPoint
	{
		mathUtils::Vec3 worldPosition;
		float worldFacingYawDegrees{0.0f};
	};
	
	[[nodiscard]] std::optional<GameplayResolvedInteractionPoint> ResolveGameplayInteractionPoint(
		const GameplayWorld& world,
		const EntityHandle objectEntity) noexcept
	{
		if (!world.IsEntityValid(objectEntity))
		{
			return std::nullopt;
		}
		
		const GameplayTransformComponent* transform = world.TryGetTransform(objectEntity);
		const GameplayInteractionPointComponent* interactionPoint = world.TryGetInteractionPoint(objectEntity);
		if (transform == nullptr || interactionPoint == nullptr)
		{
			return std::nullopt;
		}
		
		const auto& position = transform->position;
		const auto& rotation = transform->rotationDegrees;
		const auto& local = interactionPoint->localPosition;
		
		const bool bHasFiniteInput = 
			std::isfinite(position.x) && 
			std::isfinite(position.y) &&
			std::isfinite(position.z) && 
		   	std::isfinite(rotation.y) && 
		   	std::isfinite(local.x) &&
			std::isfinite(local.y) && 
			std::isfinite(local.z) &&
			std::isfinite(interactionPoint->localFacingYawDegrees);
		if (!bHasFiniteInput)
		{
			return std::nullopt;
		}
		
		const float yawRadians = mathUtils::DegToRad(rotation.y);
		const float sine = std::sin(yawRadians);
		const float cosine = std::cos(yawRadians);
		GameplayResolvedInteractionPoint resolved{
			.worldPosition = {
				position.x + (local.x * cosine) + (local.z * sine),
				position.y + local.y,
				position.z - (local.x * sine) + (local.z * cosine) },
			.worldFacingYawDegrees = rotation.y + interactionPoint->localFacingYawDegrees
		};
		
		const bool bHasFiniteOutput = 
			std::isfinite(resolved.worldPosition.x) &&
			std::isfinite(resolved.worldPosition.y) && 
			std::isfinite(resolved.worldPosition.z) &&
			std::isfinite(resolved.worldFacingYawDegrees);
		
		return bHasFiniteOutput ? std::optional{ resolved } : std::nullopt;
	}
}