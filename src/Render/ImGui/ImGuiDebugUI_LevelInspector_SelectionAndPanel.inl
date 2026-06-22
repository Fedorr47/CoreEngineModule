namespace rendern::ui::level_ui_detail
{
    void DrawParticleEmitterSelectionInspector(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        rendern::Scene& scene,
        LevelEditorUIState& uiState)
    {
        rendern::ParticleEmitter& emitter = level.particleEmitters[static_cast<std::size_t>(uiState.selectedParticleEmitter)];

        if (uiState.prevSelectedParticleEmitter != uiState.selectedParticleEmitter)
        {
            std::snprintf(uiState.nameBuf, sizeof(uiState.nameBuf), "%s", emitter.name.c_str());
            uiState.prevSelectedParticleEmitter = uiState.selectedParticleEmitter;
        }

        ImGui::Text("Particle Emitter #%d", uiState.selectedParticleEmitter);

        bool changed = false;

        if (ImGui::InputText("Name", uiState.nameBuf, sizeof(uiState.nameBuf)))
        {
            emitter.name = std::string(uiState.nameBuf);
            changed = true;
        }

        char textureIdBuf[256]{};
        std::snprintf(textureIdBuf, sizeof(textureIdBuf), "%s", emitter.textureId.c_str());
        if (ImGui::InputText("Texture Id", textureIdBuf, sizeof(textureIdBuf)))
        {
            emitter.textureId = std::string(textureIdBuf);
            changed = true;
        }

        changed |= ImGui::Checkbox("Enabled", &emitter.enabled);
        changed |= ImGui::Checkbox("Looping", &emitter.looping);
        changed |= DragVec3("Position", emitter.position, 0.05f);
        changed |= DragVec3("Position Jitter", emitter.positionJitter, 0.02f);
        changed |= DragVec3("Velocity Min", emitter.velocityMin, 0.02f);
        changed |= DragVec3("Velocity Max", emitter.velocityMax, 0.02f);

        float colorBegin[4] = { emitter.colorBegin.x, emitter.colorBegin.y, emitter.colorBegin.z, emitter.colorBegin.w };
        if (ImGui::ColorEdit4("Color Begin", colorBegin))
        {
            emitter.colorBegin = mathUtils::Vec4(colorBegin[0], colorBegin[1], colorBegin[2], colorBegin[3]);
            changed = true;
        }

        float colorEnd[4] = { emitter.colorEnd.x, emitter.colorEnd.y, emitter.colorEnd.z, emitter.colorEnd.w };
        if (ImGui::ColorEdit4("Color End", colorEnd))
        {
            emitter.colorEnd = mathUtils::Vec4(colorEnd[0], colorEnd[1], colorEnd[2], colorEnd[3]);
            changed = true;
        }

        float sizeBegin = emitter.sizeBegin;
        float sizeEnd = emitter.sizeEnd;
        if (ImGui::DragFloat("Size Begin", &sizeBegin, 0.01f, 0.001f, 100.0f, "%.3f"))
        {
            emitter.sizeBegin = std::max(0.001f, sizeBegin);
            changed = true;
        }
        if (ImGui::DragFloat("Size End", &sizeEnd, 0.01f, 0.001f, 100.0f, "%.3f"))
        {
            emitter.sizeEnd = std::max(0.001f, sizeEnd);
            changed = true;
        }

        float lifetimeMin = emitter.lifetimeMin;
        float lifetimeMax = emitter.lifetimeMax;
        if (ImGui::DragFloat("Lifetime Min", &lifetimeMin, 0.01f, 0.001f, 100.0f, "%.3f"))
        {
            emitter.lifetimeMin = std::max(0.001f, lifetimeMin);
            if (emitter.lifetimeMax < emitter.lifetimeMin)
                emitter.lifetimeMax = emitter.lifetimeMin;
            changed = true;
        }
        if (ImGui::DragFloat("Lifetime Max", &lifetimeMax, 0.01f, 0.001f, 100.0f, "%.3f"))
        {
            emitter.lifetimeMax = std::max(emitter.lifetimeMin, lifetimeMax);
            changed = true;
        }

        changed |= ImGui::DragFloat("Spawn Rate", &emitter.spawnRate, 0.1f, 0.0f, 100000.0f, "%.3f");

        int burstCount = static_cast<int>(emitter.burstCount);
        if (ImGui::DragInt("Burst Count", &burstCount, 1.0f, 0, 100000))
        {
            emitter.burstCount = static_cast<std::uint32_t>(std::max(0, burstCount));
            changed = true;
        }

        int maxParticles = static_cast<int>(emitter.maxParticles);
        if (ImGui::DragInt("Max Particles", &maxParticles, 1.0f, 0, 100000))
        {
            emitter.maxParticles = static_cast<std::uint32_t>(std::max(0, maxParticles));
            changed = true;
        }

        changed |= ImGui::DragFloat("Duration", &emitter.duration, 0.05f, 0.0f, 100000.0f, "%.3f");
        changed |= ImGui::DragFloat("Start Delay", &emitter.startDelay, 0.05f, 0.0f, 100000.0f, "%.3f");

        if (changed)
        {
            levelInst.RestartParticleEmitter(level, scene, uiState.selectedParticleEmitter);
        }

        ImGui::SeparatorText("Runtime");
        if (const rendern::ParticleEmitter* runtimeEmitter = levelInst.GetRuntimeParticleEmitter(static_cast<const rendern::Scene&>(scene), uiState.selectedParticleEmitter))
        {
            int aliveCount = 0;
            for (const rendern::Particle& particle : scene.particles)
            {
                if (particle.alive && particle.ownerEmitter == uiState.selectedParticleEmitter)
                {
                    ++aliveCount;
                }
            }

            ImGui::Text("Alive particles: %d", aliveCount);
            ImGui::Text("Elapsed: %.3f", runtimeEmitter->elapsed);
            ImGui::Text("Spawn accumulator: %.3f", runtimeEmitter->spawnAccumulator);
            ImGui::Text("Burst done: %s", runtimeEmitter->burstDone ? "Yes" : "No");
        }
        else
        {
            ImGui::TextDisabled("Runtime emitter is not instantiated.");
        }

        if (ImGui::Button("Restart Emitter"))
        {
            levelInst.RestartParticleEmitter(level, scene, uiState.selectedParticleEmitter);
        }
        ImGui::SameLine();
        if (ImGui::Button("Burst Now"))
        {
            levelInst.TriggerParticleEmitterBurst(level, scene, uiState.selectedParticleEmitter);
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete Emitter"))
        {
            levelInst.DeleteParticleEmitter(level, scene, uiState.selectedParticleEmitter);
            uiState.selectedParticleEmitter = -1;
            uiState.prevSelectedParticleEmitter = -2;
        }
    }

    void DrawLightSelectionInspector(rendern::Scene& scene, LevelEditorUIState& uiState)
    {
        scene.EditorSanitizeLightSelection(scene.lights.size());
        uiState.selectedNode = -1;
        uiState.selectedParticleEmitter = -1;
        uiState.prevSelectedNode = -2;
        uiState.prevSelectedParticleEmitter = -2;

        ImGui::SeparatorText("Light");
        rendern::ui::DrawLightInspectorDetails(scene);
    }

    void DrawSelectionInspector(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        AssetManager& assets,
        rendern::Scene& scene,
        const DerivedLists& derived,
        LevelEditorUIState& uiState)
    {
        ImGui::Separator();
        ImGui::Text("Selection");

        if (uiState.selectedParticleEmitter >= 0 && !ParticleEmitterAlive(level, uiState.selectedParticleEmitter))
            uiState.selectedParticleEmitter = -1;
        if (uiState.selectedNode >= 0 && !NodeAlive(level, uiState.selectedNode))
            uiState.selectedNode = -1;

        scene.EditorSanitizeLightSelection(scene.lights.size());
        if (scene.editorSelectedLight >= 0)
        {
            ClearTransformInspectorPendingEdit(uiState.transformInspectorPendingEdit);
            DrawLightSelectionInspector(scene, uiState);
            return;
        }

        if (uiState.selectedParticleEmitter >= 0)
        {
            ClearTransformInspectorPendingEdit(uiState.transformInspectorPendingEdit);
            uiState.selectedNode = -1;
            DrawParticleEmitterSelectionInspector(level, levelInst, scene, uiState);
            uiState.prevSelectedNode = -2;
            return;
        }

        {
            const int selectedCount = static_cast<int>(scene.editorSelectedNodes.size());
            if (selectedCount > 1)
            {
                ImGui::Text("Multi-selection: %d nodes", selectedCount);
                ImGui::SameLine();
                if (ImGui::SmallButton("Clear"))
                {
                    scene.EditorClearSelection();
                    uiState.selectedNode = -1;
                }
                ImGui::Text("Primary: #%d", scene.editorSelectedNode);
                uiState.selectedNode = scene.editorSelectedNode;
                ImGui::Separator();
            }
        }

        if (uiState.selectedNode >= 0 && NodeAlive(level, uiState.selectedNode))
        {
            DrawNodeSelectionInspector(level, levelInst, assets, scene, derived, uiState);
        }
        else
        {
            ImGui::TextDisabled("No node, light, or emitter selected.");
            ClearTransformInspectorPendingEdit(uiState.transformInspectorPendingEdit);
            uiState.prevSelectedNode = -2;
            uiState.prevSelectedParticleEmitter = -2;
        }
    }

    void DrawInspectorPanel(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        AssetManager& assets,
        rendern::Scene& scene,
        rendern::CameraController& camCtl,
        const DerivedLists& derived,
        LevelEditorUIState& uiState)
    {
        ImGui::BeginChild("##Inspector", ImVec2(0.0f, 0.0f), true);

        DrawCreateImportSection(level, levelInst, assets, scene, camCtl, uiState);
        DrawSelectionInspector(level, levelInst, assets, scene, derived, uiState);

        ImGui::EndChild();
    }
}