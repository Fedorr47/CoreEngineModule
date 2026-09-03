module;

#include <cmath>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

export module core:gameplay_goap_spatial_components;

import :gameplay_goap_composition_registry;
import :gameplay;
import :gameplay_object_reservation_system;

namespace rendern::goap_spatial_detail
{
    class SpatialObservation final : public IGameplayGOAPObservation
    {
    public:
        SpatialObservation(const GameplayAIObservationAsset& asset, const GameplayGOAPCompositionContext& context)
        {
            parameters_ = context.Parameters<GameplayAISpatialObservationAsset>(asset);
            target_ = context.Role(parameters_.target);
            fact_ = context.BooleanFact(parameters_.fact);
            distance_ = asset.type == "within_distance";
            if ((parameters_.requireEntity || parameters_.requireInteractionPoint)
                && !context.services.world.IsEntityValid(target_.entity))
            {
                context.Fail(asset.type, "target role '" + parameters_.target + "' has no gameplay entity");
            }
            if (parameters_.requireInteractionPoint && !context.services.world.HasInteractionPoint(target_.entity))
            {
                context.Fail(asset.type, "target role '" + parameters_.target + "' has no interaction point");
            }
            if (distance_ && (!(parameters_.radius > 0) || !std::isfinite(parameters_.radius * parameters_.radius)))
            {
                context.Fail(asset.type, "radius must be positive and its square finite");
            }
            if (!distance_ && parameters_.radius != 0)
            {
                context.Fail(asset.type, "radius is only supported by within_distance");
            }
            for (const auto& name : parameters_.requiredFacts)
            {
                inputs_.push_back(context.BooleanFact(name));
            }
        }
        std::vector<AIWorldFactId> BooleanOutputs() const override { return {fact_}; }
        std::vector<AIWorldFactId> BooleanInputs() const override { return inputs_; }
        void Observe(const GameplayWorld& world, EntityHandle agent,
            std::span<const GameplayWorldEvent>, AIAgentWorldState& facts) override
        {
            bool value = (!parameters_.requireEntity || world.IsEntityValid(target_.entity))
                && (!parameters_.requireInteractionPoint || world.HasInteractionPoint(target_.entity));
            for (const auto input : inputs_)
            {
                value = value && facts.IsFactSet(input);
            }
            if (distance_)
            {
                const auto* transform = world.TryGetTransform(agent);
                if (value && transform != nullptr)
                {
                    const auto delta = transform->position - target_.position;
                    value = mathUtils::Dot(delta, delta) <= parameters_.radius * parameters_.radius;
                }
                else
                {
                    value = false;
                }
            }
            facts.SetFact(fact_, value || (parameters_.latch && facts.IsFactSet(fact_)));
        }
        void ObserveActionEvents(const GameplayWorld& world, EntityHandle agent,
            std::span<const GameplayWorldEvent> events, AIAgentWorldState& facts) override
        {
            Observe(world, agent, events, facts);
        }
    private:
        GameplayAISpatialObservationAsset parameters_;
        GameplayAIResolvedRole target_;
        AIWorldFactId fact_;
        std::vector<AIWorldFactId> inputs_;
        bool distance_{};
    };

