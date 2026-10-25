# IGameplayGOAPContext

**class** · домен «Решение (instance)» · `src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm:18–31` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Решение (instance)».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 21 | метод | public | `virtual ~IGameplayGOAPContext() = default;` |
| 23 | метод | public | `virtual void Observe(const GameplayWorld& world,` |
| 27 | метод | public | `virtual void ObserveActionEvents(const GameplayWorld&,` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm:18–31`.

```cpp linenums="18"
    class IGameplayGOAPContext
    {
    public:
        virtual ~IGameplayGOAPContext() = default;

        virtual void Observe(const GameplayWorld& world,
            std::span<const GameplayWorldEvent> events, AIAgentWorldState& facts) = 0;

        // Domains with synchronous action events may confirm them in this update.
        virtual void ObserveActionEvents(const GameplayWorld&,
            std::span<const GameplayWorldEvent>, AIAgentWorldState&)
        {
        }
    };
```
