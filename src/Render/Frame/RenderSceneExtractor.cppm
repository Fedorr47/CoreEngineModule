module;

export module core:render_scene_extractor;

import std;
import :scene;
import :render_frame_view;

export namespace rendern
{
    class RenderSceneExtractor
    {
    public:
        [[nodiscard]] static RenderFrameView BuildFrameView(const Scene& scene)
        {
            RenderWorldSnapshot world(
                scene.camera,
                scene.materials,
                scene.drawItems,
                BuildSkinnedDrawItems(scene),
                scene.lights,
                scene.particles,
                scene.particleEmitters,
                scene.skyboxDescIndex);
            RenderDebugSnapshot debug(
                scene.debugPickRay,
                scene.gameplayMovementDebug,
                scene.externalDebugLines,
                scene.externalDebugTriangles,
                scene.externalDebugCapsules,
                scene.externalDebugArrows,
                scene.externalDebugBoxes,
                scene.externalDebugSpheres);
            const RenderEditorView editor(
                scene.editorSelectedLights,
                scene.editorSelectedParticleEmitter,
                scene.editorSelectedDrawItems,
                scene.editorSelectedSkinnedDrawItems,
                scene.editorDrawSelectedSkinnedSkeleton,
                scene.editorDrawSelectedSkinnedBounds,
                scene.editorGizmoMode,
                scene.editorTranslateGizmo,
                scene.editorRotateGizmo,
                scene.editorScaleGizmo);

            return RenderFrameView(
                std::move(world),
                std::move(debug),
                editor,
                BuildAnimationRuntimeOverlaySnapshot(scene));
        }
    private:
        [[nodiscard]] static std::vector<RenderSkinnedDrawItem> BuildSkinnedDrawItems(const Scene& scene)
        {
            std::vector<RenderSkinnedDrawItem> result;
            result.reserve(scene.skinnedDrawItems.size());
            for (const SkinnedDrawItem& source : scene.skinnedDrawItems)
            {
                RenderSkinnedDrawItem item{};
                item.asset = source.asset;
                item.transform = source.transform;
                item.material = source.material;
                item.submeshMaterials = source.submeshMaterials;
                item.skinMatrices = source.animator.skinMatrices;
                item.globalMatrices = source.animator.globalMatrices;
                if (source.animator.skeleton != nullptr)
                {
                    item.skeletonParentIndices.reserve(source.animator.skeleton->bones.size());
                    for (const SkeletonBone& bone : source.animator.skeleton->bones)
                    {
                        item.skeletonParentIndices.push_back(bone.parentIndex);
                    }
                }
                result.push_back(std::move(item));
            }
            return result;
        }

        [[nodiscard]] static AnimationRuntimeOverlaySnapshot BuildAnimationRuntimeOverlaySnapshot(const Scene& scene)
        {
            AnimationRuntimeOverlaySnapshot overlaySnapshot{};
            const auto& sourceSamples = scene.animationRuntimeDebug.samples;
            overlaySnapshot.samples.reserve(sourceSamples.size());
            for (const AnimationRuntimeDebugSample& source : sourceSamples)
            {
                AnimationRuntimeOverlaySample overlaySample{};
                overlaySample.nodeLabel = source.nodeName;
                overlaySample.currentStateName = source.currentStateName;
                overlaySample.modeName = source.modeName;
                overlaySample.primaryClipName = source.primaryClipName;
                overlaySample.secondaryClipName = source.secondaryClipName;
                overlaySample.tertiaryClipName = source.tertiaryClipName;
                overlaySample.blendParameterNameX = source.blendParameterNameX;
                overlaySample.blendParameterNameY = source.blendParameterNameY;
                overlaySample.lastNotifyId = source.lastNotifyId;
                overlaySample.blendParameterValueX = source.blendParameterValueX;
                overlaySample.blendParameterValueY = source.blendParameterValueY;
                overlaySample.normalizedTime = source.normalizedTime;
                overlaySample.primaryWeight = source.primaryWeight;
                overlaySample.secondaryWeight = source.secondaryWeight;
                overlaySample.tertiaryWeight = source.tertiaryWeight;
                overlaySample.transitionActive = source.transitionActive;
                overlaySample.controlled = source.controlled;
                overlaySnapshot.samples.push_back(std::move(overlaySample));
            }
            return overlaySnapshot;
        }
    };
}
