# GameplayAIRouteEdgeAsset

**struct** · домен «Авторинг» · `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:40–44` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Авторинг».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 42 | поле | public | `std::string from, to, traversal;` |
| 43 | поле | public | `std::optional<float> cost;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:40–44`.

```cpp linenums="40"
    struct GameplayAIRouteEdgeAsset
    {
        std::string from, to, traversal;
        std::optional<float> cost;
    };
```
