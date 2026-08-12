#pragma once

namespace physics
{
    class JoltPhysicsWorld;
}

namespace rendern
{
    class GameplayRuntime;
    struct LevelAsset;
}

namespace appRuntime
{
    [[nodiscard]] bool EnsureControlledGameplayPhysicsCharacter(
        rendern::GameplayRuntime& gameplayRuntime,
        physics::JoltPhysicsWorld& physicsWorld,
        const rendern::LevelAsset& levelAsset);

    [[nodiscard]] bool SubmitControlledGameplayPhysicsCharacterVelocity(
        rendern::GameplayRuntime& gameplayRuntime,
        physics::JoltPhysicsWorld& physicsWorld);

    [[nodiscard]] bool ApplyControlledGameplayPhysicsCharacterFeedback(
        rendern::GameplayRuntime& gameplayRuntime,
        physics::JoltPhysicsWorld& physicsWorld);

    [[nodiscard]] bool DestroyControlledGameplayPhysicsCharacter(
        rendern::GameplayRuntime& gameplayRuntime,
        physics::JoltPhysicsWorld& physicsWorld);
}