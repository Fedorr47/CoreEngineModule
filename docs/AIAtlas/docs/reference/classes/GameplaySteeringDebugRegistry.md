# GameplaySteeringDebugRegistry

**class** · домен «Диагностика» · `src/Gameplay/Navigation/GameplaySteeringDebug.cppm:28–82` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 31 | метод | public | `void SetEnabled(const bool enabled) noexcept` |
| 40 | метод | public | `[[nodiscard]] bool IsEnabled() const noexcept` |
| 45 | метод | public | `void Publish(` |
| 56 | метод | public | `void Clear(const EntityHandle agent) noexcept` |
| 61 | метод | public | `void Clear() noexcept` |
| 66 | метод | public | `[[nodiscard]] const GameplaySteeringDebugState* Find(` |
| 74 | метод | public | `States() const noexcept` |
| 80 | поле | private | `bool enabled_{false};` |
| 81 | поле | private | `std::unordered_map<EntityHandle, GameplaySteeringDebugState> states_{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplaySteeringDebug.cppm:28–82`.

```cpp linenums="28"
    class GameplaySteeringDebugRegistry
    {
    public:
        void SetEnabled(const bool enabled) noexcept
        {
            enabled_ = enabled;
            if (!enabled_)
            {
                states_.clear();
            }
        }

        [[nodiscard]] bool IsEnabled() const noexcept
        {
            return enabled_;
        }

        void Publish(
            const EntityHandle agent,
            const GameplaySteeringDebugMode mode,
            const GameplayObstacleAvoidanceDebugSnapshot& snapshot)
        {
            if (enabled_)
            {
                states_[agent] = {mode, snapshot};
            }
        }

        void Clear(const EntityHandle agent) noexcept
        {
            states_.erase(agent);
        }

        void Clear() noexcept
        {
            states_.clear();
        }

        [[nodiscard]] const GameplaySteeringDebugState* Find(
            const EntityHandle agent) const noexcept
        {
            const auto found = states_.find(agent);
            return found == states_.end() ? nullptr : &found->second;
        }

        [[nodiscard]] const std::unordered_map<EntityHandle, GameplaySteeringDebugState>&
            States() const noexcept
        {
            return states_;
        }

    private:
        bool enabled_{false};
        std::unordered_map<EntityHandle, GameplaySteeringDebugState> states_{};
    };
```
