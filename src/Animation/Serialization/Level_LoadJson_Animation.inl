[[nodiscard]] AnimationParameterType ParseAnimationControllerParameterType_(std::string_view type)
{
	if (type == "bool") return AnimationParameterType::Bool;
	if (type == "int") return AnimationParameterType::Int;
	if (type == "float") return AnimationParameterType::Float;
	if (type == "trigger") return AnimationParameterType::Trigger;
	throw std::runtime_error("Level JSON: animation controller parameter type must be bool|int|float|trigger");
}

[[nodiscard]] AnimationConditionOp ParseAnimationConditionOp_(std::string_view op)
{
	if (op == "true") return AnimationConditionOp::IfTrue;
	if (op == "false") return AnimationConditionOp::IfFalse;
	if (op == ">") return AnimationConditionOp::Greater;
	if (op == ">=") return AnimationConditionOp::GreaterEqual;
	if (op == "<") return AnimationConditionOp::Less;
	if (op == "<=") return AnimationConditionOp::LessEqual;
	if (op == "==" || op == "=") return AnimationConditionOp::Equal;
	if (op == "!=") return AnimationConditionOp::NotEqual;
	if (op == "triggered") return AnimationConditionOp::Triggered;
	throw std::runtime_error("Level JSON: animation controller condition op is invalid");
}

[[nodiscard]] AnimationProfileAsset ParseAnimationProfileAssetObject_(
	const JsonObject& object,
	const std::string& id,
	const std::string& contextPrefix)
{
	if (id.empty())
	{
		throw std::runtime_error(contextPrefix + ": profile id must not be empty");
	}
	AnimationProfileAsset profile;
	profile.id = id;
	const JsonValue* motionsValue = TryGet(object, "motions");
	if (motionsValue == nullptr || !motionsValue->IsObject())
	{
		throw std::runtime_error(contextPrefix + ".motions must be object");
	}
	// JsonObject is already normalized by JsonParser, so duplicate keys cannot be observed here.
	for (const auto& [motion, bindingValue] : motionsValue->AsObject())
	{
		if (motion.empty())
		{
			throw std::runtime_error(contextPrefix + ".motions contains an empty motion id");
		}
		if (!bindingValue.IsObject())
		{
			throw std::runtime_error(contextPrefix + ".motions." + motion + " must be object");
		}
		const JsonObject& bindingObject = bindingValue.AsObject();
		AnimationClipRef binding;
		binding.sourceAssetId = GetStringOpt(bindingObject, "sourceAssetId");
		binding.clipName = GetStringOpt(bindingObject, "clip");
		if (binding.sourceAssetId.empty())
		{
			throw std::runtime_error(contextPrefix + ".motions." + motion + ".sourceAssetId is required");
		}
		profile.motions.emplace(motion, std::move(binding));
	}
	return profile;
}

[[nodiscard]] AnimationProfileAsset LoadExternalAnimationProfileAssetFromJson_(
	std::string_view assetPath,
	const std::string& id)
{
	const std::filesystem::path absPath = corefs::ResolveAsset(std::filesystem::path(std::string(assetPath)));
	const std::string text = FILE_UTILS::ReadAllText(absPath);
	JsonParser parser(text);
	JsonValue root = parser.Parse();
	if (!root.IsObject())
	{
		throw std::runtime_error("Animation profile JSON: root must be object");
	}
	return ParseAnimationProfileAssetObject_(root.AsObject(), id, "Animation profile JSON: " + std::string(assetPath));
}

[[nodiscard]] std::vector<AnimationNotifyDesc> ParseAnimationNotifyArray_(
	const JsonArray& notifiesA,
	const std::string& contextPrefix)
{
	std::vector<AnimationNotifyDesc> notifies;
	for (const JsonValue& notifyV : notifiesA)
	{
		const JsonObject& notifyO = notifyV.AsObject();
		AnimationNotifyDesc notifyDesc;
		notifyDesc.id = GetStringOpt(notifyO, "id");
		if (notifyDesc.id.empty())
		{
			throw std::runtime_error(contextPrefix + ".id is required");
		}
		notifyDesc.timeNormalized = std::clamp(GetFloatOpt(notifyO, "time", 0.0f), 0.0f, 1.0f);
		notifyDesc.fireOnEnter = GetBoolOpt(notifyO, "fireOnEnter", false);
		notifies.push_back(std::move(notifyDesc));
	}
	return notifies;
}