    class NearestLocationObservation final : public IGameplayGOAPObservation
    {
    public:
        NearestLocationObservation(const GameplayAIObservationAsset& asset, const GameplayGOAPCompositionContext& context)
        {
            const auto& parameters = context.Parameters<GameplayAINearestLocationAsset>(asset);
            radius_ = parameters.radius;
            if (!(radius_ > 0) || !std::isfinite(radius_ * radius_) || parameters.locations.empty())
            {
                context.Fail(asset.type, "require locations and positive finite radius");
            }
            for (const auto& location : parameters.locations)
            {
                positions_.push_back(context.Role(location.target).position);
                outputs_.push_back(context.BooleanFact(location.fact));
            }
        }
        std::vector<AIWorldFactId> BooleanOutputs() const override { return outputs_; }
        void Observe(const GameplayWorld& world, EntityHandle agent,
            std::span<const GameplayWorldEvent>, AIAgentWorldState& facts) override
        {
            const auto* transform = world.TryGetTransform(agent);
            if (transform == nullptr)
            {
                return;
            }
            std::optional<std::size_t> nearest;
            float distance = radius_ * radius_;
            for (std::size_t index = 0; index < positions_.size(); ++index)
            {
                const auto delta = transform->position - positions_[index];
                const float candidate = mathUtils::Dot(delta, delta);
                if (candidate <= distance)
                {
                    nearest = index;
                    distance = candidate;
                }
            }
            // Symbolic location remains the last confirmed node while in transit.
            if (nearest)
            {
                for (std::size_t index = 0; index < outputs_.size(); ++index)
                {
                    facts.SetFact(outputs_[index], index == *nearest);
                }
            }
        }
        void ObserveActionEvents(const GameplayWorld& world, EntityHandle agent,
            std::span<const GameplayWorldEvent> events, AIAgentWorldState& facts) override
        {
            Observe(world, agent, events, facts);
        }
    private:
        float radius_{};
        std::vector<mathUtils::Vec3> positions_;
        std::vector<AIWorldFactId> outputs_;
    };

    class PickupAvailabilityObservation final : public IGameplayGOAPObservation
    {
    public:
        PickupAvailabilityObservation(const GameplayAIObservationAsset& asset, const GameplayGOAPCompositionContext& context)
            : reservations_(context.services.reservationSystem)
        {
            const auto& parameters = context.Parameters<GameplayAIPickupAvailabilityAsset>(asset);
            target_ = context.Role(parameters.target).entity;
            fact_ = context.BooleanFact(parameters.fact);
            respectReservations_ = parameters.respectReservations;
            if (!context.services.world.HasPickup(target_)
                || (respectReservations_ && !context.services.world.HasInteractionPoint(target_)))
            {
                context.Fail(asset.type, "target requires a pickup and, for reservations, an interaction point");
            }
        }
        std::vector<AIWorldFactId> BooleanOutputs() const override { return {fact_}; }
        void Observe(const GameplayWorld& world, EntityHandle agent,
            std::span<const GameplayWorldEvent>, AIAgentWorldState& facts) override
        {
            const auto* pickup = world.TryGetPickup(target_);
            const bool reserved = respectReservations_ && reservations_.IsReserved(target_)
                && !reservations_.IsReservedBy(target_, agent);
            facts.SetFact(fact_, world.IsEntityValid(target_) && pickup != nullptr && !pickup->collected && !reserved);
        }
    private:
        const GameplayObjectReservationSystem& reservations_;
        EntityHandle target_{kNullEntity};
        AIWorldFactId fact_;
        bool respectReservations_{};
    };
}

export namespace rendern
{
    void RegisterGameplayGOAPSpatialComponents(GameplayGOAPCompositionRegistry& registry)
    {
        const auto spatial = [](const auto& asset, const auto& context)
        {
            return std::make_unique<goap_spatial_detail::SpatialObservation>(asset, context);
        };
        const bool available = registry.RegisterObservation("target_available", spatial);
        const bool distance = registry.RegisterObservation("within_distance", spatial);
        const bool location = registry.RegisterObservation("nearest_location", [](const auto& asset, const auto& context)
        {
            return std::make_unique<goap_spatial_detail::NearestLocationObservation>(asset, context);
        });
        const bool pickup = registry.RegisterObservation("pickup_available", [](const auto& asset, const auto& context)
        {
            return std::make_unique<goap_spatial_detail::PickupAvailabilityObservation>(asset, context);
        });
        if (!available || !distance || !location || !pickup)
        {
            throw std::logic_error("Duplicate spatial component registration");
        }
    }
}
