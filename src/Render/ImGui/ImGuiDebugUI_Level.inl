#include "ImGuiDebugUI_LevelShared.inl"
#include "ImGuiDebugUI_LevelFile.inl"
#include "ImGuiDebugUI_LevelHierarchy.inl"
#include "ImGuiDebugUI_LevelInspector.inl"

namespace rendern::ui
{
    struct LevelEditorStatsSectionViewModel
    {
        std::size_t nodeCount{0};
        std::size_t emitterCount{0};
        std::size_t lightCount{0};
        std::size_t drawItemCount{0};
    };
    
    // Prepare UI-facing values before drwaing so this stays
    // a small ViewModel-style example, not a render/runtime snapshot bpundary.
    LevelEditorStatsSectionViewModel BuildLevelEditorStatsSectionViewModel(
        const rendern::LevelAsset& levelAsset,
        const rendern::Scene& scene)
    {
        LevelEditorStatsSectionViewModel stats{};
        stats.nodeCount = levelAsset.nodes.size();
        stats.emitterCount = levelAsset.particleEmitters.size();
        stats.lightCount = scene.lights.size();
        stats.drawItemCount = scene.drawItems.size();
        return stats;
    }
    
    static void DrawLevelEditorStatsSection(const LevelEditorStatsSectionViewModel& stats)
    {
        ImGui::Text("Nodes: %zu\tEmitters: %zu\tLights: %zu\tDrawItems: %zu",
            stats.nodeCount,
            stats.emitterCount,
            stats.lightCount,
            stats.drawItemCount);
        ImGui::Separator();
    }
    
    static void DrawLevelEditorAIInfo(LevelAsset& level)
    {
        constexpr std::string_view kAgentNodeName{
            "AI_Move_Agent"
        };

        constexpr std::string_view kRoutePointPrefix{
            "AI_Move_Point_"
        };

        bool bHasAgentNode = false;
        int routePointCount = 0;

        for (const LevelNode& node : level.nodes)
        {
            if (!node.alive)
            {
                continue;
            }

            if (node.name == kAgentNodeName)
            {
                bHasAgentNode = true;
            }

            if (node.name.starts_with(kRoutePointPrefix))
            {
                ++routePointCount;
            }
        }

        const bool bHasRequiredRoutePoints =
            routePointCount >= 2;

        ImGui::SeparatorText("AI Movement Scenario");

        ImGui::Text(
            "Agent node: %s",
            bHasAgentNode
                ? "Found"
                : "Missing");

        ImGui::Text(
            "Route points: %d (%s)",
            routePointCount,
            bHasRequiredRoutePoints
                ? "Valid"
                : "Need at least two");

        ImGui::TextUnformatted(
            "Naming: AI_Move_Point_<number>.");
    }
    
    void ResetLevelEditorUIState()
    {
        level_ui_detail::GetState() = level_ui_detail::LevelEditorUIState{};
    }
    
    void DrawLevelEditorUI(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        AssetManager& assets,
        rendern::Scene& scene,
        rendern::CameraController& camCtl,
        rendern::GameplayRuntime* gameplayRuntime)
    {
        ImGui::Begin("Level Editor");

        const LevelEditorStatsSectionViewModel stats = BuildLevelEditorStatsSectionViewModel(level, scene);
        DrawLevelEditorStatsSection(stats);
        DrawLevelEditorAIInfo(level);

        auto& uiState = level_ui_detail::GetState();

        // Selection is driven by the main viewport (mouse picking) or by this UI.
        scene.EditorSanitizeLightSelection(scene.lights.size());
        if (scene.editorSelectedLight >= 0)
        {
            uiState.selectedNode = -1;
            uiState.selectedParticleEmitter = -1;
        }
        else if (scene.editorSelectedParticleEmitter >= 0)
        {
            uiState.selectedNode = -1;
            uiState.selectedParticleEmitter = scene.editorSelectedParticleEmitter;
        }
        else if (scene.editorSelectedNode != uiState.selectedNode)
        {
            uiState.selectedNode = scene.editorSelectedNode;
            uiState.selectedParticleEmitter = -1;
        }
        else if (scene.editorSelectedNode < 0 && scene.editorSelectedParticleEmitter < 0)
        {
            uiState.selectedNode = -1;
            uiState.selectedParticleEmitter = -1;
        }

        // Temporary bridge while hierarchy selection is migrating from Scene-owned editor
        // selection state to EditorSelectionService. The ViewModel builder still reads
        // both sources defensively and remains a read-only projection.
        level_ui_detail::SyncEditorSelectionServiceWithScene(scene, uiState.selection);
        level_ui_detail::SyncSavePathWithSource(level, uiState);
        level_ui_detail::DrawFilePanel(level, scene, uiState);

        const bool canHotkey = !ImGui::GetIO().WantTextInput;
        const bool ctrlD = canHotkey && ImGui::IsKeyDown(ImGuiKey_ModCtrl) && ImGui::IsKeyPressed(ImGuiKey_D);
        if (ctrlD && scene.editorSelectedLight < 0 && scene.editorSelectedParticleEmitter < 0)
        {
            if (levelInst.DuplicateEditorNodeSelection(level, scene, assets, mathUtils::Vec3(1.0f, 0.0f, 0.0f)))
            {
                uiState.selectedNode = scene.editorSelectedNode;
                uiState.selectedParticleEmitter = -1;
            }
        }

        level_ui_detail::DerivedLists derived{};
        level_ui_detail::BuildDerivedLists(level, derived);
        const level_ui_detail::SceneHierarchyViewModel hierarchyViewModel =
            level_ui_detail::BuildSceneHierarchyViewModel(level, derived, scene, uiState.selection);

        const std::optional<level_ui_detail::SceneHierarchySelectionIntent> hierarchySelectionIntent =
            level_ui_detail::DrawHierarchyPanel(hierarchyViewModel);
        if (hierarchySelectionIntent.has_value())
        {
            level_ui_detail::ApplySceneHierarchySelectionIntent(*hierarchySelectionIntent, scene, uiState);
        }
        ImGui::SameLine();
        level_ui_detail::DrawInspectorPanel(level, levelInst, assets, scene, camCtl, derived, uiState);

        // If UI changed selection directly, push it back to Scene.
        if (scene.editorSelectedLight < 0)
        {
            if (uiState.selectedParticleEmitter != scene.editorSelectedParticleEmitter ||
                (uiState.selectedParticleEmitter < 0 && uiState.selectedNode != scene.editorSelectedNode))
            {
                if (uiState.selectedParticleEmitter >= 0)
                {
                    scene.EditorSetSelectionSingleParticleEmitter(uiState.selectedParticleEmitter);
                }
                else
                {
                    scene.EditorSetSelectionSingle(uiState.selectedNode);
                }
            }
        }

        // Push transforms to Scene if needed, then refresh derived editor draw bindings.
        levelInst.SyncTransformsIfDirty(level, scene);

        levelInst.SyncEditorRuntimeBindings(level, scene);
        levelInst.ValidateRuntimeMappingsDebug(level, scene);
        uiState.selectedNode = scene.editorSelectedNode;
        uiState.selectedParticleEmitter = scene.editorSelectedParticleEmitter;

        ImGui::End();

        level_ui_detail::DrawAnimationGraphWindow(level, levelInst, scene, uiState);
        level_ui_detail::DrawAnimationRuntimeWindow(level, levelInst, scene, uiState, gameplayRuntime);
    }
}
