module;

#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

export module core:gameplay_runtime_common;

import :gameplay;
import :input;
import :level;
import :scene;
import :animation_controller;

export namespace rendern
{
    enum class GameplayRuntimeMode : std::uint8_t
    {
        Editor = 0,
        Game
    };

    struct GameplayAnimationNotifyRecord
    {
        EntityHandle entity{ kNullEntity };
        int nodeIndex{ -1 };
        int skinnedDrawIndex{ -1 };
        AnimationNotifyEvent event{};
    };

    struct GameplayEventRecord
    {
        EntityHandle entity{ kNullEntity };
        int nodeIndex{ -1 };
        int skinnedDrawIndex{ -1 };
        std::uint64_t sequence{ 0 };
        std::string animationEventId{};
        std::string gameplayEventId{};
        std::string stateName{};
        std::string clipName{};
        float normalizedTime{ 0.0f };
    };

    struct GameplayUpdateContext
    {
        float deltaSeconds{ 0.0f };
        GameplayRuntimeMode mode{ GameplayRuntimeMode::Editor };
        const InputState* input{ nullptr };
        LevelAsset* levelAsset{ nullptr };
        LevelInstance* levelInstance{ nullptr };
        Scene* scene{ nullptr };
    };

    struct GameplayAxisKeyBinding
    {
        int negativeKey{ 0 };
        int positiveKey{ 0 };
    };

    struct GameplayButtonKeyBinding
    {
        int key{ 0 };
    };
    
    struct GameplayActionKeyBinding
    {
        int key{ 0 };
        GameplayActionKind action{ GameplayActionKind::None };
    };

    [[nodiscard]] constexpr bool IsGameplayActionBindingKeyReserved(const int key) noexcept
    {
        return key == 0x74 || key == 0x75 || key == 0x76;
    }

    struct GameplayKeyboardMouseBindings
    {
        GameplayAxisKeyBinding moveX{ 'D', 'A' };
        GameplayAxisKeyBinding moveY{ 'S', 'W' };
        GameplayButtonKeyBinding run{ 0x10 };
        std::vector<GameplayActionKeyBinding> actions{
                { 0x20, GameplayActionKind::Jump },
                { 0x78, GameplayActionKind::LightAttack },
                { 'E', GameplayActionKind::Interact }
        };
    };

    using GameplayIntentSourceCallback = std::function<void(
        EntityHandle entity,
        const GameplayUpdateContext& ctx,
        GameplayWorld& world,
        GameplayInputIntentComponent& outIntent,
        GameplayActionComponent* action)>;

    struct GameplayIntentBinding
    {
        EntityHandle entity{ kNullEntity };
        GameplayIntentSourceCallback callback{};
    };
}
