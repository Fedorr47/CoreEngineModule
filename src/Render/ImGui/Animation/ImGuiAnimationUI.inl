namespace rendern::ui::animation_ui_detail
{
#include "ImGuiAnimationGraphShared.inl"
#include "ImGuiAnimationWorkspace.inl"
#include "ImGuiAnimationRuntimeViewModel.inl"
#include "ImGuiAnimationGraphBlend.inl"
#include "ImGuiAnimationGraphDetails.inl"
#include "ImGuiAnimationGraphNodes.inl"
#include "ImGuiAnimationGraphView.inl"
}

namespace rendern::ui
{
    void RequestAnimationGraphFocus(const std::string_view stateName)
    {
        auto& uiState = animation_ui_detail::GetState();
        uiState.animationGraphWindowOpen = true;
        uiState.animationGraphRequestFocus = true;
        uiState.animationGraphSelectedStateName = stateName;
    }

    void DrawAnimationDebugUI(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        rendern::Scene& scene,
        rendern::GameplayRuntime* gameplayRuntime)
    {
        auto& uiState = animation_ui_detail::GetState();
        rendern::EditorSelectionService& selection = GetEditorSelectionService();
        animation_ui_detail::DrawAnimationSourcesWindow(level, levelInst, scene, uiState);
        animation_ui_detail::DrawAnimationProfileWindow(level, levelInst, scene, uiState);
        animation_ui_detail::DrawAnimationGraphWindow(level, levelInst, scene, uiState);
        animation_ui_detail::DrawAnimationClipInspectorWindow(level, levelInst, scene, uiState);
        animation_ui_detail::DrawAnimationRuntimeWindow(level, levelInst, scene, uiState, selection, gameplayRuntime);
    }
}