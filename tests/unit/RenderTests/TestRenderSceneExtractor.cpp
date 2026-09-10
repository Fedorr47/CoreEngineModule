#include <gtest/gtest.h>
#include <stdexcept>

import core;

using namespace rendern;

TEST(RenderSceneExtractor, WorldDataIsCapturedIndependentlyFromSceneStorage)
{
    Scene scene{};
    scene.camera.fovYDeg = 73.0f;

    Material material{};
    material.params.metallic = 0.25f;
    const MaterialHandle materialHandle = scene.AddMaterial(material);

    DrawItem draw{};
    draw.transform.position.x = 4.0f;
    draw.material = materialHandle;
    scene.drawItems.push_back(draw);

    Light light{};
    light.intensity = 2.5f;
    scene.lights.push_back(light);

    Particle particle{};
    particle.age = 0.4f;
    scene.particles.push_back(particle);

    ParticleEmitter emitter{};
    emitter.name = "Captured emitter";
    scene.particleEmitters.push_back(emitter);
    scene.skyboxDescIndex = 17;

    const RenderFrameView view = RenderSceneExtractor::BuildFrameView(scene);
    const RenderWorldSnapshot& world = view.GetWorld();

    scene.camera.fovYDeg = 45.0f;
    scene.materials.front().params.metallic = 0.9f;
    scene.drawItems.front().transform.position.x = 9.0f;
    scene.lights.front().intensity = 8.0f;
    scene.particles.front().age = 0.8f;
    scene.particleEmitters.front().name = "Changed emitter";
    scene.materials.reserve(64);
    scene.drawItems.reserve(64);
    scene.lights.reserve(64);
    scene.particles.reserve(64);
    scene.particleEmitters.reserve(64);

    EXPECT_FLOAT_EQ(world.GetCamera().fovYDeg, 73.0f);
    EXPECT_FLOAT_EQ(world.GetMaterial(materialHandle).params.metallic, 0.25f);
    ASSERT_EQ(world.GetDrawItems().size(), 1u);
    EXPECT_FLOAT_EQ(world.GetDrawItems().front().transform.position.x, 4.0f);
    EXPECT_NE(world.GetDrawItems().data(), scene.drawItems.data());
    ASSERT_EQ(world.GetLights().size(), 1u);
    EXPECT_FLOAT_EQ(world.GetLights().front().intensity, 2.5f);
    ASSERT_EQ(world.GetParticles().size(), 1u);
    EXPECT_FLOAT_EQ(world.GetParticles().front().age, 0.4f);
    ASSERT_EQ(world.GetParticleEmitters().size(), 1u);
    EXPECT_EQ(world.GetParticleEmitters().front().name, "Captured emitter");
    EXPECT_EQ(world.GetSkyboxDescIndex(), 17u);
}

TEST(RenderSceneExtractor, SkinnedFrameMatricesAreCapturedWithoutAnimatorState)
{
    Scene scene{};
    SkinnedDrawItem item{};
    item.transform.position.y = 3.0f;
    item.animator.skinMatrices.emplace_back(2.0f);
    item.animator.globalMatrices.emplace_back(4.0f);
    scene.skinnedDrawItems.push_back(std::move(item));

    const RenderFrameView view = RenderSceneExtractor::BuildFrameView(scene);
    scene.skinnedDrawItems.front().transform.position.y = 7.0f;
    scene.skinnedDrawItems.front().animator.skinMatrices.front()[0][0] = 8.0f;
    scene.skinnedDrawItems.front().animator.globalMatrices.front()[0][0] = 9.0f;
    scene.skinnedDrawItems.reserve(64);

    const auto items = view.GetWorld().GetSkinnedDrawItems();
    ASSERT_EQ(items.size(), 1u);
    EXPECT_FLOAT_EQ(items.front().transform.position.y, 3.0f);
    ASSERT_EQ(items.front().skinMatrices.size(), 1u);
    ASSERT_EQ(items.front().globalMatrices.size(), 1u);
    EXPECT_FLOAT_EQ(items.front().skinMatrices.front()[0][0], 2.0f);
    EXPECT_FLOAT_EQ(items.front().globalMatrices.front()[0][0], 4.0f);
}

