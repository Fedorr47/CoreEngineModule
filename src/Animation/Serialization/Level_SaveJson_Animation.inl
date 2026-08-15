[[nodiscard]] const char* AnimationParameterTypeToJsonString_(AnimationParameterType type) noexcept
{
	switch (type)
	{
	case AnimationParameterType::Bool: return "bool";
	case AnimationParameterType::Int: return "int";
	case AnimationParameterType::Float: return "float";
	case AnimationParameterType::Trigger: return "trigger";
	default: return "bool";
	}
}

[[nodiscard]] const char* AnimationConditionOpToJsonString_(AnimationConditionOp op) noexcept
{
	switch (op)
	{
	case AnimationConditionOp::IfTrue: return "true";
	case AnimationConditionOp::IfFalse: return "false";
	case AnimationConditionOp::Greater: return ">";
	case AnimationConditionOp::GreaterEqual: return ">=";
	case AnimationConditionOp::Less: return "<";
	case AnimationConditionOp::LessEqual: return "<=";
	case AnimationConditionOp::Equal: return "==";
	case AnimationConditionOp::NotEqual: return "!=";
	case AnimationConditionOp::Triggered: return "triggered";
	default: return "true";
	}
}

void WriteAnimationParameterLiteral_(std::ostringstream& ss, const AnimationParameterValue& value)
{
	switch (value.type)
	{
	case AnimationParameterType::Bool:
	case AnimationParameterType::Trigger:
		WriteJsonBool(ss, value.type == AnimationParameterType::Trigger ? value.triggerValue : value.boolValue);
		break;
	case AnimationParameterType::Int:
		ss << value.intValue;
		break;
	case AnimationParameterType::Float:
		ss << value.floatValue;
		break;
	}
}

void WriteAnimationStringArray_(std::ostringstream& stream, const std::vector<std::string>& values)
{
	stream << "[";
	for (std::size_t index = 0; index < values.size(); ++index)
	{
		if (index != 0) stream << ", ";
		WriteJsonEscaped(stream, values[index]);
	}
	stream << "]";
}

void WriteAnimationStateSelector_(std::ostringstream& stream, const AnimationStateSelector& selector)
{
	stream << "{";
	bool first = true;
	const auto writeField = [&](std::string_view name, const std::vector<std::string>& values)
	{
		if (values.empty()) return;
		if (!first) stream << ", ";
		first = false;
		WriteJsonEscaped(stream, name);
		stream << ": ";
		WriteAnimationStringArray_(stream, values);
	};
	writeField("states", selector.states);
	writeField("allTags", selector.allTags);
	writeField("anyTags", selector.anyTags);
	writeField("noneTags", selector.noneTags);
	stream << "}";
}

void WriteAnimationConditions_(std::ostringstream& stream, const std::vector<AnimationConditionDesc>& conditions)
{
	stream << "[";
	for (std::size_t index = 0; index < conditions.size(); ++index)
	{
		if (index != 0) stream << ", ";
		const AnimationConditionDesc& condition = conditions[index];
		stream << "{\"parameter\": ";
		WriteJsonEscaped(stream, condition.parameter);
		stream << ", \"op\": ";
		WriteJsonEscaped(stream, AnimationConditionOpToJsonString_(condition.op));
		if (condition.op != AnimationConditionOp::IfTrue && condition.op != AnimationConditionOp::IfFalse && condition.op != AnimationConditionOp::Triggered)
		{
			stream << ", \"value\": ";
			WriteAnimationParameterLiteral_(stream, condition.value);
		}
		stream << "}";
	}
	stream << "]";
}

void WriteAnimationTransition_(std::ostringstream& stream, const AnimationTransitionDesc& transition)
{
	stream << "{\"from\": ";
	WriteJsonEscaped(stream, transition.fromState);
	stream << ", \"to\": ";
	WriteJsonEscaped(stream, transition.toState);
	if (transition.hasExitTime) stream << ", \"exitTime\": " << transition.exitTimeNormalized;
	if (std::fabs(transition.blendDurationSeconds - 0.15f) > 1e-6f) stream << ", \"blendDuration\": " << transition.blendDurationSeconds;
	if (transition.priority != 0) stream << ", \"priority\": " << transition.priority;
	if (!transition.conditions.empty())
	{
		stream << ", \"conditions\": ";
		WriteAnimationConditions_(stream, transition.conditions);
	}
	stream << "}";
}

void WriteAnimationTransitionRule_(std::ostringstream& stream, const AnimationTransitionRuleDesc& rule)
{
	stream << "{\"id\": ";
	WriteJsonEscaped(stream, rule.id);
	stream << ", \"from\": ";
	WriteAnimationStateSelector_(stream, rule.from);
	stream << ", \"to\": ";
	WriteAnimationStateSelector_(stream, rule.to);
	if (!rule.conditions.empty())
	{
		stream << ", \"conditions\": ";
		WriteAnimationConditions_(stream, rule.conditions);
	}
	if (rule.hasExitTime) stream << ", \"exitTime\": " << rule.exitTimeNormalized;
	if (std::fabs(rule.blendDurationSeconds - 0.15f) > 1e-6f) stream << ", \"blendDuration\": " << rule.blendDurationSeconds;
	if (rule.priority != 0) stream << ", \"priority\": " << rule.priority;
	stream << "}";
}

