module;

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#include <limits>
#include <optional>
#include <cmath>
#include <utility>
#include <vector>

export module core:jolt_character_registry;

import :physics_types;

export namespace physics::jolt
{
    class JoltCharacterRegistry final
    {
    public:
        [[nodiscard]] PhysicsCharacterHandle AllocateHandle(
            JPH::Ref<JPH::CharacterVirtual> character, CharacterColliderDescriptor collider,
            float maximumSpeed, float maximumStepHeight);
        [[nodiscard]] JPH::CharacterVirtual* ResolveCharacter(PhysicsCharacterHandle handle) noexcept;
        [[nodiscard]] const JPH::CharacterVirtual* ResolveCharacter(PhysicsCharacterHandle handle) const noexcept;
        [[nodiscard]] bool ReleaseHandle(PhysicsCharacterHandle handle);
        [[nodiscard]] bool IsValid(PhysicsCharacterHandle handle) const noexcept;
        [[nodiscard]] bool SetDesiredVelocity(PhysicsCharacterHandle handle, const mathUtils::Vec3& velocity) noexcept;
        [[nodiscard]] bool RequestJump(PhysicsCharacterHandle handle, float verticalSpeed) noexcept;
        [[nodiscard]] std::optional<mathUtils::Vec3> GetObservedVelocity(PhysicsCharacterHandle handle) const noexcept;
        [[nodiscard]] std::optional<mathUtils::Vec3> GetDesiredVelocity(
            PhysicsCharacterHandle handle) const noexcept;
        [[nodiscard]] std::optional<CharacterColliderDescriptor> GetCollider(
            PhysicsCharacterHandle handle) const noexcept;
        [[nodiscard]] std::optional<CharacterMotionObservation> ConsumeMotionObservation(
            PhysicsCharacterHandle handle) noexcept;
        void ResetObservedVelocity(PhysicsCharacterHandle handle) noexcept;

        template<typename Visitor>
        void VisitActiveCharacters(Visitor&& visitor)
        {
            for (Slot& slot : slots_)
            {
                if (slot.bOccupied)
                {
                    std::forward<Visitor>(visitor)(
                        *slot.character, slot.desiredHorizontalVelocity,
                        slot.maximumStepHeight, 
                        slot.observedVelocity,
                        slot.motionObservation);
                }
            }
        }
        void Reset() noexcept;
        
        void ResetMotionObservations() noexcept
        {
            for (Slot& slot : slots_)
            {
                if (slot.bOccupied)
                {
                    slot.motionObservation = {};
                }
            }
        }

    private:
        struct Slot
        {
            JPH::Ref<JPH::CharacterVirtual> character;
            mathUtils::Vec3 desiredHorizontalVelocity{};
            mathUtils::Vec3 observedVelocity{};
            CharacterMotionObservation motionObservation{};
            CharacterColliderDescriptor collider{};
            float maximumSpeed{ 0.0f };
            float maximumStepHeight{ 0.0f };
            PhysicsCharacterHandle::GenerationType generation{ 1u };
            bool bOccupied{ false };
        };
        std::vector<Slot> slots_;
        std::vector<PhysicsCharacterHandle::IndexType> freeSlotIndices_;
    };
}

namespace
{
    void AdvanceGeneration(physics::PhysicsCharacterHandle::GenerationType& generation) noexcept
    {
        generation = generation == std::numeric_limits<physics::PhysicsCharacterHandle::GenerationType>::max()
            ? 1u : generation + 1u;
    }
}

physics::PhysicsCharacterHandle physics::jolt::JoltCharacterRegistry::AllocateHandle(
    JPH::Ref<JPH::CharacterVirtual> character,
    const CharacterColliderDescriptor collider,
    const float maximumSpeed,
    const float maximumStepHeight)
{
    if (character == nullptr)
    {
        return InvalidPhysicsCharacterHandle;
    }

    PhysicsCharacterHandle::IndexType index{};

    if (freeSlotIndices_.empty())
    {
        if (slots_.size() >= PhysicsCharacterHandle::InvalidIndex)
        {
            return InvalidPhysicsCharacterHandle;
        }

        freeSlotIndices_.reserve(slots_.size() + 1u);

        index = static_cast<PhysicsCharacterHandle::IndexType>(slots_.size());
        slots_.emplace_back();
    }
    else
    {
        index = freeSlotIndices_.back();
        freeSlotIndices_.pop_back();
    }

    Slot& slot = slots_[index];
    slot.character = std::move(character);
    slot.desiredHorizontalVelocity = {};
    slot.observedVelocity = {};
    slot.motionObservation = {};
    slot.collider = collider;
    slot.maximumSpeed = maximumSpeed;
    slot.maximumStepHeight = maximumStepHeight;
    slot.bOccupied = true;

    return {
        .index = index,
        .generation = slot.generation
    };
}

std::optional<physics::CharacterColliderDescriptor>
physics::jolt::JoltCharacterRegistry::GetCollider(const PhysicsCharacterHandle handle) const noexcept
{
    if (ResolveCharacter(handle) == nullptr)
    {
        return std::nullopt;
    }
    return slots_[handle.index].collider;
}

