    struct EditorViewportInteraction
    {
        rendern::TranslateGizmoController translateGizmo{};
        rendern::RotateGizmoController rotateGizmo{};
        rendern::ScaleGizmoController scaleGizmo{};
    };

    template <typename TGizmoState>
    inline void ResetGizmoState(TGizmoState& gizmo)
    {
        gizmo.visible = false;
        gizmo.hoveredAxis = rendern::GizmoAxis::None;
        gizmo.activeAxis = rendern::GizmoAxis::None;
    }

    inline void ClearAllGizmoHover(EditorViewportInteraction& interaction, rendern::Scene& scene)
    {
        interaction.translateGizmo.ClearHover(scene);
        interaction.rotateGizmo.ClearHover(scene);
        interaction.scaleGizmo.ClearHover(scene);
    }

    inline void EndAllGizmoDrags(EditorViewportInteraction& interaction, rendern::Scene& scene)
    {
        interaction.translateGizmo.EndDrag(scene);
        interaction.rotateGizmo.EndDrag(scene);
        interaction.scaleGizmo.EndDrag(scene);
        EndParticleEmitterTranslateDrag(scene);
    }

    inline void SyncCurrentGizmoVisual(
        EditorViewportInteraction& interaction,
        rendern::LevelAsset& levelAsset,
        rendern::LevelInstance& levelInstance,
        rendern::Scene& scene)
    {
        if (scene.editorGizmoMode == rendern::GizmoMode::Translate)
        {
            if (HasParticleEmitterSelection(scene))
            {
                SyncSelectedParticleEmitterTranslateGizmo(levelAsset, scene);
            }
            else
            {
                interaction.translateGizmo.SyncVisual(levelAsset, levelInstance, scene);
            }
        }
        else if (scene.editorGizmoMode == rendern::GizmoMode::Rotate)
        {
            interaction.rotateGizmo.SyncVisual(levelAsset, levelInstance, scene);
        }
        else if (scene.editorGizmoMode == rendern::GizmoMode::Scale)
        {
            interaction.scaleGizmo.SyncVisual(levelAsset, levelInstance, scene);
        }
    }

    inline void ApplyGizmoModeHotkeys(
        EditorViewportInteraction& interaction,
        rendern::Scene& scene,
        const rendern::InputState& input)
    {
        if (!input.hasFocus || input.capture.captureKeyboard || input.mouse.rmbDown)
        {
            return;
        }

        rendern::GizmoMode nextMode = scene.editorGizmoMode;
        if (input.KeyPressed('Q'))
        {
            nextMode = rendern::GizmoMode::None;
        }
        else if (input.KeyPressed('W'))
        {
            nextMode = rendern::GizmoMode::Translate;
        }
        else if (input.KeyPressed('E'))
        {
            nextMode = rendern::GizmoMode::Rotate;
        }
        else if (input.KeyPressed('R'))
        {
            nextMode = rendern::GizmoMode::Scale;
        }

        if (nextMode != scene.editorGizmoMode)
        {
            EndAllGizmoDrags(interaction, scene);
            scene.editorGizmoMode = nextMode;
        }

        if (input.KeyPressed('X') && scene.editorGizmoMode == rendern::GizmoMode::Translate)
        {
            EndAllGizmoDrags(interaction, scene);
            scene.editorTranslateSpace =
                (scene.editorTranslateSpace == rendern::GizmoSpace::World)
                ? rendern::GizmoSpace::Local
                : rendern::GizmoSpace::World;
        }
    }

    inline void SyncEditorGizmoVisuals(
        EditorViewportInteraction& interaction,
        rendern::LevelAsset& levelAsset,
        rendern::LevelInstance& levelInstance,
        rendern::Scene& scene)
    {
        levelInstance.SyncTransformsIfDirty(levelAsset, scene);

        if (scene.editorGizmoMode == rendern::GizmoMode::Translate)
        {
            if (HasParticleEmitterSelection(scene))
            {
                SyncSelectedParticleEmitterTranslateGizmo(levelAsset, scene);
            }
            else
            {
                interaction.translateGizmo.SyncVisual(levelAsset, levelInstance, scene);
            }
            ResetGizmoState(scene.editorRotateGizmo);
            ResetGizmoState(scene.editorScaleGizmo);
        }
        else if (scene.editorGizmoMode == rendern::GizmoMode::Rotate)
        {
            ResetGizmoState(scene.editorTranslateGizmo);
            ResetGizmoState(scene.editorScaleGizmo);
            interaction.rotateGizmo.SyncVisual(levelAsset, levelInstance, scene);
        }
        else if (scene.editorGizmoMode == rendern::GizmoMode::Scale)
        {
            ResetGizmoState(scene.editorTranslateGizmo);
            ResetGizmoState(scene.editorRotateGizmo);
            interaction.scaleGizmo.SyncVisual(levelAsset, levelInstance, scene);
        }
        else
        {
            ResetGizmoState(scene.editorTranslateGizmo);
            ResetGizmoState(scene.editorRotateGizmo);
            ResetGizmoState(scene.editorScaleGizmo);
        }
    }

    inline bool TryGetViewportMouseClientPos(HWND hwnd, int viewportWidth, int viewportHeight, int& outMouseX, int& outMouseY);

    struct ViewportMouseState
    {
        int x{ 0 };
        int y{ 0 };
        float xFloat{ 0.0f };
        float yFloat{ 0.0f };
        float widthFloat{ 0.0f };
        float heightFloat{ 0.0f };
    };

    inline bool TryGetViewportMouseState(
        HWND hwnd,
        int viewportWidth,
        int viewportHeight,
        ViewportMouseState& outState)
    {
        if (!TryGetViewportMouseClientPos(hwnd, viewportWidth, viewportHeight, outState.x, outState.y))
        {
            return false;
        }

        outState.xFloat = static_cast<float>(outState.x);
        outState.yFloat = static_cast<float>(outState.y);
        outState.widthFloat = static_cast<float>(viewportWidth);
        outState.heightFloat = static_cast<float>(viewportHeight);
        return true;
    }

    template <typename TGizmoController>
    inline bool HandleStandardGizmoInteraction(
        TGizmoController& gizmoController,
        rendern::LevelAsset& levelAsset,
        rendern::LevelInstance& levelInstance,
        rendern::Scene& scene,
        const ViewportMouseState& mouseState,
        const rendern::InputState& input,
        bool& outTransformChanged)
    {
        if (gizmoController.IsDragging())
        {
            if (input.KeyDown(VK_LBUTTON))
            {
                outTransformChanged = gizmoController.UpdateDrag(
                    levelAsset,
                    levelInstance,
                    scene,
                    mouseState.xFloat,
                    mouseState.yFloat,
                    mouseState.widthFloat,
                    mouseState.heightFloat,
                    input.shiftDown);
                return true;
            }

            gizmoController.EndDrag(scene);
            return true;
        }

        if (!input.KeyPressed(VK_LBUTTON))
        {
            return false;
        }

        return gizmoController.TryBeginDrag(
            levelAsset,
            levelInstance,
            scene,
            mouseState.xFloat,
            mouseState.yFloat,
            mouseState.widthFloat,
            mouseState.heightFloat);
    }

    inline bool HandleParticleEmitterTranslateInteraction(
        rendern::LevelAsset& levelAsset,
        rendern::LevelInstance& levelInstance,
        rendern::Scene& scene,
        const ViewportMouseState& mouseState,
        const rendern::InputState& input,
        bool& outTransformChanged)
    {
        if (scene.editorParticleEmitterTranslateDrag.dragging)
        {
            if (input.KeyDown(VK_LBUTTON))
            {
                outTransformChanged = UpdateParticleEmitterTranslateDrag(
                    levelAsset,
                    levelInstance,
                    scene,
                    mouseState.xFloat,
                    mouseState.yFloat,
                    mouseState.widthFloat,
                    mouseState.heightFloat);
                return true;
            }

            EndParticleEmitterTranslateDrag(scene);
            return true;
        }

        if (!input.KeyPressed(VK_LBUTTON))
        {
            return false;
        }

        return TryBeginParticleEmitterTranslateDrag(
            levelAsset,
            scene,
            mouseState.xFloat,
            mouseState.yFloat,
            mouseState.widthFloat,
            mouseState.heightFloat);
    }

