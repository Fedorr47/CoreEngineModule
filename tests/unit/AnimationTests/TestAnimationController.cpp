#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <initializer_list>
#include <cmath>
#include <unordered_map>

#include "TestSupport/TestFixtureLoader.h"
#include "TestSupport/TestTempPath.h"
#include "AnimationTestHelpers.h"

import core;

using namespace rendern;

namespace
{
	struct ConditionCase
	{
		const char* name;
		AnimationConditionDesc condition;
		AnimationParameterValue actualValue;
		bool setParameter;
		bool expected;
	};
	
	void WriteTestAssetFile(
		const std::filesystem::path& root, 
		std::string_view relativePath, 
		std::string_view content)
	{
		const std::filesystem::path target = root / std::filesystem::path(relativePath);
		std::filesystem::create_directories(target.parent_path());
		std::ofstream out(target, std::ios::binary);
		if (!out.is_open())
		{
			throw std::runtime_error("Failed to write test fixture: " + target.string());
		}
		out << content;
	}

	void ExpectLoadLevelThrowsWithMessageFragment(
		std::string_view levelPath, 
		std::string_view fragment)
	{
		try
		{
			LoadLevelAssetFromJson(levelPath);
			FAIL() << "Expected std::runtime_error for level: " << levelPath;
		}
		catch (const std::runtime_error& ex)
		{
			EXPECT_NE(std::string(ex.what()).find(fragment), std::string::npos)
				<< "missing fragment: '" << fragment << "'\nactual: '" << ex.what() << "'";
		}
	}
	
	template<typename Fn>
	void ExpectRuntimeErrorFragments(Fn&& fn, std::initializer_list<std::string_view> fragments)
	{
		try
		{
			fn();
			FAIL() << "Expected std::runtime_error";
		}
		catch (const std::runtime_error& error)
		{
			const std::string message = error.what();
			for (const std::string_view fragment : fragments)
			{
				EXPECT_NE(message.find(fragment), std::string::npos)
					<< "missing fragment: '" << fragment << "' in error: " << message;
			}
		}
	}
}

TEST(AnimationController, ParameterStoreSupportsSetTriggerConsumeAndReset)
{
	AnimationParameterStore store{};

	// Covers all parameter write paths used by animation controllers:
	// persistent bool/int/float values and one-shot trigger values.
	SetAnimationParameter(store, "isMoving", true);
	SetAnimationParameter(store, "combo", 2);
	SetAnimationParameter(store, "speed", 3.5f);
	FireAnimationTrigger(store, "attack");

	ASSERT_NE(FindAnimationParameter(store, "isMoving"), nullptr);
	EXPECT_EQ(FindAnimationParameter(store, "isMoving")->type, AnimationParameterType::Bool);
	EXPECT_EQ(FindAnimationParameter(store, "combo")->type, AnimationParameterType::Int);
	EXPECT_EQ(FindAnimationParameter(store, "speed")->type, AnimationParameterType::Float);
	EXPECT_EQ(FindAnimationParameter(store, "attack")->type, AnimationParameterType::Trigger);

	// Trigger is event-like: first consume succeeds and clears it,
	// second consume fails because the trigger is no longer active.
	EXPECT_TRUE(ConsumeAnimationTrigger(store, "attack"));
	EXPECT_FALSE(ConsumeAnimationTrigger(store, "attack"));

	// Explicit reset should clear a fired trigger before it can be consumed.
	FireAnimationTrigger(store, "attack");
	ResetAnimationTrigger(store, "attack");
	EXPECT_FALSE(ConsumeAnimationTrigger(store, "attack"));

	// Full reset is used when rebinding/reinitializing controller parameters.
	ResetAnimationParameters(store);
	EXPECT_TRUE(store.values.empty());
}

TEST(AnimationController, ArbitrarySemanticMotionResolvesToConcreteClipAtBindTime)
{
	Skeleton skeleton{};
	std::vector<AnimationClip> clips(2);
	clips[0].name = "OtherClip";
	clips[1].name = "TestClip";
	const std::vector<std::string> sourceIds{ "other_source", "test_source" };
	AnimationControllerAsset controller{ .id = "TestController", .defaultState = "SemanticState" };
	controller.states.push_back(AnimationStateDesc{
		.name = "SemanticState", .motionId = MotionId{ "Test.CustomMotion" } });
	AnimationProfileAsset profile{ .id = "TestProfile" };
	profile.motions.emplace("Test.CustomMotion", AnimationClipRef{ "test_source", "TestClip" });
	AnimationControllerRuntime runtime{};

	BindAnimationControllerStateMachine(runtime, skeleton, clips, sourceIds, controller, &profile, true, false, false);

	ASSERT_EQ(runtime.resolvedStateClipIndices.size(), 1u);
	EXPECT_EQ(runtime.resolvedStateClipIndices[0], 1);
	EXPECT_EQ(runtime.currentStateName, "SemanticState");
}

TEST(AnimationController, SemanticBindingReportsMissingMotionAndAmbiguousReferences)
{
	Skeleton skeleton{};
	std::vector<AnimationClip> clips(1);
	clips[0].name = "TestClip";
	const std::vector<std::string> sourceIds{ "test_source" };
	AnimationProfileAsset profile{ .id = "TestProfile" };
	AnimationControllerRuntime runtime{};
	AnimationControllerAsset missing{ .id = "TestController", .defaultState = "SemanticState" };
	missing.states.push_back(AnimationStateDesc{
		.name = "SemanticState", .motionId = MotionId{ "Test.MissingMotion" } });
	ExpectRuntimeErrorFragments([&]
		{
			BindAnimationControllerStateMachine(runtime, skeleton, clips, sourceIds, missing, &profile, true, false, false);
		}, { "TestController", "SemanticState", "Test.MissingMotion", "TestProfile" });

	AnimationControllerAsset ambiguous{ .id = "TestController", .defaultState = "SemanticState" };
	ambiguous.states.push_back(AnimationStateDesc{
		.name = "SemanticState", .motionId = MotionId{ "Test.CustomMotion" }, .clipName = "TestClip" });
	profile.motions.emplace("Test.CustomMotion", AnimationClipRef{ "test_source", "TestClip" });
	ExpectRuntimeErrorFragments([&]
		{
			BindAnimationControllerStateMachine(runtime, skeleton, clips, sourceIds, ambiguous, &profile, true, false, false);
		}, { "TestController", "SemanticState", "Test.CustomMotion", "direct clip" });
}

