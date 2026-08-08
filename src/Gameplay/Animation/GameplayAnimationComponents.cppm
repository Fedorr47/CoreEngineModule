module;

#include <cstdint>
#include <string>

export module core:gameplay_animation_components;

export namespace rendern
{
    struct GameplayAnimationLinkComponent
    {
        int skinnedDrawIndex{ -1 };
        std::string controllerAssetId{};
    };

    struct GameplayAnimationNotifyStateComponent
    {
        bool anyThisFrame{ false };
        bool footstepThisFrame{ false };
        bool interactionPointThisFrame{ false };
        bool actionStartedThisFrame{ false };
        bool jumpTakeoffThisFrame{ false };
        bool actionFinishedThisFrame{ false };
        bool hitWindowOpenedThisFrame{ false };
        bool hitWindowClosedThisFrame{ false };
        bool hitWindowActive{ false };
        std::uint64_t lastSequence{ 0 };
        float lastNormalizedTime{ 0.0f };
        std::string lastNotifyId{};
        std::string lastStateName{};
        std::string lastClipName{};
    };

    struct GameplayAnimationStateComponent
    {
        std::string controllerAssetId{};
        std::string currentStateName{};
        std::string previousStateName{};
        std::string modeName{};
        std::string primaryClipName{};
        std::string secondaryClipName{};
        std::string tertiaryClipName{};
        std::string blendParameterNameX{};
        std::string blendParameterNameY{};
        float blendParameterValueX{ 0.0f };
        float blendParameterValueY{ 0.0f };
        float stateNormalizedTime{ 0.0f };
        bool usesBlend1D{ false };
        bool usesBlend2D{ false };
        bool transitionActive{ false };
        bool enteredThisFrame{ false };
        bool changedThisFrame{ false };
    };
}
