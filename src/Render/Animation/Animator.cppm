module;

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

export module core:animator;

import :animation_clip;
import :math_utils;
import :skeleton;


export namespace rendern
{
	struct LocalBoneTransform
	{
		mathUtils::Vec3 translation{ 0.0f, 0.0f, 0.0f };
		mathUtils::Vec4 rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
		mathUtils::Vec3 scale{ 1.0f, 1.0f, 1.0f };

		friend constexpr bool operator==(const LocalBoneTransform& a, const LocalBoneTransform& b) noexcept
		{
			return a.translation == b.translation
				&& a.rotation == b.rotation
				&& a.scale == b.scale;
		}
	};

	struct AnimatorState
	{
		const Skeleton* skeleton{ nullptr };
		const AnimationClip* clip{ nullptr };

		float timeSeconds{ 0.0f };
		float playRate{ 1.0f };
		bool looping{ true };
		bool paused{ false };

		std::vector<int> channelIndexByBone;
		std::vector<LocalBoneTransform> localPose;
		std::vector<mathUtils::Mat4> localMatrices;
		std::vector<mathUtils::Mat4> globalMatrices;
		std::vector<mathUtils::Mat4> skinMatrices;
	};

	[[nodiscard]] inline bool IsAnimatorReady(const AnimatorState& state) noexcept
	{
		return state.skeleton != nullptr && IsValidSkeleton(*state.skeleton);
	}

	[[nodiscard]] inline std::vector<LocalBoneTransform> BuildBindPoseLocalPose(const Skeleton& skeleton)
	{
		std::vector<LocalBoneTransform> bindPose;
		bindPose.resize(skeleton.bones.size());
		for (std::size_t boneIndex = 0; boneIndex < skeleton.bones.size(); ++boneIndex)
		{
			DecomposeTRS(
				skeleton.bones[boneIndex].bindLocalTransform,
				bindPose[boneIndex].translation,
				bindPose[boneIndex].rotation,
				bindPose[boneIndex].scale);
		}
		return bindPose;
	}

	inline void ResetAnimatorToBindPose(AnimatorState& state)
	{
		if (!IsAnimatorReady(state))
		{
			state.channelIndexByBone.clear();
			state.localPose.clear();
			state.localMatrices.clear();
			state.globalMatrices.clear();
			state.skinMatrices.clear();
			return;
		}

		const std::size_t boneCount = state.skeleton->bones.size();
		state.channelIndexByBone.assign(boneCount, -1);
		state.localPose = BuildBindPoseLocalPose(*state.skeleton);
		state.localMatrices.assign(boneCount, mathUtils::Mat4(1.0f));
		state.globalMatrices.assign(boneCount, mathUtils::Mat4(1.0f));
		state.skinMatrices.assign(boneCount, mathUtils::Mat4(1.0f));
	}

	inline void RebuildAnimatorClipBinding(AnimatorState& state)
	{
		if (!IsAnimatorReady(state))
		{
			state.channelIndexByBone.clear();
			return;
		}

		state.channelIndexByBone.assign(state.skeleton->bones.size(), -1);
		if (state.clip == nullptr)
		{
			return;
		}

		for (std::size_t channelIndex = 0; channelIndex < state.clip->channels.size(); ++channelIndex)
		{
			const BoneAnimationChannel& channel = state.clip->channels[channelIndex];
			int boneIndex = channel.boneIndex;
			if (boneIndex < 0)
			{
				if (const auto found = FindBoneIndex(*state.skeleton, channel.boneName))
				{
					boneIndex = static_cast<int>(*found);
				}
			}
			if (boneIndex < 0 || boneIndex >= static_cast<int>(state.channelIndexByBone.size()))
			{
				continue;
			}
			state.channelIndexByBone[static_cast<std::size_t>(boneIndex)] = static_cast<int>(channelIndex);
		}
	}

	inline void InitializeAnimator(AnimatorState& state, const Skeleton* skeleton, const AnimationClip* clip = nullptr)
	{
		state.skeleton = skeleton;
		state.clip = clip;
		state.timeSeconds = 0.0f;
		state.playRate = 1.0f;
		state.looping = (clip != nullptr) ? clip->looping : true;
		state.paused = false;
		ResetAnimatorToBindPose(state);
		RebuildAnimatorClipBinding(state);
	}

