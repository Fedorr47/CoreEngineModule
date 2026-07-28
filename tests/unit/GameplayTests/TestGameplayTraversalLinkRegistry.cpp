#include <gtest/gtest.h>

#include <optional>

import core;

using namespace rendern;

namespace
{
    [[nodiscard]] GameplayTraversalLink MakeLink(const EntityHandle target = static_cast<EntityHandle>(1u))
    {
        return GameplayTraversalLink{
            .handle = GameplayTraversalLinkHandle{42u},
            .traversalTypeId = kDoorTraversalTypeId,
            .targetEntity = target
        };
    }
}

// Protects stable by-value registration and lookup of structurally valid links.
TEST(GameplayTraversalLinkRegistry, RegistersAndFindsValidLink)
{
    GameplayTraversalLinkRegistry registry{};
    ASSERT_TRUE(registry.Register(MakeLink()));
    const std::optional<GameplayTraversalLink> link = registry.Find(GameplayTraversalLinkHandle{42u});
    ASSERT_TRUE(link.has_value());
    EXPECT_EQ(link->targetEntity, static_cast<EntityHandle>(1u));
}

// Protects the registry from mutating when structural link identity is invalid.
TEST(GameplayTraversalLinkRegistry, RejectsInvalidLink)
{
    GameplayTraversalLinkRegistry registry{};
    EXPECT_FALSE(registry.Register(GameplayTraversalLink{}));
    EXPECT_FALSE(registry.Find(GameplayTraversalLinkHandle{}).has_value());
}

// Protects immutable descriptions by rejecting replacement under an existing handle.
TEST(GameplayTraversalLinkRegistry, RejectsDuplicateHandle)
{
    GameplayTraversalLinkRegistry registry{};
    ASSERT_TRUE(registry.Register(MakeLink(static_cast<EntityHandle>(1u))));
    EXPECT_FALSE(registry.Register(MakeLink(static_cast<EntityHandle>(2u))));
    EXPECT_EQ(registry.Find(GameplayTraversalLinkHandle{42u})->targetEntity, static_cast<EntityHandle>(1u));
}

// Protects deterministic missing lookup behavior without creating entries.
TEST(GameplayTraversalLinkRegistry, MissingLookupReturnsNull)
{
    const GameplayTraversalLinkRegistry registry{};
    EXPECT_FALSE(registry.Find(GameplayTraversalLinkHandle{99u}).has_value());
}

// Protects explicit removal and subsequent absence of a registered description.
TEST(GameplayTraversalLinkRegistry, RemoveDeletesRegisteredLink)
{
    GameplayTraversalLinkRegistry registry{};
    ASSERT_TRUE(registry.Register(MakeLink()));
    EXPECT_TRUE(registry.Remove(GameplayTraversalLinkHandle{42u}));
    EXPECT_FALSE(registry.Contains(GameplayTraversalLinkHandle{42u}));
}

// Protects the session reset boundary by clearing every registered link.
TEST(GameplayTraversalLinkRegistry, ResetClearsAllLinks)
{
    GameplayTraversalLinkRegistry registry{};
    ASSERT_TRUE(registry.Register(MakeLink()));
    registry.Reset();
    EXPECT_FALSE(registry.Find(GameplayTraversalLinkHandle{42u}).has_value());
}