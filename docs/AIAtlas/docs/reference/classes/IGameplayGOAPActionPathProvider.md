# IGameplayGOAPActionPathProvider

**class** · домен «Диагностика» · `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:31–36` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 34 | метод | public | `virtual ~IGameplayGOAPActionPathProvider() = default;` |
| 35 | метод | public | `[[nodiscard]] virtual std::optional<GameplayRoute> BuildDebugRoute(AIActionContextId context) const = 0;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:31–36`.

```cpp linenums="31"
    class IGameplayGOAPActionPathProvider
    {
    public:
        virtual ~IGameplayGOAPActionPathProvider() = default;
        [[nodiscard]] virtual std::optional<GameplayRoute> BuildDebugRoute(AIActionContextId context) const = 0;
    };
```
