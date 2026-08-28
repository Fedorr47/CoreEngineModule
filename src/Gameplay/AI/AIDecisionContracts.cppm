module;

#include <cstdint>
#include <span>
#include <vector>

export module core:ai_decision_contracts;

export import :ai_agent_world_state;

import :ai_action_contracts;

export namespace rendern
{
	namespace details
	{
		struct AIGoalIdTag
		{
		};
	}
	

	using AIGoalId = AIId<details::AIGoalIdTag>;
	
	struct AIFactCondition
	{
		AIWorldFactId factId{};
		bool bExpectedValue{ true };
		
		friend constexpr bool operator==(const AIFactCondition&, const AIFactCondition&) noexcept = default;
	};
	
	struct AIFactEffect
	{
		AIWorldFactId factId{};
		bool bValue{ true };
		
		friend constexpr bool operator==(const AIFactEffect&, const AIFactEffect&) noexcept = default;
	};
	
	enum class AINumericConditionOperator : std::uint8_t
	{
		Equal,
		NotEqual,
		Less,
		LessOrEqual,
		Greater,
		GreaterOrEqual
	};

	struct AINumericCondition
	{
		AIWorldIntegerFactId factId{};
		AINumericConditionOperator comparison{ AINumericConditionOperator::Equal };
		std::int32_t value{};
	};

	enum class AINumericEffectOperation : std::uint8_t
	{
		Set,
		Add
	};

	struct AINumericEffect
	{
		AIWorldIntegerFactId factId{};
		AINumericEffectOperation operation{ AINumericEffectOperation::Set };
		std::int32_t value{};
	};
	
	struct AIGoalDefinition
	{
		AIGoalId goalId{};
		std::vector<AIFactCondition> desiredFacts{};
	};
	
	struct AIActionDefinition
	{
		AIActionId actionId{};
		std::vector<AIFactCondition> preconditions{};
		std::vector<AIFactEffect> effects{};
		AIActionContextId contextId{};
		float baseCost{ 1.0f };
		std::vector<AINumericCondition> numericPreconditions{};
		std::vector<AINumericEffect> numericEffects{};
	};
		
	struct AIPlanStep
	{
		AIActionId actionId{};
		AIActionContextId contextId{};
		
		friend constexpr bool operator==(const AIPlanStep&, const AIPlanStep&) noexcept = default;
	};
	
	struct AIPlan
	{
		AIGoalId goalId{};
		std::vector<AIPlanStep> steps{};
	};
	
	[[nodiscard]] bool EvaluateFactCondition(
		const AIAgentWorldState& worldState,
		const AIFactCondition& condition) noexcept
	{
		return worldState.IsFactSet(condition.factId) == condition.bExpectedValue;
	}
	
	[[nodiscard]] bool AreFactConditionsSatisfied(
		const AIAgentWorldState& worldState,
		std::span<const AIFactCondition> conditions) noexcept
	{
		for (const AIFactCondition& condition : conditions)
		{
			if (!EvaluateFactCondition(worldState, condition))
			{
				return false;
			}
		}
		
		return true;
	}
	
	void ApplyFactEffects(
		AIAgentWorldState& worldState,
		std::span<const AIFactEffect> effects)
	{
		for (const AIFactEffect& effect : effects)
		{
			worldState.SetFact(effect.factId, effect.bValue);
		}
	}
	
	[[nodiscard]] constexpr bool EvaluateNumericCondition(
	const std::int32_t actual,
	const AINumericConditionOperator op,
	const std::int32_t expected) noexcept
	{
		const auto ordering = actual <=> expected;

		switch (op)
		{
		case AINumericConditionOperator::Equal:
			return ordering == 0;

		case AINumericConditionOperator::NotEqual:
			return ordering != 0;

		case AINumericConditionOperator::Less:
			return ordering < 0;

		case AINumericConditionOperator::LessOrEqual:
			return ordering <= 0;

		case AINumericConditionOperator::Greater:
			return ordering > 0;

		case AINumericConditionOperator::GreaterOrEqual:
			return ordering >= 0;

		default:
			return false;
		}
	}
	
	[[nodiscard]] bool AreNumericConditionsSatisfied(
	const AIAgentWorldState& worldState,
	std::span<const AINumericCondition> conditions) noexcept
	{
		for (const AINumericCondition& condition : conditions)
		{
			const std::int32_t actual =
				worldState.GetIntegerFact(condition.factId);

			if (!EvaluateNumericCondition(
					actual,
					condition.comparison,
					condition.value))
			{
				return false;
			}
		}
		
		return true;
	}

	[[nodiscard]] bool ApplyNumericEffects(
		AIAgentWorldState& worldState,
		std::span<const AINumericEffect> effects) noexcept
	{
		AIAgentWorldState result = worldState;
		for (const AINumericEffect& effect : effects)
		{
			switch (effect.operation)
			{
			case AINumericEffectOperation::Set:
				result.SetIntegerFact(effect.factId, effect.value);
				break;
			case AINumericEffectOperation::Add:
				{
					const std::int32_t current = result.GetIntegerFact(effect.factId);
					if ((effect.value > 0 && current > std::numeric_limits<std::int32_t>::max() - effect.value)
						|| (effect.value < 0 && current < std::numeric_limits<std::int32_t>::min() - effect.value))
					{
						return false;
					}
					result.SetIntegerFact(effect.factId, current + effect.value);
					break;
				}
			default:
				return false;
			}
		}

		worldState = result;
		return true;
	}
}