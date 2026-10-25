# PathResult

**struct** · домен «NavMesh» · `src/Navigation/Navigation.cppm:78–82` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «NavMesh».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 80 | поле | public | `QueryStatus status{ QueryStatus::NotInitialized };` |
| 81 | поле | public | `std::vector<mathUtils::Vec3> points;` |

## Объявление

Включено из исходника при сборке; `src/Navigation/Navigation.cppm:78–82`.

```cpp linenums="78"
    struct PathResult
    {
        QueryStatus status{ QueryStatus::NotInitialized };
        std::vector<mathUtils::Vec3> points;
    };
```
