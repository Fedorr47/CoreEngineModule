# AIActionRuntimeContext

**struct** · домен «Action runtime» · `src/Gameplay/AI/Runtime/AIActionRuntime.cppm:15–32` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Action runtime».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 17 | поле | public | `EntityHandle agentEntity{kNullEntity};` |
| 18 | поле | public | `AIActionId actionId{};` |
| 19 | поле | public | `AIActionContextId contextId{};` |
| 21 | метод | public | `[[nodiscard]] constexpr bool IsValid() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Runtime/AIActionRuntime.cppm:15–32`.

```cpp linenums="15"
    struct AIActionRuntimeContext
    {
        EntityHandle agentEntity{kNullEntity};
        AIActionId actionId{};
        AIActionContextId contextId{};

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            const bool bHasAgentEntity = agentEntity != kNullEntity;
            const bool bHasActionId = actionId.IsValid();

            return bHasAgentEntity && bHasActionId;
        }

        friend constexpr bool operator==(
            const AIActionRuntimeContext&,
            const AIActionRuntimeContext&) = default;
    };
```
