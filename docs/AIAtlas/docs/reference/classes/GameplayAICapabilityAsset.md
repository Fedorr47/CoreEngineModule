# GameplayAICapabilityAsset

**struct** · домен «Авторинг» · `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:30–34` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Авторинг».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 32 | поле | public | `std::string type, context;` |
| 33 | поле | public | `std::any parameters;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:30–34`.

```cpp linenums="30"
    struct GameplayAICapabilityAsset
    {
        std::string type, context;
        std::any parameters;
    };
```
