module;
#include <entt/entt.hpp>
#include <memory>
#include <utility>

module core:level_ecs_impl;

import :level_ecs;

namespace rendern
{
    struct LevelWorld::Impl
    {
        entt::registry registry{};
    };

    LevelWorld::LevelWorld()
        : impl_(std::make_unique<Impl>())
    {
    }

    LevelWorld::~LevelWorld() = default;
    LevelWorld::LevelWorld(LevelWorld&&) noexcept = default;
    LevelWorld& LevelWorld::operator=(LevelWorld&&) noexcept = default;

    EntityHandle LevelWorld::CreateEntity()
    {
        const entt::entity e = impl_->registry.create();
        return static_cast<EntityHandle>(entt::to_integral(e));
    }

    void LevelWorld::Clear()
    {
        impl_->registry.clear();
    }

    bool LevelWorld::IsEntityValid(const EntityHandle entity) const
    {
        const entt::entity e = static_cast<entt::entity>(entity);
        return impl_->registry.valid(e);
    }

    void LevelWorld::DestroyEntity(const EntityHandle entity)
    {
        const entt::entity e = static_cast<entt::entity>(entity);
        if (impl_->registry.valid(e))
        {
            impl_->registry.destroy(e);
        }
    }

    void LevelWorld::EmplaceNodeData(const EntityHandle entity, const int nodeIndex, const int parentIndex, const Transform& local, const mathUtils::Mat4& world, const Flags& flags)
    {
        const entt::entity e = static_cast<entt::entity>(entity);
        impl_->registry.emplace<LevelNodeId>(e, nodeIndex);
        impl_->registry.emplace<ParentIndex>(e, parentIndex);
        impl_->registry.emplace<LocalTransform>(e, local);
        impl_->registry.emplace<WorldTransform>(e, world);
        impl_->registry.emplace<Flags>(e, flags);
    }

    void LevelWorld::UpsertNodeData(const EntityHandle entity, const int nodeIndex, const int parentIndex, const Transform& local, const mathUtils::Mat4& world, const Flags& flags)
    {
        const entt::entity e = static_cast<entt::entity>(entity);
        impl_->registry.emplace_or_replace<LevelNodeId>(e, nodeIndex);
        impl_->registry.emplace_or_replace<ParentIndex>(e, parentIndex);
        impl_->registry.emplace_or_replace<LocalTransform>(e, local);
        impl_->registry.emplace_or_replace<WorldTransform>(e, world);
        impl_->registry.emplace_or_replace<Flags>(e, flags);
    }

    void LevelWorld::EmplaceRenderable(const EntityHandle entity, const Renderable& renderable)
    {
        const entt::entity e = static_cast<entt::entity>(entity);
        impl_->registry.emplace<Renderable>(e, renderable);
    }

    void LevelWorld::UpsertRenderable(const EntityHandle entity, const Renderable& renderable)
    {
        const entt::entity e = static_cast<entt::entity>(entity);
        impl_->registry.emplace_or_replace<Renderable>(e, renderable);
    }

    bool LevelWorld::HasRenderable(const EntityHandle entity) const
    {
        const entt::entity e = static_cast<entt::entity>(entity);
        return impl_->registry.any_of<Renderable>(e);
    }

    void LevelWorld::RemoveRenderable(const EntityHandle entity)
    {
        const entt::entity e = static_cast<entt::entity>(entity);
        impl_->registry.remove<Renderable>(e);
    }
}