#include "DevelopmentScenario.h"
#include "AppDevelopmentScenarioRuntime.h"
#include "Physics/Jolt/JoltPhysicsWorld.h"

import core;

#include "App/GameplayPhysicsCharacterIntegration.h"

namespace appDevelopment
{
    namespace
    {
        using jsonUtils::JsonArray;
        using jsonUtils::JsonObject;
        using jsonUtils::JsonParser;
        using rendern::EntityHandle;

        [[noreturn]] void Invalid(const std::string_view source, const std::string_view section,
            const std::size_t index, const std::string_view reason)
        {
            throw std::runtime_error("Development scenario '" + std::string(source) + "', " +
                std::string(section) + "[" + std::to_string(index) + "]: " + std::string(reason));
        }

        const std::string& RequiredString(const JsonObject& object, const std::string_view key,
            const std::string_view source, const std::string_view section, const std::size_t index)
        {
            const auto it = object.find(std::string(key));
            if (it == object.end() || !it->second.IsString() || it->second.AsString().empty())
                Invalid(source, section, index, "missing or empty string field '" + std::string(key) + "'");
            return it->second.AsString();
        }

        std::vector<ScenarioOperation> ParseOperations(const JsonObject& root, const std::string_view section,
            const std::string_view source)
        {
            const auto found = root.find(std::string(section));
            if (found == root.end()) return {};
            if (!found->second.IsArray()) Invalid(source, section, 0, "lifecycle must be an array");
            std::vector<ScenarioOperation> result;
            const JsonArray& array = found->second.AsArray();
            result.reserve(array.size());
            for (std::size_t i = 0; i < array.size(); ++i)
            {
                if (!array[i].IsObject()) Invalid(source, section, i, "operation must be an object");
                const JsonObject& operation = array[i].AsObject();
                const std::string& op = RequiredString(operation, "op", source, section, i);
                const auto entity = [&]() { return RequiredString(operation, "entity", source, section, i); };
                const auto slot = [&]() { return RequiredString(operation, "slot", source, section, i); };
                if (op == "captureTransform") result.emplace_back(CaptureTransformOperation{entity(), slot()});
                else if (op == "restoreTransform") result.emplace_back(RestoreTransformOperation{entity(), slot()});
                else if (op == "ensureAI") result.emplace_back(EnsureAIOperation{entity()});
                else if (op == "cancelAI") result.emplace_back(CancelAIOperation{entity()});
                else if (op == "teleportPhysicsCharacter") result.emplace_back(TeleportPhysicsCharacterOperation{entity()});
                else if (op == "setRuntimeVisibility")
                {
                    const auto visible = operation.find("visible");
                    if (visible == operation.end() || !visible->second.IsBool())
                        Invalid(source, section, i, "missing boolean field 'visible'");
                    result.emplace_back(SetRuntimeVisibilityOperation{entity(), visible->second.AsBool()});
                }
                else if (op == "ensureNodeBoundEntity") result.emplace_back(EnsureNodeBoundEntityOperation{entity()});
                else Invalid(source, section, i, "unknown operation type '" + op + "'");
            }
            return result;
        }

        std::string OperationRole(const ScenarioOperation& operation)
        {
            return std::visit([](const auto& value) { return value.entity; }, operation);
        }
    }

    DevelopmentScenarioAsset ParseDevelopmentScenarioAsset(const std::string_view json, const std::string_view sourcePath)
    {
        const auto value = JsonParser(json).Parse();
        if (!value.IsObject()) throw std::runtime_error("Development scenario '" + std::string(sourcePath) + "': root must be object");
        const JsonObject& root = value.AsObject();
        DevelopmentScenarioAsset asset{};
        asset.sourcePath = std::string(sourcePath);
        const auto stringOrEmpty = [&](const char* key) {
            const auto it = root.find(key); return it == root.end() ? std::string{} :
                (it->second.IsString() ? it->second.AsString() : throw std::runtime_error("Development scenario: '" + std::string(key) + "' must be a string"));
        };
        asset.id = stringOrEmpty("id"); asset.title = stringOrEmpty("title"); asset.description = stringOrEmpty("description");
        const auto roles = root.find("roles");
        if (roles == root.end() || !roles->second.IsObject()) throw std::runtime_error("Development scenario '" + asset.id + "': roles must be an object");
        for (const auto& [role, node] : roles->second.AsObject())
        {
            if (role.empty() || !node.IsString() || node.AsString().empty())
                throw std::runtime_error("Development scenario '" + asset.id + "': role and node names must not be empty");
            asset.roles.emplace_back(role, node.AsString());
        }
        std::ranges::sort(asset.roles, {}, &std::pair<std::string, std::string>::first);
        const std::string diagnosticName = asset.id.empty() ? asset.sourcePath : asset.id;
        asset.setup = ParseOperations(root, "setup", diagnosticName); asset.start = ParseOperations(root, "start", diagnosticName);
        asset.update = ParseOperations(root, "update", diagnosticName); asset.stop = ParseOperations(root, "stop", diagnosticName);
        asset.reset = ParseOperations(root, "reset", diagnosticName);
        ValidateDevelopmentScenarioAsset(asset);
        return asset;
    }

