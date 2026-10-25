# ProfileResolution

**struct** · домен «NavMesh» · `src/Navigation/Navigation.cppm:151–155` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «NavMesh».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 153 | поле | public | `BuildStatus status{ BuildStatus::BuildFailed };` |
| 154 | поле | public | `ProfileHandle profile{};` |

## Объявление

Включено из исходника при сборке; `src/Navigation/Navigation.cppm:151–155`.

```cpp linenums="151"
    struct ProfileResolution
    {
        BuildStatus status{ BuildStatus::BuildFailed };
        ProfileHandle profile{};
    };
```
