import core;
import std;

#include "SphereCcdSandbox.h"
#include "../AppLifecycle.h"

#if defined(CORE_USE_DX12)
#include <imgui.h>
#endif

namespace appDebugTools
{
    namespace
    {
        bool IsSandboxLevel(const std::string& levelName)
        {
            return levelName.find("sphere_ccd_sandbox") != std::string::npos;
        }

        std::vector<SphereCcdScenario> BuildDefaultScenarios()
        {
            const float r = 0.5f;
            const float dt = 1.0f / 60.0f;
            return {
                {
                    .name = "Static target direct hit",
                    .expectationLabel = "Direct centerline approach; should collide inside this frame.",
                    .input = geometry::MovingSphereCcdInput{
                        .centerA = mathUtils::Vec3(-3.0f, 0.5f, 0.0f),
                        .centerB = mathUtils::Vec3(0.0f, 0.5f, 0.0f),
                        .velocityA = mathUtils::Vec3(180.0f, 0.0f, 0.0f),
                        .velocityB = mathUtils::Vec3(0.0f, 0.0f, 0.0f),
                        .deltaTime = dt,
                        .radiusA = r,
                        .radiusB = r
                    },
                    .expectedHit = true
                },
                {
                    .name = "Fast projectile tunnel-prevention hit",
                    .expectationLabel = "Projectile crosses target between frame endpoints; CCD should still report hit.",
                    .input = geometry::MovingSphereCcdInput{
                        .centerA = mathUtils::Vec3(-8.0f, 0.5f, 0.0f),
                        .centerB = mathUtils::Vec3(0.0f, 0.5f, 0.0f),
                        .velocityA = mathUtils::Vec3(600.0f, 0.0f, 0.0f),
                        .velocityB = mathUtils::Vec3(0.0f, 0.0f, 0.0f),
                        .deltaTime = dt,
                        .radiusA = r,
                        .radiusB = r
                    },
                    .expectedHit = true
                },
                {
                    .name = "Moving target head-on hit",
                    .expectationLabel = "Both spheres move toward each other and collide within frame.",
                    .input = geometry::MovingSphereCcdInput{
                        .centerA = mathUtils::Vec3(-2.0f, 0.5f, 0.0f),
                        .centerB = mathUtils::Vec3(2.0f, 0.5f, 0.0f),
                        .velocityA = mathUtils::Vec3(90.0f, 0.0f, 0.0f),
                        .velocityB = mathUtils::Vec3(-60.0f, 0.0f, 0.0f),
                        .deltaTime = dt,
                        .radiusA = r,
                        .radiusB = r
                    },
                    .expectedHit = true
                },
                {
                    .name = "Parallel movement no-hit",
                    .expectationLabel = "Spheres move in parallel with fixed separation greater than summed radius.",
                    .input = geometry::MovingSphereCcdInput{
                        .centerA = mathUtils::Vec3(-1.0f, 0.5f, 0.0f),
                        .centerB = mathUtils::Vec3(-1.0f, 0.5f, 1.2f),
                        .velocityA = mathUtils::Vec3(120.0f, 0.0f, 0.0f),
                        .velocityB = mathUtils::Vec3(120.0f, 0.0f, 0.0f),
                        .deltaTime = dt,
                        .radiusA = r,
                        .radiusB = r
                    },
                    .expectedHit = false
                },
                {
                    .name = "Looks crossing but wrong timing no-hit",
                    .expectationLabel = "Trajectories intersect spatially, but spheres pass at different times within frame.",
                    .input = geometry::MovingSphereCcdInput{
                        .centerA = mathUtils::Vec3(-4.0f, 0.5f, 0.0f),
                        .centerB = mathUtils::Vec3(0.0f, 0.5f, -4.0f),
                        .velocityA = mathUtils::Vec3(240.0f, 0.0f, 0.0f),
                        .velocityB = mathUtils::Vec3(0.0f, 0.0f, 30.0f),
                        .deltaTime = dt,
                        .radiusA = r,
                        .radiusB = r
                    },
                    .expectedHit = false
                },
                {
                    .name = "Near-tangent contact",
                    .expectationLabel = "Projectile grazes target at edge; tangent/near-tangent case should still be deterministic.",
                    .input = geometry::MovingSphereCcdInput{
                        .centerA = mathUtils::Vec3(-3.0f, 0.5f, 0.0f),
                        .centerB = mathUtils::Vec3(0.0f, 1.5f, 0.0f),
                        .velocityA = mathUtils::Vec3(180.0f, 0.0f, 0.0f),
                        .velocityB = mathUtils::Vec3(0.0f, 0.0f, 0.0f),
                        .deltaTime = dt,
                        .radiusA = r,
                        .radiusB = r
                    },
                    .expectedHit = true
                },
                {
                    .name = "Starts overlapped",
                    .expectationLabel = "Spheres begin in overlap at frame start; solver should return immediate hit (t=0).",
                    .input = geometry::MovingSphereCcdInput{
                        .centerA = mathUtils::Vec3(0.2f, 0.5f, 0.0f),
                        .centerB = mathUtils::Vec3(0.0f, 0.5f, 0.0f),
                        .velocityA = mathUtils::Vec3(0.0f, 0.0f, 0.0f),
                        .velocityB = mathUtils::Vec3(0.0f, 0.0f, 0.0f),
                        .deltaTime = dt,
                        .radiusA = r,
                        .radiusB = r
                    },
                    .expectedHit = true
                }
            };
        }

