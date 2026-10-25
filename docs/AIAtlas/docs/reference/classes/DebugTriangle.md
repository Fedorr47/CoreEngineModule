# DebugTriangle

**struct** · домен «NavMesh» · `src/Navigation/Navigation.cppm:91–99` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «NavMesh».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 93 | поле | public | `mathUtils::Vec3 a{};` |
| 94 | поле | public | `mathUtils::Vec3 b{};` |
| 95 | поле | public | `mathUtils::Vec3 c{};` |
| 96 | поле | public | `std::uint32_t rgbaA{};` |
| 97 | поле | public | `std::uint32_t rgbaB{};` |
| 98 | поле | public | `std::uint32_t rgbaC{};` |

## Объявление

Включено из исходника при сборке; `src/Navigation/Navigation.cppm:91–99`.

```cpp linenums="91"
    struct DebugTriangle
    {
        mathUtils::Vec3 a{};
        mathUtils::Vec3 b{};
        mathUtils::Vec3 c{};
        std::uint32_t rgbaA{};
        std::uint32_t rgbaB{};
        std::uint32_t rgbaC{};
    };
```
