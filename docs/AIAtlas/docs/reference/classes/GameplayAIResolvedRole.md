# GameplayAIResolvedRole

**struct** · домен «Композиция» · `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:26–31` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Композиция».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 28 | поле | public | `std::string role;` |
| 29 | поле | public | `EntityHandle entity{kNullEntity};` |
| 30 | поле | public | `mathUtils::Vec3 position{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:26–31`.

```cpp linenums="26"
    struct GameplayAIResolvedRole
    {
        std::string role;
        EntityHandle entity{kNullEntity};
        mathUtils::Vec3 position{};
    };
```
