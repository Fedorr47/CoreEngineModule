module;

#include <unordered_map>

export module core:gameplay_navigation_debug;

import :EnTTHelpers;
import :gameplay_route;

export namespace rendern
{
    using EnTT_helpers::EntityHandle;

    // Navigation owns the resolved routes independently of GOAP intent/plan debug.
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
}