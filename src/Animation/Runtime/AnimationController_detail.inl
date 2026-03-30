	namespace detail
	{
		template<typename T>
		concept AnimationReadableType =
			std::same_as<std::remove_cvref_t<T>, bool> ||
			std::same_as<std::remove_cvref_t<T>, int> ||
			std::same_as<std::remove_cvref_t<T>, float>;

		template<AnimationReadableType T>
		[[nodiscard]] inline T GetParameterAs(const AnimationParameterValue& value) noexcept
		{
			using ValueT = std::remove_cvref_t<T>;

			switch (value.type)
			{
			case AnimationParameterType::Bool:
				if constexpr (std::same_as<ValueT, bool>)
					return value.boolValue;
				else if constexpr (std::same_as<ValueT, int>)
					return value.boolValue ? 1 : 0;
				else
					return value.boolValue ? 1.0f : 0.0f;

			case AnimationParameterType::Int:
				if constexpr (std::same_as<ValueT, bool>)
					return value.intValue != 0;
				else if constexpr (std::same_as<ValueT, int>)
					return value.intValue;
				else
					return static_cast<float>(value.intValue);

			case AnimationParameterType::Float:
				if constexpr (std::same_as<ValueT, bool>)
					return std::fabs(value.floatValue) > 1e-6f;
				else if constexpr (std::same_as<ValueT, int>)
					return static_cast<int>(value.floatValue);
				else
					return value.floatValue;

			case AnimationParameterType::Trigger:
				if constexpr (std::same_as<ValueT, bool>)
					return value.triggerValue;
				else if constexpr (std::same_as<ValueT, int>)
					return value.triggerValue ? 1 : 0;
				else
					return value.triggerValue ? 1.0f : 0.0f;

			default:
				if constexpr (std::same_as<ValueT, bool>)
					return false;
				else if constexpr (std::same_as<ValueT, int>)
					return 0;
				else
					return 0.0f;
			}
		}

		[[nodiscard]] inline bool GetParameterAsBool(const AnimationParameterValue& value) noexcept
		{
			return GetParameterAs<bool>(value);
		}

		[[nodiscard]] inline int GetParameterAsInt(const AnimationParameterValue& value) noexcept
		{
			return GetParameterAs<int>(value);
		}

		[[nodiscard]] inline float GetParameterAsFloat(const AnimationParameterValue& value) noexcept
		{
			return GetParameterAs<float>(value);
		}

		[[nodiscard]] inline int ResolveClipIndexByName(
			const std::vector<AnimationClip>& clips,
			std::string_view clipName) noexcept
		{
			if (clipName.empty())
			{
				return -1;
			}
			for (std::size_t i = 0; i < clips.size(); ++i)
			{
				if (clips[i].name == clipName)
				{
					return static_cast<int>(i);
				}
			}
			return -1;
		}

		[[nodiscard]] inline const AnimationClip* ResolveClipByIndex(
			const std::vector<AnimationClip>* clips,
			int clipIndex) noexcept
		{
			if (clips == nullptr || clipIndex < 0 || static_cast<std::size_t>(clipIndex) >= clips->size())
			{
				return nullptr;
			}
			return &(*clips)[static_cast<std::size_t>(clipIndex)];
		}

		[[nodiscard]] inline float ClipDurationSeconds(const AnimationClip* clip) noexcept
		{
			if (clip == nullptr || !IsValidAnimationClip(*clip) || clip->ticksPerSecond <= 0.0f)
			{
				return 0.0f;
			}
			return clip->durationTicks / clip->ticksPerSecond;
		}
		
		[[nodiscard]] inline int ResolveClipIndexForState(
			const AnimationControllerRuntime& runtime,
			const AnimationStateDesc& state) noexcept
		{
			if (runtime.clips == nullptr)
			{
				return -1;
			}

			const std::vector<AnimationClip>& clips = *runtime.clips;

			const bool hasSourceIds =
				runtime.clipSourceAssetIds != nullptr &&
				runtime.clipSourceAssetIds->size() == clips.size();

			const bool useSourceFilter =
				!state.clipSourceAssetId.empty() && hasSourceIds;

			if (useSourceFilter)
			{
				if (!state.clipName.empty())
				{
					for (std::size_t i = 0; i < clips.size(); ++i)
					{
						if ((*runtime.clipSourceAssetIds)[i] == state.clipSourceAssetId &&
							clips[i].name == state.clipName)
						{
							return static_cast<int>(i);
						}
					}
				}

				for (std::size_t i = 0; i < clips.size(); ++i)
				{
					if ((*runtime.clipSourceAssetIds)[i] == state.clipSourceAssetId)
					{
						return static_cast<int>(i);
					}
				}
			}

			return ResolveClipIndexByName(clips, state.clipName);
		}

		[[nodiscard]] inline int ResolveClipIndexForBlend2DPoint(
			const AnimationControllerRuntime& runtime,
			const AnimationBlend2DPoint& point) noexcept
		{
			if (runtime.clips == nullptr)
			{
				return -1;
			}

			const std::vector<AnimationClip>& clips = *runtime.clips;
			const bool hasSourceIds =
				runtime.clipSourceAssetIds != nullptr &&
				runtime.clipSourceAssetIds->size() == clips.size();
			const bool useSourceFilter = !point.clipSourceAssetId.empty() && hasSourceIds;

			if (useSourceFilter)
			{
				if (!point.clipName.empty())
				{
					for (std::size_t i = 0; i < clips.size(); ++i)
					{
						if ((*runtime.clipSourceAssetIds)[i] == point.clipSourceAssetId &&
							clips[i].name == point.clipName)
						{
							return static_cast<int>(i);
						}
					}
				}

				for (std::size_t i = 0; i < clips.size(); ++i)
				{
					if ((*runtime.clipSourceAssetIds)[i] == point.clipSourceAssetId)
					{
						return static_cast<int>(i);
					}
				}
			}

			return ResolveClipIndexByName(clips, point.clipName);
		}

		inline void SetAnimatorNormalizedTime(AnimatorState& animator, float normalizedTime) noexcept
		{
			const float durationSeconds = ClipDurationSeconds(animator.clip);
			if (durationSeconds <= 0.0f)
			{
				animator.timeSeconds = 0.0f;
				return;
			}
			animator.timeSeconds = std::clamp(normalizedTime, 0.0f, 1.0f) * durationSeconds;
		}

		struct StateSampleConfig
		{
			const AnimationStateDesc* state{ nullptr };
			int primaryClipIndex{ -1 };
			int secondaryClipIndex{ -1 };
			int tertiaryClipIndex{ -1 };
			float secondaryAlpha{ 0.0f };
			float tertiaryAlpha{ 0.0f };
			bool usesBlend1D{ false };
			bool usesBlend2D{ false };
			std::string parameterName;
			float parameterValue{ 0.0f };
			std::string parameterNameY;
			float parameterValueY{ 0.0f };
		};

		[[nodiscard]] inline StateSampleConfig BuildStateSampleConfig(
			const AnimationControllerRuntime& runtime,
			int stateIndex) noexcept
		{
			StateSampleConfig sample{};
			if (runtime.stateMachineAsset == nullptr ||
				stateIndex < 0 ||
				static_cast<std::size_t>(stateIndex) >= runtime.stateMachineAsset->states.size())
			{
				return sample;
			}

			const AnimationStateDesc& state = runtime.stateMachineAsset->states[static_cast<std::size_t>(stateIndex)];
			sample.state = &state;
			sample.primaryClipIndex =
				(static_cast<std::size_t>(stateIndex) < runtime.resolvedStateClipIndices.size())
				? runtime.resolvedStateClipIndices[static_cast<std::size_t>(stateIndex)]
				: -1;

			if (!state.blend2D.empty() && !state.blendParameterX.empty() && !state.blendParameterY.empty())
			{
				sample.usesBlend2D = true;
				sample.parameterName = state.blendParameterX;
				sample.parameterNameY = state.blendParameterY;
				if (auto it = runtime.parameters.values.find(state.blendParameterX); it != runtime.parameters.values.end())
				{
					sample.parameterValue = GetParameterAsFloat(it->second);
				}
				if (auto it = runtime.parameters.values.find(state.blendParameterY); it != runtime.parameters.values.end())
				{
					sample.parameterValueY = GetParameterAsFloat(it->second);
				}

				const std::vector<int>* resolvedIndices =
					(static_cast<std::size_t>(stateIndex) < runtime.resolvedStateBlend2DClipIndices.size())
					? &runtime.resolvedStateBlend2DClipIndices[static_cast<std::size_t>(stateIndex)]
					: nullptr;
				if (resolvedIndices == nullptr || resolvedIndices->empty())
				{
					return sample;
				}

				struct BlendCandidate
				{
					int clipIndex{ -1 };
					float weight{ 0.0f };
					float minDistanceSq{ 0.0f };
				};

				constexpr float kEpsilon = 1e-6f;
				std::unordered_map<int, BlendCandidate> aggregatedCandidates;
				aggregatedCandidates.reserve(state.blend2D.size());
				for (std::size_t pointIndex = 0; pointIndex < state.blend2D.size() && pointIndex < resolvedIndices->size(); ++pointIndex)
				{
					const int clipIndex = (*resolvedIndices)[pointIndex];
					if (clipIndex < 0)
					{
						continue;
					}
					const AnimationBlend2DPoint& point = state.blend2D[pointIndex];
					const float dx = sample.parameterValue - point.x;
					const float dy = sample.parameterValueY - point.y;
					const float distanceSq = dx * dx + dy * dy;
					if (distanceSq <= kEpsilon)
					{
						sample.primaryClipIndex = clipIndex;
						sample.secondaryClipIndex = -1;
						sample.tertiaryClipIndex = -1;
						sample.secondaryAlpha = 0.0f;
						sample.tertiaryAlpha = 0.0f;
						return sample;
					}

					const float weight = 1.0f / std::sqrt(distanceSq);
					auto [it, inserted] = aggregatedCandidates.try_emplace(
						clipIndex,
						BlendCandidate{ clipIndex, weight, distanceSq });
					if (!inserted)
					{
						it->second.weight += weight;
						it->second.minDistanceSq = std::min(it->second.minDistanceSq, distanceSq);
					}
				}

				if (aggregatedCandidates.empty())
				{
					return sample;
				}

				std::vector<BlendCandidate> candidates;
				candidates.reserve(aggregatedCandidates.size());
				for (const auto& [_, candidate] : aggregatedCandidates)
				{
					candidates.push_back(candidate);
				}

				std::sort(candidates.begin(), candidates.end(), [](const BlendCandidate& a, const BlendCandidate& b)
					{
						if (std::fabs(a.weight - b.weight) > 1e-6f)
						{
							return a.weight > b.weight;
						}
						if (std::fabs(a.minDistanceSq - b.minDistanceSq) > 1e-6f)
						{
							return a.minDistanceSq < b.minDistanceSq;
						}
						return a.clipIndex < b.clipIndex;
					});

				if (candidates.size() > 3)
				{
					candidates.resize(3);
				}

				float totalWeight = 0.0f;
				for (const BlendCandidate& candidate : candidates)
				{
					totalWeight += candidate.weight;
				}
				if (totalWeight <= kEpsilon)
				{
					sample.primaryClipIndex = candidates.front().clipIndex;
					return sample;
				}

				for (BlendCandidate& candidate : candidates)
				{
					candidate.weight /= totalWeight;
				}

				sample.primaryClipIndex = candidates[0].clipIndex;
				sample.secondaryClipIndex = (candidates.size() > 1) ? candidates[1].clipIndex : -1;
				sample.tertiaryClipIndex = (candidates.size() > 2) ? candidates[2].clipIndex : -1;
				sample.secondaryAlpha = (candidates.size() > 1) ? candidates[1].weight : 0.0f;
				sample.tertiaryAlpha = (candidates.size() > 2) ? candidates[2].weight : 0.0f;

				const float primaryWeight = std::max(0.0f, 1.0f - sample.secondaryAlpha - sample.tertiaryAlpha);
				const float normalizedSum = primaryWeight + sample.secondaryAlpha + sample.tertiaryAlpha;
				if (normalizedSum > kEpsilon)
				{
					sample.secondaryAlpha /= normalizedSum;
					sample.tertiaryAlpha /= normalizedSum;
				}
				if (sample.primaryClipIndex == sample.secondaryClipIndex)
				{
					sample.secondaryClipIndex = -1;
					sample.secondaryAlpha = 0.0f;
				}
				if (sample.primaryClipIndex == sample.tertiaryClipIndex || sample.secondaryClipIndex == sample.tertiaryClipIndex)
				{
					sample.tertiaryClipIndex = -1;
					sample.tertiaryAlpha = 0.0f;
				}
				if (sample.secondaryAlpha <= kEpsilon)
				{
					sample.secondaryClipIndex = -1;
					sample.secondaryAlpha = 0.0f;
				}
				if (sample.tertiaryAlpha <= kEpsilon)
				{
					sample.tertiaryClipIndex = -1;
					sample.tertiaryAlpha = 0.0f;
				}
				return sample;
			}

			if (state.blendParameter.empty() || state.blend1D.empty())
			{
				return sample;
			}

			sample.usesBlend1D = true;
			sample.parameterName = state.blendParameter;
			if (auto it = runtime.parameters.values.find(state.blendParameter); it != runtime.parameters.values.end())
			{
				sample.parameterValue = GetParameterAsFloat(it->second);
			}

			const std::vector<int>* resolvedIndices =
				(static_cast<std::size_t>(stateIndex) < runtime.resolvedStateBlend1DClipIndices.size())
				? &runtime.resolvedStateBlend1DClipIndices[static_cast<std::size_t>(stateIndex)]
				: nullptr;
			if (resolvedIndices == nullptr || resolvedIndices->empty())
			{
				return sample;
			}

			if (resolvedIndices->size() == 1 || state.blend1D.size() == 1)
			{
				sample.primaryClipIndex = (*resolvedIndices)[0];
				return sample;
			}

			if (sample.parameterValue <= state.blend1D.front().value)
			{
				sample.primaryClipIndex = (*resolvedIndices)[0];
				return sample;
			}
			if (sample.parameterValue >= state.blend1D.back().value)
			{
				sample.primaryClipIndex = (*resolvedIndices)[resolvedIndices->size() - 1];
				return sample;
			}

			for (std::size_t pointIndex = 0; pointIndex + 1 < state.blend1D.size(); ++pointIndex)
			{
				const AnimationBlend1DPoint& a = state.blend1D[pointIndex];
				const AnimationBlend1DPoint& b = state.blend1D[pointIndex + 1];
				if (sample.parameterValue > b.value)
				{
					continue;
				}
				sample.primaryClipIndex = (*resolvedIndices)[pointIndex];
				sample.secondaryClipIndex = (*resolvedIndices)[pointIndex + 1];
				const float span = b.value - a.value;
				sample.secondaryAlpha = (std::fabs(span) > 1e-6f)
					? std::clamp((sample.parameterValue - a.value) / span, 0.0f, 1.0f)
					: 1.0f;
				break;
			}

			if (sample.primaryClipIndex < 0 && sample.secondaryClipIndex >= 0)
			{
				sample.primaryClipIndex = sample.secondaryClipIndex;
				sample.secondaryClipIndex = -1;
				sample.secondaryAlpha = 0.0f;
			}
			if (sample.primaryClipIndex == sample.secondaryClipIndex)
			{
				sample.secondaryClipIndex = -1;
				sample.secondaryAlpha = 0.0f;
			}
			if (sample.secondaryAlpha <= 1e-6f)
			{
				sample.secondaryClipIndex = -1;
				sample.secondaryAlpha = 0.0f;
			}
			return sample;
		}

		inline void ResolveStateClipIndices(AnimationControllerRuntime& runtime)
		{
			runtime.resolvedStateClipIndices.clear();
			runtime.resolvedStateBlend1DClipIndices.clear();
			runtime.resolvedStateBlend2DClipIndices.clear();
			if (runtime.stateMachineAsset == nullptr || runtime.clips == nullptr)
			{
				return;
			}
			runtime.resolvedStateClipIndices.resize(runtime.stateMachineAsset->states.size(), -1);
			runtime.resolvedStateBlend1DClipIndices.resize(runtime.stateMachineAsset->states.size());
			runtime.resolvedStateBlend2DClipIndices.resize(runtime.stateMachineAsset->states.size());
			for (std::size_t i = 0; i < runtime.stateMachineAsset->states.size(); ++i)
			{
				const AnimationStateDesc& state = runtime.stateMachineAsset->states[i];
				if (!state.blend2D.empty())
				{
					auto& resolvedBlend = runtime.resolvedStateBlend2DClipIndices[i];
					resolvedBlend.reserve(state.blend2D.size());
					for (const AnimationBlend2DPoint& point : state.blend2D)
					{
						resolvedBlend.push_back(ResolveClipIndexForBlend2DPoint(runtime, point));
					}
					runtime.resolvedStateClipIndices[i] = resolvedBlend.empty() ? -1 : resolvedBlend.front();
				}
				else if (!state.blend1D.empty())
				{
					auto& resolvedBlend = runtime.resolvedStateBlend1DClipIndices[i];
					resolvedBlend.reserve(state.blend1D.size());
					for (const AnimationBlend1DPoint& point : state.blend1D)
					{
						resolvedBlend.push_back(ResolveClipIndexByName(*runtime.clips, point.clipName));
					}
					runtime.resolvedStateClipIndices[i] = resolvedBlend.empty() ? -1 : resolvedBlend.front();
				}
				else
				{
					runtime.resolvedStateClipIndices[i] =
						ResolveClipIndexForState(runtime, runtime.stateMachineAsset->states[i]);
				}
			}
		}

		inline void ApplyParameterDefaults(AnimationParameterStore& store, const AnimationControllerAsset& asset)
		{
			store.values.clear();
			for (const AnimationParameterDesc& param : asset.parameters)
			{
				store.values[param.name] = param.defaultValue;
			}
		}

		[[nodiscard]] inline float GetAnimatorNormalizedTime(const AnimatorState& animator) noexcept
		{
			if (animator.clip == nullptr || !IsValidAnimationClip(*animator.clip) || animator.clip->ticksPerSecond <= 0.0f)
			{
				return 0.0f;
			}
			const float durationSeconds = animator.clip->durationTicks / animator.clip->ticksPerSecond;
			if (durationSeconds <= 0.0f)
			{
				return 0.0f;
			}
			const float normalized =
				NormalizeAnimationTimeSeconds(*animator.clip, animator.timeSeconds, animator.looping) /
				durationSeconds;
			return std::clamp(normalized, 0.0f, 1.0f);
		}

		[[nodiscard]] inline bool EvaluateCondition(
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


		[[nodiscard]] inline int GetBoneDepth(const Skeleton& skeleton, std::size_t boneIndex) noexcept
		{
			int depth = 0;
			int current = static_cast<int>(boneIndex);
			while (current >= 0 && static_cast<std::size_t>(current) < skeleton.bones.size())
			{
				current = skeleton.bones[static_cast<std::size_t>(current)].parentIndex;
				if (current >= 0)
				{
					++depth;
				}
			}
			return depth;
		}

		[[nodiscard]] inline bool ChannelHasMeaningfulTranslation(
			const Skeleton& skeleton,
			const BoneAnimationChannel& channel) noexcept
		{
			if (channel.boneIndex < 0 || static_cast<std::size_t>(channel.boneIndex) >= skeleton.bones.size())
			{
				return false;
			}

			mathUtils::Vec3 bindTranslation{ 0.0f, 0.0f, 0.0f };
			mathUtils::Vec4 bindRotation{ 0.0f, 0.0f, 0.0f, 1.0f };
			mathUtils::Vec3 bindScale{ 1.0f, 1.0f, 1.0f };
			DecomposeTRS(
				skeleton.bones[static_cast<std::size_t>(channel.boneIndex)].bindLocalTransform,
				bindTranslation,
				bindRotation,
				bindScale);

			for (const TranslationKey& key : channel.translationKeys)
			{
				const mathUtils::Vec3 delta = key.value - bindTranslation;
				if (std::fabs(delta.x) > 1e-4f ||
					std::fabs(delta.y) > 1e-4f ||
					std::fabs(delta.z) > 1e-4f)
				{
					return true;
				}
			}

			return false;
		}

		[[nodiscard]] inline std::size_t ResolveInPlaceMotionBoneIndex(
			const AnimationControllerRuntime& runtime,
			const AnimatorState& animator) noexcept
		{
			const Skeleton& skeleton = *animator.skeleton;
			const std::size_t rootIndex = static_cast<std::size_t>(skeleton.rootBoneIndex);
			if (rootIndex >= skeleton.bones.size())
			{
				return skeleton.bones.empty() ? 0u : skeleton.bones.size() - 1u;
			}

			if (!runtime.rootMotionBoneName.empty())
			{
				if (const auto explicitBone = FindBoneIndex(skeleton, runtime.rootMotionBoneName))
				{
					return static_cast<std::size_t>(*explicitBone);
				}
			}

			if (animator.clip == nullptr)
			{
				return rootIndex;
			}

			const auto scoreChannel = [&](const BoneAnimationChannel& channel) noexcept -> int
				{
					if (!ChannelHasMeaningfulTranslation(skeleton, channel))
					{
						return -1;
					}

					int score = 0;
					const std::string_view boneName = channel.boneName;
					if (stringUtils::ContainsInsensitive(boneName, "hips")) score += 200;
					if (stringUtils::ContainsInsensitive(boneName, "pelvis")) score += 180;
					if (stringUtils::ContainsInsensitive(boneName, "root")) score += 120;
					if (stringUtils::ContainsInsensitive(boneName, "master")) score += 80;
					if (stringUtils::ContainsInsensitive(boneName, "ctrl")) score -= 10;
					const std::size_t boneIndex = static_cast<std::size_t>(channel.boneIndex);
					score -= GetBoneDepth(skeleton, boneIndex) * 4;
					return score;
				};

			int bestScore = -1;
			std::size_t bestIndex = rootIndex;
			for (const BoneAnimationChannel& channel : animator.clip->channels)
			{
				if (channel.boneIndex < 0 || static_cast<std::size_t>(channel.boneIndex) >= skeleton.bones.size())
				{
					continue;
				}

				const int score = scoreChannel(channel);
				if (score > bestScore)
				{
					bestScore = score;
					bestIndex = static_cast<std::size_t>(channel.boneIndex);
				}
			}

			return bestIndex;
		}

		inline void ApplyRootMotionModeToAnimatorPose(AnimationControllerRuntime& runtime, AnimatorState& animator)
		{
			runtime.lastAppliedRootMotionDelta = mathUtils::Vec3(0.0f, 0.0f, 0.0f);

			if (runtime.rootMotionMode != AnimationRootMotionMode::InPlace ||
				!IsAnimatorReady(animator) ||
				animator.localPose.empty())
			{
				return;
			}

			const std::size_t motionBoneIndex = ResolveInPlaceMotionBoneIndex(runtime, animator);
			if (motionBoneIndex >= animator.localPose.size() || motionBoneIndex >= animator.skeleton->bones.size())
			{
				return;
			}

			mathUtils::Vec3 bindTranslation{ 0.0f, 0.0f, 0.0f };
			mathUtils::Vec4 bindRotation{ 0.0f, 0.0f, 0.0f, 1.0f };
			mathUtils::Vec3 bindScale{ 1.0f, 1.0f, 1.0f };
			DecomposeTRS(
				animator.skeleton->bones[motionBoneIndex].bindLocalTransform,
				bindTranslation,
				bindRotation,
				bindScale);

			LocalBoneTransform& motionTransform = animator.localPose[motionBoneIndex];
			runtime.lastAppliedRootMotionDelta = mathUtils::Vec3(
				motionTransform.translation.x - bindTranslation.x,
				0.0f,
				motionTransform.translation.z - bindTranslation.z);
			motionTransform.translation.x = bindTranslation.x;
			motionTransform.translation.z = bindTranslation.z;
		}

		inline void PushNotifyEvent(
			AnimationControllerRuntime& runtime,
			const AnimationStateDesc& state,
			const AnimationNotifyDesc& notify,
			const AnimationClip* clip)
		{
			AnimationNotifyEvent event{};
			event.sequence = ++runtime.nextNotifySequence;
			event.id = notify.id;
			event.stateName = state.name;
			event.clipName = (clip != nullptr) ? clip->name : std::string{};
			event.normalizedTime = std::clamp(notify.timeNormalized, 0.0f, 1.0f);
			runtime.pendingNotifyEvents.push_back(event);
			runtime.notifyHistory.push_back(std::move(event));

			constexpr std::size_t kMaxRetainedNotifyEvents = 64;
			if (runtime.pendingNotifyEvents.size() > kMaxRetainedNotifyEvents)
			{
				runtime.pendingNotifyEvents.erase(
					runtime.pendingNotifyEvents.begin(),
					runtime.pendingNotifyEvents.begin() + static_cast<std::ptrdiff_t>(runtime.pendingNotifyEvents.size() - kMaxRetainedNotifyEvents));
			}
			if (runtime.notifyHistory.size() > kMaxRetainedNotifyEvents)
			{
				runtime.notifyHistory.erase(
					runtime.notifyHistory.begin(),
					runtime.notifyHistory.begin() + static_cast<std::ptrdiff_t>(runtime.notifyHistory.size() - kMaxRetainedNotifyEvents));
			}
		}

		[[nodiscard]] inline bool DidNormalizedTimePass(
			float previousNormalizedTime,
			float currentNormalizedTime,
			float notifyTime,
			bool looping) noexcept
		{
			const float t = std::clamp(notifyTime, 0.0f, 1.0f);
			constexpr float kEpsilon = 1e-6f;

			if (!looping || currentNormalizedTime >= previousNormalizedTime)
			{
				return t > previousNormalizedTime + kEpsilon && t <= currentNormalizedTime + kEpsilon;
			}

			return t > previousNormalizedTime + kEpsilon || t <= currentNormalizedTime + kEpsilon;
		}

		inline void QueueCurrentStateNotifies(AnimationControllerRuntime& runtime, const AnimatorState& animator)
		{
			if (runtime.stateMachineAsset == nullptr ||
				runtime.currentStateIndex < 0 ||
				static_cast<std::size_t>(runtime.currentStateIndex) >= runtime.stateMachineAsset->states.size())
			{
				runtime.previousStateNormalizedTime = 0.0f;
				runtime.stateEnteredThisFrame = false;
				return;
			}

			const AnimationStateDesc& state = runtime.stateMachineAsset->states[static_cast<std::size_t>(runtime.currentStateIndex)];
			const float currentNormalizedTime = GetAnimatorNormalizedTime(animator);

			if (!state.notifies.empty())
			{
				const bool looping = animator.clip != nullptr && animator.looping && animator.clip->looping;
				for (const AnimationNotifyDesc& notify : state.notifies)
				{
					if (notify.id.empty())
					{
						continue;
					}

					const float notifyTime = std::clamp(notify.timeNormalized, 0.0f, 1.0f);
					if (runtime.stateEnteredThisFrame && (notify.fireOnEnter || notifyTime <= 1e-6f))
					{
						PushNotifyEvent(runtime, state, notify, animator.clip);
						continue;
					}

					if (DidNormalizedTimePass(runtime.previousStateNormalizedTime, currentNormalizedTime, notifyTime, looping))
					{
						PushNotifyEvent(runtime, state, notify, animator.clip);
					}
				}
			}

			runtime.previousStateNormalizedTime = currentNormalizedTime;
			runtime.stateEnteredThisFrame = false;
		}

		inline void SyncRuntimeBlendMetadata(AnimationControllerRuntime& runtime, const StateSampleConfig& sample)
		{
			runtime.currentStateUsesBlend1D = sample.usesBlend1D;
			runtime.currentStateUsesBlend2D = sample.usesBlend2D;
			runtime.currentBlendParameterName = sample.parameterName;
			runtime.currentBlendParameterValue = sample.parameterValue;
			runtime.currentBlendParameterNameY = sample.parameterNameY;
			runtime.currentBlendParameterValueY = sample.parameterValueY;
			runtime.currentBlendPrimaryClipName.clear();
			runtime.currentBlendSecondaryClipName.clear();
			runtime.currentBlendTertiaryClipName.clear();
			if (const AnimationClip* primaryClip = ResolveClipByIndex(runtime.clips, sample.primaryClipIndex))
			{
				runtime.currentBlendPrimaryClipName = primaryClip->name;
			}
			if (const AnimationClip* secondaryClip = ResolveClipByIndex(runtime.clips, sample.secondaryClipIndex))
			{
				runtime.currentBlendSecondaryClipName = secondaryClip->name;
			}
			if (const AnimationClip* tertiaryClip = ResolveClipByIndex(runtime.clips, sample.tertiaryClipIndex))
			{
				runtime.currentBlendTertiaryClipName = tertiaryClip->name;
			}
			runtime.blendSecondaryClipIndex = sample.secondaryClipIndex;
			runtime.blendTertiaryClipIndex = sample.tertiaryClipIndex;
			runtime.blendSecondaryAlpha = sample.secondaryAlpha;
			runtime.blendTertiaryAlpha = sample.tertiaryAlpha;
		}

		inline void SyncActiveStateAnimators(
			AnimationControllerRuntime& runtime,
			AnimatorState& primaryAnimator,
			const StateSampleConfig& sample,
			bool resetTime)
		{
			const float normalizedTime = resetTime ? 0.0f : GetAnimatorNormalizedTime(primaryAnimator);
			const AnimationClip* primaryClip = ResolveClipByIndex(runtime.clips, sample.primaryClipIndex);
			SyncAnimatorClip(
				primaryAnimator,
				runtime.skeleton,
				primaryClip,
				sample.state != nullptr ? sample.state->looping : runtime.looping,
				sample.state != nullptr ? sample.state->playRate : runtime.playRate,
				runtime.paused,
				resetTime,
				normalizedTime,
				false);

			const bool useSecondary = sample.secondaryClipIndex >= 0 && sample.secondaryAlpha > 1e-6f;
			if (useSecondary)
			{
				const AnimationClip* secondaryClip = ResolveClipByIndex(runtime.clips, sample.secondaryClipIndex);
				SyncAnimatorClip(
					runtime.blendSecondaryAnimator,
					runtime.skeleton,
					secondaryClip,
					sample.state != nullptr ? sample.state->looping : runtime.looping,
					sample.state != nullptr ? sample.state->playRate : runtime.playRate,
					runtime.paused,
					resetTime,
					normalizedTime,
					true);
			}
			else
			{
				runtime.blendSecondaryAnimator = {};
			}

			const bool useTertiary = sample.tertiaryClipIndex >= 0 && sample.tertiaryAlpha > 1e-6f;
			if (useTertiary)
			{
				const AnimationClip* tertiaryClip = ResolveClipByIndex(runtime.clips, sample.tertiaryClipIndex);
				SyncAnimatorClip(
					runtime.blendTertiaryAnimator,
					runtime.skeleton,
					tertiaryClip,
					sample.state != nullptr ? sample.state->looping : runtime.looping,
					sample.state != nullptr ? sample.state->playRate : runtime.playRate,
					runtime.paused,
					resetTime,
					normalizedTime,
					true);
			}
			else
			{
				runtime.blendTertiaryAnimator = {};
			}
			SyncRuntimeBlendMetadata(runtime, sample);
		}

		inline void ClearActiveBlendMetadata(AnimationControllerRuntime& runtime)
		{
			runtime.currentStateUsesBlend1D = false;
			runtime.currentStateUsesBlend2D = false;
			runtime.currentBlendParameterName.clear();
			runtime.currentBlendParameterValue = 0.0f;
			runtime.currentBlendParameterNameY.clear();
			runtime.currentBlendParameterValueY = 0.0f;
			runtime.currentBlendPrimaryClipName.clear();
			runtime.currentBlendSecondaryClipName.clear();
			runtime.currentBlendTertiaryClipName.clear();
			runtime.blendSecondaryAnimator = {};
			runtime.blendTertiaryAnimator = {};
			runtime.blendSecondaryClipIndex = -1;
			runtime.blendTertiaryClipIndex = -1;
			runtime.blendSecondaryAlpha = 0.0f;
			runtime.blendTertiaryAlpha = 0.0f;
		}

		inline void ApplyRuntimeState(AnimationControllerRuntime& runtime, int stateIndex, bool resetStateTracking = true)
		{
			if (runtime.stateMachineAsset == nullptr ||
				stateIndex < 0 ||
				static_cast<std::size_t>(stateIndex) >= runtime.stateMachineAsset->states.size())
			{
				runtime.currentStateIndex = -1;
				runtime.currentStateName.clear();
				runtime.legacyClipIndex = -1;
				ClearActiveBlendMetadata(runtime);
				if (resetStateTracking)
				{
					runtime.previousStateNormalizedTime = 0.0f;
					runtime.stateEnteredThisFrame = true;
				}
				return;
			}
			runtime.currentStateIndex = stateIndex;
			const AnimationStateDesc& state = runtime.stateMachineAsset->states[static_cast<std::size_t>(stateIndex)];
			runtime.currentStateName = state.name;
			runtime.looping = state.looping;
			runtime.playRate = state.playRate;
			runtime.legacyClipIndex =
				(static_cast<std::size_t>(stateIndex) < runtime.resolvedStateClipIndices.size())
				? runtime.resolvedStateClipIndices[static_cast<std::size_t>(stateIndex)]
				: -1;
			runtime.currentStateUsesBlend1D = !state.blendParameter.empty() && !state.blend1D.empty();
			runtime.currentStateUsesBlend2D = !state.blendParameterX.empty() && !state.blendParameterY.empty() && !state.blend2D.empty();
			runtime.currentBlendParameterName = runtime.currentStateUsesBlend2D ? state.blendParameterX : state.blendParameter;
			runtime.currentBlendParameterValue = 0.0f;
			runtime.currentBlendParameterNameY = runtime.currentStateUsesBlend2D ? state.blendParameterY : std::string{};
			runtime.currentBlendParameterValueY = 0.0f;
			runtime.currentBlendPrimaryClipName.clear();
			runtime.currentBlendSecondaryClipName.clear();
			runtime.currentBlendTertiaryClipName.clear();
			runtime.blendSecondaryClipIndex = -1;
			runtime.blendTertiaryClipIndex = -1;
			runtime.blendSecondaryAlpha = 0.0f;
			runtime.blendTertiaryAlpha = 0.0f;
			if (resetStateTracking)
			{
				runtime.previousStateNormalizedTime = 0.0f;
				runtime.stateEnteredThisFrame = true;
			}
		}

		[[nodiscard]] inline bool TransitionMatchesState(const AnimationTransitionDesc& transition, std::string_view currentState) noexcept
		{
			return transition.fromState.empty() || transition.fromState == "*" || transition.fromState == currentState;
		}

		inline void ResetBlendState(AnimationControllerRuntime& runtime)
		{
			runtime.transitionActive = false;
			runtime.transitionSourceStateIndex = -1;
			runtime.transitionSourceStateName.clear();
			runtime.transitionElapsedSeconds = 0.0f;
			runtime.transitionDurationSeconds = 0.0f;
			runtime.transitionSourceAnimator = {};
			runtime.transitionSourceBlendSecondaryAnimator = {};
			runtime.transitionSourceBlendTertiaryAnimator = {};
			runtime.transitionSourceSecondaryClipIndex = -1;
			runtime.transitionSourceTertiaryClipIndex = -1;
			runtime.transitionSourceSecondaryAlpha = 0.0f;
			runtime.transitionSourceTertiaryAlpha = 0.0f;
		}
	}

	inline void ResetAnimationParameters(AnimationParameterStore& store)
	{
		store.values.clear();
	}

