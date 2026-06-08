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
            return RenderFrameView(scene, BuildAnimationRuntimeOverlaySnapshot(scene));
        }
    private:
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
