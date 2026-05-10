#include <gtest/gtest.h>
#include <stdexcept>

#include "TestSupport/TestFixtureLoader.h"

import core;

using namespace rendern;

TEST(GameplayGraph, CanonicalizeTokenStripsPunctuationAndLowercases)
{
	EXPECT_EQ(CanonicalizeGameplayGraphToken("Jump_Start!!"), "jumpstart");
	EXPECT_EQ(CanonicalizeGameplayGraphToken("  Turn Left  "), "turnleft");
}

TEST(GameplayGraph, ParameterStoreSupportsTypedSetGetAndFallbacks)
{
	GameplayGraphParameterStore store{};

	SetGameplayGraphBool(store, "isMoving", true);
	SetGameplayGraphInt(store, "combo", 3);
	SetGameplayGraphFloat(store, "speed", 2.5f);
	SetGameplayGraphString(store, "state", "Run");

	EXPECT_TRUE(GetGameplayGraphBool(store, "isMoving"));
	EXPECT_EQ(GetGameplayGraphInt(store, "combo"), 3);
	EXPECT_FLOAT_EQ(GetGameplayGraphFloat(store, "speed"), 2.5f);
	EXPECT_EQ(GetGameplayGraphString(store, "state"), "Run");

	EXPECT_FALSE(GetGameplayGraphBool(store, "missingBool"));
	EXPECT_EQ(GetGameplayGraphInt(store, "missingInt", 17), 17);
	EXPECT_FLOAT_EQ(GetGameplayGraphFloat(store, "missingFloat", 9.0f), 9.0f);
	EXPECT_EQ(GetGameplayGraphString(store, "missingString", "Idle"), "Idle");
}

TEST(GameplayGraph, TriggerConsumeAndFrameResetWork)
{
	GameplayGraphInstance instance{};
	SetGameplayGraphTrigger(instance.parameters, "attack");
	PushGameplayGraphEvent(instance, "ActionFinished");
	instance.animationTriggersThisFrame.push_back("Attack");

	EXPECT_TRUE(ConsumeGameplayGraphTrigger(instance.parameters, "attack"));
	EXPECT_FALSE(ConsumeGameplayGraphTrigger(instance.parameters, "attack"));
	EXPECT_TRUE(GameplayGraphHasEvent(instance, "action_finished"));

	SetGameplayGraphTrigger(instance.parameters, "attack");
	ClearGameplayGraphFrameState(instance);

	EXPECT_FALSE(GetGameplayGraphBool(instance.parameters, "attack"));
	EXPECT_TRUE(instance.eventsThisFrame.empty());
	EXPECT_TRUE(instance.animationTriggersThisFrame.empty());
}

TEST(GameplayGraph, FindStateIndexByNameReturnsExpectedIndex)
{
	GameplayGraphLayerDesc layer{};
	layer.states.push_back(GameplayGraphStateDesc{ .name = "Idle" });
	layer.states.push_back(GameplayGraphStateDesc{ .name = "Run" });
	layer.states.push_back(GameplayGraphStateDesc{ .name = "Attack" });

	EXPECT_EQ(FindGameplayGraphStateIndex(layer, "Idle"), 0);
	EXPECT_EQ(FindGameplayGraphStateIndex(layer, "Attack"), 2);
	EXPECT_EQ(FindGameplayGraphStateIndex(layer, "Missing"), -1);
}

TEST(GameplayGraph, CompileConditionOpcodeSupportsLegacyTextTokens)
{
	EXPECT_EQ(
		CompileGameplayGraphConditionOpcode("BoolTrue"),
		GameplayGraphConditionOpcode::BoolTrue);

	EXPECT_EQ(
		CompileGameplayGraphConditionOpcode("bool_false"),
		GameplayGraphConditionOpcode::BoolFalse);

	EXPECT_EQ(
		CompileGameplayGraphConditionOpcode("FLOAT-GREATER"),
		GameplayGraphConditionOpcode::FloatGreater);

	EXPECT_EQ(
		CompileGameplayGraphConditionOpcode("float less"),
		GameplayGraphConditionOpcode::FloatLess);
}

