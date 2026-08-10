module;

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#include <limits>
#include <utility>
#include <vector>

export module core:jolt_character_registry;

import :physics_types;

export namespace physics::jolt
{
    class JoltCharacterRegistry final
    {
    public:
        [[nodiscard]] PhysicsCharacterHandle AllocateHandle(JPH::Ref<JPH::CharacterVirtual> character);
        [[nodiscard]] JPH::CharacterVirtual* ResolveCharacter(PhysicsCharacterHandle handle) noexcept;
        [[nodiscard]] const JPH::CharacterVirtual* ResolveCharacter(PhysicsCharacterHandle handle) const noexcept;
        [[nodiscard]] bool ReleaseHandle(PhysicsCharacterHandle handle);
        [[nodiscard]] bool IsValid(PhysicsCharacterHandle handle) const noexcept;
        void Reset() noexcept;

    private:
        struct Slot
        {
            JPH::Ref<JPH::CharacterVirtual> character;
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
    JPH::Ref<JPH::CharacterVirtual> character)
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
    slot.bOccupied = true;

    return {
        .index = index,
        .generation = slot.generation
    };
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
        slot.bOccupied = false;
        AdvanceGeneration(slot.generation);
        freeSlotIndices_.push_back(index);
    }
}