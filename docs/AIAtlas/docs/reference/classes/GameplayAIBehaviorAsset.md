# GameplayAIBehaviorAsset

**struct** · домен «Авторинг» · `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:57–68` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Авторинг».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 59 | поле | public | `std::string id{};` |
| 60 | поле | public | `std::string source{};` |
| 61 | поле | public | `std::string model{};` |
| 62 | поле | public | `std::string definition{};` |
| 63 | поле | public | `std::vector<GameplayAIObservationAsset> observations{};` |
| 64 | поле | public | `std::vector<GameplayAICapabilityAsset> capabilities{};` |
| 65 | поле | public | `std::string routeGraph;` |
| 66 | поле | public | `bool inspectPath{};` |
| 67 | поле | public | `std::vector<GameplayAIReactionAsset> reactions;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:57–68`.

```cpp linenums="57"
    struct GameplayAIBehaviorAsset
    {
        std::string id{};
        std::string source{};
        std::string model{};
        std::string definition{};
        std::vector<GameplayAIObservationAsset> observations{};
        std::vector<GameplayAICapabilityAsset> capabilities{};
        std::string routeGraph;
        bool inspectPath{};
        std::vector<GameplayAIReactionAsset> reactions;
    };
```
