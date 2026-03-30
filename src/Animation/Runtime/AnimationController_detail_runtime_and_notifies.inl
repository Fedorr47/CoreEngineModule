

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

