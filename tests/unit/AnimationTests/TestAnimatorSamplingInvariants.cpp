#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "AnimationTestHelpers.h"

import core;

using namespace rendern;

TEST(AnimatorSampling, NormalizesLoopingTimeIntoValidRange)
{
	AnimationClip clip = animationTest::MakeAnimatedSingleBoneClip(true);
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
	AnimationClip clip = animationTest::MakeAnimatedSingleBoneClip(false);
	const float duration = clip.durationTicks / clip.ticksPerSecond;

	EXPECT_FLOAT_EQ(NormalizeAnimationTimeSeconds(clip, -0.5f, false), 0.0f);
	EXPECT_FLOAT_EQ(NormalizeAnimationTimeSeconds(clip, duration + 0.5f, false), duration);
	EXPECT_FLOAT_EQ(NormalizeAnimationTimeSeconds(clip, 0.0f, false), 0.0f);
	EXPECT_FLOAT_EQ(NormalizeAnimationTimeSeconds(clip, duration, false), duration);
}

TEST(AnimatorSampling, OutOfRangeTimesProduceFinitePose)
{
	Skeleton skeleton = animationTest::MakeSingleBoneSkeleton();
	AnimationClip clip = animationTest::MakeAnimatedSingleBoneClip(true);

	AnimatorState state{};
	InitializeAnimator(state, &skeleton, &clip);

	const float duration = clip.durationTicks / clip.ticksPerSecond;
	const std::vector<float> times{ -0.5f, 0.0f, duration, duration + 0.5f, duration * 5.0f };
	for (float sampleTime : times)
	{
		state.timeSeconds = sampleTime;
		EvaluateAnimatorLocalPose(state);
		SCOPED_TRACE(sampleTime);
		animationTest::ExpectFiniteSingleBonePose(state);
	}
}

TEST(AnimatorSampling, SingleBoneClipProducesFinitePose)
{
	Skeleton skeleton = animationTest::MakeSingleBoneSkeleton();
	AnimationClip clip = animationTest::MakeAnimatedSingleBoneClip(true);

	AnimatorState state{};
	InitializeAnimator(state, &skeleton, &clip);
	UpdateAnimator(state, 0.25f);
	
	animationTest::ExpectFiniteSingleBonePose(state);
	EXPECT_TRUE(animationTest::IsFinite(state.timeSeconds));
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
	animationTest::ExpectFiniteVec3(blended[0].translation);
	animationTest::ExpectFiniteVec4(blended[0].rotation);
	animationTest::ExpectFiniteVec3(blended[0].scale);
	EXPECT_NEAR(blended[0].translation.x, fromPose[0].translation.x, 1e-6f);

	BlendLocalPoses(blended, fromPose, toPose, 2.0f);
	ASSERT_EQ(blended.size(), 1u);
	animationTest::ExpectFiniteVec3(blended[0].translation);
	animationTest::ExpectFiniteVec4(blended[0].rotation);
	animationTest::ExpectFiniteVec3(blended[0].scale);
	EXPECT_NEAR(blended[0].translation.x, toPose[0].translation.x, 1e-6f);
}