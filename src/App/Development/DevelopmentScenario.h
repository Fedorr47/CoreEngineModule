#pragma once

#include <memory>
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

    using ScenarioOperation = std::variant<
        CaptureTransformOperation,
        RestoreTransformOperation,
        EnsureAIOperation,
        CancelAIOperation,
        TeleportPhysicsCharacterOperation,
        SetRuntimeVisibilityOperation,
        EnsureNodeBoundEntityOperation>;

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
        void Update(ScenarioContext& context) noexcept;
        void Stop(ScenarioContext& context) noexcept;
        void Reset(ScenarioContext& context) noexcept;
        [[nodiscard]] bool IsLoaded() const noexcept;
        [[nodiscard]] bool IsRunning() const noexcept;
        [[nodiscard]] const DevelopmentScenarioAsset* GetAsset() const noexcept;
        [[nodiscard]] int GetResolvedNodeIndex(std::string_view role) const noexcept;

    private:
        friend class DevelopmentScenarioOperationExecutor;
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}