TEST(AnimationController, SemanticBindingRequiresProfileWithActionableDiagnostic)
{
	Skeleton skeleton{};
	std::vector<AnimationClip> clips(1);
	clips[0].name = "TestClip";
	const std::vector<std::string> sourceIds{ "test_source" };
	AnimationControllerAsset controller{ .id = "TestController", .defaultState = "SemanticState" };
	controller.states.push_back(AnimationStateDesc{
		.name = "SemanticState", .motionId = MotionId{ "Test.CustomMotion" } });
	AnimationControllerRuntime runtime{};
	ExpectRuntimeErrorFragments([&]
		{
			BindAnimationControllerStateMachine(runtime, skeleton, clips, sourceIds, controller, nullptr, true, false, false);
		}, { "TestController", "SemanticState", "Test.CustomMotion", "no animation profile" });
}

TEST(AnimationController, SemanticBindingRejectsMissingExplicitClipInSelectedSource)
{
	Skeleton skeleton{};
	std::vector<AnimationClip> clips(1);
	clips[0].name = "AvailableClip";
	const std::vector<std::string> sourceIds{ "test_source" };
	AnimationControllerAsset controller{ .id = "TestController", .defaultState = "SemanticState" };
	controller.states.push_back(AnimationStateDesc{
		.name = "SemanticState", .motionId = MotionId{ "Test.CustomMotion" } });
	AnimationProfileAsset profile{ .id = "TestProfile" };
	profile.motions.emplace("Test.CustomMotion", AnimationClipRef{ "test_source", "MissingClip" });
	AnimationControllerRuntime runtime{};
	ExpectRuntimeErrorFragments([&]
		{
			BindAnimationControllerStateMachine(runtime, skeleton, clips, sourceIds, controller, &profile, true, false, false);
		}, { "TestProfile", "Test.CustomMotion", "MissingClip", "test_source" });
}

TEST(AnimationController, SemanticBindingRejectsMissingSource)
{
	Skeleton skeleton{};
	std::vector<AnimationClip> clips(1);
	clips[0].name = "TestClip";
	const std::vector<std::string> sourceIds{ "available_source" };
	AnimationControllerAsset controller{ .id = "TestController", .defaultState = "SemanticState" };
	controller.states.push_back(AnimationStateDesc{
		.name = "SemanticState", .motionId = MotionId{ "Test.CustomMotion" } });
	AnimationProfileAsset profile{ .id = "TestProfile" };
	profile.motions.emplace("Test.CustomMotion", AnimationClipRef{ "missing_source", "TestClip" });
	AnimationControllerRuntime runtime{};
	ExpectRuntimeErrorFragments([&]
		{
			BindAnimationControllerStateMachine(runtime, skeleton, clips, sourceIds, controller, &profile, true, false, false);
		}, { "TestProfile", "Test.CustomMotion", "missing_source" });
}

TEST(AnimationController, SemanticBindingRejectsUnsupportedBlendStates)
{
	Skeleton skeleton{};
	std::vector<AnimationClip> clips(1);
	clips[0].name = "TestClip";
	const std::vector<std::string> sourceIds{ "test_source" };
	AnimationProfileAsset profile{ .id = "TestProfile" };
	profile.motions.emplace("Test.CustomMotion", AnimationClipRef{ "test_source", "TestClip" });

	for (const bool useBlend2D : { false, true })
	{
		AnimationControllerAsset controller{ .id = "TestController", .defaultState = "SemanticState" };
		AnimationStateDesc state{ .name = "SemanticState", .motionId = MotionId{ "Test.CustomMotion" } };
		if (useBlend2D)
		{
			state.blend2D.push_back(AnimationBlend2DPoint{ .clipName = "TestClip" });
		}
		else
		{
			state.blend1D.push_back(AnimationBlend1DPoint{ .clipName = "TestClip" });
		}
		controller.states.push_back(std::move(state));
		AnimationControllerRuntime runtime{};
		ExpectRuntimeErrorFragments([&]
			{
				BindAnimationControllerStateMachine(runtime, skeleton, clips, sourceIds, controller, &profile, true, false, false);
			}, { "TestController", "SemanticState", "Test.CustomMotion", useBlend2D ? "Blend2D" : "Blend1D",
				"not supported" });
	}
}

TEST(AnimationController, SemanticAndDirectBindingsHaveEquivalentFsmBehavior)
{
	Skeleton skeleton = animationTest::MakeSingleBoneSkeleton();
	std::vector<AnimationClip> clips{ animationTest::MakeMinimalSingleBoneClip("IdleClip"),
		animationTest::MakeMinimalSingleBoneClip("MoveClip") };
	const std::vector<std::string> sourceIds{ "test_source", "test_source" };
	auto makeController = [](bool semantic)
		{
			AnimationControllerAsset controller{ .id = semantic ? "SemanticController" : "DirectController",
				.defaultState = "Idle" };
			controller.parameters.push_back(AnimationParameterDesc{ .name = "moving",
				.defaultValue = AnimationParameterValue{ .type = AnimationParameterType::Bool, .boolValue = false } });
			controller.states.push_back(semantic
				? AnimationStateDesc{ .name = "Idle", .motionId = MotionId{ "Test.Idle" }, .looping = true, .playRate = 0.8f }
				: AnimationStateDesc{ .name = "Idle", .clipName = "IdleClip", .clipSourceAssetId = "test_source", .looping = true, .playRate = 0.8f });
			controller.states.push_back(semantic
				? AnimationStateDesc{ .name = "Move", .motionId = MotionId{ "Test.Move" }, .looping = false, .playRate = 1.25f }
				: AnimationStateDesc{ .name = "Move", .clipName = "MoveClip", .clipSourceAssetId = "test_source", .looping = false, .playRate = 1.25f });
			controller.transitions.push_back(AnimationTransitionDesc{ .fromState = "Idle", .toState = "Move",
				.blendDurationSeconds = 0.0f, .priority = 7,
				.conditions = { AnimationConditionDesc{ .parameter = "moving", .op = AnimationConditionOp::IfTrue } } });
			return controller;
		};
	AnimationControllerAsset direct = makeController(false);
	AnimationControllerAsset semantic = makeController(true);
	AnimationProfileAsset profile{ .id = "TestProfile" };
	profile.motions.emplace("Test.Idle", AnimationClipRef{ "test_source", "IdleClip" });
	profile.motions.emplace("Test.Move", AnimationClipRef{ "test_source", "MoveClip" });
	AnimationControllerRuntime directRuntime{};
	AnimationControllerRuntime semanticRuntime{};
	BindAnimationControllerStateMachine(directRuntime, skeleton, clips, sourceIds, direct, true, false, false);
	BindAnimationControllerStateMachine(semanticRuntime, skeleton, clips, sourceIds, semantic, &profile, true, false, false);
	EXPECT_EQ(directRuntime.currentStateName, semanticRuntime.currentStateName);

	SetAnimationParameter(directRuntime.parameters, "moving", true);
	SetAnimationParameter(semanticRuntime.parameters, "moving", true);
	AnimatorState directAnimator{};
	AnimatorState semanticAnimator{};
	UpdateAnimationControllerRuntime(directRuntime, directAnimator, 0.016f);
	UpdateAnimationControllerRuntime(semanticRuntime, semanticAnimator, 0.016f);
	EXPECT_EQ(directRuntime.currentStateName, "Move");
	EXPECT_EQ(directRuntime.currentStateName, semanticRuntime.currentStateName);
	EXPECT_EQ(directRuntime.looping, semanticRuntime.looping);
	EXPECT_FLOAT_EQ(directRuntime.playRate, semanticRuntime.playRate);
	EXPECT_EQ(directRuntime.debugLastTransitionSelection, semanticRuntime.debugLastTransitionSelection);
}

