module;

export module core:render_scene_extractor;

import :scene;
import :render_frame_view;

export namespace rendern
{
    class RenderSceneExtractor
    {
    public:
        [[nodiscard]] static RenderFrameView BuildFrameView(const Scene& scene) noexcept
        {
            return RenderFrameView(scene);
        }
    };
}
