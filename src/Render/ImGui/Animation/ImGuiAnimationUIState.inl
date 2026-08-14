namespace rendern::ui::animation_ui_detail
{
    struct AnimationRootTrajectoryCache
    {
        int nodeIndex{ -1 };
        int clipIndex{ -1 };
        std::uintptr_t assetIdentity{ 0 };
        std::string sourceAssetId;
        std::string clipName;
        std::string rootBoneName;
        float durationTicks{ -1.0f };
        std::size_t translationKeyCount{ 0 };
        rendern::AnimationRootTrajectoryDiagnostics diagnostics;
    };
    
    struct AnimationUIState
    {
        bool animationGraphWindowOpen = true;
        bool animationGraphRequestFocus = false;
        bool animationGraphFsmFocusSelection = true;
        bool animationGraphShowTransitionLabels = false;
        std::string animationGraphSelectedStateName;
        bool animationRuntimeWindowOpen = true;
        bool animationSourcesWindowOpen = true;
        bool animationProfileWindowOpen = true;
        bool animationClipInspectorWindowOpen = true;
        std::string selectedSourceAssetId;
        std::string selectedClipName;
        std::string selectedMotionId;
        std::unordered_map<std::string, rendern::AnimationProfileEditorState> profileEditorStates;
        AnimationRootTrajectoryCache rootTrajectoryCache;
        bool clipInspectorPlaying = false;
        bool clipInspectorLoop = true;
        float clipInspectorPlayRate = 1.0f;
        float clipInspectorTimeSeconds = 0.0f;
        int animationRuntimePinnedNodeIndex = -1;
        bool animationRuntimeObservedEntityUnavailable = false;
        float animationGraphFsmZoom = 1.0f;
        float animationGraphAssetZoom = 1.0f;
        float animationGraphBlend2DZoom = 1.0f;
        float animationRuntimeBlend2DZoom = 1.0f;
        ImVec2 animationGraphFsmPan{ 24.0f, 24.0f };
        ImVec2 animationGraphAssetPan{ 24.0f, 24.0f };
        ImVec2 animationGraphBlend2DPan{ 24.0f, 24.0f };
        ImVec2 animationRuntimeBlend2DPan{ 24.0f, 24.0f };
        std::unordered_map<std::string, ImVec2> animationGraphFsmNodePositions;
        std::unordered_map<std::string, ImVec2> animationGraphAssetNodePositions;
    };

    static AnimationUIState& GetState()
    {
        static AnimationUIState state{};
        return state;
    }
}