    DevelopmentScenarioAsset LoadDevelopmentScenarioAsset(const std::string_view path)
    {
        const std::filesystem::path resolved = corefs::ResolveAsset(std::filesystem::path(path));
        return ParseDevelopmentScenarioAsset(FILE_UTILS::ReadAllText(resolved), path);
    }

    void ValidateDevelopmentScenarioAsset(const DevelopmentScenarioAsset& asset)
    {
        if (asset.id.empty()) throw std::runtime_error("Development scenario '" + asset.sourcePath + "': empty scenario id");
        if (asset.title.empty()) throw std::runtime_error("Development scenario '" + asset.id + "': empty title");
        std::unordered_set<std::string> roles;
        for (const auto& [role, node] : asset.roles)
            if (role.empty() || node.empty() || !roles.insert(role).second) throw std::runtime_error("Development scenario '" + asset.id + "': invalid or duplicate role '" + role + "'");
        const std::string identity = asset.id + (asset.sourcePath.empty() ? std::string{} : " (" + asset.sourcePath + ")");
        std::unordered_set<std::string> captured;
        std::unordered_set<std::string> setupCaptured;
        std::unordered_set<std::string> startCaptured;
        const std::array sections{std::pair{"setup", &asset.setup}, std::pair{"start", &asset.start}, std::pair{"update", &asset.update}, std::pair{"stop", &asset.stop}, std::pair{"reset", &asset.reset}};
        for (const auto& [name, operations] : sections)
        {
            // Stop is reachable after a completed Start, but never depends on Update.
            if (std::string_view(name) == "stop")
            {
                captured = setupCaptured;
                captured.insert(startCaptured.begin(), startCaptured.end());
            }
            // Reset is legal immediately after Load, so it may only consume setup baselines.
            if (std::string_view(name) == "reset")
            {
                captured = setupCaptured;
            }
            for (std::size_t i = 0; i < operations->size(); ++i)
            {
                const ScenarioOperation& operation = (*operations)[i];
                if (!roles.contains(OperationRole(operation))) Invalid(identity, name, i, "operation references unknown role '" + OperationRole(operation) + "'");
                if (const auto* capture = std::get_if<CaptureTransformOperation>(&operation))
                {
                    captured.insert(capture->slot);
                    if (std::string_view(name) == "setup") setupCaptured.insert(capture->slot);
                    if (std::string_view(name) == "start") startCaptured.insert(capture->slot);
                }
                if (const auto* restore = std::get_if<RestoreTransformOperation>(&operation); restore && !captured.contains(restore->slot))
                    Invalid(identity, name, i, "transform slot '" + restore->slot + "' is not guaranteed to have been captured");
            }
        }
    }

    struct DevelopmentScenarioRunner::Impl
    {
        std::optional<DevelopmentScenarioAsset> asset;
        std::unordered_map<std::string, int> nodes;
        std::unordered_map<std::string, rendern::GameplayTransformComponent> transforms;
        std::unordered_set<EntityHandle> addedAI;
        std::unordered_set<EntityHandle> spawnedEntities;
        std::unordered_map<int, bool> visibilityBaselines;
        bool running{};
    };

    class DevelopmentScenarioOperationExecutor
    {
    public:
        static EntityHandle ResolveEntity(const int nodeIndex, ScenarioContext& context) noexcept
        {
            for (const EntityHandle candidate : context.gameplayRuntime.GetNodeBoundEntities())
                if (const auto* link = context.gameplayRuntime.GetWorld().TryGetNodeLink(candidate);
                    context.gameplayRuntime.GetWorld().IsEntityValid(candidate) &&
                    link && link->nodeIndex == nodeIndex) return candidate;
            return rendern::kNullEntity;
        }

