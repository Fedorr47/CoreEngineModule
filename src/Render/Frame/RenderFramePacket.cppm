module;

export module core:render_frame_packet;

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

    // Renderer-only skinned payload. Asset ownership deliberately holds the
    // resource lifetime; mutable animation runtime state is copied separately.
    struct RenderSkinnedDrawItem
    {
        SkinnedHandle asset{};
        Transform transform{};
        MaterialHandle material{};
        std::vector<MaterialHandle> submeshMaterials{};
        std::vector<mathUtils::Mat4> skinMatrices{};
        std::vector<mathUtils::Mat4> globalMatrices{};
        std::vector<int> skeletonParentIndices{};
    };

    struct RenderWorldSnapshot
    {
        RenderWorldSnapshot(
            Camera camera,
            std::vector<Material> materials,
            std::vector<DrawItem> drawItems,
            std::vector<RenderSkinnedDrawItem> skinnedDrawItems,
            std::vector<Light> lights,
            std::vector<Particle> particles,
            std::vector<ParticleEmitter> particleEmitters,
            rhi::TextureDescIndex skyboxDescIndex) noexcept
            : camera_(std::move(camera))
            , materials_(std::move(materials))
            , drawItems_(std::move(drawItems))
            , skinnedDrawItems_(std::move(skinnedDrawItems))
            , lights_(std::move(lights))
            , particles_(std::move(particles))
            , particleEmitters_(std::move(particleEmitters))
            , skyboxDescIndex_(skyboxDescIndex)
        {
        }

        [[nodiscard]] const Camera& GetCamera() const noexcept { return camera_; }
        [[nodiscard]] const std::vector<Material>& GetMaterials() const noexcept { return materials_; }
        [[nodiscard]] const std::vector<DrawItem>& GetDrawItems() const noexcept { return drawItems_; }
        [[nodiscard]] const std::vector<RenderSkinnedDrawItem>& GetSkinnedDrawItems() const noexcept { return skinnedDrawItems_; }
        [[nodiscard]] const std::vector<Light>& GetLights() const noexcept { return lights_; }
        [[nodiscard]] const std::vector<Particle>& GetParticles() const noexcept { return particles_; }
        [[nodiscard]] const std::vector<ParticleEmitter>& GetParticleEmitters() const noexcept { return particleEmitters_; }
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
        Camera camera_;
        std::vector<Material> materials_;
        std::vector<DrawItem> drawItems_;
        std::vector<RenderSkinnedDrawItem> skinnedDrawItems_;
        std::vector<Light> lights_;
        std::vector<Particle> particles_;
        std::vector<ParticleEmitter> particleEmitters_;
        rhi::TextureDescIndex skyboxDescIndex_;
    };

    struct RenderDebugSnapshot
    {
        RenderDebugSnapshot(
            DebugRay pickRay,
            GameplayMovementDebugState gameplayMovement,
            std::vector<ExternalDebugLine> lines,
            std::vector<ExternalDebugTriangle> triangles,
            std::vector<ExternalDebugCapsule> capsules,
            std::vector<ExternalDebugArrow> arrows,
            std::vector<ExternalDebugBox> boxes,
            std::vector<ExternalDebugSphere> spheres) noexcept
            : pickRay_(std::move(pickRay))
            , gameplayMovement_(std::move(gameplayMovement))
            , lines_(std::move(lines))
            , triangles_(std::move(triangles))
            , capsules_(std::move(capsules))
            , arrows_(std::move(arrows))
            , boxes_(std::move(boxes))
            , spheres_(std::move(spheres))
        {
        }

        [[nodiscard]] const DebugRay& GetPickRay() const noexcept { return pickRay_; }
        [[nodiscard]] const GameplayMovementDebugState& GetGameplayMovement() const noexcept { return gameplayMovement_; }
        [[nodiscard]] const std::vector<ExternalDebugLine>& GetLines() const noexcept { return lines_; }
        [[nodiscard]] const std::vector<ExternalDebugTriangle>& GetTriangles() const noexcept { return triangles_; }
        [[nodiscard]] const std::vector<ExternalDebugCapsule>& GetCapsules() const noexcept { return capsules_; }
        [[nodiscard]] const std::vector<ExternalDebugArrow>& GetArrows() const noexcept { return arrows_; }
        [[nodiscard]] const std::vector<ExternalDebugBox>& GetBoxes() const noexcept { return boxes_; }
        [[nodiscard]] const std::vector<ExternalDebugSphere>& GetSpheres() const noexcept { return spheres_; }

    private:
        DebugRay pickRay_;
        GameplayMovementDebugState gameplayMovement_;
        std::vector<ExternalDebugLine> lines_;
        std::vector<ExternalDebugTriangle> triangles_;
        std::vector<ExternalDebugCapsule> capsules_;
        std::vector<ExternalDebugArrow> arrows_;
        std::vector<ExternalDebugBox> boxes_;
        std::vector<ExternalDebugSphere> spheres_;
    };

    struct RenderEditorSnapshot
    {
        RenderEditorSnapshot(
            std::vector<int> selectedLights,
            int selectedParticleEmitter,
            std::vector<int> selectedDrawItems,
            std::vector<int> selectedSkinnedDrawItems,
            bool drawSelectedSkinnedSkeleton,
            bool drawSelectedSkinnedBounds,
            GizmoMode gizmoMode,
            TranslateGizmoState translateGizmo,
            RotateGizmoState rotateGizmo,
            ScaleGizmoState scaleGizmo) noexcept
            : selectedLights_(std::move(selectedLights))
            , selectedParticleEmitter_(selectedParticleEmitter)
            , selectedDrawItems_(std::move(selectedDrawItems))
            , selectedSkinnedDrawItems_(std::move(selectedSkinnedDrawItems))
            , drawSelectedSkinnedSkeleton_(drawSelectedSkinnedSkeleton)
            , drawSelectedSkinnedBounds_(drawSelectedSkinnedBounds)
            , gizmoMode_(gizmoMode)
            , translateGizmo_(std::move(translateGizmo))
            , rotateGizmo_(std::move(rotateGizmo))
            , scaleGizmo_(std::move(scaleGizmo))
        {
        }

        [[nodiscard]] const std::vector<int>& GetSelectedLights() const noexcept { return selectedLights_; }
        [[nodiscard]] int GetSelectedParticleEmitter() const noexcept { return selectedParticleEmitter_; }
        [[nodiscard]] const std::vector<int>& GetSelectedDrawItems() const noexcept { return selectedDrawItems_; }
        [[nodiscard]] const std::vector<int>& GetSelectedSkinnedDrawItems() const noexcept { return selectedSkinnedDrawItems_; }
        [[nodiscard]] bool GetDrawSelectedSkinnedSkeleton() const noexcept { return drawSelectedSkinnedSkeleton_; }
        [[nodiscard]] bool GetDrawSelectedSkinnedBounds() const noexcept { return drawSelectedSkinnedBounds_; }
        [[nodiscard]] GizmoMode GetGizmoMode() const noexcept { return gizmoMode_; }
        [[nodiscard]] const TranslateGizmoState& GetTranslateGizmo() const noexcept { return translateGizmo_; }
        [[nodiscard]] const RotateGizmoState& GetRotateGizmo() const noexcept { return rotateGizmo_; }
        [[nodiscard]] const ScaleGizmoState& GetScaleGizmo() const noexcept { return scaleGizmo_; }

    private:
        std::vector<int> selectedLights_;
        int selectedParticleEmitter_;
        std::vector<int> selectedDrawItems_;
        std::vector<int> selectedSkinnedDrawItems_;
        bool drawSelectedSkinnedSkeleton_;
        bool drawSelectedSkinnedBounds_;
        GizmoMode gizmoMode_;
        TranslateGizmoState translateGizmo_;
        RotateGizmoState rotateGizmo_;
        ScaleGizmoState scaleGizmo_;
    };

    // Owns all Scene-derived mutable frame data required by rendering and remains
    // valid independently of the source Scene lifetime. Resource handles retain
    // their existing ownership and synchronization semantics; cross-thread
    // resource safety is outside this frame-description boundary.
    struct RenderFramePacket
    {
        RenderFramePacket(
            RenderWorldSnapshot world,
            RenderDebugSnapshot debug,
            RenderEditorSnapshot editor,
            AnimationRuntimeOverlaySnapshot animationRuntimeOverlaySnapshot)
            : world_(std::move(world))
            , debug_(std::move(debug))
            , editor_(std::move(editor))
            , animationRuntimeOverlaySnapshot_(std::move(animationRuntimeOverlaySnapshot))
        {
        }

        [[nodiscard]] const RenderWorldSnapshot& GetWorld() const noexcept { return world_; }
        [[nodiscard]] const RenderDebugSnapshot& GetDebug() const noexcept { return debug_; }
        [[nodiscard]] const RenderEditorSnapshot& GetEditor() const noexcept { return editor_; }
        [[nodiscard]] const AnimationRuntimeOverlaySnapshot& GetAnimationRuntimeOverlaySnapshot() const noexcept
        {
            return animationRuntimeOverlaySnapshot_;
        }

    private:
        RenderWorldSnapshot world_;
        RenderDebugSnapshot debug_;
        RenderEditorSnapshot editor_;
        AnimationRuntimeOverlaySnapshot animationRuntimeOverlaySnapshot_{};
    };
}