TEST(AnimationController, SemanticProfileAndNodeBindingSurviveLevelRoundTrip)
{
	test::ScopedTempPath tempPath{ test::MakeUniqueTempPath("semantic_animation_round_trip") };
	const std::filesystem::path path = tempPath.Path() / "semantic.level.json";
	LevelAsset source{};
	source.name = "SemanticRoundTrip";
	AnimationControllerAsset controller{ .id = "TestController", .defaultState = "SemanticState" };
	controller.states.push_back(AnimationStateDesc{
		.name = "SemanticState", .motionId = MotionId{ "Test.CustomMotion" } });
	source.animationControllers.emplace(controller.id, controller);
	AnimationProfileAsset profile{ .id = "TestProfile" };
	profile.motions.emplace("Test.CustomMotion", AnimationClipRef{ "test_external_source", "TestClip" });
	source.animationProfiles.emplace(profile.id, profile);
	source.nodes.push_back(LevelNode{ .name = "Character", .animationController = "TestController",
		.animationProfile = "TestProfile" });

	SaveLevelAssetToJson(path.string(), source);
	const LevelAsset reloaded = LoadLevelAssetFromJson(path.string());
	ASSERT_TRUE(reloaded.animationControllers.contains("TestController"));
	ASSERT_TRUE(reloaded.animationProfiles.contains("TestProfile"));
	ASSERT_EQ(reloaded.nodes.size(), 1u);
	EXPECT_EQ(reloaded.animationControllers.at("TestController").states[0].motionId.value, "Test.CustomMotion");
	EXPECT_EQ(reloaded.nodes[0].animationProfile, "TestProfile");
	const AnimationClipRef& binding = reloaded.animationProfiles.at("TestProfile").motions.at("Test.CustomMotion");
	EXPECT_EQ(binding.sourceAssetId, "test_external_source");
	EXPECT_EQ(binding.clipName, "TestClip");
}

TEST(AnimationController, ExternalAnimationProfileRoundTripAndSafeValidation)
{
	test::ScopedTempPath tempPath{ test::MakeUniqueTempPath("external_animation_profile") };
	const std::filesystem::path path = tempPath.Path() / "test.animationprofile.json";
	AnimationProfileAsset profile{ .id = "TestProfile" };
	profile.motions.emplace("Test.CustomMotion", AnimationClipRef{ "source_A", "ClipA" });

	SaveAnimationProfileAssetToJson(path.string(), profile);
	const AnimationProfileAsset loaded = LoadAnimationProfileAssetFromJson(path.string(), "TestProfile");
	ASSERT_TRUE(loaded.motions.contains("Test.CustomMotion"));
	EXPECT_EQ(loaded.motions.at("Test.CustomMotion").sourceAssetId, "source_A");
	EXPECT_EQ(loaded.motions.at("Test.CustomMotion").clipName, "ClipA");

	{
		std::ofstream original(path, std::ios::binary | std::ios::trunc);
		ASSERT_TRUE(original.is_open());
		original << "ORIGINAL_CONTENT";
	}
	profile.motions.at("Test.CustomMotion").sourceAssetId.clear();
	EXPECT_THROW(SaveAnimationProfileAssetToJson(path.string(), profile), std::runtime_error);
	std::ifstream preserved(path, std::ios::binary);
	ASSERT_TRUE(preserved.is_open());
	std::string content;
	preserved >> content;
	EXPECT_EQ(content, "ORIGINAL_CONTENT");
}

TEST(AnimationWorkspace, RequiredMotionIdsAreUniqueAndExcludeLegacyStates)
{
	AnimationControllerAsset controller{};
	controller.states = {
		AnimationStateDesc{ .name = "StateA", .motionId = MotionId{ "Test.MotionA" } },
		AnimationStateDesc{ .name = "StateB", .motionId = MotionId{ "Test.MotionB" } },
		AnimationStateDesc{ .name = "StateC", .motionId = MotionId{ "Test.MotionA" } },
		AnimationStateDesc{ .name = "LegacyState", .clipName = "ClipA", .clipSourceAssetId = "source_A" }
	};
	EXPECT_EQ(CollectAnimationWorkspaceRequiredMotionIds(controller),
		(std::vector<std::string>{ "Test.MotionA", "Test.MotionB" }));
}

TEST(AnimationWorkspace, AuthoredBindingRemainsSeparateFromBoundRuntimeUntilRebind)
{
	AnimationControllerAsset controller{ .id = "Controller", .defaultState = "Semantic" };
	controller.states.push_back(AnimationStateDesc{ .name = "Semantic", .motionId = MotionId{ "Test.CustomMotion" } });
	AnimationProfileAsset profile{ .id = "Profile" };
	profile.motions.emplace("Test.CustomMotion", AnimationClipRef{ "source_A", "ClipA" });
	const std::vector<AnimationClip> clips{ AnimationClip{ .name = "ClipA" }, AnimationClip{ .name = "ClipB" } };
	const std::vector<std::string> sources{ "source_A", "source_B" };
	AnimationControllerRuntime runtime{};
	runtime.resolvedStateClipIndices = { 0 };

	profile.motions.at("Test.CustomMotion") = AnimationClipRef{ "source_B", "ClipB" };
	auto resolution = BuildAnimationWorkspaceStateResolution(
		controller, controller.states[0], &profile, &runtime, clips, sources);
	EXPECT_EQ(resolution.authoredSourceAssetId, "source_B");
	EXPECT_EQ(resolution.authoredClipName, "ClipB");
	EXPECT_EQ(resolution.boundClipIndex, 0);
	EXPECT_EQ(resolution.boundClipName, "ClipA");
	EXPECT_TRUE(resolution.reloadRequired);

	runtime.resolvedStateClipIndices = { 1 };
	resolution = BuildAnimationWorkspaceStateResolution(
		controller, controller.states[0], &profile, &runtime, clips, sources);
	EXPECT_EQ(resolution.boundClipName, "ClipB");
	EXPECT_FALSE(resolution.reloadRequired);
}

