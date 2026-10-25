# GameplayAIComponentParseContext

**struct** · домен «Авторинг» · `src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm:176–180` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Авторинг».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 178 | поле | public | `const jsonUtils::JsonObject& object;` |
| 179 | поле | public | `std::string_view source, location;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm:176–180`.

```cpp linenums="176"
    struct GameplayAIComponentParseContext
    {
        const jsonUtils::JsonObject& object;
        std::string_view source, location;
    };
```
