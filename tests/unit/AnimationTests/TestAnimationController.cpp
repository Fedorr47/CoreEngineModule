#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "TestSupport/TestFixtureLoader.h"

import core;

using namespace rendern;

namespace
{
	Skeleton MakingSingleBoneSkeleton()
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

	struct ConditionCase
	{
		const char* name;
		AnimationConditionDesc condition;
		AnimationParameterValue actualValue;
		bool setParameter;
		bool expected;
	};
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

TEST(AnimationController, JsonFixtureLoaderReadsMinimalConfigText)
{
	const std::string fixtureTest = LoadTextFixture("json/valid/minimal_config.json");
	ASSERT_FALSE(fixtureTest.empty());
	
	jsonUtils::JsonParser parser(fixtureTest);
	const jsonUtils::JsonValue root = parser.Parse();
	const auto& object = root.AsObject();
	
	EXPECT_EQ(jsonUtils::GetStringOpt(object, "name", ""), "minimal");
	EXPECT_TRUE(jsonUtils::GetBoolOpt(object, "enabled", false));
}

TEST(AnimationController, EvaluateConditionBoolIntFloatMatrix)
{
	const std::vector<ConditionCase> cases{
		{ "bool_true_equals_true", { .parameter = "b", .op = AnimationConditionOp::Equal, .value = { .type = AnimationParameterType::Bool, .boolValue = true } }, { .type = AnimationParameterType::Bool, .boolValue = true }, true, true },
		{ "bool_false_equals_false", { .parameter = "b", .op = AnimationConditionOp::Equal, .value = { .type = AnimationParameterType::Bool, .boolValue = false } }, { .type = AnimationParameterType::Bool, .boolValue = false }, true, true },
		{ "bool_true_not_equal_false", { .parameter = "b", .op = AnimationConditionOp::NotEqual, .value = { .type = AnimationParameterType::Bool, .boolValue = false } }, { .type = AnimationParameterType::Bool, .boolValue = true }, true, true },
		{ "bool_false_not_equal_true", { .parameter = "b", .op = AnimationConditionOp::NotEqual, .value = { .type = AnimationParameterType::Bool, .boolValue = true } }, { .type = AnimationParameterType::Bool, .boolValue = false }, true, true },
		{ "bool_equal_mismatch_fails", { .parameter = "b", .op = AnimationConditionOp::Equal, .value = { .type = AnimationParameterType::Bool, .boolValue = true } }, { .type = AnimationParameterType::Bool, .boolValue = false }, true, false },
		{ "bool_missing_parameter_fails", { .parameter = "missingBool", .op = AnimationConditionOp::IfTrue, .value = { .type = AnimationParameterType::Bool, .boolValue = true } }, { .type = AnimationParameterType::Bool, .boolValue = true }, false, false },
		{ "int_equal_negative", { .parameter = "i", .op = AnimationConditionOp::Equal, .value = { .type = AnimationParameterType::Int, .intValue = -3 } }, { .type = AnimationParameterType::Int, .intValue = -3 }, true, true },
		{ "int_not_equal_zero", { .parameter = "i", .op = AnimationConditionOp::NotEqual, .value = { .type = AnimationParameterType::Int, .intValue = 0 } }, { .type = AnimationParameterType::Int, .intValue = -1 }, true, true },
		{ "int_less_just_below", { .parameter = "i", .op = AnimationConditionOp::Less, .value = { .type = AnimationParameterType::Int, .intValue = 5 } }, { .type = AnimationParameterType::Int, .intValue = 4 }, true, true },
		{ "int_less_equal_at_boundary", { .parameter = "i", .op = AnimationConditionOp::LessEqual, .value = { .type = AnimationParameterType::Int, .intValue = 5 } }, { .type = AnimationParameterType::Int, .intValue = 5 }, true, true },
		{ "int_greater_just_above", { .parameter = "i", .op = AnimationConditionOp::Greater, .value = { .type = AnimationParameterType::Int, .intValue = 5 } }, { .type = AnimationParameterType::Int, .intValue = 6 }, true, true },
		{ "int_greater_equal_zero", { .parameter = "i", .op = AnimationConditionOp::GreaterEqual, .value = { .type = AnimationParameterType::Int, .intValue = 0 } }, { .type = AnimationParameterType::Int, .intValue = 0 }, true, true },
		{ "int_greater_equal_below_boundary_fails", { .parameter = "i", .op = AnimationConditionOp::GreaterEqual, .value = { .type = AnimationParameterType::Int, .intValue = 5 } }, { .type = AnimationParameterType::Int, .intValue = 4 }, true, false },
		{ "float_equal_at_tolerance_boundary", { .parameter = "f", .op = AnimationConditionOp::Equal, .value = { .type = AnimationParameterType::Float, .floatValue = 2.0f } }, { .type = AnimationParameterType::Float, .floatValue = 2.000001f }, true, true },
		{ "float_equal_outside_tolerance_fails", { .parameter = "f", .op = AnimationConditionOp::Equal, .value = { .type = AnimationParameterType::Float, .floatValue = 2.0f } }, { .type = AnimationParameterType::Float, .floatValue = 2.00001f }, true, false },
		{ "float_not_equal_outside_tolerance", { .parameter = "f", .op = AnimationConditionOp::NotEqual, .value = { .type = AnimationParameterType::Float, .floatValue = 2.0f } }, { .type = AnimationParameterType::Float, .floatValue = 2.00001f }, true, true },
		{ "float_less_negative", { .parameter = "f", .op = AnimationConditionOp::Less, .value = { .type = AnimationParameterType::Float, .floatValue = -1.0f } }, { .type = AnimationParameterType::Float, .floatValue = -1.1f }, true, true },
		{ "float_less_equal_at_boundary", { .parameter = "f", .op = AnimationConditionOp::LessEqual, .value = { .type = AnimationParameterType::Float, .floatValue = 1.5f } }, { .type = AnimationParameterType::Float, .floatValue = 1.5f }, true, true },
		{ "float_greater_just_above", { .parameter = "f", .op = AnimationConditionOp::Greater, .value = { .type = AnimationParameterType::Float, .floatValue = 1.5f } }, { .type = AnimationParameterType::Float, .floatValue = 1.5001f }, true, true },
		{ "float_greater_equal_zero", { .parameter = "f", .op = AnimationConditionOp::GreaterEqual, .value = { .type = AnimationParameterType::Float, .floatValue = 0.0f } }, { .type = AnimationParameterType::Float, .floatValue = 0.0f }, true, true },
		{ "float_less_fails_when_above", { .parameter = "f", .op = AnimationConditionOp::Less, .value = { .type = AnimationParameterType::Float, .floatValue = 1.5f } }, { .type = AnimationParameterType::Float, .floatValue = 1.5001f }, true, false }
	};
	
	for (const ConditionCase& testCase : cases)
	{
		SCOPED_TRACE(testCase.name);
		AnimationParameterStore store{};
		if (testCase.setParameter)
		{
			store.values[testCase.condition.parameter] = testCase.actualValue;
		}
		EXPECT_EQ(detail::EvaluateCondition(testCase.condition, store), testCase.expected);
	}
}

TEST(AnimationController, EvaluateConditionTriggeredAndRuntimeConsumesTrigger)
{
	AnimationConditionDesc attackTriggerCondition;
	attackTriggerCondition.parameter = "attack";
	attackTriggerCondition.op = AnimationConditionOp::Triggered;
	attackTriggerCondition.value.type = AnimationParameterType::Trigger;
	
	AnimationParameterStore store{};
	EXPECT_FALSE(detail::EvaluateCondition(attackTriggerCondition, store));
	
	FireAnimationTrigger(store, "attack");
	EXPECT_TRUE(detail::EvaluateCondition(attackTriggerCondition, store));
	
	FireAnimationTrigger(store, "other");
	EXPECT_TRUE(detail::EvaluateCondition(attackTriggerCondition, store));
	
	Skeleton skeleton = MakingSingleBoneSkeleton();
	std::vector<AnimationClip> clips{ MakeSingleBoneClip("Idle"), MakeSingleBoneClip("Attack") };
	std::vector<std::string> clipSourceAssetIds{ "hero", "hero" };
	
	AnimationControllerAsset asset{};
	asset.id = "trigger_runtime";
	asset.defaultState = "Idle";
	asset.states.push_back(AnimationStateDesc{ .name = "Idle", .clipName = "Idle" });
	asset.states.push_back(AnimationStateDesc{ .name = "Attack", .clipName = "Attack" });
	asset.transitions.push_back(AnimationTransitionDesc{
		.fromState = "Idle",
		.toState = "Attack",
		.priority = 1,
		.conditions = { attackTriggerCondition }
	});

	AnimationControllerRuntime runtime{};
	BindAnimationControllerStateMachine(runtime, skeleton, clips, clipSourceAssetIds, asset, true, false, false);
	AnimatorState animator{};

	EXPECT_EQ(runtime.currentStateName, "Idle");
	FireAnimationTrigger(runtime.parameters, "attack");
	UpdateAnimationControllerRuntime(runtime, animator, 0.016f);
	EXPECT_EQ(runtime.currentStateName, "Attack");
	ASSERT_NE(FindAnimationParameter(runtime.parameters, "attack"), nullptr);
	EXPECT_FALSE(FindAnimationParameter(runtime.parameters, "attack")->triggerValue);
	EXPECT_FALSE(detail::EvaluateCondition(attackTriggerCondition, runtime.parameters));
}