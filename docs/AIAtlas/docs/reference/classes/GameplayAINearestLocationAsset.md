# GameplayAINearestLocationAsset

**struct** · домен «Компоненты» · `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponentAssets.cppm:28–32` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Компоненты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 30 | поле | public | `float radius{};` |
| 31 | поле | public | `std::vector<GameplayAILocationAsset> locations;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponentAssets.cppm:28–32`.

```cpp linenums="28"
    struct GameplayAINearestLocationAsset
    {
        float radius{};
        std::vector<GameplayAILocationAsset> locations;
    };
```
