# NearestLocationObservation

**class** · домен «Компоненты» · `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:90–146` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IGameplayGOAPObservation`

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Компоненты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 93 | конструктор | public | `NearestLocationObservation(const GameplayAIObservationAsset& asset, const GameplayGOAPCompositionContext& context)` |
| 107 | метод | public | `std::vector<AIWorldFactId> BooleanOutputs() const override { return outputs_; }` |
| 108 | метод | public | `void Observe(const GameplayWorld& world, EntityHandle agent,` |
| 137 | метод | public | `void ObserveActionEvents(const GameplayWorld& world, EntityHandle agent,` |
| 143 | поле | private | `float radius_{};` |
| 144 | поле | private | `std::vector<mathUtils::Vec3> positions_;` |
| 145 | поле | private | `std::vector<AIWorldFactId> outputs_;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:90–146`.

```cpp linenums="90"
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
```
