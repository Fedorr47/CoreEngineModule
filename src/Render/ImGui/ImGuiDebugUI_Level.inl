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
        if (scene.editorSelectedNode != st.selectedNode)
            st.selectedNode = scene.editorSelectedNode;

        level_ui_detail::SyncSavePathWithSource(level, st);
        level_ui_detail::DrawFilePanel(level, scene, st);

        level_ui_detail::DerivedLists derived{};
        level_ui_detail::BuildDerivedLists(level, derived);

        level_ui_detail::DrawHierarchyPanel(level, derived, st);
        ImGui::SameLine();
        level_ui_detail::DrawInspectorPanel(level, levelInst, assets, scene, camCtl, derived, st);

        // Expose selection to the rest of the app (main viewport already writes here too).
        scene.editorSelectedNode = st.selectedNode;
        scene.editorSelectedDrawItem = levelInst.GetNodeDrawIndex(st.selectedNode);

        // Keep reflection-capture owner draw-item index in sync with the LevelInstance mapping.
        // Owner node index is stored in Scene (stable). Draw-item index can change (e.g. visibility toggle).
        scene.editorReflectionCaptureOwnerDrawItem = levelInst.GetNodeDrawIndex(scene.editorReflectionCaptureOwnerNode);

        // Push transforms to Scene if needed.
        levelInst.SyncTransformsIfDirty(level, scene);

        ImGui::End();
    }
}
