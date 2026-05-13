#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

import core;

using namespace rendern;

namespace
{
	Skeleton MakeSingleBoneSkeleton()
	{
		Skeleton skeleton{};
		skeleton.rootBoneIndex = 0;
		skeleton.bones.push_back(SkeletonBone{
			.name = "root",
			.parentIndex = -1,
			.inverseBindMatrix = mathUtils::Mat4(1.0f),
			.bindLocalTransform = mathUtils::Mat4(1.0f)
		});
		return skeleton;
	}

	AnimationClip MakeSingleBoneClip(bool looping)
	{
		AnimationClip clip{};
		clip.name = looping ? "LoopClip" : "OneShotClip";
		clip.durationTicks = 2.0f;
		clip.ticksPerSecond = 2.0f; // 1 second duration
		clip.looping = looping;

		BoneAnimationChannel channel{};
		channel.boneIndex = 0;
		channel.boneName = "root";
		channel.translationKeys.push_back(TranslationKey{ .timeTicks = 0.0f, .value = { 0.0f, 0.0f, 0.0f } });
		channel.translationKeys.push_back(TranslationKey{ .timeTicks = 2.0f, .value = { 2.0f, 0.0f, 0.0f } });
		channel.rotationKeys.push_back(RotationKey{ .timeTicks = 0.0f, .value = { 0.0f, 0.0f, 0.0f, 1.0f } });
		channel.rotationKeys.push_back(RotationKey{ .timeTicks = 2.0f, .value = { 0.0f, 0.0f, 0.70710677f, 0.70710677f } });
		channel.scaleKeys.push_back(ScaleKey{ .timeTicks = 0.0f, .value = { 1.0f, 1.0f, 1.0f } });
		channel.scaleKeys.push_back(ScaleKey{ .timeTicks = 2.0f, .value = { 1.2f, 0.8f, 1.1f } });
		clip.channels.push_back(std::move(channel));
		return clip;
	}

	bool IsFinite(float value)
	{
		return std::isfinite(value);
	}

	void ExpectFiniteVec3(const mathUtils::Vec3& value)
	{
		EXPECT_TRUE(IsFinite(value.x));
		EXPECT_TRUE(IsFinite(value.y));
		EXPECT_TRUE(IsFinite(value.z));
	}

	void ExpectFiniteVec4(const mathUtils::Vec4& value)
	{
		EXPECT_TRUE(IsFinite(value.x));
		EXPECT_TRUE(IsFinite(value.y));
		EXPECT_TRUE(IsFinite(value.z));
		EXPECT_TRUE(IsFinite(value.w));
	}

	void ExpectFinitePose(const AnimatorState& state)
	{
		ASSERT_EQ(state.localPose.size(), 1u);
		ExpectFiniteVec3(state.localPose[0].translation);
		ExpectFiniteVec4(state.localPose[0].rotation);
		ExpectFiniteVec3(state.localPose[0].scale);
	}
}

TEST(AnimatorSampling, NormalizesLoopingTimeIntoValidRange)
{
	AnimationClip clip = MakeSingleBoneClip(true);
	const float duration = clip.durationTicks / clip.ticksPerSecond;

	const std::vector<float> times{ -0.25f, duration + 0.25f, duration * 4.0f + 0.125f };
	for (float t : times)
	{
		const float normalized = NormalizeAnimationTimeSeconds(clip, t, true);
		EXPECT_GE(normalized, 0.0f);
		EXPECT_LT(normalized, duration);
	}
}

TEST(AnimatorSampling, ClampsNonLoopingTimeIntoValidRange)
{
	AnimationClip clip = MakeSingleBoneClip(false);
	const float duration = clip.durationTicks / clip.ticksPerSecond;

	EXPECT_FLOAT_EQ(NormalizeAnimationTimeSeconds(clip, -0.5f, false), 0.0f);
	EXPECT_FLOAT_EQ(NormalizeAnimationTimeSeconds(clip, duration + 0.5f, false), duration);
	EXPECT_FLOAT_EQ(NormalizeAnimationTimeSeconds(clip, 0.0f, false), 0.0f);
	EXPECT_FLOAT_EQ(NormalizeAnimationTimeSeconds(clip, duration, false), duration);
}

TEST(AnimatorSampling, OutOfRangeTimesProduceFinitePose)
{
	Skeleton skeleton = MakeSingleBoneSkeleton();
	AnimationClip clip = MakeSingleBoneClip(true);

	AnimatorState state{};
	InitializeAnimator(state, &skeleton, &clip);

	const float duration = clip.durationTicks / clip.ticksPerSecond;
	const std::vector<float> times{ -0.5f, 0.0f, duration, duration + 0.5f, duration * 5.0f };
	for (float sampleTime : times)
	{
		state.timeSeconds = sampleTime;
		EvaluateAnimatorLocalPose(state);
		SCOPED_TRACE(sampleTime);
		ExpectFinitePose(state);
	}
}

TEST(AnimatorSampling, SingleBoneClipProducesFinitePose)
{
	Skeleton skeleton = MakeSingleBoneSkeleton();
	AnimationClip clip = MakeSingleBoneClip(true);

	AnimatorState state{};
	InitializeAnimator(state, &skeleton, &clip);
	UpdateAnimator(state, 0.25f);

	ASSERT_EQ(state.localPose.size(), 1u);
	ExpectFinitePose(state);
	EXPECT_TRUE(IsFinite(state.timeSeconds));
}

TEST(AnimatorSampling, BlendLocalPoseOutOfRangeAlphaIsFiniteAndClamped)
{
	std::vector<LocalBoneTransform> fromPose(1);
	fromPose[0].translation = mathUtils::Vec3(0.0f, 1.0f, 2.0f);
	fromPose[0].rotation = mathUtils::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
	fromPose[0].scale = mathUtils::Vec3(1.0f, 1.0f, 1.0f);

	std::vector<LocalBoneTransform> toPose(1);
	toPose[0].translation = mathUtils::Vec3(10.0f, 11.0f, 12.0f);
	toPose[0].rotation = mathUtils::Vec4(0.0f, 0.70710677f, 0.0f, 0.70710677f);
	toPose[0].scale = mathUtils::Vec3(2.0f, 2.0f, 2.0f);

	std::vector<LocalBoneTransform> blended{};
	BlendLocalPoses(blended, fromPose, toPose, -1.0f);
	ASSERT_EQ(blended.size(), 1u);
	ExpectFiniteVec3(blended[0].translation);
	ExpectFiniteVec4(blended[0].rotation);
	ExpectFiniteVec3(blended[0].scale);
	EXPECT_NEAR(blended[0].translation.x, fromPose[0].translation.x, 1e-6f);

	BlendLocalPoses(blended, fromPose, toPose, 2.0f);
	ASSERT_EQ(blended.size(), 1u);
	ExpectFiniteVec3(blended[0].translation);
	ExpectFiniteVec4(blended[0].rotation);
	ExpectFiniteVec3(blended[0].scale);
	EXPECT_NEAR(blended[0].translation.x, toPose[0].translation.x, 1e-6f);
}