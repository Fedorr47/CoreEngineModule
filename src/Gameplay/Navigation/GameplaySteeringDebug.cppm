module;

#include <unordered_map>

export module core:gameplay_steering_debug;

import :EnTTHelpers;
import :gameplay_obstacle_avoidance;

export namespace rendern
{
    using EnTT_helpers::EntityHandle;
    
    enum class GameplaySteeringDebugMode
    {
        None,
        Route,
        Follow,
        Flee
    };

    struct GameplaySteeringDebugState
    {
        GameplaySteeringDebugMode mode{GameplaySteeringDebugMode::None};
        GameplayObstacleAvoidanceDebugSnapshot avoidance{};
    };

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
}