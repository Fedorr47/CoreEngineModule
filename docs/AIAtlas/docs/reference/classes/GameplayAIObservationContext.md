# GameplayAIObservationContext

**struct** · домен «Решение (instance)» · `src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm:24–29` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Решение (instance)».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 26 | поле | public | `const GameplayWorld& world;` |
| 27 | поле | public | `std::span<const GameplayWorldEvent> events;` |
| 28 | поле | public | `std::vector<GameplayWorldEvent>* eventOutput{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm:24–29`.

```cpp linenums="24"
    struct GameplayAIObservationContext
    {
        const GameplayWorld& world;
        std::span<const GameplayWorldEvent> events;
        std::vector<GameplayWorldEvent>* eventOutput{};
    };
```
