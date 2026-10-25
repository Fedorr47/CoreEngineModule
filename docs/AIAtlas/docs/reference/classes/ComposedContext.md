# ComposedContext

**class** · домен «Композиция» · `src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:88–116` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IGameplayGOAPContext`

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Композиция».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 91 | конструктор | public | `explicit ComposedContext(EntityHandle agent) : agent_(agent) {}` |
| 93 | метод | public | `void Observe(const GameplayWorld& world, std::span<const GameplayWorldEvent> events,` |
| 102 | метод | public | `void ObserveActionEvents(const GameplayWorld& world,` |
| 111 | поле | public | `std::vector<std::unique_ptr<IGameplayGOAPObservation>> observations;` |
| 112 | поле | public | `std::vector<std::unique_ptr<IGameplayGOAPCapability>> capabilities;` |
| 115 | поле | private | `EntityHandle agent_;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:88–116`.

```cpp linenums="88"
    class ComposedContext : public IGameplayGOAPContext
    {
    public:
        explicit ComposedContext(EntityHandle agent) : agent_(agent) {}

        void Observe(const GameplayWorld& world, std::span<const GameplayWorldEvent> events,
            AIAgentWorldState& facts) override
        {
            for (const auto& observation : observations)
            {
                observation->Observe(world, agent_, events, facts);
            }
        }

        void ObserveActionEvents(const GameplayWorld& world,
            std::span<const GameplayWorldEvent> events, AIAgentWorldState& facts) override
        {
            for (const auto& observation : observations)
            {
                observation->ObserveActionEvents(world, agent_, events, facts);
            }
        }

        std::vector<std::unique_ptr<IGameplayGOAPObservation>> observations;
        std::vector<std::unique_ptr<IGameplayGOAPCapability>> capabilities;

    private:
        EntityHandle agent_;
    };
```
