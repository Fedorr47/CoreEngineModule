# ProfileHandle

**struct** · домен «NavMesh» · `src/Navigation/Navigation.cppm:140–149` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «NavMesh».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 142 | поле | public | `std::uint32_t value{ std::numeric_limits<std::uint32_t>::max() };` |
| 144 | метод | public | `[[nodiscard]] constexpr bool IsValid() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Navigation/Navigation.cppm:140–149`.

```cpp linenums="140"
    struct ProfileHandle
    {
        std::uint32_t value{ std::numeric_limits<std::uint32_t>::max() };

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return value != std::numeric_limits<std::uint32_t>::max();
        }
        [[nodiscard]] constexpr bool operator==(const ProfileHandle&) const noexcept = default;
    };
```
