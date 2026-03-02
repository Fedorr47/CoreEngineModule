module;

#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

export module core:level_ecs;

import :scene;
import :math_utils;

export namespace rendern
{
    using EntityHandle = std::uint32_t;
    inline constexpr EntityHandle kNullEntity = (std::numeric_limits<EntityHandle>::max)();

    struct LevelNodeId
    {
        int index{ -1 };
    };

    struct ParentIndex
    {
        int parent{ -1 };
    };

    struct LocalTransform
    {
        Transform local{};
    };

    struct WorldTransform
    {
        mathUtils::Mat4 world{ 1.0f };
    };

    struct Renderable
    {
        MeshHandle mesh{};
        MaterialHandle material{};
        int drawIndex{ -1 }; // current Scene drawItems index (hybrid phase)
    };

    struct Flags
    {
        bool alive{ true };
        bool visible{ true };
    };

    class LevelWorld
    {
    public:
        LevelWorld();
        ~LevelWorld();

        LevelWorld(LevelWorld&&) noexcept;
        LevelWorld& operator=(LevelWorld&&) noexcept;

        LevelWorld(const LevelWorld&) = delete;
        LevelWorld& operator=(const LevelWorld&) = delete;

        EntityHandle CreateEntity();
        void Clear();

        bool IsEntityValid(EntityHandle entity) const;
        void DestroyEntity(EntityHandle entity);

        void EmplaceNodeData(EntityHandle entity, int nodeIndex, int parentIndex, const Transform& local, const mathUtils::Mat4& world, const Flags& flags);
        void UpsertNodeData(EntityHandle entity, int nodeIndex, int parentIndex, const Transform& local, const mathUtils::Mat4& world, const Flags& flags);

        void EmplaceRenderable(EntityHandle entity, const Renderable& renderable);
        void UpsertRenderable(EntityHandle entity, const Renderable& renderable);
        bool HasRenderable(EntityHandle entity) const;
        void RemoveRenderable(EntityHandle entity);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_{};
    };
}