TEST(AnimationWorkspace, LegacyResolutionDoesNotRequireProfile)
{
	AnimationControllerAsset controller{ .id = "Controller", .defaultState = "Legacy" };
	controller.states.push_back(AnimationStateDesc{ .name = "Legacy", .clipName = "ClipA", .clipSourceAssetId = "source_A" });
	AnimationControllerRuntime runtime{};
	runtime.resolvedStateClipIndices = { 0 };
	const auto resolution = BuildAnimationWorkspaceStateResolution(controller, controller.states[0], nullptr, &runtime,
		std::vector<AnimationClip>{ AnimationClip{ .name = "ClipA" } }, std::vector<std::string>{ "source_A" });
	EXPECT_EQ(resolution.contentMode, AnimationWorkspaceContentMode::LegacyDirect);
	EXPECT_EQ(resolution.boundClipIndex, 0);
	EXPECT_EQ(resolution.boundClipName, "ClipA");
}

TEST(AnimationWorkspace, MappingStatusesAreNonThrowingAndSourceAware)
{
	AnimationProfileAsset profile{ .id = "Profile" };
	const std::vector<std::string> registered{ "source_A", "source_B" };
	const std::vector<AnimationClip> clips{ AnimationClip{ .name = "ClipA" } };
	const std::vector<std::string> sources{ "source_A" };
	EXPECT_EQ(EvaluateAnimationWorkspaceMappingStatus(&profile, "Test.Missing", registered, clips, sources), AnimationWorkspaceMappingStatus::MissingMapping);
	profile.motions["Test.Motion"] = AnimationClipRef{ "missing", "ClipA" };
	EXPECT_EQ(EvaluateAnimationWorkspaceMappingStatus(&profile, "Test.Motion", registered, clips, sources), AnimationWorkspaceMappingStatus::MissingSource);
	profile.motions["Test.Motion"] = AnimationClipRef{ "source_A", "MissingClip" };
	EXPECT_EQ(EvaluateAnimationWorkspaceMappingStatus(&profile, "Test.Motion", registered, clips, sources), AnimationWorkspaceMappingStatus::MissingExplicitClip);
	profile.motions["Test.Motion"] = AnimationClipRef{ "source_B", "ClipB" };
	EXPECT_EQ(EvaluateAnimationWorkspaceMappingStatus(&profile, "Test.Motion", registered, clips, sources), AnimationWorkspaceMappingStatus::SourceNotLoaded);
	profile.motions["Test.Motion"] = AnimationClipRef{ "source_A", "ClipA" };
	EXPECT_EQ(EvaluateAnimationWorkspaceMappingStatus(&profile, "Test.Motion", registered, clips, sources), AnimationWorkspaceMappingStatus::Ok);
}

TEST(AnimationWorkspace, ProfileEditorStateAndRootTrajectoryAreDeterministic)
{
	std::unordered_map<std::string, AnimationProfileEditorState> states;
	states["human"].dirty = true;
	states["robot"].dirty = true;
	states["human"].dirty = false;
	EXPECT_FALSE(states["human"].dirty);
	EXPECT_TRUE(states["robot"].dirty);

	AnimationClip clip{ .name = "Synthetic", .durationTicks = 10.0f, .ticksPerSecond = 1.0f };
	BoneAnimationChannel root{ .boneName = "ExplicitRoot" };
	root.translationKeys = {
		TranslationKey{ 0.0f, mathUtils::Vec3(0.0f, 0.0f, 0.0f) },
		TranslationKey{ 1.0f, mathUtils::Vec3(0.2f, 0.1f, 0.4f) },
		TranslationKey{ 9.0f, mathUtils::Vec3(1.8f, 0.9f, 3.6f) },
		TranslationKey{ 10.0f, mathUtils::Vec3(2.0f, 1.0f, 4.0f) }
	};
	clip.channels.push_back(std::move(root));
	const auto trajectory = BuildAnimationRootTrajectoryDiagnostics(clip, "ExplicitRoot", 3);
	ASSERT_TRUE(trajectory.available);
	EXPECT_FLOAT_EQ(trajectory.first.x, 0.0f);
	EXPECT_FLOAT_EQ(trajectory.last.x, 2.0f);
	EXPECT_FLOAT_EQ(trajectory.last.y, 1.0f);
	EXPECT_FLOAT_EQ(trajectory.last.z, 4.0f);
	EXPECT_FLOAT_EQ(trajectory.delta.x, 2.0f);
	EXPECT_FLOAT_EQ(trajectory.delta.y, 1.0f);
	EXPECT_FLOAT_EQ(trajectory.delta.z, 4.0f);
	EXPECT_FLOAT_EQ(trajectory.horizontalDisplacement, std::sqrt(20.0f));
	EXPECT_FLOAT_EQ(trajectory.verticalDisplacement, 1.0f);
	ASSERT_EQ(trajectory.sampledPoints.size(), 3u);
	EXPECT_FLOAT_EQ(trajectory.sampledPoints[1].x, 1.0f);
	EXPECT_FLOAT_EQ(trajectory.sampledPoints[1].y, 0.5f);
	EXPECT_FLOAT_EQ(trajectory.sampledPoints[1].z, 2.0f);
}

TEST(AnimationController, ProfileBindingShapeAndMixedStateSerializationAreValidated)
{
	const std::filesystem::path assetRoot = corefs::FindAssetRoot();
	WriteTestAssetFile(assetRoot, "tests/animation_validation/profile_binding_shape.animationprofile.json",
		R"json({"motions":{"Test.CustomMotion":"test_source"}})json");
	WriteTestAssetFile(assetRoot, "tests/animation_validation/profile_binding_shape.level.json",
		R"json({"animationProfileAssets":{"TestProfile":{"path":"tests/animation_validation/profile_binding_shape.animationprofile.json"}}})json");
	ExpectLoadLevelThrowsWithMessageFragment("tests/animation_validation/profile_binding_shape.level.json",
		"Animation profile JSON: tests/animation_validation/profile_binding_shape.animationprofile.json.motions.Test.CustomMotion must be object");

	test::ScopedTempPath tempPath{ test::MakeUniqueTempPath("semantic_animation_invalid_save") };
	LevelAsset invalid{};
	AnimationControllerAsset controller{ .id = "TestController", .defaultState = "SemanticState" };
	controller.states.push_back(AnimationStateDesc{ .name = "SemanticState",
		.motionId = MotionId{ "Test.CustomMotion" }, .blend1D = { AnimationBlend1DPoint{ .clipName = "TestClip" } } });
	invalid.animationControllers.emplace(controller.id, controller);
	const std::filesystem::path invalidPath = tempPath.Path() / "invalid.level.json";
	std::filesystem::create_directories(invalidPath.parent_path());
	{
		std::ofstream original(invalidPath, std::ios::binary | std::ios::trunc);
		ASSERT_TRUE(original.is_open());
		original << "ORIGINAL_CONTENT";
	}
	ExpectRuntimeErrorFragments([&]
		{
			SaveLevelAssetToJson(invalidPath.string(), invalid);
		}, { "TestController", "SemanticState", "Test.CustomMotion", "Blend1D" });
	std::ifstream preservedInput(invalidPath, std::ios::binary);
	ASSERT_TRUE(preservedInput.is_open());
	std::string preservedContent;
	preservedInput >> preservedContent;
	EXPECT_EQ(preservedContent, "ORIGINAL_CONTENT");
}

