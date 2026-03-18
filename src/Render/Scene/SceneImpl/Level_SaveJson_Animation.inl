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

