namespace rendern::ui::level_ui_detail
{
    static const SceneHierarchyItemViewModel& GetSceneHierarchyItem(
        const SceneHierarchyViewModel& hierarchyViewModel,
        int itemIndex)
    {
        return hierarchyViewModel.items[static_cast<std::size_t>(itemIndex)];
    }

    static SceneHierarchySelectionIntent BuildSceneHierarchySelectionIntent(
        const SceneHierarchyItemViewModel& item,
        bool toggleSelection) noexcept
    {
        return SceneHierarchySelectionIntent
        {
            .itemId = item.id,
            .mode = toggleSelection
                ? SceneHierarchySelectionIntentMode::Toggle
                : SceneHierarchySelectionIntentMode::Replace
        };
    }

    static std::optional<SceneHierarchySelectionIntent> DrawHierarchyPanel(const SceneHierarchyViewModel& hierarchyViewModel)
    {
        std::optional<SceneHierarchySelectionIntent> selectionIntent = std::nullopt;
        ImGui::BeginChild("##Hierarchy", ImVec2(280.0f, 0.0f), true);

        auto drawNode = [&](auto&& self, int itemIndex) -> void
            {
                const SceneHierarchyItemViewModel& item = GetSceneHierarchyItem(hierarchyViewModel, itemIndex);

                ImGuiTreeNodeFlags flags =
                    ImGuiTreeNodeFlags_OpenOnArrow |
                    ImGuiTreeNodeFlags_SpanFullWidth;

                if (item.childItemIndices.empty())
                    flags |= ImGuiTreeNodeFlags_Leaf;

                if (item.isSelected)
                    flags |= ImGuiTreeNodeFlags_Selected;

                const bool open = ImGui::TreeNodeEx(
                    reinterpret_cast<void*>(static_cast<std::intptr_t>(itemIndex)),
                    flags,
                    "%s",
                    item.displayName.c_str());

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                {
                    selectionIntent = BuildSceneHierarchySelectionIntent(item, ImGui::GetIO().KeyCtrl);
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
            
            if (ImGui::Selectable(item.displayName.c_str(), item.isSelected))
            {
                selectionIntent = BuildSceneHierarchySelectionIntent(item, ImGui::GetIO().KeyCtrl);
            }
        }

        ImGui::SeparatorText("Lights");
        for (const int lightItemIndex : hierarchyViewModel.lightItemIndices)
        {
            const SceneHierarchyItemViewModel& item = GetSceneHierarchyItem(hierarchyViewModel, lightItemIndex);

            if (ImGui::Selectable(item.displayName.c_str(), item.isSelected))
            {
                selectionIntent = BuildSceneHierarchySelectionIntent(item, ImGui::GetIO().KeyCtrl);
            }
        }

        ImGui::EndChild();
        return selectionIntent;
    }

    static void ApplySceneHierarchySelectionIntent(
        const SceneHierarchySelectionIntent& selectionIntent,
        rendern::Scene& scene,
        LevelEditorUIState& uiState)
    {
        const bool toggleSelection = selectionIntent.mode == SceneHierarchySelectionIntentMode::Toggle;
        switch (selectionIntent.itemId.kind)
        {
        case SceneHierarchyItemKind::SceneNode:
            if (toggleSelection)
            {
                rendern::editor_commands::ToggleSceneNodeSelection(uiState.selection, scene, selectionIntent.itemId.index);
            }
            else
            {
                rendern::editor_commands::SelectSceneNode(uiState.selection, scene, selectionIntent.itemId.index);
            }
            uiState.selectedNode = scene.editorSelectedNode;
            uiState.selectedParticleEmitter = -1;
            break;

        case SceneHierarchyItemKind::ParticleEmitter:
            // Compatibility path: particle emitter selection still lives on Scene until
            // a larger non-node editor selection command boundary exists.
            if (toggleSelection)
            {
                scene.EditorToggleSelectionParticleEmitter(selectionIntent.itemId.index);
            }
            else
            {
                scene.EditorSetSelectionSingleParticleEmitter(selectionIntent.itemId.index);
            }
            uiState.selectedNode = -1;
            uiState.selectedParticleEmitter = scene.editorSelectedParticleEmitter;
            break;

        case SceneHierarchyItemKind::Light:
            // Compatibility path: light selection still lives on Scene until a larger
            // non-node editor selection command boundary exists.
            if (toggleSelection)
            {
                scene.EditorToggleSelectionLight(selectionIntent.itemId.index);
            }
            else
            {
                scene.EditorSetLightSelectionSingle(selectionIntent.itemId.index);
            }
            uiState.selectedNode = -1;
            uiState.selectedParticleEmitter = -1;
            break;

        case SceneHierarchyItemKind::None:
            break;
        }
    }
}
