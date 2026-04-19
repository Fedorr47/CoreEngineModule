#include <gtest/gtest.h>

#include <algorithm>
#include <variant>

#include "FakeRHI.h"

import core;

TEST(RenderGraphFakeRHI, ExecutePassesOnCpuAndRecordRHICalls)
{
    FakeRHIDevice device{};
    
    rhi::Extent2D testExtent = {640u, 360u};
    rhi::Format testFormat = rhi::Format::BGRA8_UNORM;
    
    rhi::SwapChainDesc swapChainDesc{};
    swapChainDesc.extent = testExtent;
    swapChainDesc.backbufferFormat = testFormat;
    swapChainDesc.vsync = false;
    
    FakeRHISwapChain swapChain(
        swapChainDesc,
        rhi::FrameBufferHandle{7001},
        rhi::TextureHandle {7002}
    );
    
    renderGraph::RenderGraph renderGraph;
    
    auto color = renderGraph.CreateTexture(renderGraph::RGTextureDesc{
        .extent = testExtent,
        .format = testFormat,
        .usage = renderGraph::ResourceUsage::RenderTarget,
        .debugName = "rg_color"
    });
    
    bool transientPassExecuted = false;
    renderGraph::PassAttachments transientAttachments{};
    transientAttachments.bindDepthStencil = false;
    transientAttachments.colors.push_back(color);
    renderGraph.AddPass(
        "TransientColorPass",
        std::move(transientAttachments),
        [&](renderGraph::PassContext& ctx)
        {
            transientPassExecuted = true;
            EXPECT_EQ(ctx.passExtent.width, testExtent.width);
            EXPECT_EQ(ctx.passExtent.height, testExtent.height);
            EXPECT_TRUE(ctx.resources.GetTexture(color));
            ctx.commandList.SetViewport(0,0, testExtent.width, testExtent.height);
            ctx.commandList.Draw(3);
        });
    
    bool swapChainPassExecuted = false;
    rhi::ClearDesc clearDesc{};
    clearDesc.clearColor = true;
    clearDesc.clearDepth = true;
    renderGraph.AddSwapChainPass(
        "PresentPass",
        clearDesc,
        [&](renderGraph::PassContext& ctx)
        {
            swapChainPassExecuted = true;
            EXPECT_EQ(ctx.passExtent.width, testExtent.width);
            EXPECT_EQ(ctx.passExtent.height, testExtent.height);
            ctx.commandList.Draw(6);
        }
        );
    
    renderGraph.Execute(device, swapChain);
    
    ASSERT_TRUE(transientPassExecuted);
    ASSERT_TRUE(swapChainPassExecuted);
    
    ASSERT_EQ(device.GetTextureCreates().size(), 1);
    EXPECT_EQ(device.GetTextureCreates().front().extent.width, testExtent.width);
    EXPECT_EQ(device.GetTextureCreates().front().format, testFormat);
    EXPECT_FALSE(device.GetTextureCreates().front().isCubeMap);
    
    ASSERT_EQ(device.GetFrameBufferCreates().size(), 1u);
    EXPECT_EQ(device.GetFrameBufferCreates().front().kind, "mrt");
    ASSERT_EQ(device.GetFrameBufferCreates().front().colors.size(), 1u);
    EXPECT_TRUE(device.GetFrameBufferCreates().front().colors.front());
    
    EXPECT_EQ(device.GetDestroyedFrameBuffers().size(), 1u);
    EXPECT_EQ(device.GetDestroyedTextures().size(), 1u);
    
    ASSERT_EQ(device.GetSubmittedCommandLists().size(), 1u);
    const auto& submitted = device.GetSubmittedCommandLists().front();
    ASSERT_FALSE(submitted.commands.empty());
    
    const auto beginPassCount = std::count_if(submitted.commands.begin(), submitted.commands.end(),
        [](const rhi::Command& command)
    {
        return std::holds_alternative<rhi::CommandBeginPass>(command);
    });
    const auto endPassCount = std::count_if(submitted.commands.begin(), submitted.commands.end(),
        [](const rhi::Command& command)
    {
        return std::holds_alternative<rhi::CommandEndPass>(command);
    });

    EXPECT_EQ(beginPassCount, 2);
    EXPECT_EQ(endPassCount, 2);

    bool sawSwapChainBegin = false;
    for (const auto& command : submitted.commands)
    {
        if (const auto* begin = std::get_if<rhi::CommandBeginPass>(&command))
        {
            if (begin->desc.swapChain != nullptr)
            {
                sawSwapChainBegin = true;
                EXPECT_EQ(begin->desc.frameBuffer.id, 7001u);
            }
        }
    }
    EXPECT_TRUE(sawSwapChainBegin);
}