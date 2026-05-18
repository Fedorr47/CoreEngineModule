#pragma once

#include <string>
#include <utility>
#include <cmath>
#include <gtest/gtest.h>

import core;

namespace rendern::animationTest
{
	// Minimal valid skeleton for animation controller/runtime tests.
	// We do not test hierarchy here, so a single root bone is enough.
	[[nodiscard]] inline Skeleton MakeSingleBoneSkeleton()
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

	// Minimal clip that can be bound to AnimatorState.
	// The sampled pose is not the focus for controller tests;
	// they only need a valid clip name/channel pair.
	[[nodiscard]] inline AnimationClip MakeMinimalSingleBoneClip(std::string name)
	{
		AnimationClip clip{};
		clip.name = std::move(name);
		clip.durationTicks = 10.0f;
		clip.ticksPerSecond = 10.0f;
		clip.looping = true;

		BoneAnimationChannel channel{};
		channel.boneIndex = 0;
		channel.boneName = "root";
		channel.translationKeys.push_back(TranslationKey{
			.timeTicks = 0.0f,
			.value = { 0.0f, 0.0f, 0.0f }
		});

		clip.channels.push_back(std::move(channel));
		return clip;
	}

	// Clip with T/R/S keys for sampling invariant tests.
	// The two-key setup gives boundary and interpolation paths real data to sample.
	[[nodiscard]] inline AnimationClip MakeAnimatedSingleBoneClip(bool looping)
	{
		AnimationClip clip{};
		clip.name = looping ? "LoopClip" : "OneShotClip";
		clip.durationTicks = 2.0f;
		clip.ticksPerSecond = 2.0f; // 1 second duration.
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
	
	[[nodiscard]] inline bool IsFinite(float value)
	{
		return std::isfinite(value);
	}

	inline void ExpectFiniteVec3(const mathUtils::Vec3& value)
	{
		EXPECT_TRUE(IsFinite(value.x));
		EXPECT_TRUE(IsFinite(value.y));
		EXPECT_TRUE(IsFinite(value.z));
	}

	inline void ExpectFiniteVec4(const mathUtils::Vec4& value)
	{
		EXPECT_TRUE(IsFinite(value.x));
		EXPECT_TRUE(IsFinite(value.y));
		EXPECT_TRUE(IsFinite(value.z));
		EXPECT_TRUE(IsFinite(value.w));
	}

	inline void ExpectFiniteTransform(const LocalBoneTransform& transform)
	{
		ExpectFiniteVec3(transform.translation);
		ExpectFiniteVec4(transform.rotation);
		ExpectFiniteVec3(transform.scale);
	}

	inline void ExpectFinitePose(const AnimatorState& state)
	{
		for (const LocalBoneTransform& transform : state.localPose)
		{
			ExpectFiniteTransform(transform);
		}
	}

	inline void ExpectFiniteSingleBonePose(const AnimatorState& state)
	{
		ASSERT_EQ(state.localPose.size(), 1u);
		ExpectFiniteTransform(state.localPose[0]);
	}
}