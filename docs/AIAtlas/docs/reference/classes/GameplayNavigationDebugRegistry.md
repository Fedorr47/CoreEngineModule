# GameplayNavigationDebugRegistry

**class** · домен «Диагностика» · `src/Gameplay/Navigation/GameplayNavigationDebug.cppm:15–53` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 18 | метод | public | `void Publish(const EntityHandle owner, const GameplayRoute& route)` |
| 30 | метод | public | `void Clear(const EntityHandle owner) noexcept` |
| 35 | метод | public | `void Clear() noexcept` |
| 40 | метод | public | `[[nodiscard]] const GameplayRoute* Find(const EntityHandle owner) const noexcept` |
| 46 | метод | public | `[[nodiscard]] const std::unordered_map<EntityHandle, GameplayRoute>& Routes() const noexcept` |
| 52 | поле | private | `std::unordered_map<EntityHandle, GameplayRoute> routes_{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayNavigationDebug.cppm:15–53`.

```cpp linenums="15"
    class GameplayNavigationDebugRegistry
    {
    public:
        void Publish(const EntityHandle owner, const GameplayRoute& route)
        {
            if (route.IsValid() && route.points.size() > 1u)
            {
                routes_[owner] = route;
            }
            else
            {
                routes_.erase(owner);
            }
        }

        void Clear(const EntityHandle owner) noexcept
        {
            routes_.erase(owner);
        }

        void Clear() noexcept
        {
            routes_.clear();
        }

        [[nodiscard]] const GameplayRoute* Find(const EntityHandle owner) const noexcept
        {
            const auto found = routes_.find(owner);
            return found == routes_.end() ? nullptr : &found->second;
        }

        [[nodiscard]] const std::unordered_map<EntityHandle, GameplayRoute>& Routes() const noexcept
        {
            return routes_;
        }

    private:
        std::unordered_map<EntityHandle, GameplayRoute> routes_{};
    };
```
