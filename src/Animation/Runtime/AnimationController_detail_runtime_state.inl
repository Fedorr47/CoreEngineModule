
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

