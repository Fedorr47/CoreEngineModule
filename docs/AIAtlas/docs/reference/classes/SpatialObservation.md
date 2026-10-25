# SpatialObservation

**class** · домен «Компоненты» · `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:20–88` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IGameplayGOAPObservation`

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Компоненты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 23 | конструктор | public | `SpatialObservation(const GameplayAIObservationAsset& asset, const GameplayGOAPCompositionContext& context)` |
| 51 | метод | public | `std::vector<AIWorldFactId> BooleanOutputs() const override { return {fact_}; }` |
| 52 | метод | public | `std::vector<AIWorldFactId> BooleanInputs() const override { return inputs_; }` |
| 53 | метод | public | `void Observe(const GameplayWorld& world, EntityHandle agent,` |
| 77 | метод | public | `void ObserveActionEvents(const GameplayWorld& world, EntityHandle agent,` |
| 83 | поле | private | `GameplayAISpatialObservationAsset parameters_;` |
| 84 | поле | private | `GameplayAIResolvedRole target_;` |
| 85 | поле | private | `AIWorldFactId fact_;` |
| 86 | поле | private | `std::vector<AIWorldFactId> inputs_;` |
| 87 | поле | private | `bool distance_{};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:20–80`.

```cpp linenums="20"
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
```
