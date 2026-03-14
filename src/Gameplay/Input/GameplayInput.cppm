module;

#include <cmath>
#include <vector>

export module core:gameplay_input_system;

import :gameplay;
import :gameplay_runtime_common;
import :input;

export namespace rendern
{
    [[nodiscard]] inline float NormalizeGameplayMoveAxis(const float x, const float y, float& outX, float& outY) noexcept
    {
        const float lenSq = x * x + y * y;
        if (lenSq <= 1e-8f)
        {
            outX = 0.0f;
            outY = 0.0f;
            return 0.0f;
        }

        const float len = std::sqrt(lenSq);
        outX = x / len;
        outY = y / len;
        return len;
    }

    [[nodiscard]] inline bool ReadGameplayHeldButton(const InputState& input, const GameplayButtonKeyBinding& binding) noexcept
    {
        return binding.key != 0 && input.KeyDown(binding.key);
    }

    [[nodiscard]] inline bool ReadGameplayPressedButton(const InputState& input, const GameplayButtonKeyBinding& binding) noexcept
    {
        return binding.key != 0 && input.KeyPressed(binding.key);
    }

    inline void ReadGameplayAxisFromKeys(const InputState& input, const GameplayAxisKeyBinding& binding, float& outValue) noexcept
    {
        const float positive = (binding.positiveKey != 0 && input.KeyDown(binding.positiveKey)) ? 1.0f : 0.0f;
        const float negative = (binding.negativeKey != 0 && input.KeyDown(binding.negativeKey)) ? 1.0f : 0.0f;
        outValue = positive - negative;
    }

    inline void ReadKeyboardMouseGameplayIntent(
        const InputState& input,
        const GameplayKeyboardMouseBindings& bindings,
        GameplayInputIntentComponent& outIntent)
    {
        float moveX = 0.0f;
        float moveY = 0.0f;
        ReadGameplayAxisFromKeys(input, bindings.moveX, moveX);
        ReadGameplayAxisFromKeys(input, bindings.moveY, moveY);
        NormalizeGameplayMoveAxis(moveX, moveY, outIntent.moveX, outIntent.moveY);

        outIntent.runHeld = ReadGameplayHeldButton(input, bindings.run);
        outIntent.jumpPressed = ReadGameplayPressedButton(input, bindings.jump);
        outIntent.attackPressed = ReadGameplayPressedButton(input, bindings.attack);
        outIntent.interactPressed = ReadGameplayPressedButton(input, bindings.interact);
    }

    inline void UpdateGameplayIntentSources(
        GameplayWorld& world,
        const std::vector<GameplayIntentBinding>& bindings,
        const GameplayUpdateContext& ctx)
    {
        for (const GameplayIntentBinding& binding : bindings)
        {
            if (!world.IsEntityValid(binding.entity) || !binding.callback)
            {
                continue;
            }

            GameplayInputIntentComponent* intent = world.TryGetInputIntent(binding.entity);
            if (intent == nullptr)
            {
                continue;
            }

            GameplayActionComponent* action = world.TryGetAction(binding.entity);

            *intent = {};
            binding.callback(binding.entity, ctx, world, *intent, action);
        }
    }
}
