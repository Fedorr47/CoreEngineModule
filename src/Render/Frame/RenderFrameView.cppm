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
    
    // Non-owning synchronous frame view. Every borrowed reference/span must
    // remain valid for the immediate RenderFrame call. Do not queue, retain,
    // or consume this view asynchronously.
    struct RenderFrameView
    {
        RenderFrameView(
            const Camera& camera,
            std::span<const Material> materials,
            std::span<const DrawItem> drawItems,
            std::span<const SkinnedDrawItem> skinnedDrawItems,
            std::span<const Light> lights,
            std::span<const Particle> particles,
            std::span<const ParticleEmitter> particleEmitters,
            rhi::TextureDescIndex skyboxDescIndex,
            const DebugRay& debugPickRay,
            const GameplayMovementDebugState& gameplayMovementDebug,
            std::span<const ExternalDebugLine> externalDebugLines,
            std::span<const ExternalDebugTriangle> externalDebugTriangles,
            std::span<const ExternalDebugCapsule> externalDebugCapsules,
            std::span<const ExternalDebugArrow> externalDebugArrows,
            std::span<const ExternalDebugBox> externalDebugBoxes,
            std::span<const ExternalDebugSphere> externalDebugSpheres,
            std::span<const int> editorSelectedLights,
            int editorSelectedParticleEmitter,
            std::span<const int> editorSelectedDrawItems,
            std::span<const int> editorSelectedSkinnedDrawItems,
            bool editorDrawSelectedSkinnedSkeleton,
            bool editorDrawSelectedSkinnedBounds,
            GizmoMode editorGizmoMode,
            const TranslateGizmoState& editorTranslateGizmo,
            const RotateGizmoState& editorRotateGizmo,
            const ScaleGizmoState& editorScaleGizmo,
            AnimationRuntimeOverlaySnapshot animationRuntimeOverlaySnapshot)
            : camera_(&camera)
            , materials_(materials)
            , drawItems_(drawItems)
            , skinnedDrawItems_(skinnedDrawItems)
            , lights_(lights)
            , particles_(particles)
            , particleEmitters_(particleEmitters)
            , skyboxDescIndex_(skyboxDescIndex)
            , debugPickRay_(&debugPickRay)
            , gameplayMovementDebug_(&gameplayMovementDebug)
            , externalDebugLines_(externalDebugLines)
            , externalDebugTriangles_(externalDebugTriangles)
            , externalDebugCapsules_(externalDebugCapsules)
            , externalDebugArrows_(externalDebugArrows)
            , externalDebugBoxes_(externalDebugBoxes)
            , externalDebugSpheres_(externalDebugSpheres)
            , editorSelectedLights_(editorSelectedLights)
            , editorSelectedParticleEmitter_(editorSelectedParticleEmitter)
            , editorSelectedDrawItems_(editorSelectedDrawItems)
            , editorSelectedSkinnedDrawItems_(editorSelectedSkinnedDrawItems)
            , editorDrawSelectedSkinnedSkeleton_(editorDrawSelectedSkinnedSkeleton)
            , editorDrawSelectedSkinnedBounds_(editorDrawSelectedSkinnedBounds)
            , editorGizmoMode_(editorGizmoMode)
            , editorTranslateGizmo_(&editorTranslateGizmo)
            , editorRotateGizmo_(&editorRotateGizmo)
            , editorScaleGizmo_(&editorScaleGizmo)
            , animationRuntimeOverlaySnapshot_(std::move(animationRuntimeOverlaySnapshot))
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
        [[nodiscard]] const DebugRay& GetDebugPickRay() const noexcept { return *debugPickRay_; }
        [[nodiscard]] const GameplayMovementDebugState& GetGameplayMovementDebug() const noexcept { return *gameplayMovementDebug_; }
        [[nodiscard]] std::span<const ExternalDebugLine> GetExternalDebugLines() const noexcept { return externalDebugLines_; }
        [[nodiscard]] std::span<const ExternalDebugTriangle> GetExternalDebugTriangles() const noexcept { return externalDebugTriangles_; }
        [[nodiscard]] std::span<const ExternalDebugCapsule> GetExternalDebugCapsules() const noexcept { return externalDebugCapsules_; }
        [[nodiscard]] std::span<const ExternalDebugArrow> GetExternalDebugArrows() const noexcept { return externalDebugArrows_; }
        [[nodiscard]] std::span<const ExternalDebugBox> GetExternalDebugBoxes() const noexcept { return externalDebugBoxes_; }
        [[nodiscard]] std::span<const ExternalDebugSphere> GetExternalDebugSpheres() const noexcept { return externalDebugSpheres_; }
        [[nodiscard]] std::span<const int> GetEditorSelectedLights() const noexcept { return editorSelectedLights_; }
        [[nodiscard]] int GetEditorSelectedParticleEmitter() const noexcept { return editorSelectedParticleEmitter_; }
        [[nodiscard]] std::span<const int> GetEditorSelectedDrawItems() const noexcept { return editorSelectedDrawItems_; }
        [[nodiscard]] std::span<const int> GetEditorSelectedSkinnedDrawItems() const noexcept { return editorSelectedSkinnedDrawItems_; }
        [[nodiscard]] bool GetEditorDrawSelectedSkinnedSkeleton() const noexcept { return editorDrawSelectedSkinnedSkeleton_; }
        [[nodiscard]] bool GetEditorDrawSelectedSkinnedBounds() const noexcept { return editorDrawSelectedSkinnedBounds_; }
        [[nodiscard]] GizmoMode GetEditorGizmoMode() const noexcept { return editorGizmoMode_; }
        [[nodiscard]] const TranslateGizmoState& GetEditorTranslateGizmo() const noexcept { return *editorTranslateGizmo_; }
        [[nodiscard]] const RotateGizmoState& GetEditorRotateGizmo() const noexcept { return *editorRotateGizmo_; }
        [[nodiscard]] const ScaleGizmoState& GetEditorScaleGizmo() const noexcept { return *editorScaleGizmo_; }

        [[nodiscard]] const Material& GetMaterial(MaterialHandle handle) const
        {
            if (handle.id == 0 || handle.id > materials_.size())
            {
                throw std::runtime_error("Scene::GetMaterial: invalid MaterialHandle");
            }
            return materials_[handle.id - 1];
        }
        
        [[nodiscard]] const AnimationRuntimeOverlaySnapshot& GetAnimationRuntimeOverlaySnapshot() const noexcept
        {
            return animationRuntimeOverlaySnapshot_;
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
        const DebugRay* debugPickRay_;
        const GameplayMovementDebugState* gameplayMovementDebug_;
        std::span<const ExternalDebugLine> externalDebugLines_;
        std::span<const ExternalDebugTriangle> externalDebugTriangles_;
        std::span<const ExternalDebugCapsule> externalDebugCapsules_;
        std::span<const ExternalDebugArrow> externalDebugArrows_;
        std::span<const ExternalDebugBox> externalDebugBoxes_;
        std::span<const ExternalDebugSphere> externalDebugSpheres_;
        std::span<const int> editorSelectedLights_;
        int editorSelectedParticleEmitter_;
        std::span<const int> editorSelectedDrawItems_;
        std::span<const int> editorSelectedSkinnedDrawItems_;
        bool editorDrawSelectedSkinnedSkeleton_;
        bool editorDrawSelectedSkinnedBounds_;
        GizmoMode editorGizmoMode_;
        const TranslateGizmoState* editorTranslateGizmo_;
        const RotateGizmoState* editorRotateGizmo_;
        const ScaleGizmoState* editorScaleGizmo_;
        AnimationRuntimeOverlaySnapshot animationRuntimeOverlaySnapshot_{};
    };
}