	inline void SetAnimatorClip(AnimatorState& state, const AnimationClip* clip, bool resetTime = true)
	{
		state.clip = clip;
		if (clip != nullptr)
		{
			state.looping = clip->looping;
		}
		if (resetTime)
		{
			state.timeSeconds = 0.0f;
		}
		RebuildAnimatorClipBinding(state);
	}

	inline void EvaluateAnimatorLocalPose(AnimatorState& state)
	{
		if (!IsAnimatorReady(state))
		{
			return;
		}
		if (state.localPose.size() != state.skeleton->bones.size())
		{
			ResetAnimatorToBindPose(state);
			RebuildAnimatorClipBinding(state);
		}

		state.localPose = BuildBindPoseLocalPose(*state.skeleton);
		if (state.clip == nullptr || !IsValidAnimationClip(*state.clip))
		{
			return;
		}

		const float timeSeconds = NormalizeAnimationTimeSeconds(*state.clip, state.timeSeconds, state.looping);
		const float timeTicks = timeSeconds * state.clip->ticksPerSecond;
		for (std::size_t boneIndex = 0; boneIndex < state.localPose.size(); ++boneIndex)
		{
			const int channelIndex = (boneIndex < state.channelIndexByBone.size()) ? state.channelIndexByBone[boneIndex] : -1;
			if (channelIndex < 0 || channelIndex >= static_cast<int>(state.clip->channels.size()))
			{
				continue;
			}

			const BoneAnimationChannel& channel = state.clip->channels[static_cast<std::size_t>(channelIndex)];
			LocalBoneTransform& dst = state.localPose[boneIndex];
			dst.translation = SampleTranslationKeys(channel.translationKeys, timeTicks, dst.translation);
			dst.rotation = SampleRotationKeys(channel.rotationKeys, timeTicks, dst.rotation);
			dst.scale = SampleScaleKeys(channel.scaleKeys, timeTicks, dst.scale);
		}
	}

	inline void BuildAnimatorMatrices(AnimatorState& state)
	{
		if (!IsAnimatorReady(state))
		{
			return;
		}
		const std::size_t boneCount = state.skeleton->bones.size();
		if (state.localPose.size() != boneCount)
		{
			ResetAnimatorToBindPose(state);
		}
		state.localMatrices.resize(boneCount, mathUtils::Mat4(1.0f));
		state.globalMatrices.resize(boneCount, mathUtils::Mat4(1.0f));
		state.skinMatrices.resize(boneCount, mathUtils::Mat4(1.0f));

		for (std::size_t boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
			const LocalBoneTransform& trs = state.localPose[boneIndex];
			state.localMatrices[boneIndex] = ComposeTRS(trs.translation, trs.rotation, trs.scale);
		}
		for (std::size_t boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
			const int parentIndex = state.skeleton->bones[boneIndex].parentIndex;
			if (parentIndex >= 0)
			{
				state.globalMatrices[boneIndex] = state.globalMatrices[static_cast<std::size_t>(parentIndex)] * state.localMatrices[boneIndex];
			}
			else
			{
				state.globalMatrices[boneIndex] = state.localMatrices[boneIndex];
			}
			state.skinMatrices[boneIndex] = state.globalMatrices[boneIndex] * state.skeleton->bones[boneIndex].inverseBindMatrix;
		}
	}

	inline void EvaluateAnimator(AnimatorState& state)
	{
		EvaluateAnimatorLocalPose(state);
		BuildAnimatorMatrices(state);
	}

	inline void AdvanceAnimator(AnimatorState& state, float deltaSeconds) noexcept
	{
		if (state.paused || state.clip == nullptr || !IsValidAnimationClip(*state.clip))
		{
			return;
		}
		state.timeSeconds += deltaSeconds * state.playRate;
		state.timeSeconds = NormalizeAnimationTimeSeconds(*state.clip, state.timeSeconds, state.looping);
	}

	inline void UpdateAnimator(AnimatorState& state, float deltaSeconds)
	{
		AdvanceAnimator(state, deltaSeconds);
		EvaluateAnimator(state);
	}
}