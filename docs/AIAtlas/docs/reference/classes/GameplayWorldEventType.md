# GameplayWorldEventType

**enum** · домен «Ресурсы и события» · `src/Gameplay/Events/GameplayWorldEvent.cppm:14–19` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ресурсы и события».

## Объявление

Включено из исходника при сборке; `src/Gameplay/Events/GameplayWorldEvent.cppm:14–19`.

```cpp linenums="14"
    enum class GameplayWorldEventType : std::uint8_t
    {
        PickupCollected,
        ResourcePurchased,
        HideEntityRequested
    };
```
