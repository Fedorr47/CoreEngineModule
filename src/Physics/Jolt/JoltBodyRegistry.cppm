module;

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

#include <limits>
#include <optional>
#include <utility>
#include <vector>

export module core:jolt_body_registry;

import :physics_types;

export namespace physics::jolt
{
    class JoltBodyRegistry final
    {
    public:
        JoltBodyRegistry() = default;

        JoltBodyRegistry(const JoltBodyRegistry&) = delete;
        JoltBodyRegistry& operator=(const JoltBodyRegistry&) = delete;

        JoltBodyRegistry(JoltBodyRegistry&&) = delete;
        JoltBodyRegistry& operator=(JoltBodyRegistry&&) = delete;
        
        [[nodiscard]] PhysicsBodyHandle AllocateHandle(JPH::BodyID bodyID);
        
        [[nodiscard]] std::optional<JPH::BodyID> ResolveBodyID
            (const PhysicsBodyHandle handle) const noexcept;
        
        [[nodiscard]] bool ReleaseHandle(const PhysicsBodyHandle handle);
        
        [[nodiscard]] bool IsValid(PhysicsBodyHandle handle) const noexcept;
        
        template<typename Visitor>
        void VisitActiveBodyIDs(Visitor&& visitor) const
        {
            for (const Slot& slot : slots_)
            {
                if (slot.bOccupied)
                {
                    std::forward<Visitor>(visitor)(slot.bodyID);
                }
            }
        }

        void Reset() noexcept;
        
    private:
        struct Slot
        {
            JPH::BodyID bodyID{};
            PhysicsBodyHandle::GenerationType generation{1u};
            bool bOccupied{false};
        };
        
        std::vector<Slot> slots_;
        std::vector<PhysicsBodyHandle::IndexType> freeSlotIndices_;
    };
}

physics::PhysicsBodyHandle physics::jolt::JoltBodyRegistry::AllocateHandle(JPH::BodyID bodyID)
{
    if (bodyID.IsInvalid())
    {
        return InvalidPhysicsBodyHandle;
    }
    
    PhysicsBodyHandle::IndexType slotIndex{};
    if (!freeSlotIndices_.empty())
    {
        slotIndex = freeSlotIndices_.back();
        freeSlotIndices_.pop_back();
    }
    else
    {
        if (slots_.size() >= PhysicsBodyHandle::InvalidIndex)
        {
            return InvalidPhysicsBodyHandle;
        }
        
        freeSlotIndices_.reserve(slots_.size() + 1u);
        slotIndex = static_cast<PhysicsBodyHandle::IndexType>(slots_.size());
        slots_.emplace_back();
    }
    
    Slot& slot = slots_[slotIndex];
    slot.bodyID = bodyID;
    slot.bOccupied = true;
    return PhysicsBodyHandle{.index = slotIndex, .generation = slot.generation};
}

std::optional<JPH::BodyID> physics::jolt::JoltBodyRegistry::ResolveBodyID(
    const PhysicsBodyHandle handle) const noexcept
{
    if (!handle.IsValid() || handle.index >= slots_.size())
    {
        return std::nullopt;
    }
    
    const Slot& slot = slots_[handle.index];
    if (!slot.bOccupied || slot.generation != handle.generation)
    {
        return std::nullopt;
    }

    return slot.bodyID;
}

bool physics::jolt::JoltBodyRegistry::ReleaseHandle(const PhysicsBodyHandle handle)
{
    if (!handle.IsValid() || handle.index >= slots_.size())
    {
        return false;
    }

    Slot& slot = slots_[handle.index];
    if (!slot.bOccupied || slot.generation != handle.generation)
    {
        return false;
    }

    freeSlotIndices_.push_back(handle.index);
    slot.bodyID = JPH::BodyID{};
    slot.bOccupied = false;

    if (slot.generation == std::numeric_limits<PhysicsBodyHandle::GenerationType>::max())
    {
        slot.generation = 1u;
    }
    else
    {
        ++slot.generation;
    }

    return true;
}

bool physics::jolt::JoltBodyRegistry::IsValid(const PhysicsBodyHandle handle) const noexcept
{
    return ResolveBodyID(handle).has_value();
}

void physics::jolt::JoltBodyRegistry::Reset() noexcept
{
    freeSlotIndices_.clear();
    for (PhysicsBodyHandle::IndexType index = 0u; index < slots_.size(); ++index)
    {
        Slot& slot = slots_[index];
        slot.bodyID = JPH::BodyID{};
        slot.bOccupied = false;
        if (slot.generation == std::numeric_limits<PhysicsBodyHandle::GenerationType>::max())
        {
            slot.generation = 1u;
        }
        else
        {
            ++slot.generation;
        }
        freeSlotIndices_.push_back(index);
    }
}