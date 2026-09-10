import core;
import std;

#include "AppNavigationCoordinator.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "AppLifecycle.h"

namespace appNavigationCoordinator
{
    void Advance(appLifecycle::AppState& app)
    {
        auto& runtime = app.runtimeState;
        if (runtime.navigationState == appLifecycle::AppRuntimeState::NavigationState::Pending)
        {
            const app::navigationRuntime::GeometryResult geometry =
                app::navigationRuntime::BuildLevelNavigationGeometry(*runtime.levelInstance);
            if (geometry.status == app::navigationRuntime::GeometryStatus::WaitingForMeshes)
            {
                if (!runtime.navigationWaitingLogged)
                {
                    std::cerr << "[Navigation] Waiting for mesh CPU geometry.\n";
                    runtime.navigationWaitingLogged = true;
                }
            }
            else if (geometry.status == app::navigationRuntime::GeometryStatus::Ready)
            {
                auto profiles = std::make_unique<navigation::ProfileRegistry>();
                const navigation::ProfileResolution defaultProfile =
                    profiles->Initialize(geometry.geometry, runtime.navigationBuildSettings);
                navigation::BuildStatus status = defaultProfile.status;
                if (status == navigation::BuildStatus::Succeeded && runtime.gameplayRuntime)
                {
                    const rendern::GameplayWorld& gameplayWorld = runtime.gameplayRuntime->GetWorld();
                    for (const rendern::EntityHandle entity : runtime.gameplayRuntime->GetNodeBoundEntities())
                    {
                        const auto* physicalSettings = gameplayWorld.TryGetCharacterPhysicalSettings(entity);
                        if (physicalSettings == nullptr)
                        {
                            continue;
                        }
                        const navigation::AgentSettings agentSettings =
                            app::navigationRuntime::BuildAgentSettings(*physicalSettings);
                        status = profiles->ResolveProfile(agentSettings).status;
                        if (status != navigation::BuildStatus::Succeeded)
                        {
                            std::cerr << "[Navigation] Agent profile build failed: entity=" << entity
                                << ", radius=" << agentSettings.radius
                                << ", height=" << agentSettings.height
                                << ", maximumStepHeight=" << agentSettings.maximumStepHeight
                                << ", maximumSlopeAngleDegrees="
                                << agentSettings.maximumSlopeAngleDegrees
                                << ", status=" << static_cast<int>(status) << ".\n";
                            break;
                        }
                    }
                }

                if (status == navigation::BuildStatus::Succeeded)
                {
                    const navigation::World* defaultWorld = profiles->TryGetWorld(defaultProfile.profile);
                    runtime.navigationDebugGeometry = defaultWorld->BuildDebugGeometry();
                    runtime.navigationProfiles = std::move(profiles);
                    runtime.navigationState = appLifecycle::AppRuntimeState::NavigationState::Ready;
                    std::cerr << "[Navigation] Build succeeded: " << geometry.sourceMeshCount
                        << " meshes, " << geometry.geometry.vertices.size() << " vertices, "
                        << geometry.geometry.indices.size() / 3 << " triangles.\n";
                }
                else
                {
                    runtime.navigationState = appLifecycle::AppRuntimeState::NavigationState::Failed;
                    runtime.navigationDebugGeometry = {};
                    std::cerr << "[Navigation] Build failed with status " << static_cast<int>(status) << ".\n";
                }
            }
            else
            {
                runtime.navigationState = appLifecycle::AppRuntimeState::NavigationState::Failed;
                runtime.navigationDebugGeometry = {};
                std::cerr << "[Navigation] Build failed: invalid static mesh geometry.\n";
            }
        }
    }
}
