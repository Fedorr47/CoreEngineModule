#include "Physics/LevelPhysicsRuntime.h"

#include "Physics/Jolt/JoltPhysicsWorld.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <optional>
#include <ranges>
#include <variant>

namespace 
{
    constexpr float RotationToleranceDegrees = 1.0e-4f;
    constexpr float PositionChangeTolerance = 1.0e-6f;
    
    [[nodiscard]] bool IsShapeValid(const physics::PhysicsShapeDescriptor& shape)
    {
        return std::visit([](const auto& descriptor) -> bool 
        { return descriptor.IsValid(); }, shape);
    }
    
    [[nodiscard]] bool PositionsDiffer(
        const mathUtils::Vec3& first, const mathUtils::Vec3& second) noexcept
    {
        return std::fabs(first.x - second.x) > PositionChangeTolerance
            || std::fabs(first.y - second.y) > PositionChangeTolerance
            || std::fabs(first.z - second.z) > PositionChangeTolerance;
    }
}

namespace physics
{
    LevelPhysicsRuntime::LevelPhysicsRuntime(JoltPhysicsWorld& world) noexcept : world_(world) {}
    LevelPhysicsRuntime::~LevelPhysicsRuntime() noexcept { Shutdown(); }
    
    bool LevelPhysicsRuntime::EnterGame(
            rendern::LevelAsset& levelAsset,
            rendern::LevelInstance& levelInstance,
            rendern::Scene& scene,
            std::string& errorMessage)
    {
        if (bActive_ || !world_.IsInitialized())
        {
            errorMessage = bActive_ ? "level physics world is already active." : "level physics world is not initialized.";
            return false;
        }
        world_.ResetSimulationClock();
        
        const std::size_t physicsBodyCount = static_cast<std::size_t>(std::ranges::count_if(
            levelAsset.nodes,
            [](const rendern::LevelNode& node)
            {
                return node.alive && node.physicsBody.has_value();
            }));
        bindings_.reserve(physicsBodyCount);
        
        for (std::size_t index = 0; index < levelAsset.nodes.size(); ++index)
        {
            const rendern::LevelNode& node = levelAsset.nodes[index];
            if (!node.alive || !node.physicsBody.has_value())
            {
                continue;
            }
            if (node.parent != -1)
            {
                errorMessage = "Level physics node '" + node.name + "' must be a root-level node.";
                return false;
            }
            const mathUtils::Vec3& rotation = node.transform.rotationDegrees;
            if (node.transform.useMatrix
                || std::fabs(rotation.x) > RotationToleranceDegrees
                || std::fabs(rotation.y) > RotationToleranceDegrees
                || std::fabs(rotation.z) > RotationToleranceDegrees)
            {
                errorMessage = "Level physics node '" + node.name + "' must use zero authored rotation.";
                return false;
            }
            if (index > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            {
                errorMessage = "Level physics node index cannot be represented by the runtime binding.";
                return false;
            }
            if (node.physicsBody->motionType != PhysicsMotionType::Static
                && node.physicsBody->motionType != PhysicsMotionType::Dynamic)
            {
                errorMessage = "Level physics node '" + node.name + "' has an unsupported motion type.";
                return false;
            }
            if (!IsShapeValid(node.physicsBody->shape))
            {
                errorMessage = "Level physics node '" + node.name + "' has an invalid shape.";
                return false;
            }
        }
        
        std::optional<PhysicsBodyHandle> unrecordedHandle;
        try
        {
            for (std::size_t index = 0; index < levelAsset.nodes.size(); ++index)
            {
                rendern::LevelNode& node = levelAsset.nodes[index];
                if (!node.alive || !node.physicsBody.has_value())
                {
                    continue;
                }
                // Serialized shape dimensions are authoritative and are not multiplied by visual scale.
                const PhysicsBodyDescriptor descriptor{
                    .shape = node.physicsBody->shape,
                    .transform = { .position = node.transform.position },
                    .motionType = node.physicsBody->motionType
                };
                const PhysicsBodyHandle handle = world_.CreateBody(descriptor);
                if (!handle.IsValid())
                {
                    errorMessage = "Failed to create level physics body for node '" + node.name + "'.";
                    static_cast<void>(DestroyBindings());
                    world_.ResetSimulationClock();
                    return false;
                }
                unrecordedHandle = handle;
                bindings_.push_back({
                    .nodeIndex = static_cast<int>(index),
                    .handle = handle,
                    .motionType = node.physicsBody->motionType,
                    .authoredPosition = node.transform.position
                });
                unrecordedHandle.reset();
            }
        }
        catch (...)
        {
            if (unrecordedHandle.has_value())
            {
                try
                {
                    static_cast<void>(world_.DestroyBody(*unrecordedHandle));
                }
                catch (...)
                {
                }
            }
            static_cast<void>(DestroyBindings());
            bActive_ = false;
            world_.ResetSimulationClock();
            throw;
        }
        
        world_.ResetSimulationClock();
        bActive_ = true;
        try
        {
            levelInstance.MarkTransformsDirty();
            levelInstance.SyncTransformsIfDirty(levelAsset, scene);
        }
        catch (...)
        {
            static_cast<void>(DestroyBindings());
            bActive_ = false;
            world_.ResetSimulationClock();
            throw;
        }
        return true;
    }
    
    bool LevelPhysicsRuntime::Synchronize(
        rendern::LevelAsset& levelAsset,
        rendern::LevelInstance& levelInstance,
        rendern::Scene& scene,
        std::string& errorMessage)
    {
        if (!bActive_)
        {
            return true;
        }
        bool changed = false;
        for (const Binding& binding : bindings_)
        {
            if (binding.motionType != PhysicsMotionType::Dynamic)
            {
                continue;
            }
            if (binding.nodeIndex < 0
                || static_cast<std::size_t>(binding.nodeIndex) >= levelAsset.nodes.size())
            {
                errorMessage = "Level physics binding has an invalid node index.";
                return false;
            }
            rendern::LevelNode& node = levelAsset.nodes[static_cast<std::size_t>(binding.nodeIndex)];
            if (!node.alive)
            {
                errorMessage = "Level physics binding references a node that is no longer alive.";
                return false;
            }
            if (!world_.IsBodyValid(binding.handle))
            {
                errorMessage = "Level physics binding has an invalid body handle.";
                return false;
            }
            const auto transform = world_.GetBodyTransform(binding.handle);
            if (!transform.has_value())
            {
                errorMessage = "Level physics binding became invalid for node index "
                    + std::to_string(binding.nodeIndex) + ".";
                return false;
            }
            if (PositionsDiffer(node.transform.position, transform->position))
            {
                node.transform.position = transform->position;
                changed = true;
            }
        }
        if (changed)
        {
            levelInstance.MarkTransformsDirty();
            levelInstance.SyncTransformsIfDirty(levelAsset, scene);
        }
        return true;
    }
    
    bool LevelPhysicsRuntime::LeaveGame(
        rendern::LevelAsset& levelAsset,
        rendern::LevelInstance& levelInstance,
        rendern::Scene& scene,
        std::string& errorMessage)
    {
        for (const Binding& binding : bindings_)
        {
            if (binding.nodeIndex >= 0
                && static_cast<std::size_t>(binding.nodeIndex) < levelAsset.nodes.size())
            {
                levelAsset.nodes[static_cast<std::size_t>(binding.nodeIndex)].transform.position =
                    binding.authoredPosition;
            }
        }
        const bool bindingsDestroyed = DestroyBindings();
        bActive_ = false;
        world_.ResetSimulationClock();
        levelInstance.MarkTransformsDirty();
        try
        {
            levelInstance.SyncTransformsIfDirty(levelAsset, scene);
        }
        catch (const std::exception& exception)
        {
            errorMessage = "Failed to synchronize restored level physics transforms: "
                + std::string(exception.what());
            return false;
        }
        if (!bindingsDestroyed)
        {
            errorMessage = "Failed to destroy one or more level physics bindings.";
            return false;
        }
        return true;
    }
    
    void LevelPhysicsRuntime::Shutdown() noexcept
    {
        static_cast<void>(DestroyBindings());
        bActive_ = false;
    }
    
    [[nodiscard]] bool LevelPhysicsRuntime::IsActive() const noexcept
    {
        return bActive_;
    }
    
    [[nodiscard]] std::size_t LevelPhysicsRuntime::GetBindingCount() const noexcept
    {
        return bindings_.size();
    }
    
    bool LevelPhysicsRuntime::DestroyBindings() noexcept
    {
        bool allDestroyed = true;
        for (auto iterator = bindings_.rbegin(); iterator != bindings_.rend(); ++iterator)
        {
            try
            {
                allDestroyed = world_.DestroyBody(iterator->handle) && allDestroyed;
            }
            catch (...)
            {
                // Destruction is best effort during ownership teardown.
                allDestroyed = false;
            }
        }
        bindings_.clear();
        return allDestroyed;
    }
}
