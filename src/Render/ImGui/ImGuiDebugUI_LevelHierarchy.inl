namespace rendern::ui::level_ui_detail
{
    static const SceneHierarchyItemViewModel& GetSceneHierarchyItem(
        const SceneHierarchyViewModel& hierarchyViewModel,
        int itemIndex)
    {
        return hierarchyViewModel.items[static_cast<std::size_t>(itemIndex)];
    }

    static void DrawHierarchyPanel(
        const SceneHierarchyViewModel& hierarchyViewModel,
        rendern::Scene& scene,
        LevelEditorUIState& uiState)
    {
        ImGui::BeginChild("##Hierarchy", ImVec2(280.0f, 0.0f), true);

        auto drawNode = [&](auto&& self, int itemIndex) -> void
            {
                const SceneHierarchyItemViewModel& item = GetSceneHierarchyItem(hierarchyViewModel, itemIndex);
                const int nodeIndex = item.id.index;

                ImGuiTreeNodeFlags flags =
                    ImGuiTreeNodeFlags_OpenOnArrow |
                    ImGuiTreeNodeFlags_SpanFullWidth;

                if (item.childItemIndices.empty())
                    flags |= ImGuiTreeNodeFlags_Leaf;

                if (item.isSelected)
                    flags |= ImGuiTreeNodeFlags_Selected;

                const bool open = ImGui::TreeNodeEx(
                    reinterpret_cast<void*>(static_cast<std::intptr_t>(nodeIndex)),
                    flags,
                    "%s",
                    item.displayName.c_str());

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                {
                    const bool ctrlDown = ImGui::GetIO().KeyCtrl;
                    if (ctrlDown)
                    {
                        rendern::editor_commands::ToggleSceneNodeSelection(uiState.selection, scene, nodeIndex);
                    }
                    else
                    {
                        rendern::editor_commands::SelectSceneNode(uiState.selection, scene, nodeIndex);
                    }
                    uiState.selectedNode = scene.editorSelectedNode;
                    uiState.selectedParticleEmitter = -1;
                }

                if (open)
                {
                    for (const int childItemIndex : item.childItemIndices)
                    {
                        self(self, childItemIndex);
                    }
                    ImGui::TreePop();
                }
            };

        for (const int rootItemIndex : hierarchyViewModel.sceneRootItemIndices)
        {
            drawNode(drawNode, rootItemIndex);
        }

        ImGui::SeparatorText("Particle Emitters");
        for (const int emitterItemIndex : hierarchyViewModel.particleEmitterItemIndices)
        {
            const SceneHierarchyItemViewModel& item = GetSceneHierarchyItem(hierarchyViewModel, emitterItemIndex);
            const int emitterIndex = item.id.index;
            
            if (ImGui::Selectable(item.displayName.c_str(), item.isSelected))
            {
                const bool ctrlDown = ImGui::GetIO().KeyCtrl;
                if (ctrlDown)
                {
                    scene.EditorToggleSelectionParticleEmitter(emitterIndex);
                }
                else
                {
                    scene.EditorSetSelectionSingleParticleEmitter(emitterIndex);
                }
                uiState.selectedNode = -1;
                uiState.selectedParticleEmitter = scene.editorSelectedParticleEmitter;
            }
        }

        ImGui::SeparatorText("Lights");
        for (const int lightItemIndex : hierarchyViewModel.lightItemIndices)
        {
            const SceneHierarchyItemViewModel& item = GetSceneHierarchyItem(hierarchyViewModel, lightItemIndex);
            const int lightIndex = item.id.index;

            if (ImGui::Selectable(item.displayName.c_str(), item.isSelected))
            {
                const bool ctrlDown = ImGui::GetIO().KeyCtrl;
                if (ctrlDown)
                {
                    scene.EditorToggleSelectionLight(lightIndex);
                }
                else
                {
                    scene.EditorSetLightSelectionSingle(lightIndex);
                }
                uiState.selectedNode = -1;
                uiState.selectedParticleEmitter = -1;
            }
        }

        ImGui::EndChild();
    }
}