TEST(GameplayGraph, CompileTaskOpcodeSupportsLegacyTextTokens)
{
	EXPECT_EQ(
		CompileGameplayGraphTaskOpcode("BeginActionState"),
		GameplayGraphTaskOpcode::BeginActionState);

	EXPECT_EQ(
		CompileGameplayGraphTaskOpcode("begin_action_state"),
		GameplayGraphTaskOpcode::BeginActionState);
}

TEST(GameplayGraph, CompileTaskOpcodeReturnsUnknownForUnsupportedToken)
{
	EXPECT_EQ(
		CompileGameplayGraphTaskOpcode("???"),
		GameplayGraphTaskOpcode::Unknown);
}

TEST(GameplayGraph, CompileConditionOpcodeReturnsUnknownForUnsupportedToken)
{
	EXPECT_EQ(
		CompileGameplayGraphConditionOpcode("NotARealCondition"),
		GameplayGraphConditionOpcode::Unknown);

	EXPECT_EQ(
		CompileGameplayGraphConditionOpcode("???"),
		GameplayGraphConditionOpcode::Unknown);
}

TEST(GameplayGraph, PrecompileGameplayGraphAssetCompilesTaskOpcodes)
{
	GameplayGraphAsset asset{};
	asset.id = "task_graph";

	GameplayGraphLayerDesc layer{};
	layer.name = "Base";
	layer.defaultState = "Idle";

	GameplayGraphStateDesc idle{};
	idle.name = "Idle";
	idle.onEnter = {
		GameplayGraphTaskDesc{ .name = "BeginActionState" }
	};
	idle.onUpdate = {
		GameplayGraphTaskDesc{ .name = "begin_action_state" }
	};

	GameplayGraphStateDesc run{};
	run.name = "Run";

	layer.states.push_back(std::move(idle));
	layer.states.push_back(std::move(run));
	asset.layers.push_back(std::move(layer));

	PrecompileGameplayGraphAsset(asset);

	ASSERT_EQ(asset.layers.size(), 1u);
	ASSERT_EQ(asset.layers[0].states.size(), 2u);

	const GameplayGraphStateDesc& compiledIdle = asset.layers[0].states[0];
	ASSERT_EQ(compiledIdle.onEnter.size(), 1u);
	ASSERT_EQ(compiledIdle.onUpdate.size(), 1u);

	EXPECT_EQ(
		compiledIdle.onEnter[0].opcode,
		GameplayGraphTaskOpcode::BeginActionState);

	EXPECT_EQ(
		compiledIdle.onUpdate[0].opcode,
		GameplayGraphTaskOpcode::BeginActionState);
}

TEST(GameplayGraph, PrecompileGameplayGraphAssetReportsUnknownTaskOpcodeToStderr)
{
	GameplayGraphAsset asset{};
	asset.id = "invalid_task_graph";

	GameplayGraphLayerDesc layer{};
	layer.name = "Base";
	layer.defaultState = "Idle";

	GameplayGraphStateDesc idle{};
	idle.name = "Idle";
	idle.onEnter = {
		GameplayGraphTaskDesc{ .name = "???" }
	};

	layer.states.push_back(std::move(idle));
	asset.layers.push_back(std::move(layer));

	testing::internal::CaptureStderr();
	PrecompileGameplayGraphAsset(asset);
	const std::string stderrOutput = testing::internal::GetCapturedStderr();

	EXPECT_NE(stderrOutput.find("Unknown GameplayGraphTaskOpcode"), std::string::npos);
	EXPECT_NE(stderrOutput.find("???"), std::string::npos);
}

