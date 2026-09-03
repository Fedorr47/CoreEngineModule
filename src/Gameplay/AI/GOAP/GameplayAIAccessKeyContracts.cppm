module;

#include <cstdint>
#include <string_view>

export module core:gameplay_ai_access_key_contracts;

import :ai_action_contracts;
import :ai_decision_contracts;

export namespace rendern
{
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
    inline constexpr AIActionId kAIBuyKeyActionId{3u};
    inline constexpr AIGoalId kGOAPReachDestinationGoal{0u};

}
