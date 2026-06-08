module;

export module core:render_frame_view;

import :scene;
import std;

export namespace rendern
{
    struct AnimationRuntimeOverlaySample
    {
        std::string nodeLabel;
        std::string currentStateName;
        std::string modeName;
        std::string primaryClipName;
        std::string secondaryClipName;
        std::string tertiaryClipName;
        std::string blendParameterNameX;
        std::string blendParameterNameY;
        std::string lastNotifyId;
        float blendParameterValueX{ 0.0f };
        float blendParameterValueY{ 0.0f };
        float normalizedTime{ 0.0f };
        float primaryWeight{ 0.0f };
        float secondaryWeight{ 0.0f };
        float tertiaryWeight{ 0.0f };
        bool transitionActive{ false };
        bool controlled{ false };
    };

    // Copied UI/debug overlay payload for the frame. This is intentionally
    // smaller than the Animation Runtime panel ViewModel, not a thread-safety
    // boundary, and not the final render frame packet.
    struct AnimationRuntimeOverlaySnapshot
    {
        std::vector<AnimationRuntimeOverlaySample> samples;

        [[nodiscard]] bool Empty() const noexcept
        {
            return samples.empty();
        }
    };
    
    struct RenderFrameView
    {
        RenderFrameView(const Scene& inScene, AnimationRuntimeOverlaySnapshot animationRuntimeOverlaySnapshot)
            : scene_(&inScene)
            , animationRuntimeOverlaySnapshot_(std::move(animationRuntimeOverlaySnapshot))
        {
        }

        [[nodiscard]] const Scene& GetScene() const noexcept
        {
            return *scene_;
        }
        
        [[nodiscard]] const AnimationRuntimeOverlaySnapshot& GetAnimationRuntimeOverlaySnapshot() const noexcept
        {
            return animationRuntimeOverlaySnapshot_;
        }
    
    private:
        const Scene* scene_;
        AnimationRuntimeOverlaySnapshot animationRuntimeOverlaySnapshot_{};
    };
}
