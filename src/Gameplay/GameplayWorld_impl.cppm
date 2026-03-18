module;

#include <entt/entt.hpp>
#include <utility>

module core:gameplay_impl;

import :gameplay;

namespace rendern
{
    namespace
    {
        template <typename TComponent>
        void AddOrReplaceComponent_(entt::registry& registry, const EntityHandle entity, const TComponent& value)
        {
            if (entity == kNullEntity || !registry.valid(ToEnTT(entity)))
            {
                return;
            }

            registry.emplace_or_replace<TComponent>(ToEnTT(entity), value);
        }

        template <typename TComponent>
        TComponent* TryGetComponent_(entt::registry& registry, const EntityHandle entity) noexcept
        {
            if (entity == kNullEntity || !registry.valid(ToEnTT(entity)))
            {
                return nullptr;
            }

            return registry.try_get<TComponent>(ToEnTT(entity));
        }

        template <typename TComponent>
        const TComponent* TryGetComponent_(const entt::registry& registry, const EntityHandle entity) noexcept
        {
            if (entity == kNullEntity || !registry.valid(ToEnTT(entity)))
            {
                return nullptr;
            }

            return registry.try_get<TComponent>(ToEnTT(entity));
        }

        template <typename TComponent>
        bool HasComponent_(const entt::registry& registry, const EntityHandle entity) noexcept
        {
            return entity != kNullEntity &&
                registry.valid(ToEnTT(entity)) &&
                registry.all_of<TComponent>(ToEnTT(entity));
        }

        template <typename TComponent>
        void RemoveComponent_(entt::registry& registry, const EntityHandle entity)
        {
            if (!HasComponent_<TComponent>(registry, entity))
            {
                return;
            }

            registry.remove<TComponent>(ToEnTT(entity));
        }
    }

    struct GameplayWorld::Impl
    {
        entt::registry registry{};
        std::size_t aliveCount{ 0 };
    };

    GameplayWorld::GameplayWorld()
        : impl_(std::make_unique<Impl>())
    {
    }

    GameplayWorld::~GameplayWorld() = default;

    GameplayWorld::GameplayWorld(GameplayWorld&& other) noexcept = default;
    GameplayWorld& GameplayWorld::operator=(GameplayWorld&& other) noexcept = default;

    EntityHandle GameplayWorld::CreateEntity()
    {
        const entt::entity entity = impl_->registry.create();
        ++impl_->aliveCount;
        return FromEnTT(entity);
    }

    void GameplayWorld::DestroyEntity(const EntityHandle entity)
    {
        if (!IsEntityValid(entity))
        {
            return;
        }

        impl_->registry.destroy(ToEnTT(entity));
        if (impl_->aliveCount > 0)
        {
            --impl_->aliveCount;
        }
    }

    void GameplayWorld::Clear() noexcept
    {
        impl_->registry.clear();
        impl_->aliveCount = 0;
    }

    bool GameplayWorld::IsEntityValid(const EntityHandle entity) const noexcept
    {
        if (entity == kNullEntity)
        {
            return false;
        }

        return impl_->registry.valid(ToEnTT(entity));
    }

    std::size_t GameplayWorld::GetAliveCount() const noexcept
    {
        return impl_->aliveCount;
    }

#define DEFINE_GAMEPLAY_COMPONENT_ACCESSORS(Name, Type) \
    void GameplayWorld::Add##Name(const EntityHandle entity, const Type& value) \
    { \
        AddOrReplaceComponent_<Type>(impl_->registry, entity, value); \
    } \
    void GameplayWorld::Set##Name(const EntityHandle entity, const Type& value) \
    { \
        Add##Name(entity, value); \
    } \
    Type* GameplayWorld::TryGet##Name(const EntityHandle entity) noexcept \
    { \
        return TryGetComponent_<Type>(impl_->registry, entity); \
    } \
    const Type* GameplayWorld::TryGet##Name(const EntityHandle entity) const noexcept \
    { \
        return TryGetComponent_<Type>(impl_->registry, entity); \
    } \
    bool GameplayWorld::Has##Name(const EntityHandle entity) const noexcept \
    { \
        return HasComponent_<Type>(impl_->registry, entity); \
    } \
    void GameplayWorld::Remove##Name(const EntityHandle entity) \
    { \
        RemoveComponent_<Type>(impl_->registry, entity); \
    }

#include "GameplayWorld_component_accessors.inl"

#undef DEFINE_GAMEPLAY_COMPONENT_ACCESSORS
}