inline void ParseAnimationEventBindingsInto_(
	AnimationControllerAsset& def,
	const JsonArray& bindingsA,
	const std::string& contextPrefix)
{
	for (const JsonValue& bindingV : bindingsA)
	{
		const JsonObject& bindingO = bindingV.AsObject();
		AnimationEventBindingDesc bindingDesc;
		bindingDesc.animationEventId = GetStringOpt(bindingO, "animationEvent");
		bindingDesc.gameplayEventId = GetStringOpt(bindingO, "gameplayEvent");
		if (bindingDesc.animationEventId.empty())
		{
			throw std::runtime_error(contextPrefix + ".animationEvent is required");
		}
		if (bindingDesc.gameplayEventId.empty())
		{
			throw std::runtime_error(contextPrefix + ".gameplayEvent is required");
		}
		def.eventBindings.push_back(std::move(bindingDesc));
	}
}

inline void MergeAnimationNotifyAssetIntoController_(
	AnimationControllerAsset& def,
	const JsonObject& root,
	const std::string& contextPrefix)
{
	if (auto* statesV = TryGet(root, "states"))
	{
		if (!statesV->IsObject())
		{
			throw std::runtime_error(contextPrefix + ".states must be object");
		}
		for (const auto& [stateName, notifyListV] : statesV->AsObject())
		{
			if (!notifyListV.IsArray())
			{
				throw std::runtime_error(contextPrefix + ".states." + stateName + " must be array");
			}
			const std::vector<AnimationNotifyDesc> parsed = ParseAnimationNotifyArray_(notifyListV.AsArray(), contextPrefix + ".states." + stateName + "[]");
			for (AnimationStateDesc& state : def.states)
			{
				if (state.name == stateName)
				{
					state.notifies.insert(state.notifies.end(), parsed.begin(), parsed.end());
				}
			}
		}
	}

	if (auto* clipsV = TryGet(root, "clips"))
	{
		if (!clipsV->IsObject())
		{
			throw std::runtime_error(contextPrefix + ".clips must be object");
		}
		for (const auto& [clipName, notifyListV] : clipsV->AsObject())
		{
			if (!notifyListV.IsArray())
			{
				throw std::runtime_error(contextPrefix + ".clips." + clipName + " must be array");
			}
			const std::vector<AnimationNotifyDesc> parsed = ParseAnimationNotifyArray_(notifyListV.AsArray(), contextPrefix + ".clips." + clipName + "[]");
			for (AnimationStateDesc& state : def.states)
			{
				if (state.clipName == clipName && state.blend1D.empty())
				{
					state.notifies.insert(state.notifies.end(), parsed.begin(), parsed.end());
				}
			}
		}
	}
}

inline void LoadAndMergeExternalAnimationNotifyAsset_(AnimationControllerAsset& def)
{
	if (def.notifyAssetPath.empty())
	{
		return;
	}

	const std::filesystem::path absPath = corefs::ResolveAsset(std::filesystem::path(def.notifyAssetPath));
	const std::string text = FILE_UTILS::ReadAllText(absPath);
	JsonParser parser(text);
	JsonValue root = parser.Parse();
	if (!root.IsObject())
	{
		throw std::runtime_error("Animation notify JSON: root must be object");
	}
	MergeAnimationNotifyAssetIntoController_(def, root.AsObject(), std::string("Animation notify JSON: ") + def.notifyAssetPath);
}

inline void LoadAndMergeExternalAnimationEventBindingsAsset_(AnimationControllerAsset& def)
{
	if (def.eventBindingsAssetPath.empty())
	{
		return;
	}

	const std::filesystem::path absPath = corefs::ResolveAsset(std::filesystem::path(def.eventBindingsAssetPath));
	const std::string text = FILE_UTILS::ReadAllText(absPath);
	JsonParser parser(text);
	JsonValue root = parser.Parse();
	if (!root.IsObject())
	{
		throw std::runtime_error("Animation event bindings JSON: root must be object");
	}
	const JsonValue* bindingsV = TryGet(root.AsObject(), "bindings");
	if (bindingsV == nullptr || !bindingsV->IsArray())
	{
		throw std::runtime_error(std::string("Animation event bindings JSON: ") + def.eventBindingsAssetPath + ".bindings must be array");
	}
	ParseAnimationEventBindingsInto_(def, bindingsV->AsArray(), std::string("Animation event bindings JSON: ") + def.eventBindingsAssetPath + ".bindings[]");
}

