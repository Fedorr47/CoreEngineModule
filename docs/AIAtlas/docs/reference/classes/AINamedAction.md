# AINamedAction

**struct** · домен «Решение (instance)» · `src/Gameplay/AI/GOAP/GameplayGOAPDefinitionContracts.cppm:16–22` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Решение (instance)».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 18 | поле | public | `AIActionId actionId{};` |
| 19 | поле | public | `AIActionContextId contextId{};` |
| 20 | поле | public | `std::string actionName{};` |
| 21 | поле | public | `std::string contextName{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/GameplayGOAPDefinitionContracts.cppm:16–22`.

```cpp linenums="16"
    struct AINamedAction
    {
        AIActionId actionId{};
        AIActionContextId contextId{};
        std::string actionName{};
        std::string contextName{};
    };
```
