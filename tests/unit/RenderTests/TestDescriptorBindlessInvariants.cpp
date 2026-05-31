#include <gtest/gtest.h>

#include <stdexcept>

#include "FakeRHI.h"

import core;

TEST(DescriptorAllocatorInvariants, ExhaustionFailsDeterministically)
{
    FakeRHIDevice device{};
    device.SetDescriptorCapacityForTests(2);

    const auto first = device.AllocateTextureDescriptor(rhi::TextureHandle{11});
    const auto second = device.AllocateTextureDescriptor(rhi::TextureHandle{22});
    EXPECT_NE(first, second);

    // Protects the allocation exhaustion boundary so overflow fails before backend execution.
    EXPECT_THROW(device.AllocateTextureDescriptor(rhi::TextureHandle{33}), std::runtime_error);
}

TEST(DescriptorAllocatorInvariants, FreedRangeCanBeReusedWhenSupported)
{
    FakeRHIDevice device{};
    device.SetDescriptorCapacityForTests(2);

    const auto first = device.AllocateTextureDescriptor(rhi::TextureHandle{101});
    const auto second = device.AllocateTextureDescriptor(rhi::TextureHandle{202});

    // Protects free-list/release contract: freeing one slot must allow a subsequent allocation.
    device.FreeTextureDescriptor(first);
    EXPECT_NO_THROW(device.AllocateTextureDescriptor(rhi::TextureHandle{303}));

    EXPECT_THROW(device.AllocateTextureDescriptor(rhi::TextureHandle{404}), std::runtime_error);
    EXPECT_NE(first, second);
}

TEST(BindlessAllocatorInvariants, InvalidHandleUpdateFailsDeterministically)
{
    FakeRHIDevice device{};
    rendern::BindlessTable bindless{device};

    const auto validIndex = bindless.RegisterTexture(rhi::TextureHandle{515});

    // Protects invalid handle/index rejection: unknown descriptor slot updates must fail explicitly.
    EXPECT_THROW(bindless.UpdateTexture(validIndex + 5000u, rhi::TextureHandle{616}), std::runtime_error);
}

TEST(BindlessAllocatorInvariants, OutOfRangeUpdateFailsDeterministically)
{
    FakeRHIDevice device{};
    rendern::BindlessTable bindless{device};

    bindless.RegisterTexture(rhi::TextureHandle{717});

    // Index 0 is reserved invalid; update attempts must follow deterministic guard behavior.
    EXPECT_THROW(bindless.UpdateTexture(0, rhi::TextureHandle{818}), std::runtime_error);
}

TEST(BindlessAllocatorInvariants, CapacityBoundaryRejectsPastEndIndex)
{
    FakeRHIDevice device{};
    device.SetDescriptorCapacityForTests(1);
    rendern::BindlessTable bindless{device};

    const auto index = bindless.RegisterTexture(rhi::TextureHandle{919});
    EXPECT_NE(index, 0u);

    // Protects out-of-range index/update guard at the exact capacity boundary.
    EXPECT_THROW(bindless.RegisterTexture(rhi::TextureHandle{929}), std::runtime_error);
}