TEST(RenderSceneExtractor, DebugDataIsCapturedIndependentlyFromSceneStorage)
{
    Scene scene{};
    scene.debugPickRay.enabled = true;
    scene.debugPickRay.origin.x = 1.0f;

    GameplayMovementDebugSample movementSample{};
    movementSample.planarSpeed = 2.0f;
    scene.gameplayMovementDebug.samples.push_back(movementSample);

    ExternalDebugLine line{};
    line.start.x = 3.0f;
    scene.externalDebugLines.push_back(line);
    ExternalDebugTriangle triangle{};
    triangle.a.y = 4.0f;
    scene.externalDebugTriangles.push_back(triangle);
    ExternalDebugCapsule capsule{};
    capsule.radius = 5.0f;
    scene.externalDebugCapsules.push_back(capsule);
    ExternalDebugArrow arrow{};
    arrow.end.z = 6.0f;
    scene.externalDebugArrows.push_back(arrow);
    ExternalDebugBox box{};
    box.halfExtents.x = 7.0f;
    scene.externalDebugBoxes.push_back(box);
    ExternalDebugSphere sphere{};
    sphere.radius = 8.0f;
    scene.externalDebugSpheres.push_back(sphere);

    const RenderFrameView view = RenderSceneExtractor::BuildFrameView(scene);
    const RenderDebugSnapshot& debug = view.GetDebug();

    scene.debugPickRay.enabled = false;
    scene.debugPickRay.origin.x = 10.0f;
    scene.gameplayMovementDebug.samples.front().planarSpeed = 20.0f;
    scene.gameplayMovementDebug.samples.clear();
    scene.gameplayMovementDebug.samples.reserve(64);
    scene.externalDebugLines.clear();
    scene.externalDebugTriangles.clear();
    scene.externalDebugCapsules.clear();
    scene.externalDebugArrows.clear();
    scene.externalDebugBoxes.clear();
    scene.externalDebugSpheres.clear();
    scene.externalDebugLines.reserve(64);
    scene.externalDebugTriangles.reserve(64);
    scene.externalDebugCapsules.reserve(64);
    scene.externalDebugArrows.reserve(64);
    scene.externalDebugBoxes.reserve(64);
    scene.externalDebugSpheres.reserve(64);

    EXPECT_TRUE(debug.GetPickRay().enabled);
    EXPECT_FLOAT_EQ(debug.GetPickRay().origin.x, 1.0f);
    ASSERT_EQ(debug.GetGameplayMovement().samples.size(), 1u);
    EXPECT_FLOAT_EQ(debug.GetGameplayMovement().samples.front().planarSpeed, 2.0f);
    ASSERT_EQ(debug.GetLines().size(), 1u);
    EXPECT_FLOAT_EQ(debug.GetLines().front().start.x, 3.0f);
    EXPECT_NE(debug.GetLines().data(), scene.externalDebugLines.data());
    ASSERT_EQ(debug.GetTriangles().size(), 1u);
    EXPECT_FLOAT_EQ(debug.GetTriangles().front().a.y, 4.0f);
    ASSERT_EQ(debug.GetCapsules().size(), 1u);
    EXPECT_FLOAT_EQ(debug.GetCapsules().front().radius, 5.0f);
    ASSERT_EQ(debug.GetArrows().size(), 1u);
    EXPECT_FLOAT_EQ(debug.GetArrows().front().end.z, 6.0f);
    ASSERT_EQ(debug.GetBoxes().size(), 1u);
    EXPECT_FLOAT_EQ(debug.GetBoxes().front().halfExtents.x, 7.0f);
    ASSERT_EQ(debug.GetSpheres().size(), 1u);
    EXPECT_FLOAT_EQ(debug.GetSpheres().front().radius, 8.0f);
}

TEST(RenderSceneExtractor, EditorRemainsBorrowedAndOverlayIsCopied)
{
    Scene scene{};
    scene.editorSelectedLights.push_back(3);
    AnimationRuntimeDebugSample sample{};
    sample.nodeName = "Character";
    scene.animationRuntimeDebug.samples.push_back(sample);

    const RenderFrameView view = RenderSceneExtractor::BuildFrameView(scene);

    EXPECT_EQ(view.GetEditor().GetSelectedLights().data(), scene.editorSelectedLights.data());
    scene.animationRuntimeDebug.samples.front().nodeName = "Changed";
    EXPECT_EQ(view.GetAnimationRuntimeOverlaySnapshot().samples.front().nodeLabel, "Character");
}

TEST(RenderFrameView, MaterialLookupPreservesSceneSemantics)
{
    Scene scene{};
    const MaterialHandle handle = scene.AddMaterial(Material{});
    const RenderFrameView view = RenderSceneExtractor::BuildFrameView(scene);

    EXPECT_NE(&view.GetWorld().GetMaterial(handle), &scene.materials.front());
    EXPECT_THROW(view.GetWorld().GetMaterial(MaterialHandle{}), std::runtime_error);
    EXPECT_THROW(view.GetWorld().GetMaterial(MaterialHandle{ 2u }), std::runtime_error);
}
