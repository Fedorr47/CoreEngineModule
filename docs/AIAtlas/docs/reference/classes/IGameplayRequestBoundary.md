# IGameplayRequestBoundary

**class** · домен «Диагностика» · `src/Gameplay/Navigation/GameplayRequest.cppm:81–95` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 84 | метод | public | `virtual ~IGameplayRequestBoundary() = default;` |
| 86 | метод | public | `[[nodiscard]] virtual std::optional<GameplayRequestHandle> Submit(` |
| 90 | метод | public | `[[nodiscard]] virtual std::optional<GameplayRequestStatus> TryGetStatus(` |
| 93 | метод | public | `[[nodiscard]] virtual bool Cancel(` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRequest.cppm:81–95`.

```cpp linenums="81"
    class IGameplayRequestBoundary
    {
    public:
        virtual ~IGameplayRequestBoundary() = default;

        [[nodiscard]] virtual std::optional<GameplayRequestHandle> Submit(
            const GameplayRequestContext& context,
            const TRequestt& request) = 0;

        [[nodiscard]] virtual std::optional<GameplayRequestStatus> TryGetStatus(
            GameplayRequestHandle requestHandle) const = 0;

        [[nodiscard]] virtual bool Cancel(
            GameplayRequestHandle requestHandle) = 0;
    };
```
