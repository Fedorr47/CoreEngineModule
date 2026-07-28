module;

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <utility>

export module core:gameplay_traversal_link_registry;

import :gameplay_route;
import :gameplay_traversal_link;

export namespace rendern
{
    struct GameplayTraversalLinkHandleHasher
    {
        [[nodiscard]] constexpr std::size_t operator()(const GameplayTraversalLinkHandle handle) const noexcept
        {
            return static_cast<std::size_t>(handle.value);
        }
    };

    class GameplayTraversalLinkRegistry
    {
    public:
        [[nodiscard]] bool Register(GameplayTraversalLink link)
        {
            if (!link.IsValid())
            {
                return false;
            }
            const GameplayTraversalLinkHandle handle = link.handle;
            return linksByHandle_.emplace(handle, std::move(link)).second;
        }

        [[nodiscard]] std::optional<GameplayTraversalLink> Find(const GameplayTraversalLinkHandle handle) const noexcept
        {
            const auto iterator = linksByHandle_.find(handle);
            if (iterator == linksByHandle_.end())
            {
                return std::nullopt;
            }
            return iterator->second;
        }

        [[nodiscard]] bool Contains(const GameplayTraversalLinkHandle handle) const noexcept
        {
            return linksByHandle_.contains(handle);
        }

        [[nodiscard]] bool Remove(const GameplayTraversalLinkHandle handle) noexcept
        {
            return linksByHandle_.erase(handle) != 0u;
        }

        void Reset() noexcept
        {
            linksByHandle_.clear();
        }

    private:
        std::unordered_map<
            GameplayTraversalLinkHandle, 
            GameplayTraversalLink,
            GameplayTraversalLinkHandleHasher> linksByHandle_{};
    };
}