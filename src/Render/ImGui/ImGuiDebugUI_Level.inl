#include "ImGuiDebugUI_LevelShared.inl"
#include "ImGuiDebugUI_LevelFile.inl"
#include "ImGuiDebugUI_LevelHierarchy.inl"
#include "ImGuiDebugUI_LevelInspector.inl"

namespace rendern::ui
{
    void DrawLevelEditorUI(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        AssetManager& assets,
        rendern::Scene& scene,
        rendern::CameraController& camCtl)
    {
        ImGui::Begin("Level Editor");

        ImGui::Text("Nodes: %d   DrawItems: %d", static_cast<int>(level.nodes.size()), static_cast<int>(scene.drawItems.size()));
        ImGui::Separator();

        auto& st = level_ui_detail::GetState();

        // Selection is driven by the main viewport (mouse picking) or by this UI.
        // In multi-selection mode the Scene keeps the full selection set; UI keeps the primary node.
        if (scene.editorSelectedNode != st.selectedNode)
            st.selectedNode = scene.editorSelectedNode;

        level_ui_detail::SyncSavePathWithSource(level, st);
        level_ui_detail::DrawFilePanel(level, scene, st);

        level_ui_detail::DerivedLists derived{};
        level_ui_detail::BuildDerivedLists(level, derived);

        level_ui_detail::DrawHierarchyPanel(level, derived, scene, st);
        ImGui::SameLine();
        level_ui_detail::DrawInspectorPanel(level, levelInst, assets, scene, camCtl, derived, st);

        // If UI changed primary selection directly (e.g. via Inspector actions), treat it as single selection.
        if (st.selectedNode != scene.editorSelectedNode)
        {
            scene.EditorSetSelectionSingle(st.selectedNode);
        }

        // Push transforms to Scene if needed, then refresh derived editor draw bindings.
        levelInst.SyncTransformsIfDirty(level, scene);

        levelInst.SyncEditorRuntimeBindings(level, scene);
        levelInst.ValidateRuntimeMappingsDebug(level, scene);
        st.selectedNode = scene.editorSelectedNode;

        ImGui::End();
    }
}