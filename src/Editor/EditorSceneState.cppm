module;

#include <vector>

export module core:editor_scene_state;

import :scene;

export namespace rendern
{
    struct EditorSceneState
    {
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
        }
    };
}
