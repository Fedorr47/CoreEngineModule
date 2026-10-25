# GameplayRouteFollowerOutput

**struct** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRouteFollower.cppm:28–33` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Маршруты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 30 | поле | public | `GameplayMovementIntent movement;` |
| 31 | поле | public | `GameplayRouteFollowerStatus status{GameplayRouteFollowerStatus::NotStarted};` |
| 32 | поле | public | `std::optional<GameplayTraversalLinkHandle> requiredTraversalLink{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRouteFollower.cppm:28–33`.

```cpp linenums="28"
    struct GameplayRouteFollowerOutput
    {
        GameplayMovementIntent movement;
        GameplayRouteFollowerStatus status{GameplayRouteFollowerStatus::NotStarted};
        std::optional<GameplayTraversalLinkHandle> requiredTraversalLink{};
    };
```