TEST(GameplayGraph, PrecompileGameplayGraphAssetCompilesTransitionConditionOpcodes)
{
	GameplayGraphAsset asset{};
	asset.id = "test_graph";

	GameplayGraphLayerDesc layer{};
	layer.name = "Base";
	layer.defaultState = "Idle";

	GameplayGraphStateDesc idle{};
	idle.name = "Idle";
	idle.transitions = {
		GameplayGraphTransitionDesc{
			.toState = "Run",
			.conditions = {
				GameplayGraphConditionDesc{
					.name = "BoolTrue",
					.parameter = "isMoving",
					.boolValue = true
				},
				GameplayGraphConditionDesc{
					.name = "FloatGreater",
					.parameter = "speed",
					.threshold = 0.1f
				}
			}
		}
	};

	GameplayGraphStateDesc run{};
	run.name = "Run";
	run.transitions = {
		GameplayGraphTransitionDesc{
			.toState = "Idle",
			.conditions = {
				GameplayGraphConditionDesc{
					.name = "BoolFalse",
					.parameter = "isMoving",
					.boolValue = false
				},
				GameplayGraphConditionDesc{
					.name = "FloatLess",
					.parameter = "speed",
					.threshold = 0.1f
				}
			}
		}
	};

	layer.states.push_back(std::move(idle));
	layer.states.push_back(std::move(run));
	asset.layers.push_back(std::move(layer));

	PrecompileGameplayGraphAsset(asset);

	ASSERT_EQ(asset.layers.size(), 1u);
	ASSERT_EQ(asset.layers[0].states.size(), 2u);

	const GameplayGraphTransitionDesc& idleToRun = asset.layers[0].states[0].transitions[0];
	ASSERT_EQ(idleToRun.conditions.size(), 2u);
	EXPECT_EQ(idleToRun.conditions[0].opcode, GameplayGraphConditionOpcode::BoolTrue);
	EXPECT_EQ(idleToRun.conditions[1].opcode, GameplayGraphConditionOpcode::FloatGreater);

	const GameplayGraphTransitionDesc& runToIdle = asset.layers[0].states[1].transitions[0];
	ASSERT_EQ(runToIdle.conditions.size(), 2u);
	EXPECT_EQ(runToIdle.conditions[0].opcode, GameplayGraphConditionOpcode::BoolFalse);
	EXPECT_EQ(runToIdle.conditions[1].opcode, GameplayGraphConditionOpcode::FloatLess);
}

TEST(GameplayGraph, PrecompileGameplayGraphAssetLeavesUnknownOpcodeForInvalidCondition)
{
	GameplayGraphAsset asset{};
	asset.id = "invalid_graph";

	GameplayGraphLayerDesc layer{};
	layer.name = "Base";
	layer.defaultState = "Idle";

	GameplayGraphStateDesc idle{};
	idle.name = "Idle";
	idle.transitions = {
		GameplayGraphTransitionDesc{
			.toState = "Run",
			.conditions = {
				GameplayGraphConditionDesc{
					.name = "DefinitelyUnknownCondition",
					.parameter = "flag"
				}
			}
		}
	};

	GameplayGraphStateDesc run{};
	run.name = "Run";

	layer.states.push_back(std::move(idle));
	layer.states.push_back(std::move(run));
	asset.layers.push_back(std::move(layer));

	PrecompileGameplayGraphAsset(asset);

	ASSERT_EQ(asset.layers.size(), 1u);
	ASSERT_EQ(asset.layers[0].states.size(), 2u);
	ASSERT_EQ(asset.layers[0].states[0].transitions.size(), 1u);
	ASSERT_EQ(asset.layers[0].states[0].transitions[0].conditions.size(), 1u);

	EXPECT_EQ(
		asset.layers[0].states[0].transitions[0].conditions[0].opcode,
		GameplayGraphConditionOpcode::Unknown);
}

TEST(GameplayGraph, PrecompileGameplayGraphAssetReportsUnknownConditionOpcodeToStderr)
{
	GameplayGraphAsset asset{};
	asset.id = "invalid_graph";

	GameplayGraphLayerDesc layer{};
	layer.name = "Base";
	layer.defaultState = "Idle";

	GameplayGraphStateDesc idle{};
	idle.name = "Idle";
	idle.transitions = {
		GameplayGraphTransitionDesc{
			.toState = "Run",
			.conditions = {
				GameplayGraphConditionDesc{
					.name = "???",
					.parameter = "flag"
				}
			}
		}
	};

	GameplayGraphStateDesc run{};
	run.name = "Run";

	layer.states.push_back(std::move(idle));
	layer.states.push_back(std::move(run));
	asset.layers.push_back(std::move(layer));

	testing::internal::CaptureStderr();
	PrecompileGameplayGraphAsset(asset);
	const std::string stderrOutput = testing::internal::GetCapturedStderr();

	EXPECT_NE(stderrOutput.find("Unknown GameplayGraphConditionOpcode"), std::string::npos);
	EXPECT_NE(stderrOutput.find("???"), std::string::npos);
}

