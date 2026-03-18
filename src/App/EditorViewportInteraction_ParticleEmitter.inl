    inline bool HasParticleEmitterSelection(const rendern::Scene& scene)
    {
        return scene.editorSelectedParticleEmitter >= 0 && scene.editorSelectedNode < 0 && scene.editorSelectedLights.empty();
    }

    inline bool HasLightSelection(const rendern::Scene& scene)
    {
        return !scene.editorSelectedLights.empty();
    }

    inline void SyncSelectedParticleEmitterTranslateGizmo(
        rendern::LevelAsset& levelAsset,
        rendern::Scene& scene)
    {
        if (!HasParticleEmitterSelection(scene) ||
            static_cast<std::size_t>(scene.editorSelectedParticleEmitter) >= levelAsset.particleEmitters.size())
        {
            scene.editorTranslateGizmo.visible = false;
            scene.editorTranslateGizmo.hoveredAxis = rendern::GizmoAxis::None;
            scene.editorTranslateGizmo.activeAxis = rendern::GizmoAxis::None;
            return;
        }

        const rendern::ParticleEmitter& emitter = levelAsset.particleEmitters[static_cast<std::size_t>(scene.editorSelectedParticleEmitter)];
        auto& gizmo = scene.editorTranslateGizmo;
        gizmo.visible = gizmo.enabled;
        gizmo.pivotWorld = emitter.position;
        const float distToCamera = mathUtils::Length(scene.camera.position - gizmo.pivotWorld);
        gizmo.axisLengthWorld = std::clamp(distToCamera * 0.18f, 0.35f, 4.0f);
        gizmo.axisXWorld = mathUtils::Vec3(1.0f, 0.0f, 0.0f);
        gizmo.axisYWorld = mathUtils::Vec3(0.0f, 1.0f, 0.0f);
        gizmo.axisZWorld = mathUtils::Vec3(0.0f, 0.0f, 1.0f);
    }

    inline bool TryBeginParticleEmitterTranslateDrag(
        rendern::LevelAsset& levelAsset,
        rendern::Scene& scene,
        float mouseX,
        float mouseY,
        float viewportW,
        float viewportH)
    {
        if (!HasParticleEmitterSelection(scene))
        {
            return false;
        }

        auto& gizmo = scene.editorTranslateGizmo;
        const rendern::GizmoAxis axis = rendern::HitTestTranslateGizmoAxis(scene, gizmo, mouseX, mouseY, viewportW, viewportH);
        if (axis == rendern::GizmoAxis::None)
        {
            return false;
        }

        const geometry::Ray ray = rendern::BuildMouseRay(scene, mouseX, mouseY, viewportW, viewportH);
        const mathUtils::Vec3 axisWorld = rendern::TranslateGizmoAxisDirection(gizmo, axis);
        mathUtils::Vec3 viewDir = scene.camera.target - scene.camera.position;
        if (mathUtils::Length(viewDir) <= 1e-5f)
        {
            viewDir = mathUtils::Vec3(0.0f, 0.0f, -1.0f);
        }
        else
        {
            viewDir = mathUtils::Normalize(viewDir);
        }
        const mathUtils::Vec3 planeNormal = rendern::ComputeTranslateGizmoAxisDragPlaneNormal(axisWorld, viewDir);

        mathUtils::Vec3 startHit{};
        if (!rendern::IntersectRayPlaneForGizmo(ray, gizmo.pivotWorld, planeNormal, startHit))
        {
            return false;
        }

        auto& drag = scene.editorParticleEmitterTranslateDrag;
        drag.dragging = true;
        drag.activeAxis = axis;
        drag.dragStartWorldHit = startHit;
        drag.dragStartEmitterPosition = levelAsset.particleEmitters[static_cast<std::size_t>(scene.editorSelectedParticleEmitter)].position;
        drag.dragPlaneNormal = planeNormal;
        drag.dragAxisWorld = axisWorld;
        gizmo.activeAxis = axis;
        gizmo.hoveredAxis = axis;
        return true;
    }

    inline bool UpdateParticleEmitterTranslateDrag(
        rendern::LevelAsset& levelAsset,
        rendern::LevelInstance& levelInstance,
        rendern::Scene& scene,
        float mouseX,
        float mouseY,
        float viewportW,
        float viewportH)
    {
        auto& drag = scene.editorParticleEmitterTranslateDrag;
        if (!drag.dragging || !HasParticleEmitterSelection(scene))
        {
            return false;
        }

        const geometry::Ray ray = rendern::BuildMouseRay(scene, mouseX, mouseY, viewportW, viewportH);
        mathUtils::Vec3 currentHit{};
        if (!rendern::IntersectRayPlaneForGizmo(ray, scene.editorTranslateGizmo.pivotWorld, drag.dragPlaneNormal, currentHit))
        {
            return false;
        }

        const float axisDelta = mathUtils::Dot(currentHit - drag.dragStartWorldHit, drag.dragAxisWorld);
        const mathUtils::Vec3 newPos = drag.dragStartEmitterPosition + drag.dragAxisWorld * axisDelta;
        levelAsset.particleEmitters[static_cast<std::size_t>(scene.editorSelectedParticleEmitter)].position = newPos;
        levelInstance.SetParticleEmitterPosition(levelAsset, scene, scene.editorSelectedParticleEmitter, newPos);
        SyncSelectedParticleEmitterTranslateGizmo(levelAsset, scene);
        return true;
    }

    inline void EndParticleEmitterTranslateDrag(rendern::Scene& scene)
    {
        scene.editorParticleEmitterTranslateDrag = {};
        scene.editorTranslateGizmo.activeAxis = rendern::GizmoAxis::None;
    }

