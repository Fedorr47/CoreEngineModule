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

/*--------------- CPU-side RenderGraph attachment routing start ------------------------------*/

// These tests protect CPU-side RenderGraph attachment routing. Each case verifies that a
// specific attachment shape reaches the expected RHI framebuffer/begin-pass path without
// requiring a real GPU backend.
TEST(RenderGraphPassAttachments, SwapChainPassUsesSwapChainFramebufferPath)
{
    FakeRHIDevice device{};
    const rhi::Extent2D extent{800u, 600u};
    const rhi::Format format = rhi::Format::BGRA8_UNORM;
    auto swapChain = MakeTestSwapChain(extent, format);

    renderGraph::RenderGraph graph;
    rhi::ClearDesc clear{};
    graph.AddSwapChainPass("SwapChainOnly", clear, [](renderGraph::PassContext& ctx)
    {
        ctx.commandList.Draw(3);
    });

    graph.Execute(device, swapChain);

    // Protects the swapchain pass contract: backbuffers are externally owned and should not require
    // transient framebuffer creation.
    EXPECT_TRUE(device.GetFrameBufferCreates().empty())
        << "Swapchain passes should bind the swapchain backbuffer directly and skip transient framebuffer creation.";

    const auto& submitted = device.GetSubmittedCommandLists().front();
    bool sawSwapChainBegin = false;
    for (const auto& command : submitted.commands)
    {
        if (const auto* begin = std::get_if<rhi::CommandBeginPass>(&command))
        {
            sawSwapChainBegin = begin->desc.swapChain != nullptr;
            if (sawSwapChainBegin)
            {
                // Verifies that the pass binds the current swapchain image as the active render target.
                EXPECT_EQ(begin->desc.frameBuffer.id, swapChain.GetCurrentBackBuffer().id);
                break;
            }
        }
    }
    
    // Ensures the command stream actually used the swapchain begin-pass path,
    // not just skipped framebuffer creation.
    EXPECT_TRUE(sawSwapChainBegin);
}

TEST(RenderGraphPassAttachments, MultipleColorAttachmentsUseMrtFramebufferPath)
{
    FakeRHIDevice device{};
    const rhi::Extent2D extent{256u, 256u};
    const rhi::Format format = rhi::Format::RGBA8_UNORM;
    auto swapChain = MakeTestSwapChain(extent, format);

    renderGraph::RenderGraph graph;
    const auto colorA = graph.CreateTexture({.extent = extent, .format = format, .usage = renderGraph::ResourceUsage::RenderTarget, .debugName = "mrt_a"});
    const auto colorB = graph.CreateTexture({.extent = extent, .format = format, .usage = renderGraph::ResourceUsage::RenderTarget, .debugName = "mrt_b"});

    renderGraph::PassAttachments attachments{};
    attachments.bindDepthStencil = false;
    attachments.colors = {colorA, colorB};
    graph.AddPass("MrtPass", std::move(attachments), [](renderGraph::PassContext& ctx)
    {
        ctx.commandList.Draw(3);
    });

    graph.Execute(device, swapChain);

    // Protects the multiple-render-target branch where several color attachments must create
    // one framebuffer that preserves all requested render targets.
    ASSERT_EQ(device.GetFrameBufferCreates().size(), 1u);
    const auto& fb = device.GetFrameBufferCreates().front();
    EXPECT_EQ(fb.kind, "mrt");
    EXPECT_EQ(fb.colors.size(), 2u);
    EXPECT_TRUE(fb.colors[0]);
    EXPECT_TRUE(fb.colors[1]);
    EXPECT_NE(fb.colors[0].id, fb.colors[1].id);
}

