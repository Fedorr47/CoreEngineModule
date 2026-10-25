# GameplayAIDecisionAssetReference

**struct** · домен «Авторинг» · `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:83–88` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Авторинг».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 85 | поле | public | `std::string id{};` |
| 86 | поле | public | `std::string behavior{};` |
| 87 | поле | public | `std::string bindings{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:83–88`.

```cpp linenums="83"
    struct GameplayAIDecisionAssetReference
    {
        std::string id{};
        std::string behavior{};
        std::string bindings{};
    };
```
