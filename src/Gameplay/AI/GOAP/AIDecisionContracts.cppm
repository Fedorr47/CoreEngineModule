module;

#include <cstdint>
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
		// Preconditions answer whether an action may start. Continuation conditions
        // are opt-in requirements evaluated only while its runtime is active.
        std::vector<AIFactCondition> continuationConditions{};
        std::vector<AINumericCondition> numericContinuationConditions{};
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
	
}
