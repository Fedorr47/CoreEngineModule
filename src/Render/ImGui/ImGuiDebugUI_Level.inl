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

        ImGui::Text("Nodes: %d   Emitters: %d   Lights: %d   DrawItems: %d",
            static_cast<int>(level.nodes.size()),
            static_cast<int>(level.particleEmitters.size()),
            static_cast<int>(scene.lights.size()),
            static_cast<int>(scene.drawItems.size()));
        ImGui::Separator();

        auto& st = level_ui_detail::GetState();

        // Selection is driven by the main viewport (mouse picking) or by this UI.
        scene.EditorSanitizeLightSelection(scene.lights.size());
        if (scene.editorSelectedLight >= 0)
        {
            st.selectedNode = -1;
            st.selectedParticleEmitter = -1;
        }
        else if (scene.editorSelectedParticleEmitter >= 0)
        {
            st.selectedNode = -1;
            st.selectedParticleEmitter = scene.editorSelectedParticleEmitter;
        }
        else if (scene.editorSelectedNode != st.selectedNode)
        {
            st.selectedNode = scene.editorSelectedNode;
            st.selectedParticleEmitter = -1;
        }
        else if (scene.editorSelectedNode < 0 && scene.editorSelectedParticleEmitter < 0)
        {
            st.selectedNode = -1;
            st.selectedParticleEmitter = -1;
        }

        level_ui_detail::SyncSavePathWithSource(level, st);
        level_ui_detail::DrawFilePanel(level, scene, st);

        level_ui_detail::DerivedLists derived{};
        level_ui_detail::BuildDerivedLists(level, derived);

        level_ui_detail::DrawHierarchyPanel(level, derived, scene, st);
        ImGui::SameLine();
        level_ui_detail::DrawInspectorPanel(level, levelInst, assets, scene, camCtl, derived, st);

        // If UI changed selection directly, push it back to Scene.
        if (scene.editorSelectedLight < 0)
        {
            if (st.selectedParticleEmitter != scene.editorSelectedParticleEmitter ||
                (st.selectedParticleEmitter < 0 && st.selectedNode != scene.editorSelectedNode))
            {
                if (st.selectedParticleEmitter >= 0)
                {
                    scene.EditorSetSelectionSingleParticleEmitter(st.selectedParticleEmitter);
                }
                else
                {
                    scene.EditorSetSelectionSingle(st.selectedNode);
                }
            }
        }

        // Push transforms to Scene if needed, then refresh derived editor draw bindings.
        levelInst.SyncTransformsIfDirty(level, scene);

        levelInst.SyncEditorRuntimeBindings(level, scene);
        levelInst.ValidateRuntimeMappingsDebug(level, scene);
        st.selectedNode = scene.editorSelectedNode;
        st.selectedParticleEmitter = scene.editorSelectedParticleEmitter;

        ImGui::End();
    }
}
