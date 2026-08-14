namespace rendern::ui::animation_ui_detail
{
    struct AnimationUIState
    {
        bool animationGraphWindowOpen = true;
        bool animationGraphRequestFocus = false;
        bool animationGraphFsmFocusSelection = true;
        bool animationGraphShowTransitionLabels = false;
        std::string animationGraphSelectedStateName;
        bool animationRuntimeWindowOpen = true;
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