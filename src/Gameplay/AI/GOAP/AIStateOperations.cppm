module;

#include <compare>
#include <cstdint>
#include <limits>
#include <span>

export module core:ai_state_operations;

import :ai_decision_contracts;

export namespace rendern
{
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
