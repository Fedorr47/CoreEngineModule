# GameplayRouteSearchResult

**struct** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRouteSearch.cppm:80–92` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Маршруты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 82 | поле | public | `GameplayRouteSearchStatus status{GameplayRouteSearchStatus::InvalidRequest};` |
| 83 | поле | public | `GameplayRoute route{};` |
| 86 | поле | public | `std::optional<float> totalCost{};` |
| 88 | метод | public | `[[nodiscard]] bool Succeeded() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRouteSearch.cppm:80–92`.

```cpp linenums="80"
    struct GameplayRouteSearchResult
    {
        GameplayRouteSearchStatus status{GameplayRouteSearchStatus::InvalidRequest};
        GameplayRoute route{};

        // Present only when status is Succeeded.
        std::optional<float> totalCost{};

        [[nodiscard]] bool Succeeded() const noexcept
        {
            return status == GameplayRouteSearchStatus::Succeeded;
        }
    };
```
