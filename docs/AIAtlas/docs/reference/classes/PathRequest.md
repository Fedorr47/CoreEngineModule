# PathRequest

**struct** · домен «NavMesh» · `src/Navigation/Navigation.cppm:71–76` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «NavMesh».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 73 | поле | public | `mathUtils::Vec3 start{};` |
| 74 | поле | public | `mathUtils::Vec3 goal{};` |
| 75 | поле | public | `mathUtils::Vec3 searchExtents{ 2.0f, 4.0f, 2.0f };` |

## Объявление

Включено из исходника при сборке; `src/Navigation/Navigation.cppm:71–76`.

```cpp linenums="71"
    struct PathRequest
    {
        mathUtils::Vec3 start{};
        mathUtils::Vec3 goal{};
        mathUtils::Vec3 searchExtents{ 2.0f, 4.0f, 2.0f };
    };
```