TEST(AnimationController, StateLookupAndTagsWork)
{
	AnimationControllerAsset asset{};
	
	// Tags are lightweight semantic labels used by gameplay/debug logic.
	// They should not affect state lookup by name.
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
	Skeleton skeleton = animationTest::MakeSingleBoneSkeleton();
	
	std::vector<AnimationClip> clips{};
	clips.push_back(animationTest::MakeMinimalSingleBoneClip("Idle"));
	clips.push_back(animationTest::MakeMinimalSingleBoneClip("Run"));
	
	// One source id per clip. In this test both clips come from the same logical source.
	std::vector<std::string> clipSourceAssetIds{ "hero", "hero" };

	AnimationControllerAsset asset{};
	asset.id = "hero_controller";
	asset.defaultState = "Run";
	
	// Default parameters are copied into runtime on bind.
	asset.parameters.push_back(AnimationParameterDesc{
		.name = "isMoving",
		.defaultValue = AnimationParameterValue{ .type = AnimationParameterType::Bool, .boolValue = true }
	});
	
	// Default state is Run, so runtime should start from state index 1
	// and inherit Run state's playback settings.
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

	// Request does not immediately switch state. It only stores intent;
	// actual transition/request handling happens during controller update.
	RequestAnimationControllerState(runtime, "Idle");
	EXPECT_EQ(runtime.requestedStateName, "Idle");
}

TEST(AnimationController, JsonFixtureLoaderReadsMinimalConfigText)
{
	const std::string fixtureText = LoadTextFixture("json/valid/minimal_config.json");
	ASSERT_FALSE(fixtureText.empty());
	
	jsonUtils::JsonParser parser(fixtureText);
	const jsonUtils::JsonValue root = parser.Parse();
	const auto& object = root.AsObject();
	
	EXPECT_EQ(jsonUtils::GetStringOpt(object, "name", ""), "minimal");
	EXPECT_TRUE(jsonUtils::GetBoolOpt(object, "enabled", false));
}

TEST(AnimationController, EvaluateConditionBoolIntFloatMatrix)
{
	// Table-driven coverage for condition evaluation.
	// This tests the pure condition checker without running the full controller state machine.
	//
	// The matrix intentionally covers:
	// - bool equality/inequality;
	// - missing parameter behavior;
	// - int comparison boundaries;
	// - float epsilon equality;
	// - negative values and boundary comparisons.
	
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
		
		// Some cases intentionally leave the parameter missing to verify
		// that missing parameters fail instead of silently passing.
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
	
	// First check the pure condition path:
	// missing trigger fails, fired matching trigger passes.
	AnimationParameterStore store{};
	EXPECT_FALSE(detail::EvaluateCondition(attackTriggerCondition, store));
	
	FireAnimationTrigger(store, "attack");
	EXPECT_TRUE(detail::EvaluateCondition(attackTriggerCondition, store));
	
	// Firing an unrelated trigger must not affect the already active "attack" condition.
	FireAnimationTrigger(store, "other");
	EXPECT_TRUE(detail::EvaluateCondition(attackTriggerCondition, store));
	
	Skeleton skeleton = animationTest::MakeSingleBoneSkeleton();
	std::vector<AnimationClip> clips{ animationTest::MakeMinimalSingleBoneClip("Idle"), 
		animationTest::MakeMinimalSingleBoneClip("Attack") };
	std::vector<std::string> clipSourceAssetIds{ "hero", "hero" };
	
	AnimationControllerAsset asset{};
	asset.id = "trigger_runtime";
	asset.defaultState = "Idle";
	asset.states.push_back(AnimationStateDesc{ .name = "Idle", .clipName = "Idle" });
	asset.states.push_back(AnimationStateDesc{ .name = "Attack", .clipName = "Attack" });
	
	// Runtime transition uses the same trigger condition.
	// This verifies not only that the condition passes, but also that
	// the selected transition consumes the trigger.
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
	
	// Important policy check:
	// a trigger consumed by a selected transition must be reset,
	// so it cannot repeatedly fire the same transition.
	ASSERT_NE(FindAnimationParameter(runtime.parameters, "attack"), nullptr);
	EXPECT_FALSE(FindAnimationParameter(runtime.parameters, "attack")->triggerValue);
	EXPECT_FALSE(detail::EvaluateCondition(attackTriggerCondition, runtime.parameters));
}

TEST(AnimationController, DefaultStateFallsBackToFirstStateWhenMissingAndRejectsInvalidDefault)
{
	Skeleton skeleton = animationTest::MakeSingleBoneSkeleton();
	
	std::vector<AnimationClip> clips{ animationTest::MakeMinimalSingleBoneClip("Idle"), 
		animationTest::MakeMinimalSingleBoneClip("Run") };
	std::vector<std::string> clipSourceAssetIds{ "hero", "hero" };

	AnimationControllerAsset missingDefault{};
	missingDefault.id = "missing_default";
	missingDefault.states.push_back(AnimationStateDesc{ .name = "Idle", .clipName = "Idle" });
	missingDefault.states.push_back(AnimationStateDesc{ .name = "Run", .clipName = "Run" });

	// If defaultState is not specified, binding should choose the first declared state.
	// This keeps minimal controllers usable without requiring explicit defaultState.
	AnimationControllerRuntime runtimeMissing{};
	BindAnimationControllerStateMachine(runtimeMissing, skeleton, clips, clipSourceAssetIds, missingDefault, true, false, false);
	EXPECT_EQ(runtimeMissing.currentStateIndex, 0);
	EXPECT_EQ(runtimeMissing.currentStateName, "Idle");
	
	AnimationControllerAsset invalidDefault = missingDefault;
	invalidDefault.id = "invalid_default";
	invalidDefault.defaultState = "NoSuchState";

	// If defaultState is specified but points to a missing state, this is treated
	// as invalid controller data. Runtime must not silently fallback to Idle,
	// because that could hide broken JSON/controller authoring.
	AnimationControllerRuntime runtimeInvalid{};
	BindAnimationControllerStateMachine(runtimeInvalid, skeleton, clips, clipSourceAssetIds, invalidDefault, true, false, false);
	EXPECT_EQ(runtimeInvalid.currentStateIndex, -1);
	EXPECT_TRUE(runtimeInvalid.currentStateName.empty());

	// Updating an invalidly bound controller should keep it invalid instead of
	// recovering implicitly to the first state.
	AnimatorState animator{};
	UpdateAnimationControllerRuntime(runtimeInvalid, animator, 0.0f);
	EXPECT_EQ(runtimeInvalid.currentStateIndex, -1);
	EXPECT_TRUE(runtimeInvalid.currentStateName.empty());
}

