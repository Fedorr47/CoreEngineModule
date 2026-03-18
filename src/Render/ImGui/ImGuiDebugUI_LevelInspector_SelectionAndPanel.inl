namespace rendern::ui::level_ui_detail
{
    void DrawParticleEmitterSelectionInspector(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        rendern::Scene& scene,
        LevelEditorUIState& st)
    {
        rendern::ParticleEmitter& emitter = level.particleEmitters[static_cast<std::size_t>(st.selectedParticleEmitter)];

        if (st.prevSelectedParticleEmitter != st.selectedParticleEmitter)
        {
            std::snprintf(st.nameBuf, sizeof(st.nameBuf), "%s", emitter.name.c_str());
            st.prevSelectedParticleEmitter = st.selectedParticleEmitter;
        }

        ImGui::Text("Particle Emitter #%d", st.selectedParticleEmitter);

        bool changed = false;

        if (ImGui::InputText("Name", st.nameBuf, sizeof(st.nameBuf)))
        {
            emitter.name = std::string(st.nameBuf);
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
            levelInst.RestartParticleEmitter(level, scene, st.selectedParticleEmitter);
        }

        ImGui::SeparatorText("Runtime");
        if (const rendern::ParticleEmitter* runtimeEmitter = levelInst.GetRuntimeParticleEmitter(static_cast<const rendern::Scene&>(scene), st.selectedParticleEmitter))
        {
            int aliveCount = 0;
            for (const rendern::Particle& particle : scene.particles)
            {
                if (particle.alive && particle.ownerEmitter == st.selectedParticleEmitter)
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
            levelInst.RestartParticleEmitter(level, scene, st.selectedParticleEmitter);
        }
        ImGui::SameLine();
        if (ImGui::Button("Burst Now"))
        {
            levelInst.TriggerParticleEmitterBurst(level, scene, st.selectedParticleEmitter);
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete Emitter"))
        {
            levelInst.DeleteParticleEmitter(level, scene, st.selectedParticleEmitter);
            st.selectedParticleEmitter = -1;
            st.prevSelectedParticleEmitter = -2;
        }
    }

    void DrawLightSelectionInspector(rendern::Scene& scene, LevelEditorUIState& st)
    {
        scene.EditorSanitizeLightSelection(scene.lights.size());
        st.selectedNode = -1;
        st.selectedParticleEmitter = -1;
        st.prevSelectedNode = -2;
        st.prevSelectedParticleEmitter = -2;

        ImGui::SeparatorText("Light");
        rendern::ui::DrawLightInspectorDetails(scene);
    }

    void DrawSelectionInspector(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        AssetManager& assets,
        rendern::Scene& scene,
        const DerivedLists& derived,
        LevelEditorUIState& st)
    {
        ImGui::Separator();
        ImGui::Text("Selection");

        if (st.selectedParticleEmitter >= 0 && !ParticleEmitterAlive(level, st.selectedParticleEmitter))
            st.selectedParticleEmitter = -1;
        if (st.selectedNode >= 0 && !NodeAlive(level, st.selectedNode))
            st.selectedNode = -1;

        scene.EditorSanitizeLightSelection(scene.lights.size());
        if (scene.editorSelectedLight >= 0)
        {
            DrawLightSelectionInspector(scene, st);
            return;
        }

        if (st.selectedParticleEmitter >= 0)
        {
            st.selectedNode = -1;
            DrawParticleEmitterSelectionInspector(level, levelInst, scene, st);
            st.prevSelectedNode = -2;
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
                    st.selectedNode = -1;
                }
                ImGui::Text("Primary: #%d", scene.editorSelectedNode);
                st.selectedNode = scene.editorSelectedNode;
                ImGui::Separator();
            }
        }

        if (st.selectedNode >= 0 && NodeAlive(level, st.selectedNode))
        {
            DrawNodeSelectionInspector(level, levelInst, assets, scene, derived, st);
        }
        else
        {
            ImGui::TextDisabled("No node, light, or emitter selected.");
            st.prevSelectedNode = -2;
            st.prevSelectedParticleEmitter = -2;
        }
    }

    void DrawInspectorPanel(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        AssetManager& assets,
        rendern::Scene& scene,
        rendern::CameraController& camCtl,
        const DerivedLists& derived,
        LevelEditorUIState& st)
    {
        ImGui::BeginChild("##Inspector", ImVec2(0.0f, 0.0f), true);

        DrawCreateImportSection(level, levelInst, assets, scene, camCtl, st);
        DrawSelectionInspector(level, levelInst, assets, scene, derived, st);

        ImGui::EndChild();
    }
}