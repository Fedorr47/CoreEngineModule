module;

#include <cstddef>

export module core:editor_commands;

import :scene;
import :level;
import :editor_selection_service;

namespace rendern::editor_commands::detail
{
    void MirrorSceneNodeSelectionToService(EditorSelectionService& selection, const Scene& scene) noexcept
    {
        selection.ClearSelection();
        for (const int selectedNode : scene.editorSelectedNodes)
        {
            selection.ToggleSceneNode(selectedNode);
        }
    }
}

export namespace rendern::editor_commands
{
    /* Minimal editor/debug UI command boundary.
     * Editor commands represent explicit user intent from editor/debug UI code
     * and synchronously delegate to narrow editor services or existing runtime
     * editor APIs. They intentionally avoid a central queue, command base class,
     * undo/redo, recording, replay, async dispatch or thread-safe submission
     * model until those capabilities are needed.
     * 
     * Views should call these named helpers for UI-driven mutations instead of
     * spreading direct writes across Scene, LevelInstance, render settings,
     * gameplay runtime and debug state. Future queued/undoable commands can 
     * grow behind this namespace without changing the panel-level intent names.
     */
    
    void SelectSceneNode(EditorSelectionService& selection, Scene& scene, const int nodeIndex) noexcept
    {
        selection.SelectSceneNode(nodeIndex);
        scene.EditorSetSelectionSingle(selection.GetPrimarySceneNode());
    }
    
    void ToggleSceneNodeSelection(EditorSelectionService& selection, Scene& scene, const int nodeIndex) noexcept
    {
        detail::MirrorSceneNodeSelectionToService(selection, scene);
        selection.ToggleSceneNode(nodeIndex);
        
        scene.EditorClearSelection();
        for (const int selectedNode : selection.GetSelectedSceneNodes())
        {
            scene.editorSelectedNodes.push_back(selectedNode);
        }
        scene.editorSelectedNode = selection.GetPrimarySceneNode();
    }
    
    void ClearEditorSelection(EditorSelectionService& selection, Scene& scene) noexcept
    {
        selection.ClearSelection();
        scene.EditorClearSelection();
    }
    
    void SetSceneNodeTransform(
        LevelAsset& levelAsset,
        LevelInstance& levelInstance,
        const int nodeIndex,
        const Transform& sceneNodeTransform) noexcept
    {
        if (nodeIndex < 0)
        {
            return;
        }
        
        const std::size_t nodeOffset = static_cast<std::size_t>(nodeIndex);
        if (nodeIndex >= levelAsset.nodes.size() || !levelAsset.nodes[nodeOffset].alive)
        {
            return;
        }
        
        levelAsset.nodes[nodeOffset].transform = sceneNodeTransform;
        levelInstance.MarkTransformsDirty();
    }
}