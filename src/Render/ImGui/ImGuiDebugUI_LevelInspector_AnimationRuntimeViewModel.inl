// Local read-only Animation Runtime debug UI ViewModel helpers.
// Included only from ImGuiDebugUI_LevelInspector_AnimationGraphWindow.inl while
// rendern::ui::level_ui_detail is open; this is not a public runtime API.

    struct AnimationRuntimeClipWeightViewModel
    {
        std::string role;
        std::string clipName;
        float weight{ 0.0f };
    };

    struct AnimationRuntimeParameterViewModel
    {
        std::string name;
        std::string valueText;
    };

    struct AnimationRuntimeNotifyViewModel
    {
        std::uint64_t sequence{ 0 };
        std::string id;
        std::string stateName;
        float normalizedTime{ 0.0f };
    };

    struct AnimationRuntimeViewModel
    {
        int nodeIndex{ -1 };
        std::string nodeName;
        std::string skinnedMesh;
        std::string controllerLabel;
        std::string currentStateName;
        std::string requestedStateName;
        std::string modeName;
        float normalizedTime{ 0.0f };
        float playbackSpeed{ 0.0f };
        bool looping{ false };
        bool transitionActive{ false };
        std::string transitionSourceStateName;
        float transitionAlpha{ 0.0f };
        bool hasCurrentStateDesc{ false };
        bool currentStateUsesBlend1D{ false };
        bool currentStateUsesBlend2D{ false };
        std::string blendParameterNameX;
        std::string blendParameterNameY;
        float blendParameterValueX{ 0.0f };
        float blendParameterValueY{ 0.0f };
        std::vector<AnimationRuntimeClipWeightViewModel> activeClips;
        std::vector<AnimationRuntimeParameterViewModel> parameters;
        std::vector<AnimationRuntimeNotifyViewModel> recentNotifies;
        std::vector<std::string> transitionCandidates;
        std::vector<std::string> routedGameplayEvents;
    };

    [[nodiscard]] static AnimationRuntimeViewModel BuildAnimationRuntimeViewModel(
        const AnimationGraphContext& ctx,
        const rendern::AnimationControllerRuntime& runtime,
        const rendern::AnimatorState& animator)
    {
        AnimationRuntimeViewModel viewModel{};
        viewModel.nodeIndex = ctx.nodeIndex;
        if (ctx.node != nullptr)
        {
            viewModel.nodeName = ctx.node->name;
            viewModel.skinnedMesh = ctx.node->skinnedMesh;
        }

        if (runtime.stateMachineAsset != nullptr)
        {
            viewModel.controllerLabel = runtime.stateMachineAsset->id;
        }
        else
        {
            viewModel.controllerLabel = runtime.controllerAssetId;
        }

        viewModel.currentStateName = runtime.currentStateName;
        viewModel.requestedStateName = runtime.requestedStateName;
        viewModel.modeName = runtime.currentStateUsesBlend2D ? "Blend2D" : (runtime.currentStateUsesBlend1D ? "Blend1D" : "Clip");
        viewModel.normalizedTime = AnimationRuntimeGetNormalizedTime(animator);
        viewModel.playbackSpeed = animator.playRate;
        viewModel.looping = animator.looping;
        viewModel.transitionActive = runtime.transitionActive;
        viewModel.transitionSourceStateName = runtime.transitionSourceStateName;
        viewModel.transitionAlpha = (runtime.transitionDurationSeconds > 1e-6f)
            ? std::clamp(runtime.transitionElapsedSeconds / runtime.transitionDurationSeconds, 0.0f, 1.0f)
            : (runtime.transitionActive ? 1.0f : 0.0f);
        viewModel.hasCurrentStateDesc = FindAnimationRuntimeStateDesc(runtime) != nullptr;
        viewModel.currentStateUsesBlend1D = runtime.currentStateUsesBlend1D;
        viewModel.currentStateUsesBlend2D = runtime.currentStateUsesBlend2D;
        viewModel.blendParameterNameX = runtime.currentBlendParameterName;
        viewModel.blendParameterNameY = runtime.currentBlendParameterNameY;
        viewModel.blendParameterValueX = runtime.currentBlendParameterValue;
        viewModel.blendParameterValueY = runtime.currentBlendParameterValueY;

        const float secondaryWeight = std::clamp(runtime.blendSecondaryAlpha, 0.0f, 1.0f);
        const float tertiaryWeight = std::clamp(runtime.blendTertiaryAlpha, 0.0f, 1.0f);
        const float primaryWeight = std::max(0.0f, 1.0f - secondaryWeight - tertiaryWeight);
        auto AddClip = [&viewModel](std::string role, const std::string& clipName, float weight)
        {
            if (!clipName.empty())
            {
                viewModel.activeClips.push_back(AnimationRuntimeClipWeightViewModel{ std::move(role), clipName, weight });
            }
        };
        AddClip("Primary", runtime.currentBlendPrimaryClipName, primaryWeight);
        AddClip("Secondary", runtime.currentBlendSecondaryClipName, secondaryWeight);
        AddClip("Tertiary", runtime.currentBlendTertiaryClipName, tertiaryWeight);

        viewModel.parameters.reserve(runtime.parameters.values.size());
        for (const auto& [name, value] : runtime.parameters.values)
        {
            viewModel.parameters.push_back(AnimationRuntimeParameterViewModel{ name, AnimationGraphConditionValueText(value) });
        }

        const auto& notifyHistory = rendern::PeekAnimationControllerNotifyEvents(runtime);
        const std::size_t firstNotify = notifyHistory.size() > 10 ? notifyHistory.size() - 10 : 0;
        viewModel.recentNotifies.reserve(notifyHistory.size() - firstNotify);
        for (std::size_t i = firstNotify; i < notifyHistory.size(); ++i)
        {
            const rendern::AnimationNotifyEvent& notify = notifyHistory[i];
            viewModel.recentNotifies.push_back(AnimationRuntimeNotifyViewModel{
                notify.sequence,
                notify.id,
                notify.stateName,
                notify.normalizedTime });
        }

        viewModel.transitionCandidates = runtime.debugTransitionCandidates;
        viewModel.routedGameplayEvents = runtime.recentRoutedGameplayEvents;
        return viewModel;
    }