# GameplayAIRouteGraphAsset

**struct** · домен «Авторинг» · `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:45–50` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Авторинг».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 47 | поле | public | `std::string source;` |
| 48 | поле | public | `std::vector<std::string> nodes;` |
| 49 | поле | public | `std::vector<GameplayAIRouteEdgeAsset> edges;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:45–50`.

```cpp linenums="45"
    struct GameplayAIRouteGraphAsset
    {
        std::string source;
        std::vector<std::string> nodes;
        std::vector<GameplayAIRouteEdgeAsset> edges;
    };
```
