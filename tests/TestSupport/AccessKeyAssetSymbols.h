#pragma once

#include <cstdint>
#include <string_view>

// Expected symbols of the checked-in AccessKey test fixture, not runtime wiring.
// Tests also resolve/verify these names against the compiled asset.
namespace access_key_test
{
    using namespace rendern;
    inline constexpr std::int32_t kAccessKeyPrice = 2;
    inline constexpr std::string_view kAccessKeyAIDecisionId{"access_key"};
    inline constexpr AIWorldFactId kGOAPHasAccessKeyFact{0u};
    inline constexpr AIWorldFactId kGOAPAtDestinationFact{1u};
    inline constexpr AIWorldFactId kGOAPCoinACollectedFact{2u};
    inline constexpr AIWorldFactId kGOAPCoinBCollectedFact{3u};
    inline constexpr AIWorldFactId kGOAPCoinCCollectedFact{4u};
    inline constexpr AIWorldFactId kGOAPAtAccessKeyShopFact{5u};
    inline constexpr AIWorldFactId kGOAPAtStartFact{6u};
    inline constexpr AIWorldFactId kGOAPAtCoinAFact{7u};
    inline constexpr AIWorldFactId kGOAPAtCoinBFact{8u};
    inline constexpr AIWorldFactId kGOAPAtCoinCFact{9u};
    inline constexpr AIWorldFactId kGOAPAtGoalFact{10u};
    inline constexpr AIWorldFactId kGOAPCoinAAvailableFact{11u};
    inline constexpr AIWorldFactId kGOAPCoinBAvailableFact{12u};
    inline constexpr AIWorldFactId kGOAPCoinCAvailableFact{13u};
    inline constexpr AIWorldIntegerFactId kGOAPCoinCountFact{0u};
    inline constexpr AIGoalId kGOAPReachDestinationGoal{0u};

}