TEST(GameplayGraph, DefaultHumanoidAssetComesPrecompiled)
{
	GameplayGraphAsset asset = MakeDefaultHumanoidGameplayGraphAsset();

	ASSERT_EQ(asset.layers.size(), 1u);
	ASSERT_EQ(asset.layers[0].states.size(), 2u);

	const GameplayGraphStateDesc& grounded = asset.layers[0].states[0];
	const GameplayGraphStateDesc& action = asset.layers[0].states[1];

	ASSERT_EQ(grounded.transitions.size(), 1u);
	ASSERT_EQ(action.transitions.size(), 1u);
	ASSERT_EQ(grounded.transitions[0].conditions.size(), 1u);
	ASSERT_EQ(action.transitions[0].conditions.size(), 1u);
	
	ASSERT_EQ(action.onEnter.size(), 1u);

	EXPECT_EQ(
		action.onEnter[0].opcode,
		GameplayGraphTaskOpcode::BeginActionState);
	
	EXPECT_EQ(
		grounded.transitions[0].conditions[0].opcode,
		GameplayGraphConditionOpcode::BoolTrue);

	EXPECT_EQ(
		action.transitions[0].conditions[0].opcode,
		GameplayGraphConditionOpcode::BoolFalse);
}

TEST(GameplayGraph, JsonFixtureLoaderNestedFixture)
{
	const std::string fixtureText = LoadTextFixture("json/valid/nested_config.json");
	jsonUtils::JsonParser parser(fixtureText);
	const jsonUtils::JsonValue root = parser.Parse();
	
	const auto& top = root.AsObject();
	const auto& config = jsonUtils::GetReq(top, "config").AsObject();
	const auto& graphics = jsonUtils::GetReq(config, "graphics").AsObject();
	
	EXPECT_FALSE(jsonUtils::GetBoolOpt(graphics, "vsync", true));
	
	const auto& resolution = jsonUtils::GetReq(graphics, "resolution").AsArray();
	
	ASSERT_EQ(resolution.size(), 2u);
	EXPECT_EQ(static_cast<int>(resolution[0].AsNumber()), 1920);
	EXPECT_EQ(static_cast<int>(resolution[1].AsNumber()), 1080);
	EXPECT_EQ(jsonUtils::GetStringOpt(config, "quality", ""), "high");
}

TEST(GameplayGraph, JsonFixtureLoaderMalformedFixtureThrowsClearError)
{
	const std::string fixtureText = LoadTextFixture("json/invalid/malformed_config.json");
	jsonUtils::JsonParser parser(fixtureText);

	EXPECT_THROW(
		{
			try
			{
				(void)parser.Parse();
			}
			catch (const std::runtime_error& ex)
			{
				EXPECT_NE(std::string(ex.what()).find("JSON parse error"), std::string::npos);
				throw;
			}
		},
		std::runtime_error);
}

TEST(GameplayGraph, ValidateAndNormalizeAssetInvalidTransitionTargetFails)
{
	GameplayGraphAsset asset{};
	GameplayGraphLayerDesc layer{};
	layer.name = "Base";
	layer.defaultState = "Idle";

	GameplayGraphStateDesc idle{};
	idle.name = "Idle";
	idle.transitions.push_back(GameplayGraphTransitionDesc{ .toState = "Attack" });

	GameplayGraphStateDesc move{};
	move.name = "Move";

	layer.states.push_back(std::move(idle));
	layer.states.push_back(std::move(move));
	asset.layers.push_back(std::move(layer));

	const GameplayGraphValidationResult validation = ValidateAndNormalizeGameplayGraphAsset(asset);
	EXPECT_FALSE(validation.success);
	ASSERT_FALSE(validation.diagnostics.empty());
	EXPECT_NE(validation.diagnostics[0].find("Attack"), std::string::npos);
	EXPECT_NE(validation.diagnostics[0].find("state"), std::string::npos);
}

