module;

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

export module core:gameplay_ai_access_key_observation;

import :ai_agent_world_state;
import :gameplay_ai_access_key_contracts;
import :gameplay;
import :gameplay_object_reservation_system;
import :gameplay_world_event;
import :math_utils;

export namespace rendern
{

    namespace ai_access_key_detail
    {
        enum class SpatialLocation : std::uint8_t
        {
            Start, CoinA, CoinB, CoinC, AccessKeyShop, Goal, Count
        };

        inline constexpr std::size_t kSpatialLocationCount =
            static_cast<std::size_t>(SpatialLocation::Count);
        inline constexpr float kSpatialArrivalRadius = 0.6f;
        inline constexpr float kSpatialArrivalRadiusSquared =
            kSpatialArrivalRadius * kSpatialArrivalRadius;

        struct AccessKeyFactBindings
        {
            AIWorldFactId hasAccessKey{};
            AIWorldFactId atDestination{};
            std::array<AIWorldFactId, 3> collected{};
            std::array<AIWorldFactId, 3> available{};
            std::array<AIWorldFactId, kSpatialLocationCount> spatial{};
            AIWorldIntegerFactId coins{};
        };

        class AccessKeyObservationAdapter
        {
        public:
            AccessKeyObservationAdapter(const EntityHandle agent,
                const std::array<EntityHandle, 3> coinEntities,
                const EntityHandle keyEntity, const AccessKeyFactBindings factBindings,
                const std::array<mathUtils::Vec3, kSpatialLocationCount> spatialPositions,
                GameplayObjectReservationSystem* reservationSystem) noexcept
                : agent_(agent), coinEntities_(coinEntities), keyEntity_(keyEntity),
                  factBindings_(factBindings), spatialPositions_(spatialPositions),
                  reservationSystem_(reservationSystem)
            {
            }

            void Observe(const std::span<const GameplayWorldEvent> events,
                const GameplayWorld& world, AIAgentWorldState& facts) const
            {
                for (const GameplayWorldEvent& event : events)
                {
                    if (event.instigator != agent_)
                    {
                        continue;
                    }
                    if (event.type == GameplayWorldEventType::PickupCollected)
                    {
                        for (std::size_t index = 0; index < coinEntities_.size(); ++index)
                        {
                            if (event.subject == coinEntities_[index] &&
                                !facts.IsFactSet(factBindings_.collected[index]))
                            {
                                facts.SetFact(factBindings_.collected[index], true);
                                facts.SetIntegerFact(factBindings_.coins,
                                    facts.GetIntegerFact(factBindings_.coins) + 1);
                            }
                        }
                    }
                    else if (event.type == GameplayWorldEventType::AccessKeyPurchased &&
                        event.subject == keyEntity_ &&
                        !facts.IsFactSet(factBindings_.hasAccessKey))
                    {
                        const std::int32_t coins = facts.GetIntegerFact(factBindings_.coins);
                        if (coins >= kAccessKeyPrice)
                        {
                            facts.SetIntegerFact(factBindings_.coins, coins - kAccessKeyPrice);
                            facts.SetFact(factBindings_.hasAccessKey, true);
                        }
                    }
                }

                for (std::size_t index = 0; index < coinEntities_.size(); ++index)
                {
                    const EntityHandle coin = coinEntities_[index];
                    const GameplayPickupComponent* pickup = world.TryGetPickup(coin);
                    const bool reservedByOther = reservationSystem_ != nullptr &&
                        reservationSystem_->IsReserved(coin) &&
                        !reservationSystem_->IsReservedBy(coin, agent_);
                    facts.SetFact(factBindings_.available[index],
                        world.IsEntityValid(coin) && pickup != nullptr &&
                        !pickup->collected && !reservedByOther);
                }

                const GameplayTransformComponent* transform = world.TryGetTransform(agent_);
                if (transform == nullptr)
                {
                    return;
                }
                std::optional<SpatialLocation> confirmedLocation;
                float confirmedDistanceSquared = kSpatialArrivalRadiusSquared;
                for (std::size_t index = 0; index < kSpatialLocationCount; ++index)
                {
                    const mathUtils::Vec3 delta = transform->position - spatialPositions_[index];
                    const float distanceSquared = mathUtils::Dot(delta, delta);
                    if (distanceSquared <= confirmedDistanceSquared)
                    {
                        confirmedDistanceSquared = distanceSquared;
                        confirmedLocation = static_cast<SpatialLocation>(index);
                    }
                }
                if (!confirmedLocation.has_value())
                {
                    return;
                }
                for (std::size_t index = 0; index < kSpatialLocationCount; ++index)
                {
                    facts.SetFact(factBindings_.spatial[index],
                        index == static_cast<std::size_t>(*confirmedLocation));
                }
                if (*confirmedLocation == SpatialLocation::Goal &&
                    facts.IsFactSet(factBindings_.hasAccessKey))
                {
                    facts.SetFact(factBindings_.atDestination, true);
                }
            }

        private:
            EntityHandle agent_{};
            std::array<EntityHandle, 3> coinEntities_{};
            EntityHandle keyEntity_{kNullEntity};
            AccessKeyFactBindings factBindings_{};
            std::array<mathUtils::Vec3, kSpatialLocationCount> spatialPositions_{};
            const GameplayObjectReservationSystem* reservationSystem_{};
        };
    }
}