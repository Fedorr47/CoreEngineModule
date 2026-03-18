    inline void ApplyViewportPickSelection(
        rendern::LevelAsset& levelAsset,
        rendern::LevelInstance& levelInstance,
        rendern::Scene& scene,
        const ViewportMouseState& mouseState,
        const rendern::InputState& input)
    {
        const bool ctrlDown = input.KeyDown(VK_CONTROL) || input.KeyDown(VK_LCONTROL) || input.KeyDown(VK_RCONTROL);

        const rendern::PickResult pick = rendern::PickEditorObjectUnderScreenPoint(
            scene,
            levelInstance,
            mouseState.xFloat,
            mouseState.yFloat,
            mouseState.widthFloat,
            mouseState.heightFloat);

        scene.debugPickRay.enabled = true;
        scene.debugPickRay.origin = pick.rayOrigin;
        scene.debugPickRay.direction = pick.rayDir;
        scene.debugPickRay.hit = ((pick.nodeIndex >= 0) || (pick.particleEmitterIndex >= 0) || (pick.lightIndex >= 0)) && std::isfinite(pick.t);
        scene.debugPickRay.length = scene.debugPickRay.hit ? pick.t : scene.camera.farZ;

        if (scene.debugPickRay.hit && levelInstance.IsNodeAlive(levelAsset, pick.nodeIndex))
        {
            if (ctrlDown)
            {
                scene.EditorToggleSelectionNode(pick.nodeIndex);
            }
            else
            {
                scene.EditorSetSelectionSingle(pick.nodeIndex);
            }
            return;
        }

        if (scene.debugPickRay.hit && levelInstance.IsValidParticleEmitterIndex(pick.particleEmitterIndex))
        {
            if (ctrlDown)
            {
                scene.EditorToggleSelectionParticleEmitter(pick.particleEmitterIndex);
            }
            else
            {
                scene.EditorSetSelectionSingleParticleEmitter(pick.particleEmitterIndex);
            }
            return;
        }

        if (scene.debugPickRay.hit && pick.lightIndex >= 0)
        {
            if (ctrlDown)
            {
                scene.EditorToggleSelectionLight(pick.lightIndex);
            }
            else
            {
                scene.EditorSetLightSelectionSingle(pick.lightIndex);
            }
            return;
        }

        if (!ctrlDown)
        {
            scene.EditorClearSelection();
        }
    }

    inline bool TryGetViewportMouseClientPos(HWND hwnd, int viewportWidth, int viewportHeight, int& outMouseX, int& outMouseY)
    {
        int x = 0;
        int y = 0;
        if (!appWin32::TryGetCursorPosClient(hwnd, x, y))
        {
            return false;
        }

        if (x < 0 || y < 0 || x >= viewportWidth || y >= viewportHeight)
        {
            return false;
        }

        outMouseX = x;
        outMouseY = y;
        return true;
    }

    inline void UpdateViewportGizmoHover(
        EditorViewportInteraction& interaction,
        HWND hwnd,
        int viewportWidth,
        int viewportHeight,
        rendern::Scene& scene,
        const rendern::InputState& input)
    {
        if (!input.hasFocus || input.mouse.rmbDown || input.capture.captureMouse)
        {
            ClearAllGizmoHover(interaction, scene);
            return;
        }

        ViewportMouseState mouseState{};
        if (!TryGetViewportMouseState(hwnd, viewportWidth, viewportHeight, mouseState))
        {
            ClearAllGizmoHover(interaction, scene);
            return;
        }

        if (scene.editorGizmoMode == rendern::GizmoMode::Translate)
        {
            if (HasParticleEmitterSelection(scene))
            {
                scene.editorTranslateGizmo.hoveredAxis = rendern::HitTestTranslateGizmoAxis(scene, scene.editorTranslateGizmo, mouseState.xFloat, mouseState.yFloat, mouseState.widthFloat, mouseState.heightFloat);
                scene.editorRotateGizmo.hoveredAxis = rendern::GizmoAxis::None;
                scene.editorScaleGizmo.hoveredAxis = rendern::GizmoAxis::None;
            }
            else
            {
                interaction.translateGizmo.UpdateHover(scene, mouseState.xFloat, mouseState.yFloat, mouseState.widthFloat, mouseState.heightFloat);
                interaction.rotateGizmo.ClearHover(scene);
                interaction.scaleGizmo.ClearHover(scene);
            }
        }
        else if (scene.editorGizmoMode == rendern::GizmoMode::Rotate)
        {
            interaction.rotateGizmo.UpdateHover(scene, mouseState.xFloat, mouseState.yFloat, mouseState.widthFloat, mouseState.heightFloat);
            interaction.translateGizmo.ClearHover(scene);
            interaction.scaleGizmo.ClearHover(scene);
        }
        else if (scene.editorGizmoMode == rendern::GizmoMode::Scale)
        {
            interaction.scaleGizmo.UpdateHover(scene, mouseState.xFloat, mouseState.yFloat, mouseState.widthFloat, mouseState.heightFloat);
            interaction.translateGizmo.ClearHover(scene);
            interaction.rotateGizmo.ClearHover(scene);
        }
        else
        {
            ClearAllGizmoHover(interaction, scene);
        }
    }

    inline void SyncTransformsAndCurrentGizmoVisual(
        EditorViewportInteraction& interaction,
        rendern::LevelAsset& levelAsset,
        rendern::LevelInstance& levelInstance,
        rendern::Scene& scene)
    {
        if (!HasLightSelection(scene))
        {
            levelInstance.MarkTransformsDirty();
            levelInstance.SyncTransformsIfDirty(levelAsset, scene);
        }
        SyncCurrentGizmoVisual(interaction, levelAsset, levelInstance, scene);
    }

    inline void HandleViewportMouseInteraction(
        EditorViewportInteraction& interaction,
        HWND hwnd,
        int viewportWidth,
        int viewportHeight,
        rendern::LevelAsset& levelAsset,
        rendern::LevelInstance& levelInstance,
        rendern::Scene& scene,
        const rendern::InputState& input)
    {
        if (!input.hasFocus || input.mouse.rmbDown || input.capture.captureMouse)
        {
            if (!input.KeyDown(VK_LBUTTON))
            {
                EndAllGizmoDrags(interaction, scene);
            }
            return;
        }

        ViewportMouseState mouseState{};
        if (!TryGetViewportMouseState(hwnd, viewportWidth, viewportHeight, mouseState))
        {
            if (!input.KeyDown(VK_LBUTTON))
            {
                EndAllGizmoDrags(interaction, scene);
            }
            return;
        }

        bool gizmoConsumed = false;
        bool transformChanged = false;

        if (scene.editorGizmoMode == rendern::GizmoMode::Translate)
        {
            gizmoConsumed = HasParticleEmitterSelection(scene)
                ? HandleParticleEmitterTranslateInteraction(levelAsset, levelInstance, scene, mouseState, input, transformChanged)
                : HandleStandardGizmoInteraction(interaction.translateGizmo, levelAsset, levelInstance, scene, mouseState, input, transformChanged);
        }
        else if (scene.editorGizmoMode == rendern::GizmoMode::Rotate)
        {
            gizmoConsumed = HandleStandardGizmoInteraction(interaction.rotateGizmo, levelAsset, levelInstance, scene, mouseState, input, transformChanged);
        }
        else if (scene.editorGizmoMode == rendern::GizmoMode::Scale)
        {
            gizmoConsumed = HandleStandardGizmoInteraction(interaction.scaleGizmo, levelAsset, levelInstance, scene, mouseState, input, transformChanged);
        }

        if (transformChanged)
        {
            SyncTransformsAndCurrentGizmoVisual(interaction, levelAsset, levelInstance, scene);
        }

        if (!gizmoConsumed && input.KeyPressed(VK_LBUTTON))
        {
            ApplyViewportPickSelection(levelAsset, levelInstance, scene, mouseState, input);
        }
    }
