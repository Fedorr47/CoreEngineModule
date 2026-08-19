#include <gtest/gtest.h>

#include <memory>

import core;

using namespace rendern;

namespace
{
    class NullBinding final : public IAIActionBinding
    {
    public:
        explicit NullBinding(bool& destroyed) : destroyed_(&destroyed) {}
        ~NullBinding() override { *destroyed_ = true; }
        std::unique_ptr<IAIActionRuntime> CreateRuntime(const AIActionRuntimeContext&) override
        {
            return nullptr;
        }
    private:
        bool* destroyed_{};
    };
}

TEST(AIActionBinding, RegistryValidatesAndFindsRegistrations)
{
    AIActionBindingRegistry registry{};
    bool firstDestroyed = false;
    bool duplicateDestroyed = false;
    NullBinding first(firstDestroyed);
    NullBinding duplicate(duplicateDestroyed);
    constexpr AIActionId id{7u};

    EXPECT_FALSE(registry.Register(AIActionId{}, first));
    EXPECT_TRUE(registry.Register(id, first));
    EXPECT_FALSE(registry.Register(id, duplicate));
    EXPECT_TRUE(registry.Contains(id));
    EXPECT_EQ(registry.Find(id), &first);
    EXPECT_EQ(registry.Find(AIActionId{8u}), nullptr);
    EXPECT_FALSE(registry.Remove(AIActionId{8u}));
}

TEST(AIActionBinding, RemoveAndResetDoNotOwnBindings)
{
    AIActionBindingRegistry registry{};
    bool firstDestroyed = false;
    bool secondDestroyed = false;
    NullBinding first(firstDestroyed);
    NullBinding second(secondDestroyed);

    ASSERT_TRUE(registry.Register(AIActionId{1u}, first));
    ASSERT_TRUE(registry.Register(AIActionId{2u}, second));
    EXPECT_TRUE(registry.Remove(AIActionId{1u}));
    EXPECT_EQ(registry.Find(AIActionId{1u}), nullptr);
    EXPECT_FALSE(firstDestroyed);
    registry.Reset();
    EXPECT_EQ(registry.Find(AIActionId{2u}), nullptr);
    EXPECT_FALSE(secondDestroyed);
}