TEST(AnimationController, TriggerTransitionRequiresAllConditionsAndDoesNotConsumeOnFailure)
{
	Skeleton skeleton = animationTest::MakeSingleBoneSkeleton();
	std::vector<AnimationClip> clips{ animationTest::MakeMinimalSingleBoneClip("Idle"), 
		animationTest::MakeMinimalSingleBoneClip("Attack") };
	std::vector<std::string> clipSourceAssetIds{ "hero", "hero" };

	AnimationControllerAsset asset{};
	asset.id = "trigger_gate";
	asset.defaultState = "Idle";
	asset.states.push_back(AnimationStateDesc{ .name = "Idle", .clipName = "Idle" });
	asset.states.push_back(AnimationStateDesc{ .name = "Attack", .clipName = "Attack" });
	
	// Attack transition requires both:
	// 1. a one-shot trigger request;
	// 2. a persistent gameplay gate saying the action is currently allowed.
	asset.transitions.push_back(AnimationTransitionDesc{
		.fromState = "Idle",
		.toState = "Attack",
		.priority = 1,
		.conditions = {
			AnimationConditionDesc{
				.parameter = "attack",
				.op = AnimationConditionOp::Triggered,
				.value = AnimationParameterValue{ .type = AnimationParameterType::Trigger }
			},
			AnimationConditionDesc{
				.parameter = "canAttack",
				.op = AnimationConditionOp::Equal,
				.value = AnimationParameterValue{ .type = AnimationParameterType::Bool, .boolValue = true }
			}
		}
	});

	AnimationControllerRuntime runtime{};
	BindAnimationControllerStateMachine(runtime, skeleton, clips, clipSourceAssetIds, asset, true, false, false);
	AnimatorState animator{};

	// Trigger is fired, but the gate condition blocks the transition.
	// Current policy: a trigger is consumed only by the selected matched transition,
	// so a failed transition attempt must leave the trigger active.
	SetAnimationParameter(runtime.parameters, "canAttack", false);
	FireAnimationTrigger(runtime.parameters, "attack");
	UpdateAnimationControllerRuntime(runtime, animator, 0.016f);
	EXPECT_EQ(runtime.currentStateName, "Idle");
	ASSERT_NE(FindAnimationParameter(runtime.parameters, "attack"), nullptr);
	EXPECT_TRUE(FindAnimationParameter(runtime.parameters, "attack")->triggerValue);

	// Once the gate opens, the still-buffered trigger should allow the transition.
	// After the transition is selected, the trigger must be consumed/reset.
	SetAnimationParameter(runtime.parameters, "canAttack", true);
	UpdateAnimationControllerRuntime(runtime, animator, 0.016f);
	EXPECT_EQ(runtime.currentStateName, "Attack");
	ASSERT_NE(FindAnimationParameter(runtime.parameters, "attack"), nullptr);
	EXPECT_FALSE(FindAnimationParameter(runtime.parameters, "attack")->triggerValue);
}

TEST(AnimationController, TransitionBlendDurationRespectsZeroAndPositiveValues)
{
	Skeleton skeleton = animationTest::MakeSingleBoneSkeleton();
	std::vector<AnimationClip> clips{ animationTest::MakeMinimalSingleBoneClip("Idle"), 
		animationTest::MakeMinimalSingleBoneClip("Run") };
	std::vector<std::string> clipSourceAssetIds{ "hero", "hero" };

	AnimationControllerAsset asset{};
	asset.id = "duration_checks";
	asset.defaultState = "Idle";
	asset.parameters.push_back(AnimationParameterDesc{
		.name = "speed",
		.defaultValue = AnimationParameterValue{ .type = AnimationParameterType::Float, .floatValue = 0.0f }
	});
	asset.states.push_back(AnimationStateDesc{ .name = "Idle", .clipName = "Idle" });
	asset.states.push_back(AnimationStateDesc{ .name = "Run", .clipName = "Run" });
	
	// Idle -> Run uses a positive blend duration, so runtime should enter
	// transition blending after the state switch.
	asset.transitions.push_back(AnimationTransitionDesc{
		.fromState = "Idle",
		.toState = "Run",
		.blendDurationSeconds = 0.25f,
		.priority = 2,
		.conditions = { 
			AnimationConditionDesc{ 
				.parameter = "speed", 
				.op = AnimationConditionOp::Greater, 
				.value = AnimationParameterValue{ .type = AnimationParameterType::Float, .floatValue = 0.5f } } }
	});
		
	// Run -> Idle uses zero duration, so it should switch immediately
	// without leaving transitionActive set.
	asset.transitions.push_back(AnimationTransitionDesc{
		.fromState = "Run",
		.toState = "Idle",
		.blendDurationSeconds = 0.0f,
		.priority = 2,
		.conditions = { 
			AnimationConditionDesc{ 
				.parameter = "speed", 
				.op = AnimationConditionOp::LessEqual, 
				.value = AnimationParameterValue{ .type = AnimationParameterType::Float, .floatValue = 0.0f } } }
	});

	AnimationControllerRuntime runtime{};
	BindAnimationControllerStateMachine(runtime, skeleton, clips, clipSourceAssetIds, asset, true, false, false);
	AnimatorState animator{};

	// Positive duration transition: state changes to Run and transition blending starts.
	SetAnimationParameter(runtime.parameters, "speed", 1.0f);
	UpdateAnimationControllerRuntime(runtime, animator, 0.016f);
	EXPECT_EQ(runtime.currentStateName, "Run");
	EXPECT_TRUE(runtime.transitionActive);
	EXPECT_FLOAT_EQ(runtime.transitionDurationSeconds, 0.25f);

	// Advance enough time to finish the 0.25s blend.
	UpdateAnimationControllerRuntime(runtime, animator, 0.25f);
	EXPECT_FALSE(runtime.transitionActive);

	// Zero duration transition: state changes back to Idle immediately,
	// but no active blend should remain.
	SetAnimationParameter(runtime.parameters, "speed", 0.0f);
	UpdateAnimationControllerRuntime(runtime, animator, 0.016f);
	EXPECT_EQ(runtime.currentStateName, "Idle");
	EXPECT_FALSE(runtime.transitionActive);
}

