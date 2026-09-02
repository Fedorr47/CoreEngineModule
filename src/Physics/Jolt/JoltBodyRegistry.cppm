module;

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

#include <limits>
#include <optional>
#include <unordered_map>
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
        
        [[nodiscard]] PhysicsBodyHandle AllocateHandle(JPH::BodyID bodyID,
            SurfaceTypeId surface = DefaultSurfaceType,
            PhysicsShapeDescriptor shape = {},
            PhysicsMotionType motionType = PhysicsMotionType::Static);
        
        [[nodiscard]] std::optional<JPH::BodyID> ResolveBodyID
            (const PhysicsBodyHandle handle) const noexcept;
        
        [[nodiscard]] std::optional<PhysicsBodyHandle> ResolveHandle(
            JPH::BodyID bodyID) const noexcept;
        
        [[nodiscard]] std::optional<SurfaceTypeId> ResolveSurface(
            PhysicsBodyHandle handle) const noexcept;
        
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
        
        template<typename Visitor>
        void VisitActiveBodies(Visitor&& visitor) const
        {
            for (PhysicsBodyHandle::IndexType index = 0u; index < slots_.size(); ++index)
            {
                const Slot& slot = slots_[index];
                if (slot.bOccupied)
                {
                    std::forward<Visitor>(visitor)(
                        PhysicsBodyHandle{ index, slot.generation }, slot.bodyID, slot.shape, slot.motionType);
                }
            }
        }

        void Reset() noexcept;
        
    private:
        struct Slot
        {
            JPH::BodyID bodyID{};
            PhysicsBodyHandle::GenerationType generation{1u};
            SurfaceTypeId surface{ InvalidSurfaceType };
            PhysicsShapeDescriptor shape{};
            PhysicsMotionType motionType{ PhysicsMotionType::Static };
            bool bOccupied{false};
        };
        
        std::vector<Slot> slots_;
        std::vector<PhysicsBodyHandle::IndexType> freeSlotIndices_;
        std::unordered_map<JPH::uint32, PhysicsBodyHandle> handlesByBodyID_;
    };
}

physics::PhysicsBodyHandle physics::jolt::JoltBodyRegistry::AllocateHandle(
    JPH::BodyID bodyID, const SurfaceTypeId surface,
    PhysicsShapeDescriptor shape, const PhysicsMotionType motionType)
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
    
    const PhysicsBodyHandle handle{.index = slotIndex, .generation = slots_[slotIndex].generation};
    try
    {
        const bool inserted = handlesByBodyID_.emplace(bodyID.GetIndexAndSequenceNumber(), handle).second;
        if (!inserted)
        {
            freeSlotIndices_.push_back(slotIndex);
            return InvalidPhysicsBodyHandle;
        }
    }
    catch (...)
    {
        freeSlotIndices_.push_back(slotIndex);
        throw;
    }
    
    Slot& slot = slots_[slotIndex];
    slot.bodyID = bodyID;
    slot.surface = surface;
    slot.shape = std::move(shape);
    slot.motionType = motionType;
    slot.bOccupied = true;
    return handle;
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

std::optional<physics::SurfaceTypeId> physics::jolt::JoltBodyRegistry::ResolveSurface(
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
    return slot.surface;
}

std::optional<physics::PhysicsBodyHandle> physics::jolt::JoltBodyRegistry::ResolveHandle(
    const JPH::BodyID bodyID) const noexcept
{
    if (bodyID.IsInvalid())
    {
        return std::nullopt;
    }

    const auto iterator = handlesByBodyID_.find(bodyID.GetIndexAndSequenceNumber());
    if (iterator == handlesByBodyID_.end() || !IsValid(iterator->second))
    {
        return std::nullopt;
    }
    return iterator->second;
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
    handlesByBodyID_.erase(slot.bodyID.GetIndexAndSequenceNumber());
    slot.bodyID = JPH::BodyID{};
    slot.surface = InvalidSurfaceType;
    slot.shape = {};
    slot.motionType = PhysicsMotionType::Static;
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
    handlesByBodyID_.clear();
    freeSlotIndices_.clear();
    for (PhysicsBodyHandle::IndexType index = 0u; index < slots_.size(); ++index)
    {
        Slot& slot = slots_[index];
        slot.bodyID = JPH::BodyID{};
        slot.surface = InvalidSurfaceType;
        slot.shape = {};
        slot.motionType = PhysicsMotionType::Static;
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