#include "DevelopmentScenario.h"
#include "AppDevelopmentScenarioRuntime.h"
#include "Physics/Jolt/JoltPhysicsWorld.h"

import core;

#include "App/GameplayPhysicsCharacterIntegration.h"

namespace appDevelopment
{
    const char* ToString(const ScenarioOperationResultStatus status) noexcept
    {
        switch (status)
        {
        case ScenarioOperationResultStatus::Running: return "Running";
        case ScenarioOperationResultStatus::Succeeded: return "Succeeded";
        case ScenarioOperationResultStatus::NoPath: return "NoPath";
        case ScenarioOperationResultStatus::Failed: return "Failed";
        case ScenarioOperationResultStatus::Cancelled: return "Cancelled";
        default: return "NotStarted";
        }
    }
    
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
            // Scenario JSON numbers are stored as double. Restrict authored
            // integer fields to the range where every integer is represented
            // exactly, otherwise distinct handles may collapse to one value.
            constexpr double MaxExactJsonInteger = 9007199254740991.0; // 2^53 - 1
            const double typeMax =
                static_cast<double>(std::numeric_limits<Integer>::max());
            const double safeMax =
                std::min(typeMax, MaxExactJsonInteger);
            
            if (!std::isfinite(value) ||
                value < 0.0 ||
                std::trunc(value) != value ||
                value > safeMax)
            {
                Invalid(
                    source,
                    section,
                    index,
                    "field '" + std::string(key) +
                    "' must be an exactly representable non-negative integer");
            }
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
                const auto whenPickupCollected = [&]() -> std::string
                {
                    const auto condition = operation.find("whenPickupCollected");
                    if (condition == operation.end())
                    {
                        return {};
                    }
                    if (!condition->second.IsString() || condition->second.AsString().empty())
                    {
                        Invalid(source, section, i,
                            "field 'whenPickupCollected' must be a non-empty string");
                    }
                    return condition->second.AsString();
                };
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
                    result.emplace_back(SetRuntimeVisibilityOperation{
                        entity(), visible->second.AsBool(), whenPickupCollected()});
                }
                else if (op == "ensureNodeBoundEntity") result.emplace_back(EnsureNodeBoundEntityOperation{entity()});
                else if (op == "ensurePickup") result.emplace_back(EnsurePickupOperation{entity(),
                    static_cast<float>(RequiredNumber(operation, "collectionRadius", source, section, i))});
                else if (op == "ensureInteractionPoint")
                    result.emplace_back(EnsureInteractionPointOperation{entity()});
                else if (op == "resetPickup") result.emplace_back(ResetPickupOperation{entity()});
                else if (op == "setPickupCollected")
                {
                    const auto collected = operation.find("collected");
                    if (collected == operation.end() || !collected->second.IsBool())
                    {
                        Invalid(source, section, i, "missing boolean field 'collected'");
                    }
                    result.emplace_back(SetPickupCollectedOperation{
                        entity(), collected->second.AsBool(), whenPickupCollected()});
                }
                else if (op == "removeCharacterPhysicalSettings")
                    result.emplace_back(RemoveCharacterPhysicalSettingsOperation{entity()});
                else if (op == "setCharacterPhysicalSettings")
                {
                    result.emplace_back(SetCharacterPhysicalSettingsOperation{
                        entity(),
                        static_cast<float>(RequiredNumber(operation, "radius", source, section, i)),
                        static_cast<float>(RequiredNumber(operation, "cylinderHeight", source, section, i)),
                        static_cast<float>(RequiredNumber(operation, "maximumSlopeAngleDegrees", source, section, i)),
                        static_cast<float>(RequiredNumber(operation, "maximumStepHeight", source, section, i)),
                        static_cast<float>(RequiredNumber(operation, "mass", source, section, i))});
                }
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
                else if (op == "startMoveTo")
                {
                    StartMoveToOperation move{};
                    move.entity = entity();
                    const auto nodes = operation.find("nodes");
                    if (nodes == operation.end() || !nodes->second.IsArray()) Invalid(source, section, i, "missing array field 'nodes'");
                    for (const auto& node : nodes->second.AsArray())
                    {
                        if (!node.IsString() || node.AsString().empty()) Invalid(source, section, i, "graph node must be a role name");
                        move.nodes.push_back(node.AsString());
                    }
                    const auto edges = operation.find("edges");
                    if (edges == operation.end() || !edges->second.IsArray()) Invalid(source, section, i, "missing array field 'edges'");
                    for (const auto& edge : edges->second.AsArray())
                    {
                        if (!edge.IsObject()) Invalid(source, section, i, "graph edge must be an object");
                        move.edges.push_back({
                            RequiredString(edge.AsObject(), "from", source, section, i),
                            RequiredString(edge.AsObject(), "to", source, section, i),
                            static_cast<float>(RequiredNumber(edge.AsObject(), "cost", source, section, i))});
                    }
                    move.start = RequiredString(operation, "start", source, section, i);
                    move.goal = RequiredString(operation, "goal", source, section, i);
                    move.acceptanceRadius = static_cast<float>(RequiredNumber(operation, "acceptanceRadius", source, section, i));
                    move.slowingRadius = static_cast<float>(RequiredNumber(operation, "slowingRadius", source, section, i));
                    const auto wantsRun = operation.find("wantsRun");
                    if (wantsRun == operation.end() || !wantsRun->second.IsBool()) Invalid(source, section, i, "missing boolean field 'wantsRun'");
                    move.wantsRun = wantsRun->second.AsBool();
                    result.emplace_back(std::move(move));
                }
                else if (op == "startFollowTarget")
                {
                    const auto wantsRun = operation.find("wantsRun");
                    if (wantsRun == operation.end() || !wantsRun->second.IsBool())
                        Invalid(source, section, i, "missing boolean field 'wantsRun'");
                    result.emplace_back(StartFollowTargetOperation{entity(),
                        RequiredString(operation, "target", source, section, i),
                        static_cast<float>(RequiredNumber(operation, "acceptanceRadius", source, section, i)),
                        wantsRun->second.AsBool()});
                }
                else if (op == "startFleeTarget")
                {
                    const auto wantsRun = operation.find("wantsRun");
                    if (wantsRun == operation.end() || !wantsRun->second.IsBool())
                        Invalid(source, section, i, "missing boolean field 'wantsRun'");
                    result.emplace_back(StartFleeTargetOperation{entity(),
                        RequiredString(operation, "target", source, section, i),
                        static_cast<float>(RequiredNumber(operation, "triggerRadius", source, section, i)),
                        static_cast<float>(RequiredNumber(operation, "safeRadius", source, section, i)),
                        wantsRun->second.AsBool()});
                }
                else if (op == "startNavigationPath")
                {
                    StartNavigationPathOperation path{};
                    path.entity = entity();
                    path.target = RequiredString(operation, "target", source, section, i);
                    const auto extents = operation.find("searchExtents");
                    if (extents == operation.end() || !extents->second.IsArray() ||
                        extents->second.AsArray().size() != 3)
                    {
                        Invalid(source, section, i, "field 'searchExtents' must be a three-number array");
                    }
                    for (std::size_t component = 0; component < 3; ++component)
                    {
                        const auto& value = extents->second.AsArray()[component];
                        if (!value.IsNumber())
                        {
                            Invalid(source, section, i, "field 'searchExtents' must be a three-number array");
                        }
                        path.searchExtents[component] = static_cast<float>(value.AsNumber());
                    }
                    path.acceptanceRadius = static_cast<float>(RequiredNumber(operation, "acceptanceRadius", source, section, i));
                    path.slowingRadius = static_cast<float>(RequiredNumber(operation, "slowingRadius", source, section, i));
                    const auto wantsRun = operation.find("wantsRun");
                    if (wantsRun == operation.end() || !wantsRun->second.IsBool())
                    {
                        Invalid(source, section, i, "missing boolean field 'wantsRun'");
                    }
                    path.wantsRun = wantsRun->second.AsBool();
                    path.result = RequiredString(operation, "result", source, section, i);
                    result.emplace_back(std::move(path));
                }
                else if (op == "startAIDecision")
                {
                    result.emplace_back(StartAIDecisionOperation{entity(),
                        RequiredString(operation, "decision", source, section, i),
                        RequiredString(operation, "result", source, section, i)});
                }
                else if (op == "cancelAIDecision")
                {
                    result.emplace_back(CancelAIDecisionOperation{entity()});
                }
                else Invalid(source, section, i, "unknown operation type '" + op + "'");
            }
            return result;
        }

        std::string OperationRole(const ScenarioOperation& operation)
        {
            return std::visit([](const auto& value) { return value.entity; }, operation);
        }
        
        std::string_view OperationName(const ScenarioOperation& operation) noexcept
        {
            return std::visit([](const auto& value) -> std::string_view
            {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, CaptureTransformOperation>) { return "captureTransform"; }
                else if constexpr (std::is_same_v<T, RestoreTransformOperation>) { return "restoreTransform"; }
                else if constexpr (std::is_same_v<T, EnsureAIOperation>) { return "ensureAI"; }
                else if constexpr (std::is_same_v<T, CancelAIOperation>) { return "cancelAI"; }
                else if constexpr (std::is_same_v<T, TeleportPhysicsCharacterOperation>) { return "teleportPhysicsCharacter"; }
                else if constexpr (std::is_same_v<T, SetRuntimeVisibilityOperation>) { return "setRuntimeVisibility"; }
                else if constexpr (std::is_same_v<T, EnsureNodeBoundEntityOperation>) { return "ensureNodeBoundEntity"; }
                else if constexpr (std::is_same_v<T, EnsurePickupOperation>) { return "ensurePickup"; }
                else if constexpr (std::is_same_v<T, EnsureInteractionPointOperation>) { return "ensureInteractionPoint"; }
                else if constexpr (std::is_same_v<T, ResetPickupOperation>) { return "resetPickup"; }
                else if constexpr (std::is_same_v<T, SetPickupCollectedOperation>) { return "setPickupCollected"; }
                else if constexpr (std::is_same_v<T, RemoveCharacterPhysicalSettingsOperation>) { return "removeCharacterPhysicalSettings"; }
                else if constexpr (std::is_same_v<T, SetCharacterPhysicalSettingsOperation>) { return "setCharacterPhysicalSettings"; }
                else if constexpr (std::is_same_v<T, ResetEntitySimulationStateOperation>) { return "resetEntitySimulationState"; }
                else if constexpr (std::is_same_v<T, RegisterJumpTraversalLinkOperation>) { return "registerJumpTraversalLink"; }
                else if constexpr (std::is_same_v<T, RemoveTraversalLinkOperation>) { return "removeTraversalLink"; }
                else if constexpr (std::is_same_v<T, StartFollowRouteOperation>) { return "startFollowRoute"; }
                else if constexpr (std::is_same_v<T, StartMoveToOperation>) { return "startMoveTo"; }
                else if constexpr (std::is_same_v<T, StartFollowTargetOperation>) { return "startFollowTarget"; }
                else if constexpr (std::is_same_v<T, StartFleeTargetOperation>) { return "startFleeTarget"; }
                else if constexpr (std::is_same_v<T, StartNavigationPathOperation>) { return "startNavigationPath"; }
                else if constexpr (std::is_same_v<T, StartAIDecisionOperation>) { return "startAIDecision"; }
                else if constexpr (std::is_same_v<T, CancelAIDecisionOperation>) { return "cancelAIDecision"; }
                else
                {
                    static_assert(sizeof(T) == 0, "Unhandled DevelopmentScenario operation");
                }
                }, operation);
        }
            
        std::vector<std::string> AdditionalOperationRoles(const ScenarioOperation& operation)
        {
            if (const auto* visibility = std::get_if<SetRuntimeVisibilityOperation>(&operation);
                visibility != nullptr && !visibility->whenPickupCollected.empty())
            {
                return {visibility->whenPickupCollected};
            }
            if (const auto* pickup = std::get_if<SetPickupCollectedOperation>(&operation);
                pickup != nullptr && !pickup->whenPickupCollected.empty())
            {
                return {pickup->whenPickupCollected};
            }
            if (const auto* link = std::get_if<RegisterJumpTraversalLinkOperation>(&operation))
                return {link->takeoff, link->landing};
            if (const auto* route = std::get_if<StartFollowRouteOperation>(&operation)) return route->points;
            if (const auto* move = std::get_if<StartMoveToOperation>(&operation))
            {
                std::vector<std::string> result = move->nodes;
                result.push_back(move->start);
                result.push_back(move->goal);
                for (const auto& edge : move->edges) { result.push_back(edge.from); result.push_back(edge.to); }
                return result;
            }
            if (const auto* follow = std::get_if<StartFollowTargetOperation>(&operation)) return {follow->target};
            if (const auto* flee = std::get_if<StartFleeTargetOperation>(&operation)) return {flee->target};
            if (const auto* path = std::get_if<StartNavigationPathOperation>(&operation))
            {
                return {path->target};
            }
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
        std::unordered_set<std::string> resultNames;
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
                const std::string operationContext = std::string(OperationName(operation)) + ": ";
                if (!roles.contains(OperationRole(operation)))
                {
                    Invalid(identity, name, i, operationContext + "unknown entity role '" + OperationRole(operation) + "'");
                }
                for (const auto& role : AdditionalOperationRoles(operation))
                {
                    if (!roles.contains(role))
                    {
                        Invalid(identity, name, i, operationContext + "unknown entity role '" + role + "'");
                    }
                }
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
                if (const auto* move = std::get_if<StartMoveToOperation>(&operation))
                {
                    std::unordered_set<std::string> graphRoles;
                    for (const auto& role : move->nodes)
                        if (!graphRoles.insert(role).second) Invalid(identity, name, i, "duplicate MoveTo graph node role '" + role + "'");
                    if (move->nodes.empty() || !graphRoles.contains(move->start) || !graphRoles.contains(move->goal) ||
                        !std::isfinite(move->acceptanceRadius) || !std::isfinite(move->slowingRadius) ||
                        move->acceptanceRadius < 0.0f || move->slowingRadius < move->acceptanceRadius)
                        Invalid(identity, name, i, "invalid MoveTo parameters");
                    for (const auto& edge : move->edges)
                        if (!graphRoles.contains(edge.from) || !graphRoles.contains(edge.to) ||
                            !std::isfinite(edge.cost) || edge.cost < 0.0f)
                            Invalid(identity, name, i, "invalid MoveTo graph edge");
                }
                if (const auto* follow = std::get_if<StartFollowTargetOperation>(&operation);
                    follow && (!std::isfinite(follow->acceptanceRadius) || follow->acceptanceRadius < 0.0f))
                    Invalid(identity, name, i, "invalid FollowTarget parameters");
                if (const auto* flee = std::get_if<StartFleeTargetOperation>(&operation);
                    flee && (!std::isfinite(flee->triggerRadius) || !std::isfinite(flee->safeRadius) ||
                    flee->triggerRadius < 0.0f || flee->safeRadius < flee->triggerRadius))
                    Invalid(identity, name, i, "invalid FleeTarget parameters");
                if (const auto* pickup = std::get_if<EnsurePickupOperation>(&operation))
                {
                    if (std::string_view(name) != "setup")
                    {
                        Invalid(identity, name, i, "ensurePickup is only valid in setup");
                    }
                    if (!std::isfinite(pickup->collectionRadius) || pickup->collectionRadius <= 0.0f)
                    {
                        Invalid(identity, name, i,
                            "field 'collectionRadius' must be finite and greater than zero");
                    }
                }
                if (std::holds_alternative<EnsureInteractionPointOperation>(operation) &&
                    std::string_view(name) != "setup")
                {
                    Invalid(identity, name, i, "ensureInteractionPoint is only valid in setup");
                }
                if (const auto* physical = std::get_if<SetCharacterPhysicalSettingsOperation>(&operation))
                {
                    if (std::string_view(name) != "setup")
                    {
                        Invalid(identity, name, i,
                            "setCharacterPhysicalSettings is only valid in setup");
                    }
                    if (!std::isfinite(physical->radius) || physical->radius <= 0.0f)
                    {
                        Invalid(identity, name, i, "field 'radius' must be finite and greater than zero");
                    }
                    if (!std::isfinite(physical->cylinderHeight) || physical->cylinderHeight <= 0.0f)
                    {
                        Invalid(identity, name, i, "field 'cylinderHeight' must be finite and greater than zero");
                    }
                    if (!std::isfinite(physical->maximumSlopeAngleDegrees) ||
                        physical->maximumSlopeAngleDegrees < 0.0f ||
                        physical->maximumSlopeAngleDegrees >= 90.0f)
                    {
                        Invalid(identity, name, i, "field 'maximumSlopeAngleDegrees' must be finite and in [0, 90)");
                    }
                    if (!std::isfinite(physical->maximumStepHeight) || physical->maximumStepHeight < 0.0f ||
                        physical->maximumStepHeight >= physical->cylinderHeight + 2.0f * physical->radius)
                    {
                        Invalid(identity, name, i, "field 'maximumStepHeight' must be finite, non-negative, and less than total height");
                    }
                    if (!std::isfinite(physical->mass) || physical->mass <= 0.0f)
                    {
                        Invalid(identity, name, i, "field 'mass' must be finite and greater than zero");
                    }
                }
                if (const auto* path = std::get_if<StartNavigationPathOperation>(&operation))
                {
                    if (std::string_view(name) != "start")
                    {
                        Invalid(identity, name, i,
                            "startNavigationPath is only valid in start");
                    }
                    if (!std::ranges::all_of(path->searchExtents,
                            [](const float value) { return std::isfinite(value) && value > 0.0f; }))
                    {
                        Invalid(identity, name, i, "field 'searchExtents' components must be finite and greater than zero");
                    }
                    if (!std::isfinite(path->acceptanceRadius) || path->acceptanceRadius < 0.0f)
                    {
                        Invalid(identity, name, i, "field 'acceptanceRadius' must be finite and non-negative");
                    }
                    if (!std::isfinite(path->slowingRadius) || path->slowingRadius < path->acceptanceRadius)
                    {
                        Invalid(identity, name, i, "field 'slowingRadius' must be finite and at least acceptanceRadius");
                    }
                    if (!resultNames.insert(path->result).second)
                    {
                        Invalid(identity, name, i, "duplicate result name '" + path->result + "'");
                    }
                }
                if (const auto* decision = std::get_if<StartAIDecisionOperation>(&operation))
                {
                    if (std::string_view(name) != "start")
                    {
                        Invalid(identity, name, i, "startAIDecision is only valid in start");
                    }
                    if (!resultNames.insert(decision->result).second)
                    {
                        Invalid(identity, name, i, "duplicate result name '" + decision->result + "'");
                    }
                }
                if (std::holds_alternative<CancelAIDecisionOperation>(operation) &&
                    std::string_view(name) != "start" && std::string_view(name) != "stop" &&
                    std::string_view(name) != "reset")
                {
                    Invalid(identity, name, i,
                        "cancelAIDecision is only valid in start, stop, or reset");
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
        std::unordered_map<EntityHandle,
            std::optional<rendern::GameplayCharacterPhysicalSettingsComponent>> physicalBaselines;
        std::unordered_map<EntityHandle,
            std::optional<rendern::GameplayPickupComponent>> pickupBaselines;
        std::unordered_map<EntityHandle,
            std::optional<rendern::GameplayInteractionPointComponent>> interactionPointBaselines;
        std::vector<ScenarioOperationResult> results;
        std::unordered_map<std::string, EntityHandle> resultEntities;
        std::unordered_set<std::string> decisionResults;
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
                const auto conditionSatisfied = [&](const std::string& role) -> std::optional<bool>
                {
                    if (role.empty())
                    {
                        return true;
                    }
                    const auto triggerNode = instance.nodes.find(role);
                    if (triggerNode == instance.nodes.end())
                    {
                        return std::nullopt;
                    }
                    const EntityHandle triggerEntity = ResolveEntity(triggerNode->second, context);
                    if (triggerEntity == rendern::kNullEntity)
                    {
                        return std::nullopt;
                    }
                    const rendern::GameplayPickupComponent* triggerPickup =
                        world.TryGetPickup(triggerEntity);
                    if (triggerPickup == nullptr)
                    {
                        return std::nullopt;
                    }
                    return triggerPickup->collected;
                };
                if constexpr (std::is_same_v<T, SetRuntimeVisibilityOperation>)
                {
                    const std::optional<bool> condition =
                        conditionSatisfied(op.whenPickupCollected);
                    if (!condition.has_value())
                    {
                        return false;
                    }
                    if (!*condition)
                    {
                        return true;
                    }
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
                    if (entity == rendern::kNullEntity)
                    {
                        return false;
                    }
                    
                    if constexpr (std::is_same_v<T, EnsurePickupOperation>)
                    {
                        if (!instance.pickupBaselines.contains(entity))
                        {
                            const auto* existing = world.TryGetPickup(entity);
                            instance.pickupBaselines.emplace(entity, existing != nullptr
                                ? std::optional{*existing} : std::nullopt);
                        }
                        world.SetPickup(entity, {.collectionRadius=op.collectionRadius, .collected=false});
                        return true;
                    }
                    else if constexpr (std::is_same_v<T, EnsureInteractionPointOperation>)
                    {
                        if (!instance.interactionPointBaselines.contains(entity))
                        {
                            const auto* existing = world.TryGetInteractionPoint(entity);
                            instance.interactionPointBaselines.emplace(entity, existing != nullptr
                                ? std::optional{*existing} : std::nullopt);
                        }
                        world.SetInteractionPoint(entity, rendern::GameplayInteractionPointComponent{});
                        return true;
                    }
                    else if constexpr (std::is_same_v<T, ResetPickupOperation>)
                    {
                        if (auto* pickup = world.TryGetPickup(entity)) pickup->collected = false;
                        return world.HasPickup(entity);
                    }
                    else if constexpr (std::is_same_v<T, SetPickupCollectedOperation>)
                    {
                        const std::optional<bool> condition =
                            conditionSatisfied(op.whenPickupCollected);
                        if (!condition.has_value())
                        {
                            return false;
                        }
                        if (!*condition)
                        {
                            return true;
                        }
                        rendern::GameplayPickupComponent* pickup = world.TryGetPickup(entity);
                        if (pickup == nullptr)
                        {
                            return false;
                        }
                        pickup->collected = op.collected;
                        return true;
                    }
                    else if constexpr (std::is_same_v<T, CaptureTransformOperation>)
                    {
                        const auto* transform = world.TryGetTransform(entity);
                        if (!transform)
                        {
                            return false;
                        }

                        instance.transforms[op.slot] = *transform;
                        return true;
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
                    else if constexpr (std::is_same_v<T, CancelAIDecisionOperation>)
                    {
                        context.gameplayRuntime.CancelAIDecision(entity);
                        return true;
                    }
                    else if constexpr (std::is_same_v<T, StartAIDecisionOperation>)
                    {
                        auto result = std::ranges::find(instance.results, op.result,
                            &ScenarioOperationResult::name);
                        if (result == instance.results.end() ||
                            !context.gameplayRuntime.HasAIDecisionDefinition(op.decision) ||
                            !context.gameplayRuntime.StartAIDecision(entity, op.decision))
                        {
                            if (result != instance.results.end())
                            {
                                result->status = ScenarioOperationResultStatus::Failed;
                            }
                            return false;
                        }
                        result->status = ScenarioOperationResultStatus::Running;
                        instance.resultEntities[op.result] = entity;
                        instance.decisionResults.insert(op.result);
                        return true;
                    }
                    else if constexpr (std::is_same_v<T, RemoveCharacterPhysicalSettingsOperation>)
                    {
                        if (instance.spawnedEntities.contains(entity)) world.RemoveCharacterPhysicalSettings(entity);
                        return true;
                    }
                    else if constexpr (std::is_same_v<T, SetCharacterPhysicalSettingsOperation>)
                    {
                        if (!instance.physicalBaselines.contains(entity))
                        {
                            const auto* existing = world.TryGetCharacterPhysicalSettings(entity);
                            instance.physicalBaselines.emplace(entity, existing != nullptr
                                ? std::optional{*existing} : std::nullopt);
                        }
                        world.SetCharacterPhysicalSettings(entity, {
                            .radius = op.radius,
                            .cylinderHeight = op.cylinderHeight,
                            .maximumSlopeAngleDegrees = op.maximumSlopeAngleDegrees,
                            .maximumStepHeight = op.maximumStepHeight,
                            .mass = op.mass});
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
                    else if constexpr (std::is_same_v<T, StartMoveToOperation>)
                    {
                        rendern::GameplayRouteGraph graph{};
                        std::unordered_map<std::string, rendern::GameplayRouteNodeId> ids;
                        graph.nodes.reserve(op.nodes.size());
                        for (std::size_t index = 0; index < op.nodes.size(); ++index)
                        {
                            const auto node = instance.nodes.find(op.nodes[index]);
                            if (node == instance.nodes.end()) return false;
                            const rendern::GameplayRouteNodeId id{
                                static_cast<rendern::GameplayRouteNodeId::ValueType>(index + 1u)};
                            ids.emplace(op.nodes[index], id);
                            graph.nodes.push_back({
                                id, context.level.nodes[static_cast<std::size_t>(node->second)].transform.position});
                        }
                        graph.edges.reserve(op.edges.size());
                        for (const auto& edge : op.edges)
                            graph.edges.push_back({ids.at(edge.from), ids.at(edge.to), edge.cost, {}});
                        if (!graph.IsValid()) return false;
                        rendern::GameplayArrivalSteeringSettings steering{};
                        steering.acceptanceRadius = op.acceptanceRadius;
                        steering.slowingRadius = op.slowingRadius;
                        steering.wantsRun = op.wantsRun;
                        return context.gameplayRuntime.StartAIMoveTo(
                            entity, graph, ids.at(op.start), ids.at(op.goal), steering) ==
                            rendern::AIActionExecutionStatus::Running;
                    }
                    else if constexpr (std::is_same_v<T, StartFollowTargetOperation>)
                    {
                        const auto targetNode = instance.nodes.find(op.target);
                        if (targetNode == instance.nodes.end()) return false;
                        const EntityHandle target = ResolveEntity(targetNode->second, context);
                        if (target == rendern::kNullEntity) return false;
                        rendern::AIFollowTargetSettings settings{};
                        settings.steering.acceptanceRadius = op.acceptanceRadius;
                        settings.steering.wantsRun = op.wantsRun;
                        return context.gameplayRuntime.StartAIFollowTarget(entity, target, settings) ==
                            rendern::AIActionExecutionStatus::Running;
                    }
                    else if constexpr (std::is_same_v<T, StartFleeTargetOperation>)
                    {
                        const auto targetNode = instance.nodes.find(op.target);
                        if (targetNode == instance.nodes.end()) return false;
                        const EntityHandle target = ResolveEntity(targetNode->second, context);
                        if (target == rendern::kNullEntity) return false;
                        rendern::AIFleeTargetSettings settings{};
                        settings.triggerRadius = op.triggerRadius;
                        settings.safeRadius = op.safeRadius;
                        settings.steering.wantsRun = op.wantsRun;
                        return context.gameplayRuntime.StartAIFleeTarget(entity, target, settings) ==
                            rendern::AIActionExecutionStatus::Running;
                    }
                    else if constexpr (std::is_same_v<T, StartNavigationPathOperation>)
                    {
                        auto result = std::ranges::find(instance.results, op.result,
                            &ScenarioOperationResult::name);
                        if (result == instance.results.end() || context.navigationProfiles == nullptr)
                        {
                            return false;
                        }
                        const auto* transform = world.TryGetTransform(entity);
                        const auto* physical = world.TryGetCharacterPhysicalSettings(entity);
                        const auto target = instance.nodes.find(op.target);
                        if (transform == nullptr || physical == nullptr || target == instance.nodes.end())
                        {
                            result->status = ScenarioOperationResultStatus::Failed;
                            return false;
                        }
                        const navigation::ProfileResolution profile = context.navigationProfiles->ResolveProfile(
                            app::navigationRuntime::BuildAgentSettings(*physical));
                        const navigation::World* navigationWorld = context.navigationProfiles->TryGetWorld(profile.profile);
                        if (profile.status != navigation::BuildStatus::Succeeded || navigationWorld == nullptr)
                        {
                            result->status = ScenarioOperationResultStatus::Failed;
                            return false;
                        }
                        const navigation::PathResult path = navigationWorld->FindPath({
                            .start = transform->position,
                            .goal = context.level.nodes[static_cast<std::size_t>(target->second)].transform.position,
                            .searchExtents = {op.searchExtents[0], op.searchExtents[1], op.searchExtents[2]}});
                        if (path.status == navigation::QueryStatus::NoPath)
                        {
                            result->status = ScenarioOperationResultStatus::NoPath;
                            instance.resultEntities.erase(op.result);
                            return true;
                        }
                        if (path.status != navigation::QueryStatus::Succeeded || path.points.size() < 2)
                        {
                            result->status = ScenarioOperationResultStatus::Failed;
                            return false;
                        }
                        rendern::GameplayRoute route{};
                        route.points.reserve(path.points.size());
                        for (const mathUtils::Vec3& point : path.points)
                        {
                            route.points.push_back({.worldPosition = point});
                        }
                        route.segmentAnnotations.resize(route.points.size() - 1);
                        rendern::GameplayArrivalSteeringSettings steering{};
                        steering.acceptanceRadius = op.acceptanceRadius;
                        steering.slowingRadius = op.slowingRadius;
                        steering.wantsRun = op.wantsRun;
                        if (context.gameplayRuntime.StartAIFollowRoute(entity, std::move(route), steering) !=
                            rendern::AIActionExecutionStatus::Running)
                        {
                            result->status = ScenarioOperationResultStatus::Failed;
                            return false;
                        }
                        result->status = ScenarioOperationResultStatus::Running;
                        instance.resultEntities[op.result] = entity;
                        return true;
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
        for (const ScenarioOperation& operation : asset.start)
        {
            if (const auto* path = std::get_if<StartNavigationPathOperation>(&operation))
            {
                impl_->results.push_back({path->result, ScenarioOperationResultStatus::NotStarted});
            }
            if (const auto* decision = std::get_if<StartAIDecisionOperation>(&operation))
            {
                impl_->results.push_back({decision->result, ScenarioOperationResultStatus::NotStarted});
            }
        }
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
        for (const auto& [entity, baseline] : impl_->pickupBaselines)
        {
            if (!world.IsEntityValid(entity) || impl_->spawnedEntities.contains(entity)) continue;
            if (baseline.has_value()) world.SetPickup(entity, *baseline);
            else if (world.HasPickup(entity)) world.RemovePickup(entity);
        }
        for (const auto& [entity, baseline] : impl_->interactionPointBaselines)
        {
            if (!world.IsEntityValid(entity) || impl_->spawnedEntities.contains(entity)) continue;
            if (baseline.has_value()) world.SetInteractionPoint(entity, *baseline);
            else if (world.HasInteractionPoint(entity)) world.RemoveInteractionPoint(entity);
        }
        for (const auto& [entity, baseline] : impl_->physicalBaselines)
        {
            if (!world.IsEntityValid(entity) || impl_->spawnedEntities.contains(entity))
            {
                continue;
            }
            if (baseline.has_value())
            {
                world.SetCharacterPhysicalSettings(entity, *baseline);
            }
            else if (world.HasCharacterPhysicalSettings(entity))
            {
                world.RemoveCharacterPhysicalSettings(entity);
            }
        }
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
        impl_->physicalBaselines.clear(); impl_->pickupBaselines.clear();
        impl_->interactionPointBaselines.clear(); impl_->results.clear(); impl_->resultEntities.clear();
        impl_->decisionResults.clear();
    }
    bool DevelopmentScenarioRunner::CanStart(const ScenarioContext& context) const noexcept
    {
        if (!impl_->asset || impl_->running)
        {
            return false;
        }
        for (const ScenarioOperation& operation : impl_->asset->start)
        {
            if (std::holds_alternative<StartNavigationPathOperation>(operation) &&
                context.navigationProfiles == nullptr)
            {
                return false;
            }
            if (const auto* decision = std::get_if<StartAIDecisionOperation>(&operation);
                decision != nullptr &&
                !context.gameplayRuntime.HasAIDecisionDefinition(decision->decision))
            {
                return false;
            }
        }
        return true;
    }
    bool DevelopmentScenarioRunner::Start(ScenarioContext& context)
    {
        if (!CanStart(context)) return false;
        context.gameplayRuntime.ClearCurrentWorldEvents();
        for (ScenarioOperationResult& result : impl_->results)
        {
            result.status = ScenarioOperationResultStatus::NotStarted;
        }
        impl_->resultEntities.clear();
        impl_->decisionResults.clear();
        if (!DevelopmentScenarioOperationExecutor::ExecuteAll(impl_->asset->start, *this, context))
        {
            (void)DevelopmentScenarioOperationExecutor::ExecuteAll(impl_->asset->stop, *this, context);
            for (ScenarioOperationResult& result : impl_->results)
            {
                if (result.status == ScenarioOperationResultStatus::Running)
                {
                    result.status = ScenarioOperationResultStatus::Cancelled;
                }
            }
            impl_->resultEntities.clear();
            impl_->decisionResults.clear();
            impl_->running = false; return false;
        }
        impl_->running = true; return true;
    }
    void DevelopmentScenarioRunner::Update(ScenarioContext& context) noexcept
    {
        if (impl_->running && !DevelopmentScenarioOperationExecutor::ExecuteAll(impl_->asset->update, *this, context)) Stop(context);
        for (ScenarioOperationResult& result : impl_->results)
        {
            if (result.status != ScenarioOperationResultStatus::Running)
            {
                continue;
            }
            const auto entity = impl_->resultEntities.find(result.name);
            if (entity == impl_->resultEntities.end())
            {
                result.status = ScenarioOperationResultStatus::Failed;
                continue;
            }
            if (impl_->decisionResults.contains(result.name))
            {
                switch (context.gameplayRuntime.GetAIDecisionStatus(entity->second))
                {
                case rendern::AIPlanExecutionStatus::NotStarted:
                    result.status = ScenarioOperationResultStatus::Failed; break;
                case rendern::AIPlanExecutionStatus::ReadyToStartStep: [[fallthrough]];
                case rendern::AIPlanExecutionStatus::RunningStep: break;
                case rendern::AIPlanExecutionStatus::Succeeded:
                    result.status = ScenarioOperationResultStatus::Succeeded; break;
                case rendern::AIPlanExecutionStatus::Cancelled:
                    result.status = ScenarioOperationResultStatus::Cancelled; break;
                case rendern::AIPlanExecutionStatus::Failed:
                    result.status = ScenarioOperationResultStatus::Failed; break;
                }
                continue;
            }
            switch (context.gameplayRuntime.GetAIActionStatus(entity->second))
            {
            case rendern::AIActionExecutionStatus::Running: break;
            case rendern::AIActionExecutionStatus::Succeeded:
                result.status = ScenarioOperationResultStatus::Succeeded; break;
            case rendern::AIActionExecutionStatus::Cancelled:
                result.status = ScenarioOperationResultStatus::Cancelled; break;
            default: result.status = ScenarioOperationResultStatus::Failed; break;
            }
        }
    }
    void DevelopmentScenarioRunner::Stop(ScenarioContext& context) noexcept
    {
        if (impl_->asset && impl_->running) (void)DevelopmentScenarioOperationExecutor::ExecuteAll(impl_->asset->stop, *this, context);
        for (ScenarioOperationResult& result : impl_->results)
        {
            if (result.status == ScenarioOperationResultStatus::Running)
            {
                result.status = ScenarioOperationResultStatus::Cancelled;
            }
        }
        impl_->running = false;
    }
    void DevelopmentScenarioRunner::Reset(ScenarioContext& context) noexcept
    {
        if (!impl_->asset) return; Stop(context);
        (void)DevelopmentScenarioOperationExecutor::ExecuteAll(impl_->asset->reset, *this, context);
        for (ScenarioOperationResult& result : impl_->results)
        {
            result.status = ScenarioOperationResultStatus::NotStarted;
        }
        impl_->resultEntities.clear();
        impl_->decisionResults.clear();
    }
    bool DevelopmentScenarioRunner::IsLoaded() const noexcept { return impl_->asset.has_value(); }
    bool DevelopmentScenarioRunner::IsRunning() const noexcept { return impl_->running; }
    const DevelopmentScenarioAsset* DevelopmentScenarioRunner::GetAsset() const noexcept { return impl_->asset ? &*impl_->asset : nullptr; }
    int DevelopmentScenarioRunner::GetResolvedNodeIndex(const std::string_view role) const noexcept { const auto it = impl_->nodes.find(std::string(role)); return it == impl_->nodes.end() ? -1 : it->second; }
    const std::vector<ScenarioOperationResult>& DevelopmentScenarioRunner::GetResults() const noexcept
    {
        return impl_->results;
    }
}