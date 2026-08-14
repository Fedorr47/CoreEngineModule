#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "App/GameplayPhysicsCharacterIntegration.h"

import core;
import app.navigation_agent_size_development_scenario;

#include "AppDevelopmentScenarioRuntime.h"

namespace appDevelopment
{
    namespace
    {
        [[nodiscard]] const char* ToText(
            const rendern::AIActionExecutionStatus status) noexcept
        {
            switch (status)
            {
            case rendern::AIActionExecutionStatus::Running: return "Running";
            case rendern::AIActionExecutionStatus::Succeeded: return "Succeeded";
            case rendern::AIActionExecutionStatus::Failed: return "Failed";
            case rendern::AIActionExecutionStatus::Cancelled: return "Cancelled";
            default: return "NotStarted";
            }
        }

        [[nodiscard]] const char* ToText(
            const app::navigationRuntime::AgentSizeScenarioStatus status) noexcept
        {
            switch (status)
            {
            case app::navigationRuntime::AgentSizeScenarioStatus::Moving: return "Moving";
            case app::navigationRuntime::AgentSizeScenarioStatus::Reached: return "Reached";
            case app::navigationRuntime::AgentSizeScenarioStatus::NoPath: return "NoPath";
            case app::navigationRuntime::AgentSizeScenarioStatus::Failed: return "Failed";
            default: return "NotStarted";
            }
        }

        [[nodiscard]] rendern::GameplayUpdateContext MakeGameplayContext(
            const ScenarioContext& context) noexcept
        {
            rendern::GameplayUpdateContext result{};
            result.mode = context.gameplayMode;
            result.levelAsset = &context.level;
            result.levelInstance = &context.levelInstance;
            result.scene = &context.scene;
            return result;
        }
    }

    struct AppDevelopmentScenarioRuntime::Impl
    {
        ScenarioKind kind{ScenarioKind::None};
        app::navigationRuntime::NavigationAgentSizeDevelopmentScenario agentSizeScenario{};
        rendern::GameplayAIJumpTraversalDevelopmentScenarioState jumpTraversalScenario{};
        rendern::GameplayRuntimeMode lastMode{rendern::GameplayRuntimeMode::Editor};
    };

    AppDevelopmentScenarioRuntime::AppDevelopmentScenarioRuntime()
        : impl_(new Impl{})
    {
    }

    AppDevelopmentScenarioRuntime::~AppDevelopmentScenarioRuntime()
    {
        delete impl_;
    }

    void AppDevelopmentScenarioRuntime::Reset() noexcept
    {
        impl_->kind = ScenarioKind::None;
        impl_->agentSizeScenario.Reset();
        impl_->jumpTraversalScenario.Reset();
        impl_->lastMode = rendern::GameplayRuntimeMode::Editor;
    }

    void AppDevelopmentScenarioRuntime::OnLevelLoaded(ScenarioContext& context)
    {
        Reset();
        if (app::navigationRuntime::IsAgentSizeScenario(context.level))
        {
            impl_->kind = ScenarioKind::NavigationAgentSize;
        }
        else if (rendern::IsGameplayAIStepDebugScenario(context.level))
        {
            impl_->kind = ScenarioKind::AIPhysicsStep;
        }
        else if (rendern::IsGameplayAIJumpTraversalDevelopmentScenario(context.level))
        {
            impl_->kind = ScenarioKind::AIJumpTraversal;
        }
        else if (rendern::IsGameplayAIMovementDevelopmentScenario(context.level))
        {
            impl_->kind = ScenarioKind::AIMovement;
        }

        if (impl_->kind == ScenarioKind::NavigationAgentSize)
        {
            impl_->agentSizeScenario.Prepare(
                context.gameplayRuntime, MakeGameplayContext(context));
        }
        else if (impl_->kind == ScenarioKind::AIJumpTraversal)
        {
            rendern::PrepareGameplayAIJumpTraversalDevelopmentScenario(
                impl_->jumpTraversalScenario, context.level);
        }
        impl_->lastMode = context.gameplayMode;
    }

