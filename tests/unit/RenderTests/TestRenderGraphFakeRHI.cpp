#include <gtest/gtest.h>

#include <algorithm>
#include <variant>

#include "FakeRHI.h"

import core;

namespace
{
    std::size_t CountTextureHandleOccurrences(const std::vector<rhi::TextureHandle>& handles, rhi::TextureHandle target)
    {
        return static_cast<std::size_t>(std::count_if(handles.begin(), handles.end(), [&](rhi::TextureHandle handle)
        {
            return handle.id == target.id;
        }));
    }
    
    FakeRHISwapChain MakeTestSwapChain(rhi::Extent2D extent, rhi::Format format)
    {
        rhi::SwapChainDesc swapChainDesc{};
        swapChainDesc.extent = extent;
        swapChainDesc.backbufferFormat = format;
        swapChainDesc.vsync = false;
        
        return FakeRHISwapChain(
                swapChainDesc,
                rhi::FrameBufferHandle{7001},
                 rhi::TextureHandle{7002});
    }
}

TEST(RenderGraphFakeRHI, ExecutePassesOnCpuAndRecordRHICalls)
{
    FakeRHIDevice device{};

    rhi::Extent2D testExtent = {640u, 360u};
    rhi::Format testFormat = rhi::Format::BGRA8_UNORM;
    auto swapChain = MakeTestSwapChain(testExtent, testFormat);

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

    // Transient textures are cached across Execute() calls and are released explicitly.
    EXPECT_EQ(device.GetDestroyedTextures().size(), 0u);

    renderGraph.ReleaseCachedResources(device);

    EXPECT_EQ(device.GetDestroyedTextures().size(), 1u);
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

// Imported textures model externally owned resources, such as swapchain backbuffers
// or renderer-provided targets. RenderGraph may reference and bind them during
// execution, but their lifetime must remain outside of the graph cache.
TEST(RenderGraphResourceOwnership, ImportedTextureIsNotDestroyedByGraph)
{
    FakeRHIDevice device{};
    const rhi::Extent2D extent{320u, 180u};
    const rhi::Format format = rhi::Format::BGRA8_UNORM;
    auto swapChain = MakeTestSwapChain(extent, format);

    renderGraph::RenderGraph graph;
    const rhi::TextureHandle importedHandle{424242u};
    const auto imported = graph.ImportTexture(importedHandle, renderGraph::RGTextureDesc{
        .extent = extent,
        .format = format,
        .usage = renderGraph::ResourceUsage::RenderTarget,
        .debugName = "imported_external_rt"
    });

    renderGraph::PassAttachments attachments{};
    attachments.bindDepthStencil = false;
    attachments.colors.push_back(imported);
    graph.AddPass("ImportedOnly", std::move(attachments), [&](renderGraph::PassContext& ctx)
    {
        EXPECT_EQ(ctx.resources.GetTexture(imported).id, importedHandle.id);
        ctx.commandList.Draw(1);
    });

    graph.Execute(device, swapChain);
    graph.ReleaseCachedResources(device);

    EXPECT_TRUE(device.GetTextureCreates().empty()) << "Imported textures must not be created by graph.";
    EXPECT_EQ(CountTextureHandleOccurrences(device.GetDestroyedTextures(), importedHandle), 0u)
        << "Imported texture handle was destroyed by graph.";
}

// Owned textures are transient graph resources. The graph is responsible for
// creating them before use and releasing the cached RHI resource when the graph
// cache is explicitly cleaned up.
TEST(RenderGraphResourceOwnership, OwnedTextureIsDestroyedExactlyOnce)
{
    FakeRHIDevice device{};
    const rhi::Extent2D extent{512u, 512u};
    const rhi::Format format = rhi::Format::RGBA16_FLOAT;
    auto swapChain = MakeTestSwapChain(extent, format);

    renderGraph::RenderGraph graph;
    const auto owned = graph.CreateTexture(renderGraph::RGTextureDesc{
        .extent = extent,
        .format = format,
        .usage = renderGraph::ResourceUsage::RenderTarget,
        .debugName = "owned_transient_rt"
    });

    renderGraph::PassAttachments attachments{};
    attachments.bindDepthStencil = false;
    attachments.colors.push_back(owned);
    graph.AddPass("OwnedOnly", std::move(attachments), [&](renderGraph::PassContext& ctx)
    {
        EXPECT_TRUE(ctx.resources.GetTexture(owned));
        ctx.commandList.Draw(1);
    });

    graph.Execute(device, swapChain);
    ASSERT_EQ(device.GetTextureCreates().size(), 1u) << "Owned texture must be created once.";
    const auto ownedHandle = device.GetTextureCreates().front().handle;

    graph.ReleaseCachedResources(device);

    EXPECT_EQ(CountTextureHandleOccurrences(device.GetDestroyedTextures(), ownedHandle), 1u)
        << "Owned texture should be destroyed exactly once.";
}

// Mixed ownership is the important regression case: imported and owned textures
// share the same graph execution path, but cleanup must still destroy only the
// resources created by RenderGraph itself.
TEST(RenderGraphResourceOwnership, MixedImportedAndOwnedTexturesRespectOwnership)
{
    FakeRHIDevice device{};
    const rhi::Extent2D extent{1024u, 1024u};
    const rhi::Format format = rhi::Format::BGRA8_UNORM;
    auto swapChain = MakeTestSwapChain(extent, format);

    renderGraph::RenderGraph graph;
    const rhi::TextureHandle importedHandle{777777u};
    const auto imported = graph.ImportTexture(importedHandle, renderGraph::RGTextureDesc{
        .extent = extent,
        .format = format,
        .usage = renderGraph::ResourceUsage::RenderTarget,
        .debugName = "imported_color"
    });
    const auto owned = graph.CreateTexture(renderGraph::RGTextureDesc{
        .extent = extent,
        .format = format,
        .usage = renderGraph::ResourceUsage::RenderTarget,
        .debugName = "owned_color"
    });

    renderGraph::PassAttachments importedPass{};
    importedPass.bindDepthStencil = false;
    importedPass.colors.push_back(imported);
    graph.AddPass("ImportedPass", std::move(importedPass), [&](renderGraph::PassContext& ctx)
    {
        EXPECT_EQ(ctx.resources.GetTexture(imported).id, importedHandle.id);
        ctx.commandList.Draw(2);
    });

    renderGraph::PassAttachments ownedPass{};
    ownedPass.bindDepthStencil = false;
    ownedPass.colors.push_back(owned);
    graph.AddPass("OwnedPass", std::move(ownedPass), [&](renderGraph::PassContext& ctx)
    {
        EXPECT_TRUE(ctx.resources.GetTexture(owned));
        ctx.commandList.Draw(3);
    });

    graph.Execute(device, swapChain);
    ASSERT_EQ(device.GetTextureCreates().size(), 1u) << "Only the owned texture should be created by graph.";
    const auto ownedHandle = device.GetTextureCreates().front().handle;

    graph.ReleaseCachedResources(device);

    EXPECT_EQ(CountTextureHandleOccurrences(device.GetDestroyedTextures(), ownedHandle), 1u)
        << "Owned texture should be destroyed once in mixed graph.";
    EXPECT_EQ(CountTextureHandleOccurrences(device.GetDestroyedTextures(), importedHandle), 0u)
        << "Imported texture must never be destroyed in mixed graph.";
}