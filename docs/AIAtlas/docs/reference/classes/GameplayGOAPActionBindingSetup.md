# GameplayGOAPActionBindingSetup

**struct** · домен «Решение (instance)» · `src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm:43–50` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Решение (instance)».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 45 | поле | public | `AIActionId actionId{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm:43–50`.

```cpp linenums="43"
    struct GameplayGOAPActionBindingSetup
    {
        AIActionId actionId{};
        // Called once after context, facts and the event buffer have stable addresses.
        // Captures may borrow from setup.context, but not from the temporary setup.
        std::function<std::unique_ptr<IAIActionBinding>(
            AIAgentWorldState&, std::vector<GameplayWorldEvent>&)> create{};
    };
```
