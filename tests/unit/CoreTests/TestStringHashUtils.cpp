#include <gtest/gtest.h>

#include <array>
#include <functional>
#include <string>
#include <string_view>

import core;

using namespace rendern;

namespace
{
    struct LowerCaseHelper
    {
        std::string_view input;
        std::string_view expected;
    };
    
    struct ContainsCaseHelper
    {
        std::string_view input;
        std::string_view needle;
        bool expected;
    };
    
    std::size_t HashCanonicalToken(std::string_view token)
    {
        std::size_t seed = 0;
        hashUtils::HashCombine(seed, std::hash<std::string>{}(stringUtils::ToLowerAsciiCopy(token)));
        return seed;
    }
}

TEST(StringUtils, ToLowerAsciiCopyCanonicalizesCaseAndKeepPunctuation)
{
    const std::array<LowerCaseHelper, 8> lowerCases = {
        LowerCaseHelper{"PlayerHealth", "playerhealth"},
        LowerCaseHelper{"Render/Shadow.Map", "render/shadow.map"},
        LowerCaseHelper{"UI_MENU-OPEN", "ui_menu-open"},
        LowerCaseHelper{"  Config.Token  ", "  config.token  "},
        LowerCaseHelper{"AlreadyNormalized", "alreadynormalized"},
        LowerCaseHelper{"", ""},
        LowerCaseHelper{"A_B-C.D/E", "a_b-c.d/e"},
        LowerCaseHelper{"Token!@#", "token!@#"}
    };
    
    for (const LowerCaseHelper& lowerCasePair : lowerCases)
    {
        SCOPED_TRACE(std::string(lowerCasePair.input));
        EXPECT_EQ(stringUtils::ToLowerAsciiCopy(lowerCasePair.input), lowerCasePair.expected);
    }
}

TEST(StringUtils, ContainsInsensitiveMatchesCaseInsensitiveWithPunctuation)
{
    const std::array<ContainsCaseHelper, 8> containsCases = {
        ContainsCaseHelper{"Gameplay/Event.OnHit", "event.onhit", true},
        ContainsCaseHelper{"Config-Key:Value", "key:value", true},
        ContainsCaseHelper{"A_B-C.D/E", "b-c.d", true},
        ContainsCaseHelper{"tokenized-value", "TOKENIZED", true},
        ContainsCaseHelper{"", "", false},
        ContainsCaseHelper{"short", "longer", false},
        ContainsCaseHelper{"abc", "", false},
        ContainsCaseHelper{"player.health", "player-health", false}
    };
    
    for (const ContainsCaseHelper& containsCasePair : containsCases)
    {
        SCOPED_TRACE(std::string(containsCasePair.input) + " | " + std::string(containsCasePair.needle));
        EXPECT_EQ(stringUtils::ContainsInsensitive(containsCasePair.input, containsCasePair.needle), containsCasePair.expected);
    }
}

TEST(HashUtils, HashCombineIsDetermenisticForRepeatedSequences)
{
    const std::array<std::size_t, 5> sequence = {0u, 1u, 42u, 1991u, 999999u};
    
    std::size_t first = 0;
    for (const std::size_t value : sequence)
    {
        hashUtils::HashCombine(first, value);
    }
    
    std::size_t second = 0;
    for (const std::size_t value : sequence)
    {
        hashUtils::HashCombine(second, value);
    }
    
    EXPECT_EQ(first, second);
    
    std::size_t reversed = 0;
    for (auto it = sequence.rbegin(); it != sequence.rend(); ++it)
    {
        hashUtils::HashCombine(reversed, *it);
    }
    
    EXPECT_NE(first, reversed);
}

TEST(HashUtils, CanonicalEquivalentTokensProduceEquivalentDerivedHashes)
{
    const std::array<std::array<std::string_view, 3>, 4> equivalentGroups = {
        std::array<std::string_view, 3>{"Player.Health", "player.health", "PLAYER.HEALTH"},
        std::array<std::string_view, 3>{"Config/SpawnRate", "config/spawnrate", "CONFIG/SPAWNRATE"},
        std::array<std::string_view, 3>{"UI_MENU-OPEN", "ui_menu-open", "Ui_Menu-Open"},
        std::array<std::string_view, 3>{"", "", ""}
    };
    
    for (const std::array<std::string_view, 3>& group : equivalentGroups)
    {
        const std::size_t expected = HashCanonicalToken(group[0]);
        for (const std::string_view token : group)
        {
            SCOPED_TRACE(std::string(token));
            EXPECT_EQ(HashCanonicalToken(token), expected);
        }
    }
}
