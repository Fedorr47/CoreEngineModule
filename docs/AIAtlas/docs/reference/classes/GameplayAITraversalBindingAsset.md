# GameplayAITraversalBindingAsset

**struct** · домен «Авторинг» · `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:51–55` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Авторинг».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 53 | поле | public | `std::string name, target, type;` |
| 54 | поле | public | `std::uint64_t handle{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:51–55`.

```cpp linenums="51"
    struct GameplayAITraversalBindingAsset
    {
        std::string name, target, type;
        std::uint64_t handle{};
    };
```