TEST(RenderGraphPassAttachments, CubeFaceAttachmentUsesCubeFaceFramebufferPath)
{
    FakeRHIDevice device{};
    const rhi::Extent2D extent{128u, 128u};
    auto swapChain = MakeTestSwapChain(extent, rhi::Format::RGBA16_FLOAT);

    renderGraph::RenderGraph graph;
    const auto cubeColor = graph.CreateTexture({
        .extent = extent,
        .format = rhi::Format::RGBA16_FLOAT,
        .usage = renderGraph::ResourceUsage::RenderTarget,
        .type = renderGraph::TextureType::Cube,
        .debugName = "cube_face_rt"});

    renderGraph::PassAttachments attachments{};
    attachments.bindDepthStencil = false;
    attachments.colors = {cubeColor};
    attachments.colorCubeFace = 3u;
    graph.AddPass("CubeFacePass", std::move(attachments), [](renderGraph::PassContext& ctx)
    {
        ctx.commandList.Draw(3);
    });

    graph.Execute(device, swapChain);

    // Protects the all-faces cubemap path used when the pass targets the whole cube instead of
    // a single face or face/mip slice.
    ASSERT_EQ(device.GetFrameBufferCreates().size(), 1u);
    const auto& fb = device.GetFrameBufferCreates().front();
    EXPECT_EQ(fb.kind, "cube_face");
    ASSERT_TRUE(fb.cubeFace.has_value());
    EXPECT_EQ(*fb.cubeFace, 3u);
    EXPECT_FALSE(fb.cubeMip.has_value());
    EXPECT_FALSE(fb.cubeAllFaces);
}

TEST(RenderGraphPassAttachments, CubeMipAttachmentPropagatesMipLevel)
{
    FakeRHIDevice device{};
    const rhi::Extent2D extent{128u, 128u};
    auto swapChain = MakeTestSwapChain(extent, rhi::Format::RGBA16_FLOAT);

    renderGraph::RenderGraph graph;
    const auto cubeColor = graph.CreateTexture({
        .extent = extent,
        .format = rhi::Format::RGBA16_FLOAT,
        .usage = renderGraph::ResourceUsage::RenderTarget,
        .type = renderGraph::TextureType::Cube,
        .debugName = "cube_mip_rt"});

    renderGraph::PassAttachments attachments{};
    attachments.bindDepthStencil = false;
    attachments.colors = {cubeColor};
    attachments.colorCubeFace = 5u;
    attachments.colorCubeMip = 2u;
    graph.AddPass("CubeMipPass", std::move(attachments), [](renderGraph::PassContext& ctx)
    {
        ctx.commandList.Draw(3);
    });

    graph.Execute(device, swapChain);

    // Protects the cubemap slice path: RenderGraph must preserve both face and mip selection
    // before the pass reaches the backend framebuffer creation API.
    ASSERT_EQ(device.GetFrameBufferCreates().size(), 1u);
    const auto& fb = device.GetFrameBufferCreates().front();
    EXPECT_EQ(fb.kind, "cube_face_mip");
    ASSERT_TRUE(fb.cubeFace.has_value());
    ASSERT_TRUE(fb.cubeMip.has_value());
    EXPECT_EQ(*fb.cubeFace, 5u);
    EXPECT_EQ(*fb.cubeMip, 2u);
}

TEST(RenderGraphPassAttachments, CubeAllFacesAttachmentUsesAllFacesFramebufferPath)
{
    FakeRHIDevice device{};
    const rhi::Extent2D extent{128u, 128u};
    auto swapChain = MakeTestSwapChain(extent, rhi::Format::RGBA16_FLOAT);

    renderGraph::RenderGraph graph;
    const auto cubeColor = graph.CreateTexture({
        .extent = extent,
        .format = rhi::Format::RGBA16_FLOAT,
        .usage = renderGraph::ResourceUsage::RenderTarget,
        .type = renderGraph::TextureType::Cube,
        .debugName = "cube_all_faces_rt"});

    renderGraph::PassAttachments attachments{};
    attachments.bindDepthStencil = false;
    attachments.colors = {cubeColor};
    attachments.colorCubeAllFaces = true;
    graph.AddPass("CubeAllFacesPass", std::move(attachments), [](renderGraph::PassContext& ctx)
    {
        ctx.commandList.Draw(3);
    });

    graph.Execute(device, swapChain);

    // Protects the all-faces cubemap path: targeting the whole cube must not be treated as
    // a single face or face/mip slice.
    ASSERT_EQ(device.GetFrameBufferCreates().size(), 1u);
    const auto& fb = device.GetFrameBufferCreates().front();
    EXPECT_EQ(fb.kind, "cube");
    EXPECT_TRUE(fb.cubeAllFaces);
    EXPECT_FALSE(fb.cubeFace.has_value());
    EXPECT_FALSE(fb.cubeMip.has_value());
}
/*--------------- CPU-side RenderGraph attachment routing end ------------------------------*/