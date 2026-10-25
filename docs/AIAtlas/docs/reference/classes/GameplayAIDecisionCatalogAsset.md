# GameplayAIDecisionCatalogAsset

**struct** · домен «Авторинг» · `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:90–94` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Авторинг».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 92 | поле | public | `std::string source{};` |
| 93 | поле | public | `std::vector<GameplayAIDecisionAssetReference> decisions{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:90–94`.

```cpp linenums="90"
    struct GameplayAIDecisionCatalogAsset
    {
        std::string source{};
        std::vector<GameplayAIDecisionAssetReference> decisions{};
    };
```