        void EnsureInitializedForCurrentLevel(appLifecycle::AppState& app)
        {
            auto& state = app.runtimeState.sphereCcdSandbox;
            if (state.activeLevelName == app.launchState.currentLevelName)
            {
                return;
            }

            ResetSphereCcdSandbox(state);
            state.activeLevelName = app.launchState.currentLevelName;
            if (!IsSandboxLevel(state.activeLevelName))
            {
                return;
            }

            state.scenarios = BuildDefaultScenarios();
            state.enabled = !state.scenarios.empty();
        }

        void StepScenarioInternal(SphereCcdSandboxState& state, int direction)
        {
            if (!state.enabled || state.scenarios.empty())
            {
                return;
            }
            const int count = static_cast<int>(state.scenarios.size());
            const int current = static_cast<int>(state.scenarioIndex);
            const int next = (current + direction + count) % count;
            state.scenarioIndex = static_cast<std::size_t>(next);
            state.playbackSeconds = 0.0f;
            state.autoplaySeconds = 0.0f;
        }

        void PushSandboxDebugPrimitives(rendern::Scene& scene, const SphereCcdScenario& scenario, const geometry::MovingSphereCcdResult& result, float playbackAlpha)
        {
            auto& debug = scene.debugPrimitives;
            const std::uint32_t colA = rendern::debugDraw::PackRGBA8(255, 190, 70, 255);
            const std::uint32_t colB = rendern::debugDraw::PackRGBA8(80, 220, 255, 255);
            const std::uint32_t colSweep = rendern::debugDraw::PackRGBA8(210, 210, 210, 255);
            const std::uint32_t colHit = rendern::debugDraw::PackRGBA8(80, 255, 110, 255);
            const std::uint32_t colMiss = rendern::debugDraw::PackRGBA8(255, 80, 80, 255);
            const std::uint32_t colResult = result.hit ? colHit : colMiss;

            const mathUtils::Vec3 centerAEnd = scenario.input.centerA + scenario.input.velocityA * scenario.input.deltaTime;
            const mathUtils::Vec3 centerBEnd = scenario.input.centerB + scenario.input.velocityB * scenario.input.deltaTime;
            const mathUtils::Vec3 centerACurrent = scenario.input.centerA + scenario.input.velocityA * (scenario.input.deltaTime * playbackAlpha);
            const mathUtils::Vec3 centerBCurrent = scenario.input.centerB + scenario.input.velocityB * (scenario.input.deltaTime * playbackAlpha);

            debug.wireSpheres.push_back({ .center = scenario.input.centerA, .radius = scenario.input.radiusA, .rgba = colA, .segments = 20u, .overlay = true });
            debug.wireSpheres.push_back({ .center = scenario.input.centerB, .radius = scenario.input.radiusB, .rgba = colB, .segments = 20u, .overlay = true });
            debug.wireSpheres.push_back({ .center = centerACurrent, .radius = scenario.input.radiusA, .rgba = colA, .segments = 20u, .overlay = true });
            debug.wireSpheres.push_back({ .center = centerBCurrent, .radius = scenario.input.radiusB, .rgba = colB, .segments = 20u, .overlay = true });

            debug.lines.push_back({ .a = scenario.input.centerA, .b = centerAEnd, .rgba = colSweep, .overlay = true });
            debug.lines.push_back({ .a = scenario.input.centerB, .b = centerBEnd, .rgba = colSweep, .overlay = true });

            if (result.hit)
            {
                const mathUtils::Vec3 hitPoint = result.centerAAtHit - result.normalFromBToA * scenario.input.radiusA;
                debug.crosses.push_back({ .center = hitPoint, .halfSize = std::max(0.06f, scenario.input.radiusA * 0.2f), .rgba = colResult, .overlay = true });
                debug.arrows.push_back({
                    .start = hitPoint,
                    .end = hitPoint + result.normalFromBToA * std::max(0.2f, scenario.input.radiusA + scenario.input.radiusB),
                    .rgba = colResult,
                    .headFrac = 0.18f,
                    .headWidthFrac = 0.10f,
                    .overlay = true
                });
            }
            else
            {
                const mathUtils::Vec3 missMark = (centerACurrent + centerBCurrent) * 0.5f;
                debug.crosses.push_back({ .center = missMark, .halfSize = 0.06f, .rgba = colMiss, .overlay = true });
            }
        }
    }

