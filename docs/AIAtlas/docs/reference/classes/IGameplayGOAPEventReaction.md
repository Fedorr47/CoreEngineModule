# IGameplayGOAPEventReaction

**class** · домен «Решение (instance)» · `src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm:35–41` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Решение (instance)».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 38 | метод | public | `virtual ~IGameplayGOAPEventReaction() = default;` |
| 39 | метод | public | `virtual void React(const GameplayWorld& world, std::span<const GameplayWorldEvent> events,` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm:35–41`.

```cpp linenums="35"
    class IGameplayGOAPEventReaction
    {
    public:
        virtual ~IGameplayGOAPEventReaction() = default;
        virtual void React(const GameplayWorld& world, std::span<const GameplayWorldEvent> events,
            const AIAgentWorldState& facts, std::vector<GameplayWorldEvent>& output) = 0;
    };
```
