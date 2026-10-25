# IGameplayGOAPObservation

**class** · домен «Композиция» · `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:35–50` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Композиция».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 38 | метод | public | `virtual ~IGameplayGOAPObservation() = default;` |
| 39 | метод | public | `[[nodiscard]] virtual std::vector<AIWorldFactId> BooleanOutputs() const = 0;` |
| 40 | метод | public | `[[nodiscard]] virtual std::vector<AIWorldIntegerFactId> IntegerOutputs() const { return {}; }` |
| 41 | метод | public | `[[nodiscard]] virtual std::vector<AIWorldFactId> BooleanInputs() const { return {}; }` |
| 42 | метод | public | `[[nodiscard]] virtual std::vector<AIWorldIntegerFactId> IntegerInputs() const { return {}; }` |
| 43 | метод | public | `virtual void Observe(const GameplayWorld& world, EntityHandle agent,` |
| 46 | метод | public | `virtual void ObserveActionEvents(const GameplayWorld&, EntityHandle,` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:35–50`.

```cpp linenums="35"
    class IGameplayGOAPObservation
    {
    public:
        virtual ~IGameplayGOAPObservation() = default;
        [[nodiscard]] virtual std::vector<AIWorldFactId> BooleanOutputs() const = 0;
        [[nodiscard]] virtual std::vector<AIWorldIntegerFactId> IntegerOutputs() const { return {}; }
        [[nodiscard]] virtual std::vector<AIWorldFactId> BooleanInputs() const { return {}; }
        [[nodiscard]] virtual std::vector<AIWorldIntegerFactId> IntegerInputs() const { return {}; }
        virtual void Observe(const GameplayWorld& world, EntityHandle agent,
            std::span<const GameplayWorldEvent> events, AIAgentWorldState& facts) = 0;

        virtual void ObserveActionEvents(const GameplayWorld&, EntityHandle,
            std::span<const GameplayWorldEvent>, AIAgentWorldState&)
        {
        }
    };
```
