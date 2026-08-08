

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

		[[nodiscard]] inline bool AnimationStateIndexHasTag(
			const AnimationControllerRuntime& runtime,
			const int stateIndex,
			const std::string_view tag) noexcept
		{
			if (runtime.stateMachineAsset == nullptr ||
				stateIndex < 0 ||
				static_cast<std::size_t>(stateIndex) >= runtime.stateMachineAsset->states.size())
			{
				return false;
			}

			const AnimationStateDesc& state = runtime.stateMachineAsset->states[static_cast<std::size_t>(stateIndex)];
			for (const std::string& candidate : state.tags)
			{
				if (candidate == tag)
				{
					return true;
				}
			}

			return false;
		}

		[[nodiscard]] inline bool ShouldStripTurnInPlaceYaw(const AnimationControllerRuntime& runtime) noexcept
		{
			constexpr std::string_view kTurnInPlaceTag = "turn_in_place";
			if (AnimationStateIndexHasTag(runtime, runtime.currentStateIndex, kTurnInPlaceTag))
			{
				return true;
			}

			return runtime.transitionActive &&
				AnimationStateIndexHasTag(runtime, runtime.transitionSourceStateIndex, kTurnInPlaceTag);
		}

		[[nodiscard]] inline bool ShouldPreserveIdlePose(
			const AnimationControllerRuntime& runtime) noexcept
		{
			constexpr std::string_view kIdleTag = "idle";

			if (runtime.transitionActive)
			{
				return false;
			}

			return AnimationStateIndexHasTag(
				runtime,
				runtime.currentStateIndex,
				kIdleTag);
		}

		[[nodiscard]] inline mathUtils::Vec4 ConjugateRootMotionQuat(const mathUtils::Vec4& rotation) noexcept
		{
			const mathUtils::Vec4 normalized = mathUtils::NormalizeQuat(rotation);
			return mathUtils::Vec4(-normalized.x, -normalized.y, -normalized.z, normalized.w);
		}

		[[nodiscard]] inline mathUtils::Vec4 RemoveRootMotionYawRelativeToBind(
			const mathUtils::Vec4& rotation,
			const mathUtils::Vec4& bindRotation) noexcept
		{
			const mathUtils::Vec4 normalizedBind = mathUtils::NormalizeQuat(bindRotation);
			const mathUtils::Vec4 relativeRotation = mathUtils::MultiplyRootMotionQuats(
				ConjugateRootMotionQuat(normalizedBind),
				mathUtils::NormalizeQuat(rotation));

			const float sinYaw = 2.0f *
				(relativeRotation.w * relativeRotation.y + relativeRotation.x * relativeRotation.z);
			const float cosYaw = 1.0f - 2.0f *
				(relativeRotation.x * relativeRotation.x + relativeRotation.y * relativeRotation.y);
			const float yawRadians = std::atan2(sinYaw, cosYaw);
			const float halfYawRadians = yawRadians * 0.5f;
			const mathUtils::Vec4 yawRotation(
				0.0f,
				std::sin(halfYawRadians),
				0.0f,
				std::cos(halfYawRadians));

			const mathUtils::Vec4 rotationWithoutYaw = MultiplyRootMotionQuats(
				ConjugateRootMotionQuat(yawRotation),
				relativeRotation);
			return MultiplyRootMotionQuats(normalizedBind, rotationWithoutYaw);
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
			
			// Idle clips keep their authored skeletal motion.
			// Gameplay still owns the entity world transform.
			if (ShouldPreserveIdlePose(runtime))
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

			// Gameplay owns actor-facing yaw. Turn-in-place clips provide footwork and pose only.
			if (ShouldStripTurnInPlaceYaw(runtime))
			{
				motionTransform.rotation = RemoveRootMotionYawRelativeToBind(
					motionTransform.rotation,
					bindRotation);
			}
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
