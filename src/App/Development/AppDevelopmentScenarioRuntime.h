#pragma once

namespace physics
{
    class JoltPhysicsWorld;
}

namespace navigation
{
    class ProfileRegistry;
}

namespace rendern
{
    class GameplayRuntime;
    struct LevelAsset;
    class LevelInstance;
    class Scene;
    enum class GameplayRuntimeMode : unsigned char;
}

namespace appDevelopment
{
    enum class ScenarioKind
    {
        None,
        DataDriven,
        AIGOAPAccessKey
    };

    enum class ScenarioCommand
    {
        Start,
        Reset,
        Stop
    };

    struct ScenarioContext
    {
        rendern::GameplayRuntime& gameplayRuntime;
        rendern::LevelAsset& level;
        rendern::LevelInstance& levelInstance;
        rendern::Scene& scene;
        rendern::GameplayRuntimeMode gameplayMode;

        physics::JoltPhysicsWorld* physicsWorld{};
        navigation::ProfileRegistry* navigationProfiles{};
    };

    struct ScenarioStatusRow
    {
        const char* label{""};
        const char* value{""};
    };

    struct ScenarioView
    {
        bool active{};

        const char* title{""};
        const char* description{""};

        const char* startLabel{"Start"};
        const char* resetLabel{"Reset"};
        const char* stopLabel{"Stop"};

        ScenarioStatusRow statuses[4]{};
        unsigned int statusCount{};

        bool canStart{};
        bool canReset{};
        bool canStop{};
        bool commandsEnabled{};
    };

    class AppDevelopmentScenarioRuntime
    {
    public:
        AppDevelopmentScenarioRuntime();
        ~AppDevelopmentScenarioRuntime();

        AppDevelopmentScenarioRuntime(
            const AppDevelopmentScenarioRuntime&) = delete;

        AppDevelopmentScenarioRuntime& operator=(
            const AppDevelopmentScenarioRuntime&) = delete;

        void Reset() noexcept;
        void Reset(ScenarioContext& context) noexcept;
        void OnLevelLoaded(ScenarioContext& context);
        void Update(ScenarioContext& context) noexcept;

        void Execute(
            ScenarioCommand command,
            ScenarioContext& context);

        [[nodiscard]]
        ScenarioKind GetActiveKind() const noexcept;

        [[nodiscard]]
        ScenarioView GetView(
            const ScenarioContext& context) const noexcept;

    private:
        struct Impl;
        Impl* impl_{};
    };
}