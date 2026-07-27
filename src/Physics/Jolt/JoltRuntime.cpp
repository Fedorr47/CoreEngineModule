#include "Physics/Jolt/JoltRuntime.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <memory>
#include <mutex>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

namespace
{
    constexpr std::size_t TemporaryAllocatorSize = 16u * 1024u * 1024u;

    std::mutex joltRuntimeMutex;

    void TraceJoltMessage(const char* format, ...)
    {
        std::va_list arguments;
        va_start(arguments, format);

        std::vfprintf(stderr, format, arguments);
        std::fputc('\n', stderr);

        va_end(arguments);
    }

#ifdef JPH_ENABLE_ASSERTS
    bool HandleJoltAssertion(
        const char* expression,
        const char* message,
        const char* file,
        JPH::uint line)
    {
        std::fprintf(
            stderr,
            "[Jolt] Assertion failed: %s:%u: (%s) %s\n",
            file,
            line,
            expression,
            message != nullptr ? message : "");

        // Return true to request a debugger break in Jolt.
        return true;
    }
#endif
}

namespace physics
{
    struct JoltRuntime::Implementation
    {
        std::unique_ptr<JPH::Factory> factory;
        std::unique_ptr<JPH::TempAllocatorImpl> temporaryAllocator;
        std::unique_ptr<JPH::JobSystemSingleThreaded> jobSystem;

        bool bTypesRegistered = false;
    };

    JoltRuntime::~JoltRuntime()
    {
        Shutdown();
    }

    bool JoltRuntime::Initialize()
    {
        std::scoped_lock lock(joltRuntimeMutex);

        if (impl_ != nullptr)
        {
            return true;
        }

        // Jolt's factory is process-wide. Another initialized runtime means
        // that this object cannot safely take ownership of Jolt.
        if (JPH::Factory::sInstance != nullptr)
        {
            return false;
        }

        JPH::RegisterDefaultAllocator();

        JPH::Trace = TraceJoltMessage;

#ifdef JPH_ENABLE_ASSERTS
        JPH::AssertFailed = HandleJoltAssertion;
#endif

        auto newImplementation = std::make_unique<Implementation>();

        newImplementation->temporaryAllocator = std::make_unique<JPH::TempAllocatorImpl>(TemporaryAllocatorSize);

        // Physics remains inline for now. Jolt jobs execute immediately on
        // the calling thread instead of creating another worker pool.
        newImplementation->jobSystem = std::make_unique<JPH::JobSystemSingleThreaded>(JPH::cMaxPhysicsJobs);

        newImplementation->factory = std::make_unique<JPH::Factory>();

        JPH::Factory::sInstance = newImplementation->factory.get();

        JPH::RegisterTypes();
        newImplementation->bTypesRegistered = true;

        impl_ = newImplementation.release();
        return true;
    }

    void JoltRuntime::Shutdown() noexcept
    {
        std::scoped_lock lock(joltRuntimeMutex);

        if (impl_ == nullptr)
        {
            return;
        }

        assert(JPH::Factory::sInstance == impl_->factory.get());

        if (impl_->bTypesRegistered)
        {
            JPH::UnregisterTypes();
            impl_->bTypesRegistered = false;
        }

        JPH::Factory::sInstance = nullptr;

        delete impl_;
        impl_ = nullptr;

        JPH::Trace = nullptr;

#ifdef JPH_ENABLE_ASSERTS
        JPH::AssertFailed = nullptr;
#endif
    }

    bool JoltRuntime::IsInitialized() const noexcept
    {
        return impl_ != nullptr;
    }

    JPH::TempAllocator& JoltRuntime::GetTempAllocator() noexcept
    {
        assert(impl_ != nullptr);
        return *impl_->temporaryAllocator;
    }

    JPH::JobSystem& JoltRuntime::GetJobSystem() noexcept
    {
        assert(impl_ != nullptr);
        return *impl_->jobSystem;
    }
}