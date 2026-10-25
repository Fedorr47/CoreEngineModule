# GameplayGOAPPreparedCapabilityGroup

**struct** · домен «Композиция» · `src/Gameplay/AI/Composition/GameplayGOAPDecisionTemplate.cppm:22–27` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Композиция».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 24 | поле | public | `std::string type;` |
| 25 | поле | public | `std::vector<GameplayAICapabilityAsset> assets;` |
| 26 | поле | public | `GameplayGOAPCapabilityRegistration registration;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/GameplayGOAPDecisionTemplate.cppm:22–27`.

```cpp linenums="22"
    struct GameplayGOAPPreparedCapabilityGroup
    {
        std::string type;
        std::vector<GameplayAICapabilityAsset> assets;
        GameplayGOAPCapabilityRegistration registration;
    };
```
