# GameplayAIMoveToAsset

**struct** · домен «Компоненты» · `src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponentAssets.cppm:13–20` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Компоненты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 15 | поле | public | `std::string source, target;` |
| 16 | поле | public | `float acceptanceRadius{0.4f}, slowingRadius{1.5f};` |
| 17 | поле | public | `bool wantsRun{};` |
| 18 | поле | public | `float sourceRadius{};` |
| 19 | поле | public | `bool reserveTarget{}, routeCost{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponentAssets.cppm:13–20`.

```cpp linenums="13"
    struct GameplayAIMoveToAsset
    {
        std::string source, target;
        float acceptanceRadius{0.4f}, slowingRadius{1.5f};
        bool wantsRun{};
        float sourceRadius{};
        bool reserveTarget{}, routeCost{};
    };
```
