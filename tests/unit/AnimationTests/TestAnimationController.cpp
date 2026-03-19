#include <gtest/gtest.h>
#include <string>
#include <vector>

import core;

using namespace rendern;

namespace
{
	AnimationClip MakeSingleBoneClip(const std::string& name)
	{
		AnimationClip clip{};
		clip.name = name;
		clip.durationTicks = 10.0f;
		clip.ticksPerSecond = 10.0f;
		clip.looping = true;

		BoneAnimationChannel channel{};
		channel.boneIndex = 0;
		channel.boneName = "root";
		channel.translationKeys.push_back(TranslationKey{ .timeTicks = 0.0f, .value = { 0.0f, 0.0f, 0.0f } });
		clip.channels.push_back(std::move(channel));
		return clip;
	}
}

TEST(AnimationController, ParameterStoreSupportsSetTriggerConsumeAndReset)
{
	AnimationParameterStore store{};

	SetAnimationParameter(store, "isMoving", true);
	SetAnimationParameter(store, "combo", 2);
	SetAnimationParameter(store, "speed", 3.5f);
	FireAnimationTrigger(store, "attack");

	ASSERT_NE(FindAnimationParameter(store, "isMoving"), nullptr);
	EXPECT_EQ(FindAnimationParameter(store, "isMoving")->type, AnimationParameterType::Bool);
	EXPECT_EQ(FindAnimationParameter(store, "combo")->type, AnimationParameterType::Int);
	EXPECT_EQ(FindAnimationParameter(store, "speed")->type, AnimationParameterType::Float);
	EXPECT_EQ(FindAnimationParameter(store, "attack")->type, AnimationParameterType::Trigger);

	EXPECT_TRUE(ConsumeAnimationTrigger(store, "attack"));
	EXPECT_FALSE(ConsumeAnimationTrigger(store, "attack"));

	FireAnimationTrigger(store, "attack");
	ResetAnimationTrigger(store, "attack");
	EXPECT_FALSE(ConsumeAnimationTrigger(store, "attack"));

	ResetAnimationParameters(store);
	EXPECT_TRUE(store.values.empty());
}

TEST(AnimationController, StateLookupAndTagsWork)
{
	AnimationControllerAsset asset{};
	asset.states.push_back(AnimationStateDesc{ .name = "Idle", .tags = { "Locomotion", "Grounded" } });
	asset.states.push_back(AnimationStateDesc{ .name = "Attack", .tags = { "Action" } });

	EXPECT_EQ(FindAnimationControllerStateIndex(asset, "Idle"), 0);
	EXPECT_EQ(FindAnimationControllerStateIndex(asset, "Attack"), 1);
	EXPECT_EQ(FindAnimationControllerStateIndex(asset, "Missing"), -1);

	const AnimationStateDesc* idle = FindAnimationControllerState(asset, "Idle");
	ASSERT_NE(idle, nullptr);
	EXPECT_TRUE(AnimationStateHasTag(*idle, "Locomotion"));
	EXPECT_FALSE(AnimationStateHasTag(*idle, "Action"));
}

TEST(AnimationController, BindStateMachineAppliesDefaultStateAndParameterDefaults)
{
	Skeleton skeleton{};
	skeleton.rootBoneIndex = 0;
	skeleton.bones.push_back(SkeletonBone{
		.name = "root",
		.parentIndex = -1,
		.inverseBindMatrix = mathUtils::Mat4(1.0f),
		.bindLocalTransform = mathUtils::Mat4(1.0f)
	});

	std::vector<AnimationClip> clips{};
	clips.push_back(MakeSingleBoneClip("Idle"));
	clips.push_back(MakeSingleBoneClip("Run"));
	std::vector<std::string> clipSourceAssetIds{ "hero", "hero" };

	AnimationControllerAsset asset{};
	asset.id = "hero_controller";
	asset.defaultState = "Run";
	asset.parameters.push_back(AnimationParameterDesc{
		.name = "isMoving",
		.defaultValue = AnimationParameterValue{ .type = AnimationParameterType::Bool, .boolValue = true }
	});
	asset.states.push_back(AnimationStateDesc{ .name = "Idle", .clipName = "Idle", .looping = true, .playRate = 1.0f });
	asset.states.push_back(AnimationStateDesc{ .name = "Run", .clipName = "Run", .looping = true, .playRate = 1.25f });

	AnimationControllerRuntime runtime{};
	BindAnimationControllerStateMachine(runtime, skeleton, clips, clipSourceAssetIds, asset, true, false, false);

	EXPECT_EQ(runtime.mode, AnimationControllerMode::StateMachine);
	EXPECT_EQ(runtime.controllerAssetId, "hero_controller");
	EXPECT_EQ(runtime.currentStateName, "Run");
	EXPECT_EQ(runtime.currentStateIndex, 1);
	ASSERT_NE(FindAnimationParameter(runtime.parameters, "isMoving"), nullptr);
	EXPECT_TRUE(FindAnimationParameter(runtime.parameters, "isMoving")->boolValue);
	EXPECT_FLOAT_EQ(runtime.playRate, 1.25f);

	RequestAnimationControllerState(runtime, "Idle");
	EXPECT_EQ(runtime.requestedStateName, "Idle");
}
