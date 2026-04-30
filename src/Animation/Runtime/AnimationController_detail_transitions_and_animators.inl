
		[[nodiscard]] bool EvaluateCondition(
			const AnimationConditionDesc& condition,
			const AnimationParameterStore& store) noexcept
		{
			auto it = store.values.find(condition.parameter);
			if (it == store.values.end())
			{
				return false;
			}
			const AnimationParameterValue& param = it->second;

			switch (condition.op)
			{
			case AnimationConditionOp::IfTrue: return GetParameterAsBool(param);
			case AnimationConditionOp::IfFalse: return !GetParameterAsBool(param);
			case AnimationConditionOp::Greater: return GetParameterAsFloat(param) > GetParameterAsFloat(condition.value);
			case AnimationConditionOp::GreaterEqual: return GetParameterAsFloat(param) >= GetParameterAsFloat(condition.value);
			case AnimationConditionOp::Less: return GetParameterAsFloat(param) < GetParameterAsFloat(condition.value);
			case AnimationConditionOp::LessEqual: return GetParameterAsFloat(param) <= GetParameterAsFloat(condition.value);
			case AnimationConditionOp::Equal:
				if (condition.value.type == AnimationParameterType::Bool || param.type == AnimationParameterType::Bool)
				{
					return GetParameterAsBool(param) == GetParameterAsBool(condition.value);
				}
				if (condition.value.type == AnimationParameterType::Int || param.type == AnimationParameterType::Int)
				{
					return GetParameterAsInt(param) == GetParameterAsInt(condition.value);
				}
				return std::fabs(GetParameterAsFloat(param) - GetParameterAsFloat(condition.value)) <= 1e-6f;
			case AnimationConditionOp::NotEqual:
				if (condition.value.type == AnimationParameterType::Bool || param.type == AnimationParameterType::Bool)
				{
					return GetParameterAsBool(param) != GetParameterAsBool(condition.value);
				}
				if (condition.value.type == AnimationParameterType::Int || param.type == AnimationParameterType::Int)
				{
					return GetParameterAsInt(param) != GetParameterAsInt(condition.value);
				}
				return std::fabs(GetParameterAsFloat(param) - GetParameterAsFloat(condition.value)) > 1e-6f;
			case AnimationConditionOp::Triggered:
				return param.type == AnimationParameterType::Trigger && param.triggerValue;
			default:
				return false;
			}
		}

		inline void ConsumeTransitionTriggers(AnimationParameterStore& store, const AnimationTransitionDesc& transition)
		{
			for (const AnimationConditionDesc& condition : transition.conditions)
			{
				if (condition.op == AnimationConditionOp::Triggered)
				{
					auto it = store.values.find(condition.parameter);
					if (it != store.values.end())
					{
						it->second.triggerValue = false;
					}
				}
			}
		}

		[[nodiscard]] inline int FindStateIndexByName(
			const AnimationControllerAsset& asset,
			std::string_view stateName) noexcept
		{
			for (std::size_t i = 0; i < asset.states.size(); ++i)
			{
				if (asset.states[i].name == stateName)
				{
					return static_cast<int>(i);
				}
			}
			return -1;
		}

		inline void SyncAnimatorClip(
			AnimatorState& animator,
			const Skeleton* skeleton,
			const AnimationClip* clip,
			bool looping,
			float playRate,
			bool paused,
			bool resetTime,
			float normalizedTime,
			bool syncNormalizedWhenUnchanged)
		{
			const bool needsInit = !IsAnimatorReady(animator) || animator.skeleton != skeleton;
			const bool clipChanged = needsInit || animator.clip != clip;
			if (needsInit)
			{
				InitializeAnimator(animator, skeleton, clip);
			}
			else if (clipChanged)
			{
				SetAnimatorClip(animator, clip, looping, playRate, true);
			}
			animator.looping = (clip != nullptr) ? (looping && clip->looping) : looping;
			animator.playRate = playRate;
			animator.paused = paused;
			if (resetTime)
			{
				animator.timeSeconds = 0.0f;
			}
			else if (clipChanged || syncNormalizedWhenUnchanged)
			{
				SetAnimatorNormalizedTime(animator, normalizedTime);
			}
		}

		inline void EvaluateAnimatorPairToLocalPose(
			AnimatorState& primaryAnimator,
			AnimatorState* secondaryAnimator,
			float secondaryAlpha,
			AnimatorState* tertiaryAnimator = nullptr,
			float tertiaryAlpha = 0.0f)
		{
			EvaluateAnimatorLocalPose(primaryAnimator);
			const float primaryWeight = std::max(0.0f, 1.0f - secondaryAlpha - tertiaryAlpha);
			float accumulatedWeight = primaryWeight;

			if (secondaryAnimator != nullptr && IsAnimatorReady(*secondaryAnimator) && secondaryAnimator->clip != nullptr && secondaryAlpha > 1e-6f)
			{
				EvaluateAnimatorLocalPose(*secondaryAnimator);
				const std::vector<LocalBoneTransform> primaryPose = primaryAnimator.localPose;
				const float denom = std::max(primaryWeight + secondaryAlpha, 1e-6f);
				const float alpha01 = std::clamp(secondaryAlpha / denom, 0.0f, 1.0f);
				BlendLocalPoses(primaryAnimator.localPose, primaryPose, secondaryAnimator->localPose, alpha01);
				accumulatedWeight = primaryWeight + secondaryAlpha;
			}

			if (tertiaryAnimator != nullptr && IsAnimatorReady(*tertiaryAnimator) && tertiaryAnimator->clip != nullptr && tertiaryAlpha > 1e-6f)
			{
				EvaluateAnimatorLocalPose(*tertiaryAnimator);
				const std::vector<LocalBoneTransform> blendedPose = primaryAnimator.localPose;
				const float denom = std::max(accumulatedWeight + tertiaryAlpha, 1e-6f);
				const float alpha012 = std::clamp(tertiaryAlpha / denom, 0.0f, 1.0f);
				BlendLocalPoses(primaryAnimator.localPose, blendedPose, tertiaryAnimator->localPose, alpha012);
			}
		}
