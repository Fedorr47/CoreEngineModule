module;

#include <vector>

export module core:editor_scene_state;

import :scene;
import :editor_selection_service;

export namespace rendern
{
    struct EditorSceneState
    {
        /* Canonical editor/debug UI selection boundary for new code.
         * Existing Scene/editor panel flows still own their legacy selection mirrors until they
         * are explicitly migrated to route writes through this service.
        */
        EditorSelectionService selection;
        
        /* Transitional compatibility state for legacy editor/debug UI panels.
         * Do not add new selection flows here; preferable way is EditorSelectionService (above)
         */
        std::vector<int> selectedNodes;
        std::vector<int> selectedLights;
        std::vector<int> selectedDrawItems;
        std::vector<int> selectedSkinnedDrawItems;

        bool gizmoTranslateActive{ false };
        bool gizmoRotateActive{ false };
        bool gizmoScaleActive{ false };

        void ClearSelection()
        {
            selectedNodes.clear();
            selectedLights.clear();
            selectedDrawItems.clear();
            selectedSkinnedDrawItems.clear();
            selectedNodes.clear();
        }
    };
}
