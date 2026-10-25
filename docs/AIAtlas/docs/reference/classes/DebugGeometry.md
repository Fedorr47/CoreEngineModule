# DebugGeometry

**struct** · домен «NavMesh» · `src/Navigation/Navigation.cppm:101–105` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «NavMesh».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 103 | поле | public | `std::vector<DebugLine> lines;` |
| 104 | поле | public | `std::vector<DebugTriangle> triangles;` |

## Объявление

Включено из исходника при сборке; `src/Navigation/Navigation.cppm:101–105`.

```cpp linenums="101"
    struct DebugGeometry
    {
        std::vector<DebugLine> lines;
        std::vector<DebugTriangle> triangles;
    };
```
