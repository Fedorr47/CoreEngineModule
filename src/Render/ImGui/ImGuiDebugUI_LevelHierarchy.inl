namespace rendern::ui::level_ui_detail
{
    static const char* LightTypeLabel(rendern::LightType type) noexcept
    {
        switch (type)
        {
        case rendern::LightType::Directional: return "Directional";
        case rendern::LightType::Point:       return "Point";
        case rendern::LightType::Spot:        return "Spot";
        }
        return "Unknown";
    }

    static void DrawHierarchyPanel(
        rendern::LevelAsset& level,
        const DerivedLists& derived,
        rendern::Scene& scene,
        LevelEditorUIState& st)
    {
        ImGui::BeginChild("##Hierarchy", ImVec2(280.0f, 0.0f), true);

        auto drawNode = [&](auto&& self, int idx) -> void
            {
                const auto& n = level.nodes[static_cast<std::size_t>(idx)];

                ImGuiTreeNodeFlags flags =
                    ImGuiTreeNodeFlags_OpenOnArrow |
                    ImGuiTreeNodeFlags_SpanFullWidth;

                if (derived.children[static_cast<std::size_t>(idx)].empty())
                    flags |= ImGuiTreeNodeFlags_Leaf;

                if (scene.EditorIsNodeSelected(idx))
                    flags |= ImGuiTreeNodeFlags_Selected;

                char label[256]{};
                const char* name = n.name.empty() ? "<unnamed>" : n.name.c_str();
                if (!n.mesh.empty())
                    std::snprintf(label, sizeof(label), "%d: %s  [mesh=%s]", idx, name, n.mesh.c_str());
                else
                    std::snprintf(label, sizeof(label), "%d: %s", idx, name);

                const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<std::intptr_t>(idx)), flags, "%s", label);

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                {
                    const bool ctrlDown = ImGui::GetIO().KeyCtrl;
                    if (ctrlDown)
                    {
                        scene.EditorToggleSelectionNode(idx);
                    }
                    else
                    {
                        scene.EditorSetSelectionSingle(idx);
                    }
                    st.selectedNode = scene.editorSelectedNode;
                    st.selectedParticleEmitter = -1;
                }

                if (open)
                {
                    for (int ch : derived.children[static_cast<std::size_t>(idx)])
                        self(self, ch);
                    ImGui::TreePop();
                }
            };

        for (int r : derived.roots)
            drawNode(drawNode, r);

        ImGui::SeparatorText("Particle Emitters");
        for (std::size_t i = 0; i < level.particleEmitters.size(); ++i)
        {
            const rendern::ParticleEmitter& emitter = level.particleEmitters[i];
            char label[256]{};
            const char* name = emitter.name.empty() ? "<unnamed emitter>" : emitter.name.c_str();
            std::snprintf(label, sizeof(label), "%d: %s%s", static_cast<int>(i), name, emitter.enabled ? "" : "  [disabled]");

            if (ImGui::Selectable(label, scene.EditorIsParticleEmitterSelected(static_cast<int>(i))))
            {
                const bool ctrlDown = ImGui::GetIO().KeyCtrl;
                if (ctrlDown)
                {
                    scene.EditorToggleSelectionParticleEmitter(static_cast<int>(i));
                }
                else
                {
                    scene.EditorSetSelectionSingleParticleEmitter(static_cast<int>(i));
                }
                st.selectedNode = -1;
                st.selectedParticleEmitter = scene.editorSelectedParticleEmitter;
            }
        }

        ImGui::SeparatorText("Lights");
        scene.EditorSanitizeLightSelection(scene.lights.size());
        for (std::size_t i = 0; i < scene.lights.size(); ++i)
        {
            const rendern::Light& light = scene.lights[i];
            char label[256]{};
            std::snprintf(
                label,
                sizeof(label),
                "%d: %s%s",
                static_cast<int>(i),
                LightTypeLabel(light.type),
                light.intensity > 0.00001f ? "" : "  [disabled]");

            if (ImGui::Selectable(label, scene.EditorIsLightSelected(static_cast<int>(i))))
            {
                const bool ctrlDown = ImGui::GetIO().KeyCtrl;
                if (ctrlDown)
                {
                    scene.EditorToggleSelectionLight(static_cast<int>(i));
                }
                else
                {
                    scene.EditorSetLightSelectionSingle(static_cast<int>(i));
                }
                st.selectedNode = -1;
                st.selectedParticleEmitter = -1;
            }
        }

        ImGui::EndChild();
    }
}
