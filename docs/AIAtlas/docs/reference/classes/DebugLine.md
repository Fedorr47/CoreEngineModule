# DebugLine

**struct** · домен «NavMesh» · `src/Navigation/Navigation.cppm:84–89` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «NavMesh».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 86 | поле | public | `mathUtils::Vec3 start{};` |
| 87 | поле | public | `mathUtils::Vec3 end{};` |
| 88 | поле | public | `std::uint32_t rgba{};` |

## Объявление

Включено из исходника при сборке; `src/Navigation/Navigation.cppm:84–89`.

```cpp linenums="84"
    struct DebugLine
    {
        mathUtils::Vec3 start{};
        mathUtils::Vec3 end{};
        std::uint32_t rgba{};
    };
```
