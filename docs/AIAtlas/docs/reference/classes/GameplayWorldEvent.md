# GameplayWorldEvent

**struct** · домен «Ресурсы и события» · `src/Gameplay/Events/GameplayWorldEvent.cppm:21–28` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ресурсы и события».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 23 | поле | public | `GameplayWorldEventType type{};` |
| 24 | поле | public | `EntityHandle instigator{ kNullEntity };` |
| 25 | поле | public | `EntityHandle subject{ kNullEntity };` |
| 27 | поле | public | `std::string receiptId{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Events/GameplayWorldEvent.cppm:21–28`.

```cpp linenums="21"
    struct GameplayWorldEvent
    {
        GameplayWorldEventType type{};
        EntityHandle instigator{ kNullEntity };
        EntityHandle subject{ kNullEntity };
        // Owned semantic identity, required for ResourcePurchased. Other events leave it empty.
        std::string receiptId{};
    };
```
