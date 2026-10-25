# PickupAvailabilityObservation

**class** · домен «Компоненты» · `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:148–178` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IGameplayGOAPObservation`

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Компоненты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 151 | конструктор | public | `PickupAvailabilityObservation(const GameplayAIObservationAsset& asset, const GameplayGOAPCompositionContext& context)` |
| 152 | метод | public | `: reservations_(context.services.reservationSystem)` |
| 164 | метод | public | `std::vector<AIWorldFactId> BooleanOutputs() const override { return {fact_}; }` |
| 165 | метод | public | `void Observe(const GameplayWorld& world, EntityHandle agent,` |
| 174 | поле | private | `const GameplayObjectReservationSystem& reservations_;` |
| 175 | поле | private | `EntityHandle target_{kNullEntity};` |
| 176 | поле | private | `AIWorldFactId fact_;` |
| 177 | поле | private | `bool respectReservations_{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:148–178`.

```cpp linenums="148"
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
```
