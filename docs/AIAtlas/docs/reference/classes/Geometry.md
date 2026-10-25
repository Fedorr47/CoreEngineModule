# Geometry

**struct** · домен «NavMesh» · `src/Navigation/Navigation.cppm:16–20` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «NavMesh».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 18 | поле | public | `std::vector<mathUtils::Vec3> vertices;` |
| 19 | поле | public | `std::vector<std::uint32_t> indices;` |

## Объявление

Включено из исходника при сборке; `src/Navigation/Navigation.cppm:16–20`.

```cpp linenums="16"
    struct Geometry
    {
        std::vector<mathUtils::Vec3> vertices;
        std::vector<std::uint32_t> indices;
    };
```