    void ResetSphereCcdSandbox(SphereCcdSandboxState& state)
    {
        state = {};
    }

    void UpdateSphereCcdSandbox(appLifecycle::AppState& app, float deltaSeconds)
    {
        EnsureInitializedForCurrentLevel(app);
        auto& state = app.runtimeState.sphereCcdSandbox;
        if (!state.enabled || state.scenarios.empty())
        {
            return;
        }

        if (state.autoplay)
        {
            state.autoplaySeconds += deltaSeconds;
            if (state.autoplaySeconds >= 2.0f)
            {
                state.autoplaySeconds = 0.0f;
                StepScenarioInternal(state, 1);
            }
        }

        const SphereCcdScenario& scenario = state.scenarios[state.scenarioIndex];
        state.playbackSeconds += deltaSeconds;
        const float duration = std::max(scenario.input.deltaTime, 1e-5f);
        while (state.playbackSeconds > duration)
        {
            state.playbackSeconds -= duration;
        }
        const float playbackAlpha = std::clamp(state.playbackSeconds / duration, 0.0f, 1.0f);

        state.lastResult = geometry::SolveMovingSphereSphereCcd(scenario.input);
        state.lastPass = state.lastResult.hit == scenario.expectedHit;
        state.lastScenarioName = scenario.name;
        state.lastExpectation = scenario.expectationLabel;

        PushSandboxDebugPrimitives(app.runtimeState.scene, scenario, state.lastResult, playbackAlpha);
    }

    void StepSphereCcdSandboxScenario(appLifecycle::AppState& app)
    {
        StepScenarioInternal(app.runtimeState.sphereCcdSandbox, 1);
    }

    void ToggleSphereCcdSandboxAutoplay(appLifecycle::AppState& app)
    {
        auto& state = app.runtimeState.sphereCcdSandbox;
        state.autoplay = !state.autoplay;
        state.autoplaySeconds = 0.0f;
    }

    void DrawSphereCcdSandboxUi([[maybe_unused]] SphereCcdSandboxState& state)
    {
#if defined(CORE_USE_DX12)
        if (!state.enabled || state.scenarios.empty())
        {
            return;
        }

        const SphereCcdScenario& scenario = state.scenarios[state.scenarioIndex];
        ImGui::Begin("Sphere CCD Sandbox");
        ImGui::Text("Scenario: %s", state.lastScenarioName.c_str());
        ImGui::Text("Expected: %s", scenario.expectedHit ? "Hit" : "No hit");
        ImGui::Text("Actual:   %s", state.lastResult.hit ? "Hit" : "No hit");
        ImGui::Text("PASS/FAIL: %s", state.lastPass ? "PASS" : "FAIL");
        ImGui::Text("time01: %.5f", state.lastResult.time01);
        ImGui::Text("Discriminant: %.6f", state.lastResult.discriminant);
        ImGui::Text("startsOverlapped: %s", state.lastResult.startsOverlapped ? "true" : "false");
        ImGui::Text("tangent: %s", state.lastResult.tangent ? "true" : "false");
        if (!state.lastExpectation.empty())
        {
            ImGui::Separator();
            ImGui::TextWrapped("%s", state.lastExpectation.c_str());
        }
        ImGui::Separator();
        ImGui::TextUnformatted("Controls: F7 next scenario, F8 autoplay toggle.");
        ImGui::End();
#endif
    }
}
