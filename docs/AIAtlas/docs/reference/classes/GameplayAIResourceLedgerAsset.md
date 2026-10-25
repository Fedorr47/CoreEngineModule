# GameplayAIResourceLedgerAsset

**struct** · домен «Компоненты» · `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponentAssets.cppm:23–28` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Компоненты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 25 | поле | public | `std::string fact;` |
| 26 | поле | public | `std::vector<GameplayAIResourcePickupAsset> pickups;` |
| 27 | поле | public | `std::vector<GameplayAIResourceReceiptAsset> receipts;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponentAssets.cppm:23–28`.

```cpp linenums="23"
    struct GameplayAIResourceLedgerAsset
    {
        std::string fact;
        std::vector<GameplayAIResourcePickupAsset> pickups;
        std::vector<GameplayAIResourceReceiptAsset> receipts;
    };
```
