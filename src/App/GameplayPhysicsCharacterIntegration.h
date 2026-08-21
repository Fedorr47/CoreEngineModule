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
    [[nodiscard]] bool EnsureGameplayPhysicsCharacters(
        rendern::GameplayRuntime& gameplayRuntime,
        physics::JoltPhysicsWorld& physicsWorld,
        const rendern::LevelAsset& levelAsset);

    [[nodiscard]] bool SubmitGameplayPhysicsCharacterVelocities(
        rendern::GameplayRuntime& gameplayRuntime,
        physics::JoltPhysicsWorld& physicsWorld);

    [[nodiscard]] bool ApplyGameplayPhysicsCharacterFeedback(
        rendern::GameplayRuntime& gameplayRuntime,
        physics::JoltPhysicsWorld& physicsWorld);
    
    [[nodiscard]] bool TeleportGameplayPhysicsCharacterToGameplayTransform(
        rendern::GameplayRuntime& gameplayRuntime,
        physics::JoltPhysicsWorld& physicsWorld,
        rendern::EntityHandle entity);

    [[nodiscard]] bool DestroyGameplayPhysicsCharacters(
        rendern::GameplayRuntime& gameplayRuntime,
        physics::JoltPhysicsWorld& physicsWorld);
    
    [[nodiscard]] bool DestroyGameplayPhysicsCharacter(
        rendern::GameplayRuntime& gameplayRuntime,
        physics::JoltPhysicsWorld& physicsWorld,
        rendern::EntityHandle entity);
}