[[nodiscard]] std::vector<AnimationConditionDesc> ParseAnimationConditions_(
	const AnimationControllerAsset& controller,
	const JsonValue* conditionsValue,
	const std::string& context)
{
	std::vector<AnimationConditionDesc> conditions;
	if (conditionsValue == nullptr) return conditions;
	if (!conditionsValue->IsArray()) throw std::runtime_error(context + " must be array");
	for (const JsonValue& conditionValue : conditionsValue->AsArray())
	{
		const JsonObject& object = conditionValue.AsObject();
		AnimationConditionDesc condition;
		condition.parameter = GetStringOpt(object, "parameter");
		if (condition.parameter.empty()) throw std::runtime_error(context + "[].parameter is required");
		const AnimationParameterDesc* parameter = FindAnimationParameterDesc(controller, condition.parameter);
		if (parameter == nullptr) throw std::runtime_error(context + "[] references unknown parameter '" + condition.parameter + "'");
		condition.value.type = parameter->defaultValue.type;
		condition.op = ParseAnimationConditionOp_(GetStringOpt(object, "op", "true"));

		const bool comparisonOp = condition.op == AnimationConditionOp::Greater || condition.op == AnimationConditionOp::GreaterEqual ||
			condition.op == AnimationConditionOp::Less || condition.op == AnimationConditionOp::LessEqual ||
			condition.op == AnimationConditionOp::Equal || condition.op == AnimationConditionOp::NotEqual;

		const JsonValue* value = TryGet(object, "value");
		const bool requiresValue = comparisonOp;
		if (requiresValue && value == nullptr) throw std::runtime_error(context + "[].value is required for comparison operator");
		if (value != nullptr)
		{
			switch (condition.value.type)
			{
			case AnimationParameterType::Bool:
			case AnimationParameterType::Trigger:
				if (!value->IsBool()) throw std::runtime_error(context + "[].value must be bool");
				condition.value.boolValue = value->AsBool();
				condition.value.triggerValue = value->AsBool();
				break;
			case AnimationParameterType::Int:
				if (!value->IsNumber()) throw std::runtime_error(context + "[].value must be number");
				condition.value.intValue = static_cast<int>(value->AsNumber());
				break;
			case AnimationParameterType::Float:
				if (!value->IsNumber()) throw std::runtime_error(context + "[].value must be number");
				condition.value.floatValue = static_cast<float>(value->AsNumber());
				break;
			}
		}
		conditions.push_back(std::move(condition));
	}
	return conditions;
}

