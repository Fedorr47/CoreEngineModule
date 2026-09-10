#include <gtest/gtest.h>
#include <stdexcept>

import core;

using namespace rendern;

TEST(RenderSceneExtractor, ExposesExplicitBorrowedRenderInputsAndCopiedOverlay)
{
    Scene scene{};
    scene.camera.fovYDeg = 73.0f;
    scene.drawItems.emplace_back();
    scene.lights.emplace_back();
    scene.skyboxDescIndex = 17;
    scene.externalDebugLines.push_back(ExternalDebugLine{
        .start = { 1.0f, 2.0f, 3.0f },
        .end = { 4.0f, 5.0f, 6.0f },
        .rgba = 0xff00ffffu
    });
    scene.editorSelectedLights.push_back(3);
    scene.editorDrawSelectedSkinnedSkeleton = true;
    AnimationRuntimeDebugSample overlaySample{};
    overlaySample.nodeName = "Character";
    overlaySample.currentStateName = "Run";
    scene.animationRuntimeDebug.samples.push_back(overlaySample);

    const RenderFrameView view = RenderSceneExtractor::BuildFrameView(scene);
    const RenderWorldView& world = view.GetWorld();
    const RenderDebugView& debug = view.GetDebug();
    const RenderEditorView& editor = view.GetEditor();

    EXPECT_EQ(&world.GetCamera(), &scene.camera);
    EXPECT_FLOAT_EQ(world.GetCamera().fovYDeg, 73.0f);
    ASSERT_EQ(world.GetDrawItems().size(), 1u);
    EXPECT_EQ(world.GetDrawItems().data(), scene.drawItems.data());
    ASSERT_EQ(world.GetLights().size(), 1u);
    EXPECT_EQ(world.GetLights().data(), scene.lights.data());
    EXPECT_EQ(world.GetSkyboxDescIndex(), 17u);
    ASSERT_EQ(debug.GetLines().size(), 1u);
    EXPECT_EQ(debug.GetLines().data(), scene.externalDebugLines.data());
    ASSERT_EQ(editor.GetSelectedLights().size(), 1u);
    EXPECT_EQ(editor.GetSelectedLights().front(), 3);
    EXPECT_TRUE(editor.GetDrawSelectedSkinnedSkeleton());

    ASSERT_EQ(view.GetAnimationRuntimeOverlaySnapshot().samples.size(), 1u);
    EXPECT_EQ(view.GetAnimationRuntimeOverlaySnapshot().samples.front().nodeLabel, "Character");
    EXPECT_EQ(view.GetAnimationRuntimeOverlaySnapshot().samples.front().currentStateName, "Run");

    scene.animationRuntimeDebug.samples.front().nodeName = "Changed after extraction";
    EXPECT_EQ(view.GetAnimationRuntimeOverlaySnapshot().samples.front().nodeLabel, "Character");
}

TEST(RenderFrameView, MaterialLookupPreservesSceneSemantics)
{
    Scene scene{};
    Material material{};
    const MaterialHandle handle = scene.AddMaterial(material);
    const RenderFrameView view = RenderSceneExtractor::BuildFrameView(scene);

    EXPECT_EQ(&view.GetWorld().GetMaterial(handle), &scene.materials.front());
    EXPECT_THROW(view.GetWorld().GetMaterial(MaterialHandle{}), std::runtime_error);
    EXPECT_THROW(view.GetWorld().GetMaterial(MaterialHandle{ 2u }), std::runtime_error);
}
