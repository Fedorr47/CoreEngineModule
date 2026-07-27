#include <gtest/gtest.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

namespace
{
    class JoltRuntimeScope final
    {
    public:
        JoltRuntimeScope()
        {
            JPH::RegisterDefaultAllocator();

            JPH::Factory::sInstance = new JPH::Factory();
            JPH::RegisterTypes();
        }

        ~JoltRuntimeScope()
        {
            JPH::UnregisterTypes();

            delete JPH::Factory::sInstance;
            JPH::Factory::sInstance = nullptr;
        }

        JoltRuntimeScope(const JoltRuntimeScope&) = delete;
        JoltRuntimeScope& operator=(const JoltRuntimeScope&) = delete;
    };
}

TEST(JoltDependencySmoke, InitializesRuntimeAndCreatesShape)
{
    // The smoke test owns the process-wide Jolt factory for its full lifetime.
    ASSERT_EQ(JPH::Factory::sInstance, nullptr);

    const JoltRuntimeScope runtimeScope{};

    JPH::SphereShapeSettings shapeSettings{ 0.5f };
    shapeSettings.SetEmbedded();

    const JPH::ShapeSettings::ShapeResult shapeResult =
        shapeSettings.Create();

    EXPECT_FALSE(shapeResult.HasError());
    EXPECT_NE(shapeResult.Get(), nullptr);
}