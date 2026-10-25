# BuildSettings

**struct** · домен «NavMesh» · `src/Navigation/Navigation.cppm:32–44` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «NavMesh».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 34 | поле | public | `AgentSettings agent{};` |
| 35 | поле | public | `float cellSize{ 0.2f };` |
| 36 | поле | public | `float cellHeight{ 0.1f };` |
| 37 | поле | public | `float regionMinSize{ 2.0f };` |
| 38 | поле | public | `float regionMergeSize{ 8.0f };` |
| 39 | поле | public | `float edgeMaxLength{ 12.0f };` |
| 40 | поле | public | `float edgeMaxError{ 1.3f };` |
| 41 | поле | public | `int verticesPerPolygon{ 6 };` |
| 42 | поле | public | `float detailSampleDistance{ 6.0f };` |
| 43 | поле | public | `float detailSampleMaxError{ 1.0f };` |

## Объявление

Включено из исходника при сборке; `src/Navigation/Navigation.cppm:32–44`.

```cpp linenums="32"
    struct BuildSettings
    {
        AgentSettings agent{};
        float cellSize{ 0.2f };
        float cellHeight{ 0.1f };
        float regionMinSize{ 2.0f };
        float regionMergeSize{ 8.0f };
        float edgeMaxLength{ 12.0f };
        float edgeMaxError{ 1.3f };
        int verticesPerPolygon{ 6 };
        float detailSampleDistance{ 6.0f };
        float detailSampleMaxError{ 1.0f };
    };
```