std::optional<physics::CharacterMotionObservation>
physics::jolt::JoltCharacterRegistry::ConsumeMotionObservation(
    const PhysicsCharacterHandle handle) noexcept
{
    if (ResolveCharacter(handle) == nullptr)
    {
        return std::nullopt;
    }
    Slot& slot = slots_[handle.index];
    const CharacterMotionObservation result = slot.motionObservation;
    slot.motionObservation = {};
    return result;
}

std::optional<mathUtils::Vec3> physics::jolt::JoltCharacterRegistry::GetObservedVelocity(
    const PhysicsCharacterHandle handle) const noexcept
{
    if (ResolveCharacter(handle) == nullptr)
    {
        return std::nullopt;
    }
    return slots_[handle.index].observedVelocity;
}

std::optional<mathUtils::Vec3> physics::jolt::JoltCharacterRegistry::GetDesiredVelocity(
    const PhysicsCharacterHandle handle) const noexcept
{
    if (ResolveCharacter(handle) == nullptr)
    {
        return std::nullopt;
    }
    return slots_[handle.index].desiredHorizontalVelocity;
}

void physics::jolt::JoltCharacterRegistry::ResetObservedVelocity(
    const PhysicsCharacterHandle handle) noexcept
{
    if (ResolveCharacter(handle) != nullptr)
    {
        slots_[handle.index].observedVelocity = {};
        slots_[handle.index].motionObservation = {};
    }
}

bool physics::jolt::JoltCharacterRegistry::SetDesiredVelocity(
    const PhysicsCharacterHandle handle, const mathUtils::Vec3& velocity) noexcept
{
    if (!mathUtils::IsFinite(velocity) || ResolveCharacter(handle) == nullptr)
    {
        return false;
    }

    Slot& slot = slots_[handle.index];
    const float horizontalLength = std::hypot(velocity.x, velocity.z);
    const float scale = horizontalLength > slot.maximumSpeed
        ? slot.maximumSpeed / horizontalLength : 1.0f;
    slot.desiredHorizontalVelocity = {
        velocity.x * scale, 0.0f, velocity.z * scale
    };
    return true;
}

bool physics::jolt::JoltCharacterRegistry::RequestJump(
    const PhysicsCharacterHandle handle, const float verticalSpeed) noexcept
{
    JPH::CharacterVirtual* character = ResolveCharacter(handle);
    if (character == nullptr || !std::isfinite(verticalSpeed) || verticalSpeed <= 0.0f)
    {
        return false;
    }

    if (character->GetGroundState() != JPH::CharacterBase::EGroundState::OnGround)
    {
        return false;
    }

    const JPH::Vec3 up = character->GetUp();
    character->UpdateGroundVelocity();
    JPH::Vec3 velocity = character->GetLinearVelocity();
    if ((velocity - character->GetGroundVelocity()).Dot(up) > 0.0f)
    {
        return false;
    }

    velocity += up * (verticalSpeed - velocity.Dot(up));
    character->SetLinearVelocity(velocity);
    return true;
}

JPH::CharacterVirtual* physics::jolt::JoltCharacterRegistry::ResolveCharacter(
    const PhysicsCharacterHandle handle) noexcept
{
    return const_cast<JPH::CharacterVirtual*>(std::as_const(*this).ResolveCharacter(handle));
}

const JPH::CharacterVirtual* physics::jolt::JoltCharacterRegistry::ResolveCharacter(
    const PhysicsCharacterHandle handle) const noexcept
{
    if (!handle.IsValid() || handle.index >= slots_.size())
    {
        return nullptr;
    }
    const Slot& slot = slots_[handle.index];
    return slot.bOccupied && slot.generation == handle.generation ? slot.character.GetPtr() : nullptr;
}

bool physics::jolt::JoltCharacterRegistry::ReleaseHandle(const PhysicsCharacterHandle handle)
{
    if (ResolveCharacter(handle) == nullptr)
    {
        return false;
    }
    Slot& slot = slots_[handle.index];
    slot.character = nullptr;
    slot.desiredHorizontalVelocity = {};
    slot.observedVelocity = {};
    slot.motionObservation = {};
    slot.collider = {};
    slot.bOccupied = false;
    AdvanceGeneration(slot.generation);
    freeSlotIndices_.push_back(handle.index);
    return true;
}

bool physics::jolt::JoltCharacterRegistry::IsValid(const PhysicsCharacterHandle handle) const noexcept
{
    return ResolveCharacter(handle) != nullptr;
}

void physics::jolt::JoltCharacterRegistry::Reset() noexcept
{
    freeSlotIndices_.clear();
    for (PhysicsCharacterHandle::IndexType index = 0; index < slots_.size(); ++index)
    {
        Slot& slot = slots_[index];
        slot.character = nullptr;
        slot.desiredHorizontalVelocity = {};
        slot.observedVelocity = {};
        slot.motionObservation = {};
        slot.collider = {};
        slot.maximumSpeed = 0.0f;
        slot.maximumStepHeight = 0.0f;
        slot.bOccupied = false;
        AdvanceGeneration(slot.generation);
        freeSlotIndices_.push_back(index);
    }
}