TEST(AnimationController, ExternalNotifyAndEventBindingAssetsHappyPathLoad)
{
	const std::filesystem::path assetRoot = corefs::FindAssetRoot();

	WriteTestAssetFile(assetRoot, "tests/animation_validation/notify_valid.animnotify.json", R"json({
		"states": {"Idle": [{"id": "footstep", "time": 0.25, "fireOnEnter": true}]},
		"clips": {"Idle": [{"id": "from_clip", "time": 0.50}]}
	})json");
	WriteTestAssetFile(assetRoot, "tests/animation_validation/bindings_valid.animbindings.json", R"json({
		"bindings": [{"animationEvent": "footstep", "gameplayEvent": "Gameplay.Footstep"}]
	})json");
	WriteTestAssetFile(assetRoot, "tests/animation_validation/controller_valid.level.json", R"json({
		"animationControllers": {
			"hero": {
				"defaultState": "Idle",
				"states": {"Idle": {"clip": "Idle"}},
				"notifyAsset": "tests/animation_validation/notify_valid.animnotify.json",
				"eventBindingsAsset": "tests/animation_validation/bindings_valid.animbindings.json"
			}
		}
	})json");

	const LevelAsset level = LoadLevelAssetFromJson("tests/animation_validation/controller_valid.level.json");
	ASSERT_TRUE(level.animationControllers.contains("hero"));
	const AnimationControllerAsset& controller = level.animationControllers.at("hero");
	ASSERT_EQ(controller.states.size(), 1u);
	EXPECT_EQ(controller.states[0].notifies.size(), 2u);
	EXPECT_EQ(controller.eventBindings.size(), 1u);
}

TEST(AnimationController, ExternalNotifyAssetValidationErrorsAreActionable)
{
	const std::filesystem::path assetRoot = corefs::FindAssetRoot();

	// These malformed fixtures intentionally target distinct notify-asset validation branches
	// so future regressions are easy to localize from the failure message fragment.
	WriteTestAssetFile(assetRoot, "tests/animation_validation/notify_root_array.animnotify.json", "[]");
	WriteTestAssetFile(assetRoot, "tests/animation_validation/notify_states_wrong_type.animnotify.json", R"json({"states": []})json");
	WriteTestAssetFile(assetRoot, "tests/animation_validation/notify_state_entry_wrong_type.animnotify.json", R"json({"states": {"Idle": 1}})json");
	WriteTestAssetFile(assetRoot, "tests/animation_validation/notify_entry_missing_id.animnotify.json", R"json({"states": {"Idle": [{"time": 0.5}]}})json");
	WriteTestAssetFile(assetRoot, "tests/animation_validation/notify_entry_wrong_time_type.animnotify.json", R"json({"states": {"Idle": [{"id": "n", "time": "bad"}]}})json");
	WriteTestAssetFile(assetRoot, "tests/animation_validation/notify_entry_wrong_fire_on_enter_type.animnotify.json", R"json({"states": {"Idle": [{"id": "n", "fireOnEnter": 1}]}})json");

	const std::string baseLevel = R"json({"animationControllers":{"hero":{"defaultState":"Idle","states":{"Idle":{"clip":"Idle"}},"notifyAsset":"%s"}}})json";
	auto writeLevelForNotify = [&](const char* levelName, const char* notifyRelPath)
	{
		std::string content = baseLevel;
		content.replace(content.find("%s"), 2, notifyRelPath);
		WriteTestAssetFile(assetRoot, std::string("tests/animation_validation/") + levelName, content);
	};

	writeLevelForNotify("notify_root_array.level.json", "tests/animation_validation/notify_root_array.animnotify.json");
	writeLevelForNotify("notify_states_wrong_type.level.json", "tests/animation_validation/notify_states_wrong_type.animnotify.json");
	writeLevelForNotify("notify_state_entry_wrong_type.level.json", "tests/animation_validation/notify_state_entry_wrong_type.animnotify.json");
	writeLevelForNotify("notify_entry_missing_id.level.json", "tests/animation_validation/notify_entry_missing_id.animnotify.json");
	writeLevelForNotify("notify_entry_wrong_time_type.level.json", "tests/animation_validation/notify_entry_wrong_time_type.animnotify.json");
	writeLevelForNotify("notify_entry_wrong_fire_on_enter_type.level.json", "tests/animation_validation/notify_entry_wrong_fire_on_enter_type.animnotify.json");

	ExpectLoadLevelThrowsWithMessageFragment(
		"tests/animation_validation/notify_root_array.level.json", 
		"Animation notify JSON: root must be object");
	ExpectLoadLevelThrowsWithMessageFragment(
		"tests/animation_validation/notify_states_wrong_type.level.json", 
		".states must be object");
	ExpectLoadLevelThrowsWithMessageFragment(
		"tests/animation_validation/notify_state_entry_wrong_type.level.json", 
		".states.Idle must be array");
	ExpectLoadLevelThrowsWithMessageFragment(
		"tests/animation_validation/notify_entry_missing_id.level.json", 
		".states.Idle[].id is required");
	ExpectLoadLevelThrowsWithMessageFragment(
	"tests/animation_validation/notify_entry_wrong_time_type.level.json",
	"expected number at 'time'");
	ExpectLoadLevelThrowsWithMessageFragment(
		"tests/animation_validation/notify_entry_wrong_fire_on_enter_type.level.json",
		"expected bool at 'fireOnEnter'");
}

