// Local read-only Animation Runtime debug UI ViewModel helpers.
// Included only from Animation/ImGuiAnimationUI.inl while
// rendern::ui::animation_ui_detail is open; this is not a public runtime API.

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

    struct AnimationRuntimeBlend1DSampleViewModel
    {
        std::string clipName;
        std::string clipSourceAssetId;
        float position{ 0.0f };
        float weight{ 0.0f };
        bool active{ false };
    };

    struct AnimationRuntimeBlend1DViewModel
    {
        bool available{ false };
        bool live{ false };
        std::string stateName;
        std::string parameterName;
        float inputValue{ 0.0f };
        std::vector<AnimationRuntimeBlend1DSampleViewModel> samples;
    };

    struct AnimationRuntimeBlend2DSampleViewModel
    {
        std::string clipName;
        std::string clipSourceAssetId;
        float x{ 0.0f };
        float y{ 0.0f };
        float weight{ 0.0f };
        bool active{ false };
    };

    struct AnimationRuntimeBlend2DViewModel
    {
        bool available{ false };
        bool live{ false };
        std::string stateName;
        std::string parameterNameX;
        std::string parameterNameY;
        float inputValueX{ 0.0f };
        float inputValueY{ 0.0f };
        std::vector<AnimationRuntimeBlend2DSampleViewModel> samples;
    };

    struct AnimationRuntimeViewModel
    {
        int nodeIndex{ -1 };
        std::string nodeName;
        std::string skinnedMesh;
        std::string controllerLabel;
        std::string currentStateName;
        std::string currentStateDisplayName;
        std::string requestedStateName;
        std::string requestedStateDisplayName;
        std::string modeName;
        float normalizedTime{ 0.0f };
        float playbackSpeed{ 0.0f };
        bool looping{ false };
        std::string loopingText;
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
        AnimationRuntimeBlend1DViewModel blend1DDisplayData;
        AnimationRuntimeBlend2DViewModel blend2DDisplayData;
        std::vector<AnimationRuntimeParameterViewModel> parameters;
        std::vector<AnimationRuntimeNotifyViewModel> recentNotifies;
        std::vector<std::string> transitionCandidates;
        std::vector<std::string> routedGameplayEvents;
    };

    [[nodiscard]] static float AnimationRuntimeGetClipWeight(
        const rendern::AnimationControllerRuntime& runtime,
        const std::string& clipName)
    {
        if (clipName.empty())
        {
            return 0.0f;
        }

        const float secondaryWeight = std::clamp(runtime.blendSecondaryAlpha, 0.0f, 1.0f);
        const float tertiaryWeight = std::clamp(runtime.blendTertiaryAlpha, 0.0f, 1.0f);
        const float primaryWeight = std::max(0.0f, 1.0f - secondaryWeight - tertiaryWeight);
        if (clipName == runtime.currentBlendPrimaryClipName)
        {
            return primaryWeight;
        }
        if (clipName == runtime.currentBlendSecondaryClipName)
        {
            return secondaryWeight;
        }
        if (clipName == runtime.currentBlendTertiaryClipName)
        {
            return tertiaryWeight;
        }
        return 0.0f;
    }

    [[nodiscard]] static AnimationRuntimeBlend1DViewModel BuildAnimationRuntimeBlend1DViewModel(
        const rendern::AnimationStateDesc* currentState,
        const rendern::AnimationControllerRuntime& runtime)
    {
        AnimationRuntimeBlend1DViewModel blend1DDisplayData{};
        if (currentState == nullptr || currentState->blend1D.empty())
        {
            return blend1DDisplayData;
        }

        blend1DDisplayData.available = true;
        blend1DDisplayData.live = runtime.currentStateName == currentState->name && runtime.currentStateUsesBlend1D;
        blend1DDisplayData.stateName = currentState->name;
        blend1DDisplayData.parameterName = currentState->blendParameter;
        blend1DDisplayData.inputValue = blend1DDisplayData.live ? runtime.currentBlendParameterValue : 0.0f;
        blend1DDisplayData.samples.reserve(currentState->blend1D.size());
        for (const rendern::AnimationBlend1DPoint& sample : currentState->blend1D)
        {
            const float sampleWeight = blend1DDisplayData.live ? AnimationRuntimeGetClipWeight(runtime, sample.clipName) : 0.0f;
            AnimationRuntimeBlend1DSampleViewModel sampleDisplayData{};
            sampleDisplayData.clipName = sample.clipName;
            //sampleDisplayData.clipSourceAssetId = sample.clipSourceAssetId;
            sampleDisplayData.position = sample.value;
            sampleDisplayData.weight = sampleWeight;
            sampleDisplayData.active = sampleWeight > 1e-6f;
            blend1DDisplayData.samples.push_back(std::move(sampleDisplayData));
        }
        return blend1DDisplayData;
    }

    [[nodiscard]] static AnimationRuntimeBlend2DViewModel BuildAnimationRuntimeBlend2DViewModel(
        const rendern::AnimationStateDesc* blend2DState,
        const rendern::AnimationControllerRuntime& runtime)
    {
        AnimationRuntimeBlend2DViewModel blend2DDisplayData{};
        if (blend2DState == nullptr || blend2DState->blend2D.empty())
        {
            return blend2DDisplayData;
        }

        blend2DDisplayData.available = true;
        blend2DDisplayData.live = runtime.currentStateName == blend2DState->name && runtime.currentStateUsesBlend2D;
        blend2DDisplayData.stateName = blend2DState->name;
        blend2DDisplayData.parameterNameX = blend2DState->blendParameterX;
        blend2DDisplayData.parameterNameY = blend2DState->blendParameterY;
        blend2DDisplayData.inputValueX = blend2DDisplayData.live ? runtime.currentBlendParameterValue : 0.0f;
        blend2DDisplayData.inputValueY = blend2DDisplayData.live ? runtime.currentBlendParameterValueY : 0.0f;
        blend2DDisplayData.samples.reserve(blend2DState->blend2D.size());
        for (const rendern::AnimationBlend2DPoint& sample : blend2DState->blend2D)
        {
            AnimationRuntimeBlend2DSampleViewModel sampleDisplayData{};
            sampleDisplayData.clipName = sample.clipName;
            sampleDisplayData.clipSourceAssetId = sample.clipSourceAssetId;
            sampleDisplayData.x = sample.x;
            sampleDisplayData.y = sample.y;
            blend2DDisplayData.samples.push_back(std::move(sampleDisplayData));
        }
        return blend2DDisplayData;
    }

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
        viewModel.currentStateDisplayName = runtime.currentStateName.empty() ? "<none>" : runtime.currentStateName;
        viewModel.requestedStateName = runtime.requestedStateName;
        viewModel.requestedStateDisplayName = runtime.requestedStateName.empty() ? "<none>" : runtime.requestedStateName;
        viewModel.modeName = runtime.currentStateUsesBlend2D ? "Blend2D" : (runtime.currentStateUsesBlend1D ? "Blend1D" : "Clip");
        viewModel.normalizedTime = AnimationRuntimeGetNormalizedTime(animator);
        viewModel.playbackSpeed = animator.playRate;
        viewModel.looping = animator.looping;
        viewModel.loopingText = animator.looping ? "true" : "false";
        viewModel.transitionActive = runtime.transitionActive;
        viewModel.transitionSourceStateName = runtime.transitionSourceStateName;
        viewModel.transitionAlpha = (runtime.transitionDurationSeconds > 1e-6f)
            ? std::clamp(runtime.transitionElapsedSeconds / runtime.transitionDurationSeconds, 0.0f, 1.0f)
            : (runtime.transitionActive ? 1.0f : 0.0f);
        const rendern::AnimationStateDesc* currentState = FindAnimationRuntimeStateDesc(runtime);
        viewModel.hasCurrentStateDesc = currentState != nullptr;
        viewModel.currentStateUsesBlend1D = runtime.currentStateUsesBlend1D;
        viewModel.currentStateUsesBlend2D = runtime.currentStateUsesBlend2D;
        viewModel.blendParameterNameX = runtime.currentBlendParameterName;
        viewModel.blendParameterNameY = runtime.currentBlendParameterNameY;
        viewModel.blendParameterValueX = runtime.currentBlendParameterValue;
        viewModel.blendParameterValueY = runtime.currentBlendParameterValueY;

        viewModel.blend1DDisplayData = BuildAnimationRuntimeBlend1DViewModel(currentState, runtime);
        viewModel.blend2DDisplayData = BuildAnimationRuntimeBlend2DViewModel(FindAnimationRuntimeBlend2DPreviewState(runtime), runtime);

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