    void AppDevelopmentScenarioRuntime::Update(ScenarioContext& context) noexcept
    {
        if (impl_->kind == ScenarioKind::NavigationAgentSize)
        {
            impl_->agentSizeScenario.Update(context.gameplayRuntime);
            if (impl_->lastMode == rendern::GameplayRuntimeMode::Game &&
                context.gameplayMode == rendern::GameplayRuntimeMode::Editor)
            {
                impl_->agentSizeScenario.ResetExecutionState();
            }
        }
        impl_->lastMode = context.gameplayMode;
    }

    void AppDevelopmentScenarioRuntime::Execute(
        const ScenarioCommand command, ScenarioContext& context)
    {
        if (context.gameplayMode != rendern::GameplayRuntimeMode::Game)
        {
            return;
        }

        switch (impl_->kind)
        {
        case ScenarioKind::AIMovement:
            if (command == ScenarioCommand::Start)
            {
                const rendern::EntityHandle entity = rendern::ResetGameplayAIMovementDevelopmentScenario(
                    context.gameplayRuntime, context.level);
                if (context.physicsWorld != nullptr && entity != rendern::kNullEntity)
                {
                    (void)appRuntime::TeleportGameplayPhysicsCharacterToGameplayTransform(
                        context.gameplayRuntime, *context.physicsWorld, entity);
                }
                (void)rendern::StartGameplayAIMovementDevelopmentScenario(
                    context.gameplayRuntime, MakeGameplayContext(context));
            }
            else if (command == ScenarioCommand::Stop)
            {
                rendern::CancelGameplayAIMovementDevelopmentScenario(
                    context.gameplayRuntime, context.level);
            }
            else if (command == ScenarioCommand::Reset)
            {
                const rendern::EntityHandle entity = rendern::ResetGameplayAIMovementDevelopmentScenario(
                    context.gameplayRuntime, context.level);
                if (context.physicsWorld != nullptr && entity != rendern::kNullEntity)
                {
                    (void)appRuntime::TeleportGameplayPhysicsCharacterToGameplayTransform(
                        context.gameplayRuntime, *context.physicsWorld, entity);
                }
            }
            break;

        case ScenarioKind::AIPhysicsStep:
            if (command == ScenarioCommand::Start)
            {
                (void)rendern::StartGameplayAIStepDebugRoute(
                    context.gameplayRuntime, MakeGameplayContext(context));
            }
            else if (command == ScenarioCommand::Stop)
            {
                rendern::CancelGameplayAIStepDebugRoute(
                    context.gameplayRuntime, context.level);
            }
            else if (command == ScenarioCommand::Reset)
            {
                const rendern::EntityHandle entity = rendern::ResetGameplayAIStepDebugNPC(
                    context.gameplayRuntime, context.level);
                if (context.physicsWorld != nullptr && entity != rendern::kNullEntity)
                {
                    (void)appRuntime::TeleportGameplayPhysicsCharacterToGameplayTransform(
                        context.gameplayRuntime, *context.physicsWorld, entity);
                }
            }
            break;
            
        case ScenarioKind::AIJumpTraversal:
            if (command == ScenarioCommand::Start)
            {
                const rendern::EntityHandle entity = rendern::ResetGameplayAIJumpTraversalDevelopmentScenario(
                    context.gameplayRuntime, context.level, impl_->jumpTraversalScenario);
                if (context.physicsWorld != nullptr && entity != rendern::kNullEntity)
                {
                    (void)appRuntime::TeleportGameplayPhysicsCharacterToGameplayTransform(
                        context.gameplayRuntime, *context.physicsWorld, entity);
                }
                (void)rendern::StartGameplayAIJumpTraversalDevelopmentScenario(
                    context.gameplayRuntime, MakeGameplayContext(context));
            }
            else if (command == ScenarioCommand::Stop)
            {
                rendern::CancelGameplayAIJumpTraversalDevelopmentScenario(
                    context.gameplayRuntime, context.level);
            }
            else if (command == ScenarioCommand::Reset)
            {
                const rendern::EntityHandle entity = rendern::ResetGameplayAIJumpTraversalDevelopmentScenario(
                    context.gameplayRuntime, context.level, impl_->jumpTraversalScenario);
                if (context.physicsWorld != nullptr && entity != rendern::kNullEntity)
                {
                    (void)appRuntime::TeleportGameplayPhysicsCharacterToGameplayTransform(
                        context.gameplayRuntime, *context.physicsWorld, entity);
                }
            }
            break;
            
        case ScenarioKind::NavigationAgentSize:
            if (command == ScenarioCommand::Start && context.navigationProfiles != nullptr)
            {
                impl_->agentSizeScenario.Start(
                    context.gameplayRuntime, *context.navigationProfiles, context.level);
            }
            else if (command == ScenarioCommand::Reset)
            {
                const auto result = impl_->agentSizeScenario.ResetToInitialState(
                    context.gameplayRuntime, context.level);
                if (context.physicsWorld != nullptr)
                {
                    if (result.smallEntity != rendern::kNullEntity)
                        (void)appRuntime::TeleportGameplayPhysicsCharacterToGameplayTransform(
                            context.gameplayRuntime, *context.physicsWorld, result.smallEntity);
                    if (result.largeEntity != rendern::kNullEntity)
                        (void)appRuntime::TeleportGameplayPhysicsCharacterToGameplayTransform(
                            context.gameplayRuntime, *context.physicsWorld, result.largeEntity);
                }
            }
            break;

        default:
            break;
        }
    }

