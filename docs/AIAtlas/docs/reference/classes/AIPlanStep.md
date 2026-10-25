# AIPlanStep

**struct** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:91–97` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ядро GOAP».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 93 | поле | public | `AIActionId actionId{};` |
| 94 | поле | public | `AIActionContextId contextId{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:91–97`.

```cpp linenums="91"
    struct AIPlanStep
    {
        AIActionId actionId{};
        AIActionContextId contextId{};

        friend constexpr bool operator==(const AIPlanStep&, const AIPlanStep&) noexcept = default;
    };
```