TEST(GameplayGraph, ValidateAndNormalizeAssetInvalidInitialStateFails)
{
	GameplayGraphAsset asset{};
	GameplayGraphLayerDesc layer{};
	layer.name = "Base";
	layer.defaultState = "MissingInitial";
	layer.states.push_back(GameplayGraphStateDesc{ .name = "Idle" });
	asset.layers.push_back(std::move(layer));

	const GameplayGraphValidationResult validation = ValidateAndNormalizeGameplayGraphAsset(asset);
	EXPECT_FALSE(validation.success);
	ASSERT_FALSE(validation.diagnostics.empty());
	EXPECT_NE(validation.diagnostics[0].find("MissingInitial"), std::string::npos);
}

TEST(GameplayGraph, ValidateAndNormalizeAssetUnknownBoolParameterInConditionFails)
{
	GameplayGraphAsset asset{};
	GameplayGraphLayerDesc layer{};
	layer.name = "Base";
	layer.defaultState = "Idle";
	layer.declaredParameters = { "speed" };

	GameplayGraphStateDesc idle{};
	idle.name = "Idle";
	idle.transitions.push_back(GameplayGraphTransitionDesc{
		.toState = "Run",
		.conditions = { GameplayGraphConditionDesc{ .name = "BoolTrue", .parameter = "isGrounded", .boolValue = true } }
	});
	GameplayGraphStateDesc run{};
	run.name = "Run";

	layer.states.push_back(std::move(idle));
	layer.states.push_back(std::move(run));
	asset.layers.push_back(std::move(layer));

	const GameplayGraphValidationResult validation = ValidateAndNormalizeGameplayGraphAsset(asset);
	EXPECT_FALSE(validation.success);
	ASSERT_FALSE(validation.diagnostics.empty());
	EXPECT_NE(validation.diagnostics[0].find("isGrounded"), std::string::npos);
	EXPECT_NE(validation.diagnostics[0].find("parameter"), std::string::npos);
}

TEST(GameplayGraph, ValidateAndNormalizeAssetUnknownTriggerParameterFails)
{
	GameplayGraphAsset asset{};
	GameplayGraphLayerDesc layer{};
	layer.name = "Base";
	layer.defaultState = "Idle";
	layer.declaredParameters = { "speed" };

	GameplayGraphStateDesc idle{};
	idle.name = "Idle";
	idle.transitions.push_back(GameplayGraphTransitionDesc{
		.toState = "Run",
		.conditions = { GameplayGraphConditionDesc{ .name = "BoolTrue", .parameter = "AttackPressed", .boolValue = true } }
	});
	GameplayGraphStateDesc run{};
	run.name = "Run";

	layer.states.push_back(std::move(idle));
	layer.states.push_back(std::move(run));
	asset.layers.push_back(std::move(layer));

	const GameplayGraphValidationResult validation = ValidateAndNormalizeGameplayGraphAsset(asset);
	EXPECT_FALSE(validation.success);
	ASSERT_FALSE(validation.diagnostics.empty());
	EXPECT_NE(validation.diagnostics[0].find("AttackPressed"), std::string::npos);
}

TEST(GameplayGraph, ValidateAndNormalizeAssetMinimalValidGraphAppliesDefaults)
{
	GameplayGraphAsset asset{};
	asset.id = "minimal";

	GameplayGraphLayerDesc layer{};
	layer.name = "Base";
	layer.states.push_back(GameplayGraphStateDesc{ .name = "Idle" });
	asset.layers.push_back(std::move(layer));

	const GameplayGraphValidationResult validation = ValidateAndNormalizeGameplayGraphAsset(asset);
	EXPECT_TRUE(validation.success);
	EXPECT_TRUE(validation.diagnostics.empty());
	ASSERT_EQ(asset.layers.size(), 1u);
	ASSERT_EQ(asset.layers[0].states.size(), 1u);
	EXPECT_EQ(asset.layers[0].defaultState, "Idle");
	EXPECT_TRUE(asset.layers[0].states[0].transitions.empty());
}