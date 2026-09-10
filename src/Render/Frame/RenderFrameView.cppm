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

    struct RenderWorldView
    {
        RenderWorldView(
            const Camera& camera,
            std::span<const Material> materials,
            std::span<const DrawItem> drawItems,
            std::span<const SkinnedDrawItem> skinnedDrawItems,
            std::span<const Light> lights,
            std::span<const Particle> particles,
            std::span<const ParticleEmitter> particleEmitters,
            rhi::TextureDescIndex skyboxDescIndex) noexcept
            : camera_(&camera)
            , materials_(materials)
            , drawItems_(drawItems)
            , skinnedDrawItems_(skinnedDrawItems)
            , lights_(lights)
            , particles_(particles)
            , particleEmitters_(particleEmitters)
            , skyboxDescIndex_(skyboxDescIndex)
        {
        }

        [[nodiscard]] const Camera& GetCamera() const noexcept { return *camera_; }
        [[nodiscard]] std::span<const Material> GetMaterials() const noexcept { return materials_; }
        [[nodiscard]] std::span<const DrawItem> GetDrawItems() const noexcept { return drawItems_; }
        [[nodiscard]] std::span<const SkinnedDrawItem> GetSkinnedDrawItems() const noexcept { return skinnedDrawItems_; }
        [[nodiscard]] std::span<const Light> GetLights() const noexcept { return lights_; }
        [[nodiscard]] std::span<const Particle> GetParticles() const noexcept { return particles_; }
        [[nodiscard]] std::span<const ParticleEmitter> GetParticleEmitters() const noexcept { return particleEmitters_; }
        [[nodiscard]] rhi::TextureDescIndex GetSkyboxDescIndex() const noexcept { return skyboxDescIndex_; }

        [[nodiscard]] const Material& GetMaterial(MaterialHandle handle) const
        {
            if (handle.id == 0 || handle.id > materials_.size())
            {
                throw std::runtime_error("Scene::GetMaterial: invalid MaterialHandle");
            }
            return materials_[handle.id - 1];
        }

    private:
        const Camera* camera_;
        std::span<const Material> materials_;
        std::span<const DrawItem> drawItems_;
        std::span<const SkinnedDrawItem> skinnedDrawItems_;
        std::span<const Light> lights_;
        std::span<const Particle> particles_;
        std::span<const ParticleEmitter> particleEmitters_;
        rhi::TextureDescIndex skyboxDescIndex_;
    };

    struct RenderDebugView
    {
        RenderDebugView(
            const DebugRay& pickRay,
            const GameplayMovementDebugState& gameplayMovement,
            std::span<const ExternalDebugLine> lines,
            std::span<const ExternalDebugTriangle> triangles,
            std::span<const ExternalDebugCapsule> capsules,
            std::span<const ExternalDebugArrow> arrows,
            std::span<const ExternalDebugBox> boxes,
            std::span<const ExternalDebugSphere> spheres) noexcept
            : pickRay_(&pickRay)
            , gameplayMovement_(&gameplayMovement)
            , lines_(lines)
            , triangles_(triangles)
            , capsules_(capsules)
            , arrows_(arrows)
            , boxes_(boxes)
            , spheres_(spheres)
        {
        }

        [[nodiscard]] const DebugRay& GetPickRay() const noexcept { return *pickRay_; }
        [[nodiscard]] const GameplayMovementDebugState& GetGameplayMovement() const noexcept { return *gameplayMovement_; }
        [[nodiscard]] std::span<const ExternalDebugLine> GetLines() const noexcept { return lines_; }
        [[nodiscard]] std::span<const ExternalDebugTriangle> GetTriangles() const noexcept { return triangles_; }
        [[nodiscard]] std::span<const ExternalDebugCapsule> GetCapsules() const noexcept { return capsules_; }
        [[nodiscard]] std::span<const ExternalDebugArrow> GetArrows() const noexcept { return arrows_; }
        [[nodiscard]] std::span<const ExternalDebugBox> GetBoxes() const noexcept { return boxes_; }
        [[nodiscard]] std::span<const ExternalDebugSphere> GetSpheres() const noexcept { return spheres_; }

    private:
        const DebugRay* pickRay_;
        const GameplayMovementDebugState* gameplayMovement_;
        std::span<const ExternalDebugLine> lines_;
        std::span<const ExternalDebugTriangle> triangles_;
        std::span<const ExternalDebugCapsule> capsules_;
        std::span<const ExternalDebugArrow> arrows_;
        std::span<const ExternalDebugBox> boxes_;
        std::span<const ExternalDebugSphere> spheres_;
    };

    struct RenderEditorView
    {
        RenderEditorView(
            std::span<const int> selectedLights,
            int selectedParticleEmitter,
            std::span<const int> selectedDrawItems,
            std::span<const int> selectedSkinnedDrawItems,
            bool drawSelectedSkinnedSkeleton,
            bool drawSelectedSkinnedBounds,
            GizmoMode gizmoMode,
            const TranslateGizmoState& translateGizmo,
            const RotateGizmoState& rotateGizmo,
            const ScaleGizmoState& scaleGizmo) noexcept
            : selectedLights_(selectedLights)
            , selectedParticleEmitter_(selectedParticleEmitter)
            , selectedDrawItems_(selectedDrawItems)
            , selectedSkinnedDrawItems_(selectedSkinnedDrawItems)
            , drawSelectedSkinnedSkeleton_(drawSelectedSkinnedSkeleton)
            , drawSelectedSkinnedBounds_(drawSelectedSkinnedBounds)
            , gizmoMode_(gizmoMode)
            , translateGizmo_(&translateGizmo)
            , rotateGizmo_(&rotateGizmo)
            , scaleGizmo_(&scaleGizmo)
        {
        }

        [[nodiscard]] std::span<const int> GetSelectedLights() const noexcept { return selectedLights_; }
        [[nodiscard]] int GetSelectedParticleEmitter() const noexcept { return selectedParticleEmitter_; }
        [[nodiscard]] std::span<const int> GetSelectedDrawItems() const noexcept { return selectedDrawItems_; }
        [[nodiscard]] std::span<const int> GetSelectedSkinnedDrawItems() const noexcept { return selectedSkinnedDrawItems_; }
        [[nodiscard]] bool GetDrawSelectedSkinnedSkeleton() const noexcept { return drawSelectedSkinnedSkeleton_; }
        [[nodiscard]] bool GetDrawSelectedSkinnedBounds() const noexcept { return drawSelectedSkinnedBounds_; }
        [[nodiscard]] GizmoMode GetGizmoMode() const noexcept { return gizmoMode_; }
        [[nodiscard]] const TranslateGizmoState& GetTranslateGizmo() const noexcept { return *translateGizmo_; }
        [[nodiscard]] const RotateGizmoState& GetRotateGizmo() const noexcept { return *rotateGizmo_; }
        [[nodiscard]] const ScaleGizmoState& GetScaleGizmo() const noexcept { return *scaleGizmo_; }

    private:
        std::span<const int> selectedLights_;
        int selectedParticleEmitter_;
        std::span<const int> selectedDrawItems_;
        std::span<const int> selectedSkinnedDrawItems_;
        bool drawSelectedSkinnedSkeleton_;
        bool drawSelectedSkinnedBounds_;
        GizmoMode gizmoMode_;
        const TranslateGizmoState* translateGizmo_;
        const RotateGizmoState* rotateGizmo_;
        const ScaleGizmoState* scaleGizmo_;
    };

    // RenderFrameView and its nested views are non-owning synchronous views.
    // Every borrowed reference/span must remain valid for the immediate
    // RenderFrame call. Do not queue, retain, cache, or consume them
    // asynchronously.
    struct RenderFrameView
    {
        RenderFrameView(
            RenderWorldView world,
            RenderDebugView debug,
            RenderEditorView editor,
            AnimationRuntimeOverlaySnapshot animationRuntimeOverlaySnapshot)
            : world_(world)
            , debug_(debug)
            , editor_(editor)
            , animationRuntimeOverlaySnapshot_(std::move(animationRuntimeOverlaySnapshot))
        {
        }

        [[nodiscard]] const RenderWorldView& GetWorld() const noexcept { return world_; }
        [[nodiscard]] const RenderDebugView& GetDebug() const noexcept { return debug_; }
        [[nodiscard]] const RenderEditorView& GetEditor() const noexcept { return editor_; }
        [[nodiscard]] const AnimationRuntimeOverlaySnapshot& GetAnimationRuntimeOverlaySnapshot() const noexcept
        {
            return animationRuntimeOverlaySnapshot_;
        }

    private:
        RenderWorldView world_;
        RenderDebugView debug_;
        RenderEditorView editor_;
        AnimationRuntimeOverlaySnapshot animationRuntimeOverlaySnapshot_{};
    };
}
