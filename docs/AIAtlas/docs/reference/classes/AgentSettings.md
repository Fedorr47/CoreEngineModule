# AgentSettings

**struct** · домен «NavMesh» · `src/Navigation/Navigation.cppm:22–30` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «NavMesh».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 24 | поле | public | `float radius{ 0.4f };` |
| 25 | поле | public | `float height{ 1.8f };` |
| 26 | поле | public | `float maximumStepHeight{ 0.4f };` |
| 27 | поле | public | `float maximumSlopeAngleDegrees{ 45.0f };` |
| 29 | метод | public | `[[nodiscard]] bool IsValid() const noexcept;` |

## Объявление

Включено из исходника при сборке; `src/Navigation/Navigation.cppm:22–30`.

```cpp linenums="22"
    struct AgentSettings
    {
        float radius{ 0.4f };
        float height{ 1.8f };
        float maximumStepHeight{ 0.4f };
        float maximumSlopeAngleDegrees{ 45.0f };

        [[nodiscard]] bool IsValid() const noexcept;
    };
```