[[nodiscard]] AnimationControllerAsset ParseAnimationControllerAssetObject_(
	const JsonObject& cd,
	const std::string& id,
	const std::string& contextPrefix)
{
	AnimationControllerAsset def;
	def.id = id;
	def.defaultState = GetStringOpt(cd, "defaultState");
	def.notifyAssetPath = GetStringOpt(cd, "notifyAsset");
	def.eventBindingsAssetPath = GetStringOpt(cd, "eventBindingsAsset");

	if (auto* paramsV = TryGet(cd, "parameters"))
	{
		const JsonObject& paramsO = paramsV->AsObject();
		for (const auto& [paramName, paramV] : paramsO)
		{
			const JsonObject& pd = paramV.AsObject();
			AnimationParameterDesc paramDesc;
			paramDesc.name = paramName;
			paramDesc.defaultValue.type = ParseAnimationControllerParameterType_(GetStringOpt(pd, "type", "bool"));
			if (auto* defaultV = TryGet(pd, "default"))
			{
				switch (paramDesc.defaultValue.type)
				{
				case AnimationParameterType::Bool:
				case AnimationParameterType::Trigger:
					if (!defaultV->IsBool()) throw std::runtime_error(contextPrefix + ".parameters." + paramName + ".default must be bool");
					paramDesc.defaultValue.boolValue = defaultV->AsBool();
					paramDesc.defaultValue.triggerValue = defaultV->AsBool();
					break;
				case AnimationParameterType::Int:
					if (!defaultV->IsNumber()) throw std::runtime_error(contextPrefix + ".parameters." + paramName + ".default must be number");
					paramDesc.defaultValue.intValue = static_cast<int>(defaultV->AsNumber());
					break;
				case AnimationParameterType::Float:
					if (!defaultV->IsNumber()) throw std::runtime_error(contextPrefix + ".parameters." + paramName + ".default must be number");
					paramDesc.defaultValue.floatValue = static_cast<float>(defaultV->AsNumber());
					break;
				}
			}
			def.parameters.push_back(std::move(paramDesc));
		}
	}

	if (auto* statesV = TryGet(cd, "states"))
	{
		const JsonObject& statesO = statesV->AsObject();
		for (const auto& [stateName, stateV] : statesO)
		{
			const JsonObject& sd = stateV.AsObject();
			AnimationStateDesc stateDesc;
			stateDesc.name = stateName;
			stateDesc.motionId.value = GetStringOpt(sd, "motion");
			stateDesc.clipName = GetStringOpt(sd, "clip");
			stateDesc.clipSourceAssetId = GetStringOpt(sd, "clipSourceAssetId");
			stateDesc.looping = GetBoolOpt(sd, "loop", true);
			stateDesc.playRate = GetFloatOpt(sd, "playRate", 1.0f);
			if (auto* tagsV = TryGet(sd, "tags"))
			{
				if (!tagsV->IsArray())
				{
					throw std::runtime_error(contextPrefix + ".states." + stateName + ".tags must be array");
				}
				for (const JsonValue& tagV : tagsV->AsArray())
				{
					if (!tagV.IsString())
					{
						throw std::runtime_error(contextPrefix + ".states." + stateName + ".tags[] must be string");
					}
					stateDesc.tags.push_back(tagV.AsString());
				}
			}
			if (auto* blendV = TryGet(sd, "blend1D"))
			{
				const JsonObject& bd = blendV->AsObject();
				stateDesc.blendParameter = GetStringOpt(bd, "parameter");
				if (stateDesc.blendParameter.empty())
				{
					throw std::runtime_error(contextPrefix + ".states." + stateName + ".blend1D.parameter is required");
				}
				if (const AnimationParameterDesc* paramDesc = FindAnimationParameterDesc(def, stateDesc.blendParameter))
				{
					if (paramDesc->defaultValue.type == AnimationParameterType::Trigger)
					{
						throw std::runtime_error(contextPrefix + ".states." + stateName + ".blend1D.parameter must not be trigger");
					}
				}
				else
				{
					throw std::runtime_error(contextPrefix + ".states." + stateName + ".blend1D.parameter references unknown parameter");
				}
				auto* pointsV = TryGet(bd, "points");
				if (pointsV == nullptr || !pointsV->IsArray())
				{
					throw std::runtime_error(contextPrefix + ".states." + stateName + ".blend1D.points must be array");
				}
				for (const JsonValue& pointV : pointsV->AsArray())
				{
					const JsonObject& pd = pointV.AsObject();
					AnimationBlend1DPoint point;
					point.clipName = GetStringOpt(pd, "clip");
					if (point.clipName.empty())
					{
						throw std::runtime_error(contextPrefix + ".states." + stateName + ".blend1D.points[].clip is required");
					}
					point.value = GetFloatOpt(pd, "value", 0.0f);
					stateDesc.blend1D.push_back(std::move(point));
				}
				if (stateDesc.blend1D.empty())
				{
					throw std::runtime_error(contextPrefix + ".states." + stateName + ".blend1D.points must not be empty");
				}
				std::sort(stateDesc.blend1D.begin(), stateDesc.blend1D.end(), [](const AnimationBlend1DPoint& a, const AnimationBlend1DPoint& b)
					{
						return a.value < b.value;
					});
				if (stateDesc.clipName.empty())
				{
					stateDesc.clipName = stateDesc.blend1D.front().clipName;
				}
			}
			if (auto* blendV = TryGet(sd, "blend2D"))
			{
				const JsonObject& bd = blendV->AsObject();
				stateDesc.blendParameterX = GetStringOpt(bd, "parameterX");
				stateDesc.blendParameterY = GetStringOpt(bd, "parameterY");
				if (stateDesc.blendParameterX.empty())
				{
					throw std::runtime_error(contextPrefix + ".states." + stateName + ".blend2D.parameterX is required");
				}
				if (stateDesc.blendParameterY.empty())
				{
					throw std::runtime_error(contextPrefix + ".states." + stateName + ".blend2D.parameterY is required");
				}
				for (const std::string_view parameterName : { std::string_view(stateDesc.blendParameterX), std::string_view(stateDesc.blendParameterY) })
				{
					if (const AnimationParameterDesc* paramDesc = FindAnimationParameterDesc(def, parameterName))
					{
						if (paramDesc->defaultValue.type == AnimationParameterType::Trigger)
						{
							throw std::runtime_error(contextPrefix + ".states." + stateName + ".blend2D parameters must not be trigger");
						}
					}
					else
					{
						throw std::runtime_error(contextPrefix + ".states." + stateName + ".blend2D parameter references unknown parameter");
					}
				}
				auto* pointsV = TryGet(bd, "points");
				if (pointsV == nullptr || !pointsV->IsArray())
				{
					throw std::runtime_error(contextPrefix + ".states." + stateName + ".blend2D.points must be array");
				}
				for (const JsonValue& pointV : pointsV->AsArray())
				{
					const JsonObject& pd = pointV.AsObject();
					AnimationBlend2DPoint point;
					point.clipName = GetStringOpt(pd, "clip");
					point.clipSourceAssetId = GetStringOpt(pd, "clipSourceAssetId");
					if (point.clipName.empty() && point.clipSourceAssetId.empty())
					{
						throw std::runtime_error(contextPrefix + ".states." + stateName + ".blend2D.points[] requires clip or clipSourceAssetId");
					}
					point.x = GetFloatOpt(pd, "x", 0.0f);
					point.y = GetFloatOpt(pd, "y", 0.0f);
					stateDesc.blend2D.push_back(std::move(point));
				}
				if (stateDesc.blend2D.empty())
				{
					throw std::runtime_error(contextPrefix + ".states." + stateName + ".blend2D.points must not be empty");
				}
				if (stateDesc.clipName.empty())
				{
					stateDesc.clipName = stateDesc.blend2D.front().clipName;
					stateDesc.clipSourceAssetId = stateDesc.blend2D.front().clipSourceAssetId;
				}
			}
			if (auto* notifiesV = TryGet(sd, "notifies"))
			{
				if (!notifiesV->IsArray())
				{
					throw std::runtime_error(contextPrefix + ".states." + stateName + ".notifies must be array");
				}
				stateDesc.notifies = ParseAnimationNotifyArray_(notifiesV->AsArray(), contextPrefix + ".states." + stateName + ".notifies[]");
			}
			ValidateAnimationStateContentMode(def, stateDesc);
			if (stateDesc.motionId.empty() && stateDesc.clipName.empty() && stateDesc.clipSourceAssetId.empty() &&
				stateDesc.blend1D.empty() && stateDesc.blend2D.empty())
			{
				throw std::runtime_error(contextPrefix + ".states." + stateName +
					" must define motion, clip, clipSourceAssetId, blend1D, or blend2D");
			}
			def.states.push_back(std::move(stateDesc));
		}
	}
	if (def.states.empty())
	{
		throw std::runtime_error(contextPrefix + ".states must not be empty");
	}

	if (auto* transitionsV = TryGet(cd, "transitions"))
	{
		const JsonArray& transitionsA = transitionsV->AsArray();
		for (const JsonValue& transitionV : transitionsA)
		{
			const JsonObject& td = transitionV.AsObject();
			AnimationTransitionDesc transitionDesc;
			transitionDesc.fromState = GetStringOpt(td, "from");
			transitionDesc.toState = GetStringOpt(td, "to");
			if (transitionDesc.toState.empty())
			{
				throw std::runtime_error(contextPrefix + ".transitions[].to is required");
			}
			transitionDesc.hasExitTime = TryGet(td, "exitTime") != nullptr;
			transitionDesc.exitTimeNormalized = GetFloatOpt(td, "exitTime", 1.0f);
			transitionDesc.blendDurationSeconds = GetFloatOpt(td, "blendDuration", 0.15f);
			transitionDesc.priority = GetIntOpt(td, "priority", 0);

			transitionDesc.conditions = ParseAnimationConditions_(def, TryGet(td, "conditions"), contextPrefix + ".transitions[].conditions");
			def.transitions.push_back(std::move(transitionDesc));
		}
	}
	if (auto* rulesV = TryGet(cd, "transitionRules"))
	{
		if (!rulesV->IsArray()) throw std::runtime_error(contextPrefix + ".transitionRules must be array");
		const auto parseStringArray = [&](const JsonObject& object, std::string_view key, const std::string& path)
		{
			std::vector<std::string> values;
			const JsonValue* value = TryGet(object, key);
			if (value == nullptr) return values;
			if (!value->IsArray()) throw std::runtime_error(path + " must be array");
			for (const JsonValue& item : value->AsArray())
			{
				if (!item.IsString())
				{
					throw std::runtime_error(path + "[] must be string");
				}
				values.push_back(item.AsString());
			}
			return values;
		};
		const auto parseSelector = [&](const JsonObject& object, const std::string& path)
		{
			return AnimationStateSelector{
				parseStringArray(object, "states", path + ".states"),
				parseStringArray(object, "allTags", path + ".allTags"),
				parseStringArray(object, "anyTags", path + ".anyTags"),
				parseStringArray(object, "noneTags", path + ".noneTags") };
		};
		for (const JsonValue& ruleValue : rulesV->AsArray())
		{
			const JsonObject& object = ruleValue.AsObject();
			AnimationTransitionRuleDesc rule;
			rule.id = GetStringOpt(object, "id");
			const JsonValue* from = TryGet(object, "from");
			const JsonValue* to = TryGet(object, "to");
			if (from == nullptr || !from->IsObject()) throw std::runtime_error(contextPrefix + ".transitionRules[].from must be object");
			if (to == nullptr || !to->IsObject()) throw std::runtime_error(contextPrefix + ".transitionRules[].to must be object");
			rule.from = parseSelector(from->AsObject(), contextPrefix + ".transitionRules[].from");
			rule.to = parseSelector(to->AsObject(), contextPrefix + ".transitionRules[].to");
			rule.conditions = ParseAnimationConditions_(def, TryGet(object, "conditions"), contextPrefix + ".transitionRules[].conditions");
			rule.hasExitTime = TryGet(object, "exitTime") != nullptr;
			rule.exitTimeNormalized = GetFloatOpt(object, "exitTime", 1.0f);
			rule.blendDurationSeconds = GetFloatOpt(object, "blendDuration", 0.15f);
			rule.priority = GetIntOpt(object, "priority", 0);
			def.transitionRules.push_back(std::move(rule));
		}
	}

	if (auto* eventBindingsV = TryGet(cd, "eventBindings"))
	{
		if (!eventBindingsV->IsArray())
		{
			throw std::runtime_error(contextPrefix + ".eventBindings must be array");
		}
		ParseAnimationEventBindingsInto_(def, eventBindingsV->AsArray(), contextPrefix + ".eventBindings[]");
	}

	LoadAndMergeExternalAnimationNotifyAsset_(def);
	LoadAndMergeExternalAnimationEventBindingsAsset_(def);
	(void)BuildEffectiveAnimationTransitions(def);
	return def;
}

[[nodiscard]] AnimationControllerAsset LoadExternalAnimationControllerAssetFromJson_(
	std::string_view assetRelativePath,
	std::string_view controllerId)
{
	const std::filesystem::path absPath = corefs::ResolveAsset(std::filesystem::path(std::string(assetRelativePath)));
	const std::string text = FILE_UTILS::ReadAllText(absPath);

	JsonParser parser(text);
	JsonValue root = parser.Parse();
	if (!root.IsObject())
	{
		throw std::runtime_error("Animation controller JSON: root must be object");
	}

	return ParseAnimationControllerAssetObject_(
		root.AsObject(),
		std::string(controllerId),
		std::string("Animation controller JSON: ") + std::string(assetRelativePath));
}

// -----------------------------
// Loader API
// -----------------------------
