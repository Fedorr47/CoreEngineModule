#pragma once

#include <cstdint>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace appDevelopment
{
    struct ScenarioContext;

    struct CaptureTransformOperation { std::string entity; std::string slot; };
    struct RestoreTransformOperation { std::string entity; std::string slot; };
    struct EnsureAIOperation { std::string entity; };
    struct CancelAIOperation { std::string entity; };
    struct TeleportPhysicsCharacterOperation { std::string entity; };
    struct SetRuntimeVisibilityOperation { std::string entity; bool visible{}; };
    struct EnsureNodeBoundEntityOperation { std::string entity; };
    struct RemoveCharacterPhysicalSettingsOperation { std::string entity; };
    struct SetCharacterPhysicalSettingsOperation
    {
        std::string entity;
        float radius{};
        float cylinderHeight{};
        float maximumSlopeAngleDegrees{};
        float maximumStepHeight{};
        float mass{};
    };
    struct ResetEntitySimulationStateOperation { std::string entity; };
    struct RegisterJumpTraversalLinkOperation
    {
        std::string entity; // target entity role
        std::uint64_t handle{};
        std::string takeoff;
        std::string landing;
        float verticalSpeed{};
        float takeoffTolerance{};
        float landingHorizontalTolerance{};
        float landingVerticalTolerance{};
    };
    struct RemoveTraversalLinkOperation { std::string entity; std::uint64_t handle{}; };
    struct RouteSegmentTraversal { std::size_t segment{}; std::uint64_t link{}; };
    struct StartFollowRouteOperation
    {
        std::string entity;
        std::vector<std::string> points;
        std::vector<RouteSegmentTraversal> traversals;
        float acceptanceRadius{};
        float slowingRadius{};
        bool wantsRun{};
    };
    struct MoveToEdge
    {
        std::string from;
        std::string to;
        float cost{};
    };
    struct StartMoveToOperation
    {
        std::string entity;
        std::vector<std::string> nodes;
        std::vector<MoveToEdge> edges;
        std::string start;
        std::string goal;
        float acceptanceRadius{};
        float slowingRadius{};
        bool wantsRun{};
    };
    
    struct StartNavigationPathOperation
    {
        std::string entity;
        std::string target;
        std::array<float, 3> searchExtents{};
        float acceptanceRadius{};
        float slowingRadius{};
        bool wantsRun{};
        std::string result;
    };

    enum class ScenarioOperationResultStatus
    {
        NotStarted,
        Running,
        Succeeded,
        NoPath,
        Failed,
        Cancelled
    };

    struct ScenarioOperationResult
    {
        std::string name;
        ScenarioOperationResultStatus status{ScenarioOperationResultStatus::NotStarted};
    };

    [[nodiscard]] const char* ToString(ScenarioOperationResultStatus status) noexcept;

    using ScenarioOperation = std::variant<
        CaptureTransformOperation,
        RestoreTransformOperation,
        EnsureAIOperation,
        CancelAIOperation,
        TeleportPhysicsCharacterOperation,
        SetRuntimeVisibilityOperation,
        EnsureNodeBoundEntityOperation,
        RemoveCharacterPhysicalSettingsOperation,
        SetCharacterPhysicalSettingsOperation,
        ResetEntitySimulationStateOperation,
        RegisterJumpTraversalLinkOperation,
        RemoveTraversalLinkOperation,
        StartFollowRouteOperation,
        StartMoveToOperation,
        StartNavigationPathOperation>;
    
    struct DevelopmentScenarioAsset
    {
        std::string id;
        std::string title;
        std::string description;
        // Authored roles are deliberately ordered for deterministic validation/resolution.
        std::vector<std::pair<std::string, std::string>> roles;
        std::vector<ScenarioOperation> setup;
        std::vector<ScenarioOperation> start;
        std::vector<ScenarioOperation> update;
        std::vector<ScenarioOperation> stop;
        std::vector<ScenarioOperation> reset;
        std::string sourcePath;
    };

    [[nodiscard]] DevelopmentScenarioAsset LoadDevelopmentScenarioAsset(
        std::string_view assetRelativeOrAbsolutePath);
    [[nodiscard]] DevelopmentScenarioAsset ParseDevelopmentScenarioAsset(
        std::string_view json, std::string_view sourcePath = {});
    void ValidateDevelopmentScenarioAsset(const DevelopmentScenarioAsset& asset);

    class DevelopmentScenarioOperationExecutor;

    class DevelopmentScenarioRunner
    {
    public:
        DevelopmentScenarioRunner();
        ~DevelopmentScenarioRunner();
        DevelopmentScenarioRunner(DevelopmentScenarioRunner&&) noexcept;
        DevelopmentScenarioRunner& operator=(DevelopmentScenarioRunner&&) noexcept;
        DevelopmentScenarioRunner(const DevelopmentScenarioRunner&) = delete;
        DevelopmentScenarioRunner& operator=(const DevelopmentScenarioRunner&) = delete;

        bool Load(const DevelopmentScenarioAsset& asset, ScenarioContext& context);
        void Unload(ScenarioContext& context) noexcept;
        bool Start(ScenarioContext& context);
        [[nodiscard]] bool CanStart(const ScenarioContext& context) const noexcept;
        void Update(ScenarioContext& context) noexcept;
        void Stop(ScenarioContext& context) noexcept;
        void Reset(ScenarioContext& context) noexcept;
        [[nodiscard]] bool IsLoaded() const noexcept;
        [[nodiscard]] bool IsRunning() const noexcept;
        [[nodiscard]] const DevelopmentScenarioAsset* GetAsset() const noexcept;
        [[nodiscard]] int GetResolvedNodeIndex(std::string_view role) const noexcept;
        [[nodiscard]] const std::vector<ScenarioOperationResult>& GetResults() const noexcept;
        
    private:
        friend class DevelopmentScenarioOperationExecutor;
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}