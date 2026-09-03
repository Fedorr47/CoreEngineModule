#include <gtest/gtest.h>

#include <string_view>

import core;

namespace
{
    constexpr std::string_view kDefinitionJson = R"json(
{
  "id": "parse_test",
  "facts": [
    { "name": "enabled", "type": "bool" },
    { "name": "resource", "type": "int" }
  ],
  "goals": [
    {
      "name": "reach_target",
      "score": 2.5,
      "facts": [
        { "fact": "enabled", "value": true }
      ]
    }
  ],
  "actions": [
    {
      "action": "move_to",
      "context": "target_a",
      "cost": 3.0,
      "preconditions": [
        { "fact": "enabled", "value": false }
      ],
      "effects": [
        { "fact": "enabled", "value": true }
      ],
      "numericPreconditions": [
        { "fact": "resource", "op": ">=", "value": 2 }
      ],
      "numericEffects": [
        { "fact": "resource", "op": "add", "value": -1 }
      ]
    }
  ]
}
)json";
}

TEST(GameplayGOAPDefinitionAsset, ParsePreservesAuthoredSchemaWithoutCompilation)
{
    const rendern::GameplayGOAPDefinitionAsset asset =
        rendern::ParseGameplayGOAPDefinitionAsset(kDefinitionJson, "parse-test.json");

    EXPECT_EQ(asset.id, "parse_test");
    EXPECT_EQ(asset.source, "parse-test.json");

    ASSERT_EQ(asset.facts.size(), 2u);
    EXPECT_EQ(asset.facts[0].name, "enabled");
    EXPECT_EQ(asset.facts[0].type, rendern::GameplayGOAPFactType::Boolean);
    EXPECT_EQ(asset.facts[1].name, "resource");
    EXPECT_EQ(asset.facts[1].type, rendern::GameplayGOAPFactType::Integer);

    ASSERT_EQ(asset.goals.size(), 1u);
    EXPECT_EQ(asset.goals[0].name, "reach_target");
    EXPECT_FLOAT_EQ(asset.goals[0].score, 2.5f);
    ASSERT_EQ(asset.goals[0].facts.size(), 1u);
    EXPECT_EQ(asset.goals[0].facts[0].fact, "enabled");
    EXPECT_TRUE(asset.goals[0].facts[0].value);

    ASSERT_EQ(asset.actions.size(), 1u);
    const rendern::GameplayGOAPAuthoredAction& action = asset.actions[0];
    EXPECT_EQ(action.action, "move_to");
    EXPECT_EQ(action.context, "target_a");
    EXPECT_FLOAT_EQ(action.cost, 3.0f);

    ASSERT_EQ(action.preconditions.size(), 1u);
    EXPECT_EQ(action.preconditions[0].fact, "enabled");
    EXPECT_FALSE(action.preconditions[0].value);

    ASSERT_EQ(action.effects.size(), 1u);
    EXPECT_EQ(action.effects[0].fact, "enabled");
    EXPECT_TRUE(action.effects[0].value);

    ASSERT_EQ(action.numericPreconditions.size(), 1u);
    EXPECT_EQ(action.numericPreconditions[0].fact, "resource");
    EXPECT_EQ(action.numericPreconditions[0].operation, ">=");
    EXPECT_EQ(action.numericPreconditions[0].value, 2);

    ASSERT_EQ(action.numericEffects.size(), 1u);
    EXPECT_EQ(action.numericEffects[0].fact, "resource");
    EXPECT_EQ(action.numericEffects[0].operation, "add");
    EXPECT_EQ(action.numericEffects[0].value, -1);
}

TEST(GameplayGOAPDefinitionAsset, ParseRejectsWrongAuthoredNumericType)
{
    constexpr std::string_view json = R"json(
{
  "id": "parse_test",
  "facts": [],
  "goals": [
    { "name": "reach_target", "score": 1.0, "facts": [] }
  ],
  "actions": [
    {
      "action": "move_to",
      "context": "target_a",
      "cost": 1.0,
      "numericPreconditions": [
        { "fact": "resource", "op": ">=", "value": 1.5 }
      ]
    }
  ]
}
)json";

    try
    {
        (void)rendern::ParseGameplayGOAPDefinitionAsset(json, "parse-test.json");
        FAIL() << "Expected std::runtime_error";
    }
    catch (const std::runtime_error& error)
    {
        EXPECT_STREQ(
            error.what(),
            "GOAP definition 'parse-test.json', actions 'target_a': "
            "numeric value must be a finite 32-bit integer");
    }
}

TEST(GameplayGOAPDefinitionAsset, ParsesBooleanAndNumericContinuationConditions)
{
    const auto asset = rendern::ParseGameplayGOAPDefinitionAsset(R"json({
        "id":"continuation", "facts":[], "goals":[], "actions":[{
            "action":"move_to", "context":"goal", "cost":1,
            "continuationConditions":[{"fact":"available", "value":true}],
            "numericContinuationConditions":[{"fact":"resource", "op":">=", "value":1}]
        }]})json", "continuation.json");
    ASSERT_EQ(asset.actions.front().continuationConditions.size(), 1u);
    EXPECT_EQ(asset.actions.front().continuationConditions.front().fact, "available");
    EXPECT_TRUE(asset.actions.front().continuationConditions.front().value);
    ASSERT_EQ(asset.actions.front().numericContinuationConditions.size(), 1u);
    EXPECT_EQ(asset.actions.front().numericContinuationConditions.front().operation, ">=");
    EXPECT_EQ(asset.actions.front().numericContinuationConditions.front().value, 1);
}
