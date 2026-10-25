# GameplayRequestContext

**struct** · домен «Диагностика» · `src/Gameplay/Navigation/GameplayRequest.cppm:40–52` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 42 | поле | public | `EntityHandle requesterEntity{kNullEntity};` |
| 44 | метод | public | `[[nodiscard]] constexpr bool IsValid() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRequest.cppm:40–52`.

```cpp linenums="40"
    struct GameplayRequestContext
    {
        EntityHandle requesterEntity{kNullEntity};

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return requesterEntity != kNullEntity;
        }

        friend constexpr bool operator==(
            const GameplayRequestContext&,
            const GameplayRequestContext&) noexcept = default;
    };
```
