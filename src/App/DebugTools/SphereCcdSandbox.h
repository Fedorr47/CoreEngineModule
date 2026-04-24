#pragma once

#include <string>
#include <vector>

namespace appLifecycle
{
    struct AppState;
}

namespace appDebugTools
{
    struct SphereCcdScenario
    {
        std::string name{};
        std::string expectationLabel{};
        geometry::MovingSphereCcdInput input{};
        bool expectedHit{ false };
    };

    struct SphereCcdSandboxState
    {
        bool enabled{ false };
        bool autoplay{ false };
        std::size_t scenarioIndex{ 0 };
        float playbackSeconds{ 0.0f };
        float autoplaySeconds{ 0.0f };
        std::string activeLevelName{};
        std::vector<SphereCcdScenario> scenarios{};
        geometry::MovingSphereCcdResult lastResult{};
        bool lastPass{ false };
        std::string lastScenarioName{};
        std::string lastExpectation{};
    };

    void ResetSphereCcdSandbox(SphereCcdSandboxState& state);
    void UpdateSphereCcdSandbox(appLifecycle::AppState& app, float deltaSeconds);
    void StepSphereCcdSandboxScenario(appLifecycle::AppState& app);
    void ToggleSphereCcdSandboxAutoplay(appLifecycle::AppState& app);
    void DrawSphereCcdSandboxUi(SphereCcdSandboxState& state);
}