TEST(AnimationController, ExternalEventBindingAssetValidationErrorsAreActionable)
{
	const std::filesystem::path assetRoot = corefs::FindAssetRoot();
	WriteTestAssetFile(assetRoot, "tests/animation_validation/bindings_root_array.animbindings.json", "[]");
	WriteTestAssetFile(assetRoot, "tests/animation_validation/bindings_missing_collection.animbindings.json", R"json({"x": []})json");
	WriteTestAssetFile(assetRoot, "tests/animation_validation/bindings_collection_wrong_type.animbindings.json", R"json({"bindings": {}})json");
	WriteTestAssetFile(assetRoot, "tests/animation_validation/bindings_entry_not_object.animbindings.json", R"json({"bindings": [1]})json");
	WriteTestAssetFile(assetRoot, "tests/animation_validation/bindings_missing_animation_event.animbindings.json", R"json({"bindings": [{"gameplayEvent": "G"}]})json");
	WriteTestAssetFile(assetRoot, "tests/animation_validation/bindings_missing_gameplay_event.animbindings.json", R"json({"bindings": [{"animationEvent": "N"}]})json");
	WriteTestAssetFile(assetRoot, "tests/animation_validation/bindings_wrong_animation_event_type.animbindings.json", R"json({"bindings": [{"animationEvent": 1, "gameplayEvent": "G"}]})json");
	WriteTestAssetFile(assetRoot, "tests/animation_validation/bindings_wrong_gameplay_event_type.animbindings.json", R"json({"bindings": [{"animationEvent": "N", "gameplayEvent": 2}]})json");

	const std::string baseLevel = R"json({"animationControllers":{"hero":{"defaultState":"Idle","states":{"Idle":{"clip":"Idle"}},"eventBindingsAsset":"%s"}}})json";
	auto writeLevelForBindings = [&](const char* levelName, const char* relPath)
	{
		std::string content = baseLevel;
		content.replace(content.find("%s"), 2, relPath);
		WriteTestAssetFile(assetRoot, std::string("tests/animation_validation/") + levelName, content);
	};

	writeLevelForBindings("bindings_root_array.level.json", "tests/animation_validation/bindings_root_array.animbindings.json");
	writeLevelForBindings("bindings_missing_collection.level.json", "tests/animation_validation/bindings_missing_collection.animbindings.json");
	writeLevelForBindings("bindings_collection_wrong_type.level.json", "tests/animation_validation/bindings_collection_wrong_type.animbindings.json");
	writeLevelForBindings("bindings_entry_not_object.level.json", "tests/animation_validation/bindings_entry_not_object.animbindings.json");
	writeLevelForBindings("bindings_missing_animation_event.level.json", "tests/animation_validation/bindings_missing_animation_event.animbindings.json");
	writeLevelForBindings("bindings_missing_gameplay_event.level.json", "tests/animation_validation/bindings_missing_gameplay_event.animbindings.json");
	writeLevelForBindings("bindings_wrong_animation_event_type.level.json", "tests/animation_validation/bindings_wrong_animation_event_type.animbindings.json");
	writeLevelForBindings("bindings_wrong_gameplay_event_type.level.json", "tests/animation_validation/bindings_wrong_gameplay_event_type.animbindings.json");

	ExpectLoadLevelThrowsWithMessageFragment(
	"tests/animation_validation/bindings_root_array.level.json",
	"Animation event bindings JSON: root must be object");
	ExpectLoadLevelThrowsWithMessageFragment(
		"tests/animation_validation/bindings_missing_collection.level.json",
		".bindings must be array");
	ExpectLoadLevelThrowsWithMessageFragment(
		"tests/animation_validation/bindings_collection_wrong_type.level.json",
		".bindings must be array");
	ExpectLoadLevelThrowsWithMessageFragment(
		"tests/animation_validation/bindings_entry_not_object.level.json",
		"expected object");
	ExpectLoadLevelThrowsWithMessageFragment(
		"tests/animation_validation/bindings_missing_animation_event.level.json",
		".bindings[].animationEvent is required");
	ExpectLoadLevelThrowsWithMessageFragment(
		"tests/animation_validation/bindings_missing_gameplay_event.level.json",
		".bindings[].gameplayEvent is required");
	ExpectLoadLevelThrowsWithMessageFragment(
		"tests/animation_validation/bindings_wrong_animation_event_type.level.json",
		"expected string at 'animationEvent'");

	ExpectLoadLevelThrowsWithMessageFragment(
		"tests/animation_validation/bindings_wrong_gameplay_event_type.level.json",
		"expected string at 'gameplayEvent'");
}

TEST(AnimationController, LevelAssetSectionSchemaFailuresAreActionable)
{
	const std::filesystem::path assetRoot = corefs::FindAssetRoot();

	struct SchemaFailureCase
	{
		const char* name;
		const char* levelJson;
		const char* expectedFragment;
	};

	const std::vector<SchemaFailureCase> cases{
		// Required fields for mesh/model/texture/animation entries.
		{ "meshes_missing_path", R"json({"meshes":{"heroMesh":{}}})json", "Level JSON: meshes.heroMesh.path is required" },
		{ "models_missing_path", R"json({"models":{"heroModel":{}}})json", "Level JSON: models.heroModel.path is required" },
		{ "textures_tex2d_missing_path", R"json({"textures":{"heroAlbedo":{"kind":"tex2d"}}})json", "Level JSON: textures.heroAlbedo.path is required for tex2d" },
		{ "animations_missing_path", R"json({"animations":{"heroIdle":{}}})json", "Level JSON: animations.heroIdle.path is required" },
		{ "animation_controller_assets_missing_path", R"json({"animationControllerAssets":{"heroCtrl":{}}})json", "Level JSON: animationControllerAssets.heroCtrl.path is required" },

		// Enum-like validation branches for texture kind/source and animation controller config.
		{ "textures_invalid_kind", R"json({"textures":{"heroTex":{"kind":"volume","path":"dummy.png"}}})json", "Level JSON: textures.heroTex.kind must be tex2d|cube" },
		{ "textures_cube_invalid_source", R"json({"textures":{"sky":{"kind":"cube","source":"invalid"}}})json", "Level JSON: textures.sky.source must be cross|auto|faces" },
		{ "animation_controllers_invalid_parameter_type", R"json({"animationControllers":{"hero":{"defaultState":"Idle","parameters":{"speed":{"type":"integer"}},"states":{"Idle":{"clip":"Idle"}}}}})json", "Level JSON: animation controller parameter type must be bool|int|float|trigger" },
		{ "animation_controllers_invalid_condition_op", R"json({"animationControllers": {"hero": {"defaultState": "Idle","parameters": {"speed": { "type": "float", "default": 0.0 }},"states": {"Idle": {"clip": "Idle"},"Run": {"clip": "Idle"}},"transitions": [{"from": "Idle","to": "Run","conditions": [{"parameter": "speed","op": "approx","value": 1.0}]}]}}})json", "Level JSON: animation controller condition op is invalid"},
	};

	for (const auto& testCase : cases)
	{
		const std::string levelPath = std::string("tests/animation_validation/") + testCase.name + ".level.json";
		WriteTestAssetFile(assetRoot, levelPath, testCase.levelJson);
		SCOPED_TRACE(testCase.name);
		ExpectLoadLevelThrowsWithMessageFragment(levelPath, testCase.expectedFragment);
	}
}