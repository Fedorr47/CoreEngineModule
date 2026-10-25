# GameplayGOAPDecisionSetup

**struct** · домен «Решение (instance)» · `src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm:52–58` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Решение (instance)».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 54 | поле | public | `GameplayGOAPDecisionDefinition definition{};` |
| 55 | поле | public | `std::unique_ptr<IGameplayGOAPContext> context{};` |
| 56 | поле | public | `std::vector<GameplayGOAPActionBindingSetup> actionBindings{};` |
| 57 | поле | public | `std::vector<std::unique_ptr<IGameplayGOAPEventReaction>> eventReactions{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm:52–58`.

```cpp linenums="52"
    struct GameplayGOAPDecisionSetup
    {
        GameplayGOAPDecisionDefinition definition{};
        std::unique_ptr<IGameplayGOAPContext> context{};
        std::vector<GameplayGOAPActionBindingSetup> actionBindings{};
        std::vector<std::unique_ptr<IGameplayGOAPEventReaction>> eventReactions{};
    };
```
