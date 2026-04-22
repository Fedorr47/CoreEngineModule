#include <gtest/gtest.h>

#include <array>
#include <stdexcept>
#include <string>
#include <string_view>

import core;

using namespace rendern;

namespace
{
    jsonUtils::JsonValue ParseJson(std::string_view text)
    {
        jsonUtils::JsonParser parser(text);
        return parser.Parse();
    }
}

TEST(JsonUtils, ParseObjectSuccessWithTypedAccess)
{
    const jsonUtils::JsonValue root = ParseJson(R"json({
        "name":"core",
        "enabled":true,
        "count":5,
        "arr":[1,2,3],
        "nested":{"mode":"strict"}
    })json");

    ASSERT_TRUE(root.IsObject());
    const auto& object = root.AsObject();

    EXPECT_EQ(jsonUtils::GetStringOpt(object, "name", ""), "core");
    EXPECT_TRUE(jsonUtils::GetBoolOpt(object, "enabled", false));
    EXPECT_EQ(jsonUtils::GetIntOpt(object, "count", 0), 5);

    const jsonUtils::JsonValue& nestedValue = jsonUtils::GetReq(object, "nested");
    ASSERT_TRUE(nestedValue.IsObject());
    EXPECT_EQ(jsonUtils::GetStringOpt(nestedValue.AsObject(), "mode", ""), "strict");

    const jsonUtils::JsonValue* arrayValue = jsonUtils::TryGet(object, "arr");
    ASSERT_NE(arrayValue, nullptr);
    ASSERT_TRUE(arrayValue->IsArray());
    EXPECT_EQ(arrayValue->AsArray().size(), 3u);
}

TEST(JsonUtils, ParseObjectFailureCasesThrow)
{
    const std::array<std::string_view, 3> invalidInputs = {
        R"json({"name" "core"})json",
        R"json({"name":"core",})json",
        R"json({"name":1)json"
    };

    for (const std::string_view input : invalidInputs)
    {
        SCOPED_TRACE(std::string(input));
        EXPECT_THROW(ParseJson(input), std::runtime_error);
    }
}

TEST(JsonUtils, ParseArraySuccessAndFailureCases)
{
    const jsonUtils::JsonValue validArray = ParseJson(R"json([true,false,null,{"k":"v"},[1,2,3]])json");
    ASSERT_TRUE(validArray.IsArray());
    ASSERT_EQ(validArray.AsArray().size(), 5u);
    EXPECT_TRUE(validArray.AsArray()[0].IsBool());
    EXPECT_TRUE(validArray.AsArray()[2].IsNull());
    EXPECT_TRUE(validArray.AsArray()[3].IsObject());

    const std::array<std::string_view, 2> invalidArrays = {
        R"json([1,,2])json",
        R"json([1,2,)json"
    };

    for (const std::string_view input : invalidArrays)
    {
        SCOPED_TRACE(std::string(input));
        EXPECT_THROW(ParseJson(input), std::runtime_error);
    }
}

TEST(JsonUtils, ParseNumbersSuccessAndMalformedFailureCases)
{
    const jsonUtils::JsonValue parsed = ParseJson(R"json([0,-1,42,3.5,-0.125,6.02e23,-2E-2])json");
    ASSERT_TRUE(parsed.IsArray());
    const auto& values = parsed.AsArray();
    ASSERT_EQ(values.size(), 7u);

    EXPECT_DOUBLE_EQ(values[0].AsNumber(), 0.0);
    EXPECT_DOUBLE_EQ(values[1].AsNumber(), -1.0);
    EXPECT_DOUBLE_EQ(values[2].AsNumber(), 42.0);
    EXPECT_DOUBLE_EQ(values[3].AsNumber(), 3.5);
    EXPECT_DOUBLE_EQ(values[4].AsNumber(), -0.125);
    EXPECT_DOUBLE_EQ(values[5].AsNumber(), 6.02e23);
    EXPECT_DOUBLE_EQ(values[6].AsNumber(), -2e-2);

    const std::array<std::string_view, 4> invalidNumbers = {
        R"json({"n":-})json",
        R"json({"n":1e})json",
        R"json({"n":1e+})json",
        R"json({"n":--1})json"
    };

    for (const std::string_view input : invalidNumbers)
    {
        SCOPED_TRACE(std::string(input));
        EXPECT_THROW(ParseJson(input), std::runtime_error);
    }
}

TEST(JsonUtils, ParseStringEscapesSuccessAndInvalidEscapeFailure)
{
    const jsonUtils::JsonValue parsed = ParseJson("{\"s\":\"line\\n\\t\\\\\\\"\\/\\u0041\"}");
    ASSERT_TRUE(parsed.IsObject());

    const std::string escaped = jsonUtils::GetReq(parsed.AsObject(), "s").AsString();
    EXPECT_NE(escaped.find('\n'), std::string::npos);
    EXPECT_NE(escaped.find('\t'), std::string::npos);
    EXPECT_NE(escaped.find('\\'), std::string::npos);
    EXPECT_NE(escaped.find('\"'), std::string::npos);
    EXPECT_NE(escaped.find('/'), std::string::npos);
    EXPECT_NE(escaped.find('?'), std::string::npos);

    EXPECT_THROW(ParseJson("{\"s\":\"bad\\x\"}"), std::runtime_error);
    EXPECT_THROW(ParseJson("{\"s\":\"bad\\u12\"}"), std::runtime_error);
}

TEST(JsonUtils, ParseRejectsTrailingData)
{
    EXPECT_THROW(ParseJson(R"json({"ok":true} trailing)json"), std::runtime_error);
    EXPECT_THROW(ParseJson(R"json([1,2,3] 0)json"), std::runtime_error);
}

TEST(JsonUtils, TypedAccessFailurePathsThrowAndDefaultsApply)
{
    const jsonUtils::JsonValue root = ParseJson(R"json({"name":"core","enabled":true,"count":3,"node":{}})json");
    ASSERT_TRUE(root.IsObject());
    const auto& object = root.AsObject();

    EXPECT_EQ(jsonUtils::GetStringOpt(object, "missing", "fallback"), "fallback");
    EXPECT_FALSE(jsonUtils::GetBoolOpt(object, "missingBool", false));
    EXPECT_FLOAT_EQ(jsonUtils::GetFloatOpt(object, "missingFloat", 3.25f), 3.25f);
    EXPECT_EQ(jsonUtils::GetIntOpt(object, "missingInt", 7), 7);
    EXPECT_EQ(jsonUtils::TryGet(object, "missingPtr"), nullptr);

    EXPECT_THROW(([&] {
    [[maybe_unused]] const auto& unused = jsonUtils::GetReq(object, "missingReq");
}()), std::runtime_error);

    EXPECT_THROW(([&] {
        [[maybe_unused]] const auto unused = jsonUtils::GetStringOpt(object, "count", "");
    }()), std::runtime_error);

    EXPECT_THROW(([&] {
        [[maybe_unused]] const auto unused = jsonUtils::GetBoolOpt(object, "name", false);
    }()), std::runtime_error);

    EXPECT_THROW(([&] {
        [[maybe_unused]] const auto unused = jsonUtils::GetFloatOpt(object, "node", 0.0f);
    }()), std::runtime_error);

    EXPECT_THROW(([&] {
        [[maybe_unused]] const auto unused = jsonUtils::GetIntOpt(object, "enabled", 0);
    }()), std::runtime_error);

    const jsonUtils::JsonValue& name = jsonUtils::GetReq(object, "name");
    EXPECT_THROW(([&] {
    [[maybe_unused]] const auto& unused = name.AsArray();
    }()), std::runtime_error);
}