        static bool Execute(const ScenarioOperation& operation, DevelopmentScenarioRunner& runner, ScenarioContext& context)
        {
            auto& instance = *runner.impl_;
            return std::visit([&](const auto& op) -> bool {
                const auto nodeIt = instance.nodes.find(op.entity);
                if (nodeIt == instance.nodes.end()) return false;
                auto& world = context.gameplayRuntime.GetWorld();
                using T = std::decay_t<decltype(op)>;
                if constexpr (std::is_same_v<T, SetRuntimeVisibilityOperation>)
                {
                    instance.visibilityBaselines.try_emplace(
                        nodeIt->second, context.levelInstance.IsNodeRuntimeVisible(nodeIt->second));
                    return context.levelInstance.SetNodeRuntimeVisible(context.level, context.scene, nodeIt->second, op.visible);
                }
                else if constexpr (std::is_same_v<T, EnsureNodeBoundEntityOperation>)
                {
                    if (ResolveEntity(nodeIt->second, context) != rendern::kNullEntity) return true;
                    rendern::GameplayUpdateContext gameplayContext{};
                    gameplayContext.mode = context.gameplayMode;
                    gameplayContext.levelAsset = &context.level;
                    gameplayContext.levelInstance = &context.levelInstance;
                    gameplayContext.scene = &context.scene;
                    const EntityHandle spawned = context.gameplayRuntime.SpawnNodeBoundEntity(
                        gameplayContext, nodeIt->second, false);
                    if (spawned == rendern::kNullEntity) return false;
                    instance.spawnedEntities.insert(spawned);
                    return true;
                }
                else
                {
                    const EntityHandle entity = ResolveEntity(nodeIt->second, context);
                    if (entity == rendern::kNullEntity) return false;
                    if constexpr (std::is_same_v<T, CaptureTransformOperation>)
                    {
                        const auto* transform = world.TryGetTransform(entity); if (!transform) return false;
                        instance.transforms[op.slot] = *transform; return true;
                    }
                    else if constexpr (std::is_same_v<T, RestoreTransformOperation>)
                    {
                        const auto baseline = instance.transforms.find(op.slot); if (baseline == instance.transforms.end() || !world.HasTransform(entity)) return false;
                        world.SetTransform(entity, baseline->second); return true;
                    }
                    else if constexpr (std::is_same_v<T, EnsureAIOperation>)
                    {
                        if (!world.HasAI(entity)) { world.AddAI(entity); instance.addedAI.insert(entity); }
                        return true;
                    }
                    else if constexpr (std::is_same_v<T, CancelAIOperation>) { context.gameplayRuntime.CancelAIAction(entity); return true; }
                    else if constexpr (std::is_same_v<T, TeleportPhysicsCharacterOperation>)
                        return context.physicsWorld && appRuntime::TeleportGameplayPhysicsCharacterToGameplayTransform(context.gameplayRuntime, *context.physicsWorld, entity);
                }
            }, operation);
        }

        static bool ExecuteAll(const std::vector<ScenarioOperation>& operations,
            DevelopmentScenarioRunner& runner, ScenarioContext& context)
        { for (const auto& operation : operations) if (!Execute(operation, runner, context)) return false; return true; }
    };

    DevelopmentScenarioRunner::DevelopmentScenarioRunner() : impl_(std::make_unique<Impl>()) {}
    DevelopmentScenarioRunner::~DevelopmentScenarioRunner() = default;
    DevelopmentScenarioRunner::DevelopmentScenarioRunner(DevelopmentScenarioRunner&&) noexcept = default;
    DevelopmentScenarioRunner& DevelopmentScenarioRunner::operator=(DevelopmentScenarioRunner&&) noexcept = default;

