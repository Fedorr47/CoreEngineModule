#include <gtest/gtest.h>

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
