module;

export module core:render_frame_view;

import :scene;

export namespace rendern
{
    struct RenderFrameView
    {
        explicit RenderFrameView(const Scene& inScene) noexcept
            : scene_(&inScene)
        {
        }

        [[nodiscard]] const Scene& GetScene() const noexcept
        {
            return *scene_;
        }

    private:
        const Scene* scene_;
    };
}
