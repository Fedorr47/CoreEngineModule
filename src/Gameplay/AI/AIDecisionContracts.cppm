module;

#include <cstdint>
#include <limits>
#include <span>
#include <vector>

export module core:ai_decision_contracts;

export import :ai_agent_world_state;

export namespace rendern
{
	namespace details
	{
		struct AIGoalIdTag
		{
		};

		struct AIActionIdTag
		{
		};
	}
	
	template <typename TTag>
	struct AIId
	{
		using ValueType = std::uint16_t;

		static constexpr ValueType InvalidValue =
			std::numeric_limits<ValueType>::max();

		ValueType value{ InvalidValue };

		constexpr AIId() noexcept = default;

		explicit constexpr AIId(const ValueType inValue) noexcept
			: value{ inValue }
		{
		}

		[[nodiscard]] constexpr bool IsValid() const noexcept
		{
			return value != InvalidValue;
		}

		friend constexpr bool operator==(
			const AIId&,
			const AIId&) noexcept = default;
	};

	using AIGoalId = AIId<details::AIGoalIdTag>;
	using AIActionId = AIId<details::AIActionIdTag>;
	
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
		float baseCost{ 1.0f };
	};
	
	enum class AIActionExecutionStatus : std::uint8_t
	{
		NotStarted,
		Running,
		Succeeded,
		Failed,
		Cancelled
	};
	
	struct AIPlanStep
	{
		AIActionId actionId{};
		
		friend constexpr bool operator==(const AIPlanStep&, const AIPlanStep&) noexcept = default;
	};
	
	struct AIPlan
	{
		AIGoalId goalId{};
		std::vector<AIPlanStep> steps{};
	};
	
	[[nodiscard]] bool AreFactConditionsSatisfied(
		const AIAgentWorldState& worldState,
		std::span<const AIFactCondition> conditions) noexcept
	{
		for (const AIFactCondition& condition : conditions)
		{
			if (worldState.IsFactSet(condition.factId) != condition.bExpectedValue)
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
}