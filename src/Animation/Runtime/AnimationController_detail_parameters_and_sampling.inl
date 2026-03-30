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
