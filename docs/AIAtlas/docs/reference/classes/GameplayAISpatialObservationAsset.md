# GameplayAISpatialObservationAsset

**struct** · домен «Компоненты» · `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponentAssets.cppm:13–21` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Компоненты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 15 | поле | public | `std::string target, fact;` |
| 16 | поле | public | `float radius{};` |
| 17 | поле | public | `bool requireInteractionPoint{};` |
| 18 | поле | public | `bool requireEntity{true};` |
| 19 | поле | public | `bool latch{};` |
| 20 | поле | public | `std::vector<std::string> requiredFacts;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponentAssets.cppm:13–21`.

```cpp linenums="13"
    struct GameplayAISpatialObservationAsset
    {
        std::string target, fact;
        float radius{};
        bool requireInteractionPoint{};
        bool requireEntity{true};
        bool latch{};
        std::vector<std::string> requiredFacts;
    };
```