    bool DevelopmentScenarioRunner::Load(const DevelopmentScenarioAsset& asset, ScenarioContext& context)
    {
        Unload(context);
        
        // A failed physics teardown deliberately keeps the previous scenario
        // loaded so ownership of its spawned entities can be retried safely.
        if (impl_->asset.has_value())
        {
            return false;
        }
        
        ValidateDevelopmentScenarioAsset(asset);
        impl_->asset = asset;
        for (const auto& [role, nodeName] : asset.roles)
        {
            const auto node = std::ranges::find_if(context.level.nodes, [&](const auto& value) { return value.alive && value.name == nodeName; });
            if (node == context.level.nodes.end()) { Unload(context); return false; }
            const int index = static_cast<int>(std::distance(context.level.nodes.begin(), node)); impl_->nodes.emplace(role, index);
        }
        if (!DevelopmentScenarioOperationExecutor::ExecuteAll(impl_->asset->setup, *this, context))
        {
            (void)DevelopmentScenarioOperationExecutor::ExecuteAll(impl_->asset->stop, *this, context);
            Unload(context); return false;
        }
        return true;
    }
    void DevelopmentScenarioRunner::Unload(ScenarioContext& context) noexcept
    {
        if (!impl_->asset) return;
        if (impl_->running) Stop(context);
        auto& world = context.gameplayRuntime.GetWorld();
        for (const EntityHandle entity : impl_->addedAI)
        {
            context.gameplayRuntime.CancelAIAction(entity);
            if (world.IsEntityValid(entity) && world.HasAI(entity)) world.RemoveAI(entity);
        }
        for (const auto& [nodeIndex, visible] : impl_->visibilityBaselines)
            (void)context.levelInstance.SetNodeRuntimeVisible(context.level, context.scene, nodeIndex, visible);
        std::unordered_set<EntityHandle> failedPhysicsTeardown;
        for (const EntityHandle entity : impl_->spawnedEntities)
        {
            if (context.physicsWorld != nullptr &&
                !appRuntime::DestroyGameplayPhysicsCharacter(
                    context.gameplayRuntime, *context.physicsWorld, entity))
            {
                failedPhysicsTeardown.insert(entity);
                continue;
            }
            (void)context.gameplayRuntime.DestroyNodeBoundEntity(entity);
        }
        impl_->addedAI.clear(); impl_->visibilityBaselines.clear(); impl_->running = false;
        if (!failedPhysicsTeardown.empty())
        {
            impl_->spawnedEntities = std::move(failedPhysicsTeardown);
            return;
        }
        impl_->asset.reset(); impl_->nodes.clear(); impl_->transforms.clear(); impl_->addedAI.clear();
        impl_->spawnedEntities.clear(); impl_->visibilityBaselines.clear(); impl_->running = false;
    }
    bool DevelopmentScenarioRunner::Start(ScenarioContext& context)
    {
        if (!impl_->asset || impl_->running) return false;
        if (!DevelopmentScenarioOperationExecutor::ExecuteAll(impl_->asset->start, *this, context))
        {
            (void)DevelopmentScenarioOperationExecutor::ExecuteAll(impl_->asset->stop, *this, context);
            impl_->running = false; return false;
        }
        impl_->running = true; return true;
    }
    void DevelopmentScenarioRunner::Update(ScenarioContext& context) noexcept
    {
        if (impl_->running && !DevelopmentScenarioOperationExecutor::ExecuteAll(impl_->asset->update, *this, context)) Stop(context);
    }
    void DevelopmentScenarioRunner::Stop(ScenarioContext& context) noexcept
    {
        if (impl_->asset && impl_->running) (void)DevelopmentScenarioOperationExecutor::ExecuteAll(impl_->asset->stop, *this, context);
        impl_->running = false;
    }
    void DevelopmentScenarioRunner::Reset(ScenarioContext& context) noexcept
    {
        if (!impl_->asset) return; Stop(context);
        (void)DevelopmentScenarioOperationExecutor::ExecuteAll(impl_->asset->reset, *this, context);
    }
    bool DevelopmentScenarioRunner::IsLoaded() const noexcept { return impl_->asset.has_value(); }
    bool DevelopmentScenarioRunner::IsRunning() const noexcept { return impl_->running; }
    const DevelopmentScenarioAsset* DevelopmentScenarioRunner::GetAsset() const noexcept { return impl_->asset ? &*impl_->asset : nullptr; }
    int DevelopmentScenarioRunner::GetResolvedNodeIndex(const std::string_view role) const noexcept { const auto it = impl_->nodes.find(std::string(role)); return it == impl_->nodes.end() ? -1 : it->second; }
}