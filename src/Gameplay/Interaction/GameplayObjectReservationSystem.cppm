module;

#include <cstddef>
#include <unordered_map>

export module core:gameplay_object_reservation_system;

import :gameplay;

export namespace rendern
{
    class GameplayObjectReservationSystem
    {
    public:
        [[nodiscard]] bool TryReserve(
            const GameplayWorld& world,
            const EntityHandle objectEntity,
            const EntityHandle agentEntity)
        {
            const bool bHasValidObject = world.IsEntityValid(objectEntity);
            const bool bHasValidAgent = world.IsEntityValid(agentEntity);
            const bool bIsDifferentEntity = objectEntity != agentEntity;
            const bool bIsAIAgent = bHasValidAgent && world.HasAI(agentEntity);
            const bool bHasInteractionPoint = bHasValidObject && world.HasInteractionPoint(objectEntity);
            if (!bHasValidObject 
                || !bHasValidAgent 
                || !bIsDifferentEntity 
                || !bIsAIAgent 
                || !bHasInteractionPoint)
            {
                return false;
            }

            const auto [it, bInserted] = ownerByObject_.try_emplace(objectEntity, agentEntity);
            return bInserted || it->second == agentEntity;
        }

        [[nodiscard]] bool Release(const EntityHandle objectEntity, const EntityHandle agentEntity) noexcept
        {
            const auto it = ownerByObject_.find(objectEntity);
            if (it == ownerByObject_.end() 
                || it->second != agentEntity 
                || agentEntity == kNullEntity)
            {
                return false;
            }
            ownerByObject_.erase(it);
            return true;
        }

        [[nodiscard]] bool IsReserved(const EntityHandle objectEntity) const noexcept
        {
            return objectEntity != kNullEntity && ownerByObject_.contains(objectEntity);
        }

        [[nodiscard]] bool IsReservedBy(
            const EntityHandle objectEntity,
            const EntityHandle agentEntity) const noexcept
        {
            return agentEntity != kNullEntity && GetReservationOwner(objectEntity) == agentEntity;
        }

        [[nodiscard]] EntityHandle GetReservationOwner(const EntityHandle objectEntity) const noexcept
        {
            if (objectEntity == kNullEntity)
            {
                return kNullEntity;
            }
            const auto it = ownerByObject_.find(objectEntity);
            return it != ownerByObject_.end() ? it->second : kNullEntity;
        }

        std::size_t CleanupInvalidReservations(const GameplayWorld& world) noexcept
        {
            std::size_t removedCount = 0;
            for (auto it = ownerByObject_.begin(); it != ownerByObject_.end();)
            {
                const bool bHasValidObject = world.IsEntityValid(it->first);
                const bool bHasValidAgent = world.IsEntityValid(it->second);
                const bool bIsAIAgent = bHasValidAgent && world.HasAI(it->second);
                const bool bHasInteractionPoint = bHasValidObject && world.HasInteractionPoint(it->first);
                if (!bHasValidObject 
                    || !bHasValidAgent 
                    || !bIsAIAgent 
                    || !bHasInteractionPoint)
                {
                    it = ownerByObject_.erase(it);
                    ++removedCount;
                }
                else
                {
                    ++it;
                }
            }
            return removedCount;
        }

        void Reset() noexcept { ownerByObject_.clear(); }

    private:
        std::unordered_map<EntityHandle, EntityHandle> ownerByObject_{};
    };
}