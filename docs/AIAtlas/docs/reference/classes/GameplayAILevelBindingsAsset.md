# GameplayAILevelBindingsAsset

**struct** · домен «Авторинг» · `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:76–81` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Авторинг».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 78 | поле | public | `std::string source{};` |
| 79 | поле | public | `std::vector<GameplayAIRoleBindingAsset> roles{};` |
| 80 | поле | public | `std::vector<GameplayAITraversalBindingAsset> traversals;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:76–81`.

```cpp linenums="76"
    struct GameplayAILevelBindingsAsset
    {
        std::string source{};
        std::vector<GameplayAIRoleBindingAsset> roles{};
        std::vector<GameplayAITraversalBindingAsset> traversals;
    };
```
