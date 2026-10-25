# ProjectionResult

**struct** · домен «NavMesh» · `src/Navigation/Navigation.cppm:65–69` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «NavMesh».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 67 | поле | public | `QueryStatus status{ QueryStatus::NotInitialized };` |
| 68 | поле | public | `mathUtils::Vec3 position{};` |

## Объявление

Включено из исходника при сборке; `src/Navigation/Navigation.cppm:65–69`.

```cpp linenums="65"
    struct ProjectionResult
    {
        QueryStatus status{ QueryStatus::NotInitialized };
        mathUtils::Vec3 position{};
    };
```
