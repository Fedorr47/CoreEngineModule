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
        
        double RequiredNumber(const JsonObject& object, const std::string_view key,
            const std::string_view source, const std::string_view section, const std::size_t index)
        {
            const auto it = object.find(std::string(key));
            if (it == object.end() || !it->second.IsNumber())
                Invalid(source, section, index, "missing numeric field '" + std::string(key) + "'");
            return it->second.AsNumber();
        }

        template<typename Integer>
        Integer RequiredInteger(const JsonObject& object, const std::string_view key,
            const std::string_view source, const std::string_view section, const std::size_t index)
        {
            const double value = RequiredNumber(object, key, source, section, index);
            if (!std::isfinite(value) || value < 0.0 || std::trunc(value) != value ||
                static_cast<long double>(value) > static_cast<long double>(std::numeric_limits<Integer>::max()))
                Invalid(source, section, index, "field '" + std::string(key) + "' must be a non-negative in-range integer");
            return static_cast<Integer>(value);
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
                else if (op == "removeCharacterPhysicalSettings")
                    result.emplace_back(RemoveCharacterPhysicalSettingsOperation{entity()});
                else if (op == "resetEntitySimulationState")
                    result.emplace_back(ResetEntitySimulationStateOperation{entity()});
                else if (op == "removeTraversalLink")
                    result.emplace_back(RemoveTraversalLinkOperation{entity(),
                        RequiredInteger<std::uint64_t>(operation, "handle", source, section, i)});
                else if (op == "registerJumpTraversalLink")
                {
                    result.emplace_back(RegisterJumpTraversalLinkOperation{
                        entity(), RequiredInteger<std::uint64_t>(operation, "handle", source, section, i),
                        RequiredString(operation, "takeoff", source, section, i),
                        RequiredString(operation, "landing", source, section, i),
                        static_cast<float>(RequiredNumber(operation, "verticalSpeed", source, section, i)),
                        static_cast<float>(RequiredNumber(operation, "takeoffTolerance", source, section, i)),
                        static_cast<float>(RequiredNumber(operation, "landingHorizontalTolerance", source, section, i)),
                        static_cast<float>(RequiredNumber(operation, "landingVerticalTolerance", source, section, i))});
                }
                else if (op == "startFollowRoute")
                {
                    StartFollowRouteOperation route{};
                    route.entity = entity();
                    const auto points = operation.find("points");
                    if (points == operation.end() || !points->second.IsArray()) Invalid(source, section, i, "missing array field 'points'");
                    for (const auto& point : points->second.AsArray())
                    {
                        if (!point.IsString() || point.AsString().empty()) Invalid(source, section, i, "route point must be a role name");
                        route.points.push_back(point.AsString());
                    }
                    const auto traversals = operation.find("segmentTraversals");
                    if (traversals != operation.end())
                    {
                        if (!traversals->second.IsArray()) Invalid(source, section, i, "segmentTraversals must be an array");
                        for (const auto& value : traversals->second.AsArray())
                        {
                            if (!value.IsObject()) Invalid(source, section, i, "segment traversal must be an object");
                            route.traversals.push_back({
                                RequiredInteger<std::size_t>(value.AsObject(), "segment", source, section, i),
                                RequiredInteger<std::uint64_t>(value.AsObject(), "link", source, section, i)});
                        }
                    }
                    route.acceptanceRadius = static_cast<float>(RequiredNumber(operation, "acceptanceRadius", source, section, i));
                    route.slowingRadius = static_cast<float>(RequiredNumber(operation, "slowingRadius", source, section, i));
                    const auto wantsRun = operation.find("wantsRun");
                    if (wantsRun == operation.end() || !wantsRun->second.IsBool()) Invalid(source, section, i, "missing boolean field 'wantsRun'");
                    route.wantsRun = wantsRun->second.AsBool();
                    result.emplace_back(std::move(route));
                }
                else Invalid(source, section, i, "unknown operation type '" + op + "'");
            }
            return result;
        }

        std::string OperationRole(const ScenarioOperation& operation)
        {
            return std::visit([](const auto& value) { return value.entity; }, operation);
        }
            
        std::vector<std::string> AdditionalOperationRoles(const ScenarioOperation& operation)
        {
            if (const auto* link = std::get_if<RegisterJumpTraversalLinkOperation>(&operation))
                return {link->takeoff, link->landing};
            if (const auto* route = std::get_if<StartFollowRouteOperation>(&operation)) return route->points;
            return {};
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
                for (const auto& role : AdditionalOperationRoles(operation))
                    if (!roles.contains(role)) Invalid(identity, name, i, "operation references unknown role '" + role + "'");
                if (const auto* link = std::get_if<RegisterJumpTraversalLinkOperation>(&operation);
                    link && (!rendern::GameplayTraversalLinkHandle{link->handle}.IsValid() ||
                    !std::isfinite(link->verticalSpeed) || link->verticalSpeed <= 0.0f ||
                    !std::isfinite(link->takeoffTolerance) || link->takeoffTolerance <= 0.0f ||
                    !std::isfinite(link->landingHorizontalTolerance) || link->landingHorizontalTolerance <= 0.0f ||
                    !std::isfinite(link->landingVerticalTolerance) || link->landingVerticalTolerance <= 0.0f))
                    Invalid(identity, name, i, "invalid Jump traversal link parameters");
                if (const auto* route = std::get_if<StartFollowRouteOperation>(&operation))
                {
                    if (route->points.size() < 2 || !std::isfinite(route->acceptanceRadius) ||
                        !std::isfinite(route->slowingRadius) || route->acceptanceRadius < 0.0f ||
                        route->slowingRadius < route->acceptanceRadius)
                        Invalid(identity, name, i, "invalid FollowRoute parameters");
                    for (const auto& traversal : route->traversals)
                        if (!rendern::GameplayTraversalLinkHandle{traversal.link}.IsValid() ||
                            traversal.segment >= route->points.size() - 1) Invalid(identity, name, i, "invalid route segment traversal");
                }
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
        std::unordered_set<rendern::GameplayTraversalLinkHandle,
            rendern::GameplayTraversalLinkHandleHasher> traversalLinks;
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
                    else if constexpr (std::is_same_v<T, RemoveCharacterPhysicalSettingsOperation>)
                    {
                        if (instance.spawnedEntities.contains(entity)) world.RemoveCharacterPhysicalSettings(entity);
                        return true;
                    }
                    else if constexpr (std::is_same_v<T, ResetEntitySimulationStateOperation>)
                    {
                        context.gameplayRuntime.ResetNodeBoundEntitySimulationState(entity);
                        return true;
                    }
                    else if constexpr (std::is_same_v<T, RemoveTraversalLinkOperation>)
                    {
                        const rendern::GameplayTraversalLinkHandle handle{op.handle};
                        if (!instance.traversalLinks.contains(handle)) return true;
                        (void)context.gameplayRuntime.RemoveGameplayTraversalLink(handle);
                        instance.traversalLinks.erase(handle);
                        return true;
                    }
                    else if constexpr (std::is_same_v<T, RegisterJumpTraversalLinkOperation>)
                    {
                        const auto takeoff = instance.nodes.find(op.takeoff);
                        const auto landing = instance.nodes.find(op.landing);
                        if (takeoff == instance.nodes.end() || landing == instance.nodes.end()) return false;
                        const rendern::GameplayTraversalLinkHandle handle{op.handle};
                        const rendern::GameplayTraversalLink link{
                            .handle = handle,
                            .traversalTypeId = rendern::kJumpTraversalTypeId,
                            .targetEntity = entity,
                            .jump = {
                                .takeoffPosition = context.level.nodes[static_cast<std::size_t>(takeoff->second)].transform.position,
                                .landingPosition = context.level.nodes[static_cast<std::size_t>(landing->second)].transform.position,
                                .verticalSpeed = op.verticalSpeed,
                                .takeoffTolerance = op.takeoffTolerance,
                                .landingHorizontalTolerance = op.landingHorizontalTolerance,
                                .landingVerticalTolerance = op.landingVerticalTolerance}};
                        if (!context.gameplayRuntime.RegisterGameplayTraversalLink(link)) return false;
                        instance.traversalLinks.insert(handle);
                        return true;
                    }
                    else if constexpr (std::is_same_v<T, StartFollowRouteOperation>)
                    {
                        rendern::GameplayRoute route{};
                        route.points.reserve(op.points.size());
                        for (const auto& role : op.points)
                        {
                            const auto point = instance.nodes.find(role);
                            if (point == instance.nodes.end()) return false;
                            route.points.push_back({context.level.nodes[static_cast<std::size_t>(point->second)].transform.position});
                        }
                        route.segmentAnnotations.resize(route.points.size() - 1);
                        for (const auto& traversal : op.traversals)
                            route.segmentAnnotations[traversal.segment].traversalLink =
                                rendern::GameplayTraversalLinkHandle{traversal.link};
                        rendern::GameplayArrivalSteeringSettings steering{};
                        steering.acceptanceRadius = op.acceptanceRadius;
                        steering.slowingRadius = op.slowingRadius;
                        steering.wantsRun = op.wantsRun;
                        return context.gameplayRuntime.StartAIFollowRoute(entity, std::move(route), steering) ==
                            rendern::AIActionExecutionStatus::Running;
                    }
                    else if constexpr (std::is_same_v<T, TeleportPhysicsCharacterOperation>)
                        return context.physicsWorld == nullptr ||
                           appRuntime::TeleportGameplayPhysicsCharacterToGameplayTransform(
                               context.gameplayRuntime, *context.physicsWorld, entity);
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
        for (const auto handle : impl_->traversalLinks)
            (void)context.gameplayRuntime.RemoveGameplayTraversalLink(handle);
        impl_->traversalLinks.clear();
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