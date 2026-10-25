# GameplayAIDecisionCreationContext

**struct** · домен «Решение (instance)» · `src/Gameplay/AI/Runtime/GameplayAIDecisionCreationContext.cppm:21–31` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Решение (instance)».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 23 | поле | public | `EntityHandle agent{kNullEntity};` |
| 24 | поле | public | `LevelAsset& level;` |
| 25 | поле | public | `GameplayWorld& world;` |
| 26 | поле | public | `const GameplayTraversalLinkRegistry& traversalLinkRegistry;` |
| 27 | поле | public | `const GameplayTraversalExecutorRegistry& traversalExecutorRegistry;` |
| 28 | поле | public | `GameplayObjectReservationSystem& reservationSystem;` |
| 30 | поле | public | `std::string* diagnostic{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Runtime/GameplayAIDecisionCreationContext.cppm:21–31`.

```cpp linenums="21"
    struct GameplayAIDecisionCreationContext
    {
        EntityHandle agent{kNullEntity};
        LevelAsset& level;
        GameplayWorld& world;
        const GameplayTraversalLinkRegistry& traversalLinkRegistry;
        const GameplayTraversalExecutorRegistry& traversalExecutorRegistry;
        GameplayObjectReservationSystem& reservationSystem;
        // Optional per-call diagnostic output. Never retained by a decision.
        std::string* diagnostic{};
    };
```