    ScenarioKind AppDevelopmentScenarioRuntime::GetActiveKind() const noexcept
    {
        return impl_->kind;
    }

    ScenarioView AppDevelopmentScenarioRuntime::GetView(
        const ScenarioContext& context) const noexcept
    {
        const bool gameMode = context.gameplayMode == rendern::GameplayRuntimeMode::Game;
        ScenarioView view{};
        view.active = impl_->kind != ScenarioKind::None;
        view.commandsEnabled = gameMode;

        switch (impl_->kind)
        {
        case ScenarioKind::AIMovement:
            view.title = "AI Movement";
            view.description = "AI_Move_Agent and ordered AI_Move_Point_<number> nodes.";
            view.startLabel = "Start / Restart AI Route";
            view.stopLabel = "Cancel AI Route";
            view.canStart = true;
            view.canStop = true;
            view.canReset = true;
            view.resetLabel = "Reset Route";
            view.statuses[0] = {
                "Route status",
                ToText(rendern::GetGameplayAIMovementDevelopmentScenarioStatus(
                    context.gameplayRuntime, context.level))
            };
            view.statusCount = 1;
            break;

        case ScenarioKind::AIPhysicsStep:
            view.title = "AI Physics Step-Up Debug";
            view.startLabel = "Start Route";
            view.resetLabel = "Reset NPC";
            view.stopLabel = "Stop Route";
            view.canStart = true;
            view.canReset = true;
            view.canStop = true;
            break;
            
        case ScenarioKind::AIJumpTraversal:
            view.title = "CR-447 AI Jump Traversal";
            view.description = "Authored flat-gap Jump link through the production FollowRoute path.";
            view.startLabel = "Start Route";
            view.resetLabel = "Reset Scenario";
            view.stopLabel = "Stop Route";
            view.canStart = true;
            view.canReset = true;
            view.canStop = true;
            view.statuses[0] = {
                "Route status",
                ToText(rendern::GetGameplayAIJumpTraversalDevelopmentScenarioStatus(
                    context.gameplayRuntime, context.level))
            };
            view.statusCount = 1;
            break;

        case ScenarioKind::NavigationAgentSize:
            view.title = "CR-445 Navigation Agent Size";
            view.description = "Identical 1.2 m openings; radii 0.2 m and 0.7 m.";
            view.startLabel = "Start Route";
            view.resetLabel = "Reset Scenario";
            view.canStart = context.navigationProfiles != nullptr;
            view.canReset = true;
            view.commandsEnabled = gameMode;
            view.statuses[0] = {"Small NPC", ToText(impl_->agentSizeScenario.GetSmallStatus())};
            view.statuses[1] = {"Large NPC", ToText(impl_->agentSizeScenario.GetLargeStatus())};
            view.statusCount = 2;
            break;

        default:
            break;
        }
        return view;
    }
}