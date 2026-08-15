module;

#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>

export module core:gameplay;

export import :gameplay_entity_components;
export import :gameplay_ai_components;
export import :gameplay_input_components;
export import :gameplay_interaction_components;
export import :gameplay_character_components;
export import :gameplay_camera_components;
export import :gameplay_action_components;
export import :gameplay_animation_components;
import :EnTTHelpers;

// TODO: Slice nodes to 
// node-bound gameplay entity
// pure gameplay entity
// transient gameplay entity

export namespace rendern
{
    using namespace EnTT_helpers;
    
    class GameplayWorld
    {
    public:
        GameplayWorld();
        ~GameplayWorld();

        GameplayWorld(GameplayWorld&& other) noexcept;
        GameplayWorld& operator=(GameplayWorld&& other) noexcept;

        GameplayWorld(const GameplayWorld&) = delete;
        GameplayWorld& operator=(const GameplayWorld&) = delete;

        [[nodiscard]] EntityHandle CreateEntity();
        void DestroyEntity(EntityHandle entity);
        void Clear() noexcept;
        [[nodiscard]] bool IsEntityValid(EntityHandle entity) const noexcept;
        [[nodiscard]] std::size_t GetAliveCount() const noexcept;

        void AddTransform(EntityHandle entity, const GameplayTransformComponent& value);
        void SetTransform(EntityHandle entity, const GameplayTransformComponent& value);
        [[nodiscard]] GameplayTransformComponent* TryGetTransform(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayTransformComponent* TryGetTransform(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasTransform(EntityHandle entity) const noexcept;
        void RemoveTransform(EntityHandle entity);

        void AddNodeLink(EntityHandle entity, const GameplayNodeLinkComponent& value);
        void SetNodeLink(EntityHandle entity, const GameplayNodeLinkComponent& value);
        [[nodiscard]] GameplayNodeLinkComponent* TryGetNodeLink(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayNodeLinkComponent* TryGetNodeLink(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasNodeLink(EntityHandle entity) const noexcept;
        void RemoveNodeLink(EntityHandle entity);

        void AddAnimationLink(EntityHandle entity, const GameplayAnimationLinkComponent& value);
        void SetAnimationLink(EntityHandle entity, const GameplayAnimationLinkComponent& value);
        [[nodiscard]] GameplayAnimationLinkComponent* TryGetAnimationLink(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayAnimationLinkComponent* TryGetAnimationLink(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasAnimationLink(EntityHandle entity) const noexcept;
        void RemoveAnimationLink(EntityHandle entity);

        void AddPlayerControlled(EntityHandle entity, const GameplayPlayerControlledComponent& value = {});
        void SetPlayerControlled(EntityHandle entity, const GameplayPlayerControlledComponent& value);
        [[nodiscard]] GameplayPlayerControlledComponent* TryGetPlayerControlled(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayPlayerControlledComponent* TryGetPlayerControlled(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasPlayerControlled(EntityHandle entity) const noexcept;
        void RemovePlayerControlled(EntityHandle entity);
        
        void AddAI(EntityHandle entity, const AIComponent& value = {});
        void SetAI(EntityHandle entity, const AIComponent& value);
        [[nodiscard]] AIComponent* TryGetAI(EntityHandle entity) noexcept;
        [[nodiscard]] const AIComponent* TryGetAI(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasAI(EntityHandle entity) const noexcept;
        void RemoveAI(EntityHandle entity);
        
        void AddInteractionPoint(EntityHandle entity, const GameplayInteractionPointComponent& value);
        void SetInteractionPoint(EntityHandle entity, const GameplayInteractionPointComponent& value);
        [[nodiscard]] GameplayInteractionPointComponent* TryGetInteractionPoint(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayInteractionPointComponent* TryGetInteractionPoint(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasInteractionPoint(EntityHandle entity) const noexcept;
        void RemoveInteractionPoint(EntityHandle entity);
        
        void AddDoor(EntityHandle entity, const GameplayDoorComponent& value = {});
        void SetDoor(EntityHandle entity, const GameplayDoorComponent& value);
        [[nodiscard]] GameplayDoorComponent* TryGetDoor(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayDoorComponent* TryGetDoor(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasDoor(EntityHandle entity) const noexcept;
        void RemoveDoor(EntityHandle entity);

        // Replaces outEntities with live AI agents sorted by EntityHandle so update
        // order does not depend on EnTT storage iteration order.
        void CollectAIEntities(std::vector<EntityHandle>& outEntities) const;

        void AddInputIntent(EntityHandle entity, const GameplayInputIntentComponent& value = {});
        void SetInputIntent(EntityHandle entity, const GameplayInputIntentComponent& value);
        [[nodiscard]] GameplayInputIntentComponent* TryGetInputIntent(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayInputIntentComponent* TryGetInputIntent(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasInputIntent(EntityHandle entity) const noexcept;
        void RemoveInputIntent(EntityHandle entity);

        void AddCharacterCommand(EntityHandle entity, const GameplayCharacterCommandComponent& value = {});
        void SetCharacterCommand(EntityHandle entity, const GameplayCharacterCommandComponent& value);
        [[nodiscard]] GameplayCharacterCommandComponent* TryGetCharacterCommand(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayCharacterCommandComponent* TryGetCharacterCommand(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasCharacterCommand(EntityHandle entity) const noexcept;
        void RemoveCharacterCommand(EntityHandle entity);

        void AddCharacterMotor(EntityHandle entity, const GameplayCharacterMotorComponent& value = {});
        void SetCharacterMotor(EntityHandle entity, const GameplayCharacterMotorComponent& value);
        [[nodiscard]] GameplayCharacterMotorComponent* TryGetCharacterMotor(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayCharacterMotorComponent* TryGetCharacterMotor(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasCharacterMotor(EntityHandle entity) const noexcept;
        void RemoveCharacterMotor(EntityHandle entity);
        
        void AddPhysicsCharacter(EntityHandle entity, const GameplayPhysicsCharacterComponent& value);
        void SetPhysicsCharacter(EntityHandle entity, const GameplayPhysicsCharacterComponent& value);
        [[nodiscard]] GameplayPhysicsCharacterComponent* TryGetPhysicsCharacter(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayPhysicsCharacterComponent* TryGetPhysicsCharacter(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasPhysicsCharacter(EntityHandle entity) const noexcept;
        void RemovePhysicsCharacter(EntityHandle entity);
        
        void AddCharacterPhysicalSettings(EntityHandle entity, const GameplayCharacterPhysicalSettingsComponent& value = {});
        void SetCharacterPhysicalSettings(EntityHandle entity, const GameplayCharacterPhysicalSettingsComponent& value);
        [[nodiscard]] GameplayCharacterPhysicalSettingsComponent* TryGetCharacterPhysicalSettings(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayCharacterPhysicalSettingsComponent* TryGetCharacterPhysicalSettings(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasCharacterPhysicalSettings(EntityHandle entity) const noexcept;
        void RemoveCharacterPhysicalSettings(EntityHandle entity);

        void AddCharacterMovementState(EntityHandle entity, const GameplayCharacterMovementStateComponent& value = {});
        void SetCharacterMovementState(EntityHandle entity, const GameplayCharacterMovementStateComponent& value);
        [[nodiscard]] GameplayCharacterMovementStateComponent* TryGetCharacterMovementState(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayCharacterMovementStateComponent* TryGetCharacterMovementState(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasCharacterMovementState(EntityHandle entity) const noexcept;
        void RemoveCharacterMovementState(EntityHandle entity);

        void AddFollowCamera(EntityHandle entity, const GameplayFollowCameraComponent& value = {});
        void SetFollowCamera(EntityHandle entity, const GameplayFollowCameraComponent& value);
        [[nodiscard]] GameplayFollowCameraComponent* TryGetFollowCamera(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayFollowCameraComponent* TryGetFollowCamera(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasFollowCamera(EntityHandle entity) const noexcept;
        void RemoveFollowCamera(EntityHandle entity);

        void AddLocomotion(EntityHandle entity, const GameplayLocomotionComponent& value = {});
        void SetLocomotion(EntityHandle entity, const GameplayLocomotionComponent& value);
        [[nodiscard]] GameplayLocomotionComponent* TryGetLocomotion(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayLocomotionComponent* TryGetLocomotion(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasLocomotion(EntityHandle entity) const noexcept;
        void RemoveLocomotion(EntityHandle entity);

        void AddAction(EntityHandle entity, const GameplayActionComponent& value = {});
        void SetAction(EntityHandle entity, const GameplayActionComponent& value);
        [[nodiscard]] GameplayActionComponent* TryGetAction(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayActionComponent* TryGetAction(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasAction(EntityHandle entity) const noexcept;
        void RemoveAction(EntityHandle entity);

        void AddAnimationNotifyState(EntityHandle entity, const GameplayAnimationNotifyStateComponent& value = {});
        void SetAnimationNotifyState(EntityHandle entity, const GameplayAnimationNotifyStateComponent& value);
        [[nodiscard]] GameplayAnimationNotifyStateComponent* TryGetAnimationNotifyState(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayAnimationNotifyStateComponent* TryGetAnimationNotifyState(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasAnimationNotifyState(EntityHandle entity) const noexcept;
        void RemoveAnimationNotifyState(EntityHandle entity);
        
        void AddAnimationState(EntityHandle entity, const GameplayAnimationStateComponent& value = {});
        void SetAnimationState(EntityHandle entity, const GameplayAnimationStateComponent& value);
        [[nodiscard]] GameplayAnimationStateComponent* TryGetAnimationState(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayAnimationStateComponent* TryGetAnimationState(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasAnimationState(EntityHandle entity) const noexcept;
        void RemoveAnimationState(EntityHandle entity);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_{};
    };

    inline void UpdateGameplayActionRequestsFromPolicies(
        GameplayWorld& world,
        const std::vector<EntityHandle>& entities,
        const GameplayActionPolicyGroup group,
        const GameplayActionDefinitions& definitions)
    {
        for (const EntityHandle entity : entities)
        {
            const GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(entity);
            const GameplayCharacterMovementStateComponent* movementState = world.TryGetCharacterMovementState(entity);
            GameplayActionComponent* action = world.TryGetAction(entity);
            if (command == nullptr || action == nullptr)
            {
                continue;
            }

            QueueGameplayActionRequestsFromPolicies(*action, movementState, command->actionIntents, group, definitions);
        }
    }
}
