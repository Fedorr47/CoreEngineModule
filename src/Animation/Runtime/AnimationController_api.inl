	[[nodiscard]] inline AnimationParameterValue* FindAnimationParameter(AnimationParameterStore& store, std::string_view name)
	{
		auto it = store.values.find(std::string(name));
		return (it == store.values.end()) ? nullptr : &it->second;
	}

	[[nodiscard]] inline const AnimationParameterValue* FindAnimationParameter(const AnimationParameterStore& store, std::string_view name)
	{
		auto it = store.values.find(std::string(name));
		return (it == store.values.end()) ? nullptr : &it->second;
	}

	[[nodiscard]] inline const AnimationParameterDesc* FindAnimationParameterDesc(const AnimationControllerAsset& asset, std::string_view name) noexcept
	{
		for (const AnimationParameterDesc& param : asset.parameters)
		{
			if (param.name == name)
			{
				return &param;
			}
		}
		return nullptr;
	}

	template <typename T>
	concept AnimationParameterTypeC =
		std::same_as<std::remove_cvref_t<T>, bool> ||
		std::same_as<std::remove_cvref_t<T>, int> ||
		std::same_as<std::remove_cvref_t<T>, float>;

	template<AnimationParameterTypeC T>
	inline void SetAnimationParameter(AnimationParameterStore& store, std::string_view name, T value)
	{
		using ValueT = std::remove_cvref_t<T>;

		AnimationParameterValue& param = store.values[std::string(name)];

		if constexpr (std::same_as<ValueT, bool>)
		{
			param.type = AnimationParameterType::Bool;
			param.boolValue = value;
		}
		else if constexpr (std::same_as<ValueT, int>)
		{
			param.type = AnimationParameterType::Int;
			param.intValue = value;
		}
		else if constexpr (std::same_as<ValueT, float>)
		{
			param.type = AnimationParameterType::Float;
			param.floatValue = value;
		}
	}

	inline void FireAnimationTrigger(AnimationParameterStore& store, std::string_view name)
	{
		AnimationParameterValue& param = store.values[std::string(name)];
		param.type = AnimationParameterType::Trigger;
		param.triggerValue = true;
	}

	inline void ResetAnimationTrigger(AnimationParameterStore& store, std::string_view name)
	{
		if (AnimationParameterValue* param = FindAnimationParameter(store, name))
		{
			if (param->type == AnimationParameterType::Trigger)
			{
				param->triggerValue = false;
			}
		}
	}

	[[nodiscard]] inline bool ConsumeAnimationTrigger(AnimationParameterStore& store, std::string_view name)
	{
		AnimationParameterValue* param = FindAnimationParameter(store, name);
		if (param == nullptr || param->type != AnimationParameterType::Trigger || !param->triggerValue)
		{
			return false;
		}
		param->triggerValue = false;
		return true;
	}

	[[nodiscard]] inline const std::vector<AnimationNotifyEvent>& PeekAnimationControllerNotifyEvents(const AnimationControllerRuntime& runtime) noexcept
	{
		return runtime.notifyHistory;
	}

	inline void ClearAnimationControllerNotifyEvents(AnimationControllerRuntime& runtime) noexcept
	{
		runtime.pendingNotifyEvents.clear();
		runtime.notifyHistory.clear();
	}

	[[nodiscard]] inline std::vector<AnimationNotifyEvent> ConsumeAnimationControllerNotifyEvents(AnimationControllerRuntime& runtime)
	{
		std::vector<AnimationNotifyEvent> out = std::move(runtime.pendingNotifyEvents);
		runtime.pendingNotifyEvents.clear();
		return out;
	}

	[[nodiscard]] inline bool IsAnimationControllerUsingLegacyClipMode(const AnimationControllerRuntime& runtime) noexcept
	{
		return runtime.mode == AnimationControllerMode::LegacyClip;
	}

	[[nodiscard]] inline const AnimationClip* ResolveLegacyAnimationClip(const AnimationControllerRuntime& runtime) noexcept
	{
		if (runtime.clips == nullptr ||
			runtime.legacyClipIndex < 0 ||
			static_cast<std::size_t>(runtime.legacyClipIndex) >= runtime.clips->size())
		{
			return nullptr;
		}

		return &(*runtime.clips)[static_cast<std::size_t>(runtime.legacyClipIndex)];
	}

	[[nodiscard]] inline int FindAnimationControllerStateIndex(const AnimationControllerAsset& asset, std::string_view stateName) noexcept
	{
		return detail::FindStateIndexByName(asset, stateName);
	}

	[[nodiscard]] inline const AnimationStateDesc* FindAnimationControllerState(const AnimationControllerAsset& asset, std::string_view stateName) noexcept
	{
		const int index = detail::FindStateIndexByName(asset, stateName);
		return (index >= 0) ? &asset.states[static_cast<std::size_t>(index)] : nullptr;
	}

	[[nodiscard]] inline bool AnimationStateHasTag(const AnimationStateDesc& state, std::string_view tag) noexcept
	{
		for (const std::string& candidate : state.tags)
		{
			if (candidate == tag)
			{
				return true;
			}
		}
		return false;
	}

	inline void ResetAnimationControllerRuntime(AnimationControllerRuntime& runtime)
	{
		runtime = {};
	}

	inline void SyncAnimationControllerLegacyClip(
		AnimationControllerRuntime& runtime,
		const Skeleton& skeleton,
		const std::vector<AnimationClip>& clips,
		int clipIndex,
		bool autoplay,
		bool looping,
		float playRate,
		bool paused,
		bool forceBindPose)
	{
		runtime.mode = AnimationControllerMode::LegacyClip;
		runtime.skeleton = &skeleton;
		runtime.clips = &clips;
		runtime.clipSourceAssetIds = nullptr;
		runtime.stateMachineAsset = nullptr;
		runtime.currentStateIndex = -1;
		runtime.resolvedStateClipIndices.clear();
		runtime.resolvedStateBlendClipIndices.clear();
		detail::ResetBlendState(runtime);
		detail::ClearActiveBlendMetadata(runtime);
		runtime.legacyClipIndex = clipIndex;
		runtime.autoplay = autoplay;
		runtime.looping = looping;
		runtime.playRate = playRate;
		runtime.paused = paused;
		runtime.forceBindPose = forceBindPose;
		runtime.currentStateName = (ResolveLegacyAnimationClip(runtime) != nullptr)
			? ResolveLegacyAnimationClip(runtime)->name
			: std::string("BindPose");
	}

	inline void BindAnimationControllerStateMachine(
		AnimationControllerRuntime& runtime,
		const Skeleton& skeleton,
		const std::vector<AnimationClip>& clips,
		const std::vector<std::string>& clipSourceAssetIds,
		const AnimationControllerAsset& asset,
		bool autoplay,
		bool paused,
		bool forceBindPose)
	{
		const bool sameAsset = runtime.mode == AnimationControllerMode::StateMachine && runtime.stateMachineAsset == &asset;
		runtime.mode = AnimationControllerMode::StateMachine;
		runtime.skeleton = &skeleton;
		runtime.clips = &clips;
		runtime.clipSourceAssetIds = &clipSourceAssetIds;
		runtime.stateMachineAsset = &asset;
		runtime.controllerAssetId = asset.id;
		runtime.autoplay = autoplay;
		runtime.paused = paused;
		runtime.forceBindPose = forceBindPose;
		detail::ResolveStateClipIndices(runtime);
		detail::ResetBlendState(runtime);
		detail::ClearActiveBlendMetadata(runtime);

		if (!sameAsset)
		{
			detail::ApplyParameterDefaults(runtime.parameters, asset);
			runtime.requestedStateName.clear();
			const int defaultIndex =
				!asset.defaultState.empty()
				? detail::FindStateIndexByName(asset, asset.defaultState)
				: (asset.states.empty() ? -1 : 0);
			detail::ApplyRuntimeState(runtime, defaultIndex, true);
		}
		else if (runtime.currentStateIndex >= 0)
		{
			detail::ApplyRuntimeState(runtime, runtime.currentStateIndex, false);
		}
	}

	inline void RefreshAnimationControllerRuntimeBindings(
		AnimationControllerRuntime& runtime,
		const Skeleton& skeleton,
		const std::vector<AnimationClip>& clips,
		const std::vector<std::string>& clipSourceAssetIds,
		bool autoplay,
		bool paused,
		bool forceBindPose)
	{
		runtime.skeleton = &skeleton;
		runtime.clips = &clips;
		runtime.clipSourceAssetIds = &clipSourceAssetIds;
		runtime.autoplay = autoplay;
		runtime.paused = paused;
		runtime.forceBindPose = forceBindPose;
		if (runtime.mode == AnimationControllerMode::StateMachine && runtime.stateMachineAsset != nullptr)
		{
			detail::ResolveStateClipIndices(runtime);
			if (runtime.currentStateIndex >= 0)
			{
				detail::ApplyRuntimeState(runtime, runtime.currentStateIndex, false);
			}
			else
			{
				detail::ClearActiveBlendMetadata(runtime);
			}
			if (runtime.transitionActive)
			{
				detail::ResetBlendState(runtime);
			}
		}
	}

	inline void RequestAnimationControllerState(AnimationControllerRuntime& runtime, std::string_view stateName)
	{
		runtime.requestedStateName = std::string(stateName);
	}

	inline void UpdateAnimationControllerRuntime(AnimationControllerRuntime& runtime, AnimatorState& animator, float deltaSeconds)
	{
		if (runtime.skeleton == nullptr)
		{
			return;
		}

		if (runtime.mode == AnimationControllerMode::StateMachine && runtime.stateMachineAsset != nullptr)
		{
			if (runtime.currentStateIndex < 0)
			{
				const int defaultIndex =
					!runtime.stateMachineAsset->defaultState.empty()
					? detail::FindStateIndexByName(*runtime.stateMachineAsset, runtime.stateMachineAsset->defaultState)
					: (runtime.stateMachineAsset->states.empty() ? -1 : 0);
				detail::ApplyRuntimeState(runtime, defaultIndex, true);
			}

			if (runtime.forceBindPose)
			{
				detail::ResetBlendState(runtime);
				detail::ClearActiveBlendMetadata(runtime);
				runtime.lastAppliedRootMotionDelta = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
				ResetAnimatorToBindPose(animator, *runtime.skeleton);
				BuildAnimatorMatrices(animator);
				return;
			}

			const detail::StateSampleConfig initialSample = detail::BuildStateSampleConfig(runtime, runtime.currentStateIndex);
			detail::SyncActiveStateAnimators(runtime, animator, initialSample, false);

			if (runtime.autoplay && !runtime.paused)
			{
				AdvanceAnimator(animator, deltaSeconds);
				if (runtime.blendSecondaryClipIndex >= 0 && IsAnimatorReady(runtime.blendSecondaryAnimator))
				{
					AdvanceAnimator(runtime.blendSecondaryAnimator, deltaSeconds);
				}
				if (runtime.transitionActive)
				{
					runtime.transitionElapsedSeconds += deltaSeconds;
					AdvanceAnimator(runtime.transitionSourceAnimator, deltaSeconds);
					if (runtime.transitionSourceSecondaryClipIndex >= 0 && IsAnimatorReady(runtime.transitionSourceBlendSecondaryAnimator))
					{
						AdvanceAnimator(runtime.transitionSourceBlendSecondaryAnimator, deltaSeconds);
					}
				}
			}

			int targetStateIndex = -1;
			const AnimationTransitionDesc* matchedTransition = nullptr;
			int matchedTransitionPriority = std::numeric_limits<int>::min();
			runtime.debugTransitionCandidates.clear();
			runtime.debugLastTransitionSelection.clear();

			if (!runtime.requestedStateName.empty())
			{
				targetStateIndex = detail::FindStateIndexByName(*runtime.stateMachineAsset, runtime.requestedStateName);
				runtime.requestedStateName.clear();
			}
			else if (!runtime.transitionActive)
			{
				for (const AnimationTransitionDesc& transition : runtime.stateMachineAsset->transitions)
				{
					if (!detail::TransitionMatchesState(transition, runtime.currentStateName))
					{
						continue;
					}

					std::string debugLabel = transition.fromState.empty() ? std::string("*") : transition.fromState;
					debugLabel += " -> ";
					debugLabel += transition.toState;

					if (transition.hasExitTime &&
						detail::GetAnimatorNormalizedTime(animator) < transition.exitTimeNormalized)
					{
						debugLabel += " [exit-time blocked]";
						runtime.debugTransitionCandidates.push_back(std::move(debugLabel));
						continue;
					}
					bool passed = true;
					for (const AnimationConditionDesc& condition : transition.conditions)
					{
						if (!detail::EvaluateCondition(condition, runtime.parameters))
						{
							passed = false;
							debugLabel += " [condition failed: ";
							debugLabel += condition.parameter;
							debugLabel += "]";
							break;
						}
					}
					if (!passed)
					{
						runtime.debugTransitionCandidates.push_back(std::move(debugLabel));
						continue;
					}
					const int candidateStateIndex = detail::FindStateIndexByName(*runtime.stateMachineAsset, transition.toState);
					if (candidateStateIndex < 0)
					{
						debugLabel += " [target missing]";
						runtime.debugTransitionCandidates.push_back(std::move(debugLabel));
						continue;
					}
					debugLabel += " [pass, priority=" + std::to_string(transition.priority) + "]";
					runtime.debugTransitionCandidates.push_back(debugLabel);
					if (matchedTransition == nullptr || transition.priority > matchedTransitionPriority)
					{
						targetStateIndex = candidateStateIndex;
						matchedTransition = &transition;
						matchedTransitionPriority = transition.priority;
						runtime.debugLastTransitionSelection = std::move(debugLabel);
					}
				}
			}

			if (targetStateIndex >= 0 && targetStateIndex != runtime.currentStateIndex)
			{
				const float blendDurationSeconds =
					(matchedTransition != nullptr)
					? std::max(0.0f, matchedTransition->blendDurationSeconds)
					: 0.0f;
				const bool canBlend =
					blendDurationSeconds > 1e-4f &&
					IsAnimatorReady(animator) &&
					animator.skeleton == runtime.skeleton &&
					animator.clip != nullptr;

				if (canBlend)
				{
					runtime.transitionSourceAnimator = animator;
					runtime.transitionSourceAnimator.paused = runtime.paused;
					runtime.transitionSourceBlendSecondaryAnimator = runtime.blendSecondaryAnimator;
					runtime.transitionSourceSecondaryClipIndex = runtime.blendSecondaryClipIndex;
					runtime.transitionSourceSecondaryAlpha = runtime.blendSecondaryAlpha;
					runtime.transitionSourceStateIndex = runtime.currentStateIndex;
					runtime.transitionSourceStateName = runtime.currentStateName;
					runtime.transitionElapsedSeconds = 0.0f;
					runtime.transitionDurationSeconds = blendDurationSeconds;
					runtime.transitionActive = true;
				}
				else
				{
					detail::ResetBlendState(runtime);
				}

				detail::ApplyRuntimeState(runtime, targetStateIndex, true);
				const detail::StateSampleConfig targetSample = detail::BuildStateSampleConfig(runtime, targetStateIndex);
				detail::SyncActiveStateAnimators(runtime, animator, targetSample, true);
				if (matchedTransition != nullptr)
				{
					detail::ConsumeTransitionTriggers(runtime.parameters, *matchedTransition);
				}
			}
			else
			{
				const detail::StateSampleConfig liveSample = detail::BuildStateSampleConfig(runtime, runtime.currentStateIndex);
				detail::SyncActiveStateAnimators(runtime, animator, liveSample, false);
			}

			animator.paused = runtime.paused;
			detail::EvaluateAnimatorPairToLocalPose(
				animator,
				(runtime.blendSecondaryClipIndex >= 0) ? &runtime.blendSecondaryAnimator : nullptr,
				runtime.blendSecondaryAlpha);

			if (runtime.transitionActive)
			{
				const bool validBlend =
					IsAnimatorReady(runtime.transitionSourceAnimator) &&
					runtime.transitionSourceAnimator.skeleton == runtime.skeleton &&
					runtime.transitionDurationSeconds > 1e-4f;
				if (validBlend)
				{
					runtime.transitionSourceAnimator.paused = runtime.paused;
					detail::EvaluateAnimatorPairToLocalPose(
						runtime.transitionSourceAnimator,
						(runtime.transitionSourceSecondaryClipIndex >= 0) ? &runtime.transitionSourceBlendSecondaryAnimator : nullptr,
						runtime.transitionSourceSecondaryAlpha);

					const float alpha = std::clamp(
						runtime.transitionElapsedSeconds / runtime.transitionDurationSeconds,
						0.0f,
						1.0f);
					const std::vector<LocalBoneTransform> targetPose = animator.localPose;
					BlendLocalPoses(animator.localPose, runtime.transitionSourceAnimator.localPose, targetPose, alpha);
					if (alpha >= 1.0f - 1e-6f)
					{
						detail::ResetBlendState(runtime);
					}
				}
				else
				{
					detail::ResetBlendState(runtime);
				}
			}

			detail::ApplyRootMotionModeToAnimatorPose(runtime, animator);
			detail::QueueCurrentStateNotifies(runtime, animator);

			BuildAnimatorMatrices(animator);
			return;
		}

		const AnimationClip* clip = ResolveLegacyAnimationClip(runtime);
		const bool needsInit = !IsAnimatorReady(animator) || animator.skeleton != runtime.skeleton;
		if (needsInit)
		{
			InitializeAnimator(animator, runtime.skeleton, clip);
		}
		else if (animator.clip != clip)
		{
			SetAnimatorClip(animator, clip, runtime.looping, runtime.playRate, true);
		}

		animator.looping = runtime.looping;
		animator.playRate = runtime.playRate;
		animator.paused = runtime.paused;

		if (runtime.forceBindPose)
		{
			runtime.lastAppliedRootMotionDelta = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
			ResetAnimatorToBindPose(animator, *runtime.skeleton);
			BuildAnimatorMatrices(animator);
			return;
		}

		if (runtime.autoplay && !animator.paused)
		{
			AdvanceAnimator(animator, deltaSeconds);
		}
		EvaluateAnimatorLocalPose(animator);
		detail::ApplyRootMotionModeToAnimatorPose(runtime, animator);
		BuildAnimatorMatrices(animator);
	}