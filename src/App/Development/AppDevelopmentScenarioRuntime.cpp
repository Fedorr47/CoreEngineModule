#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "App/GameplayPhysicsCharacterIntegration.h"
#include <algorithm>
#include <stdexcept>

import core;

#include "AppDevelopmentScenarioRuntime.h"
#include "DevelopmentScenario.h"

namespace appDevelopment
{
    struct AppDevelopmentScenarioRuntime::Impl
    {
        ScenarioKind kind{ScenarioKind::None};
        rendern::GameplayRuntimeMode lastMode{rendern::GameplayRuntimeMode::Editor};
        DevelopmentScenarioAsset dataAsset{};
        DevelopmentScenarioRunner dataRunner{};
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
        // A data-driven runner needs its ScenarioContext to cancel owned work.
        // Retain it until Reset(context) can perform a safe unload.
        if (impl_->dataRunner.IsLoaded()) 
        {
            return;
        }
        impl_->dataRunner = DevelopmentScenarioRunner{};
        impl_->dataAsset = {};
        impl_->kind = ScenarioKind::None;
        impl_->lastMode = rendern::GameplayRuntimeMode::Editor;
    }

    void AppDevelopmentScenarioRuntime::Reset(ScenarioContext& context) noexcept
    {
        impl_->dataRunner.Unload(context);
        Reset();
    }
    
    void AppDevelopmentScenarioRuntime::OnLevelLoaded(ScenarioContext& context)
    {
        Reset(context);
        
        // Do not replace a data-driven scenario whose spawned entity still owns
        // a physics binding that could not be torn down safely.
        if (impl_->dataRunner.IsLoaded())
        {
            throw std::runtime_error(
                "Development scenario: failed to unload previous data-driven scenario");
        }
        
        if (!context.level.developmentScenario.empty())
        {
            impl_->dataAsset = LoadDevelopmentScenarioAsset(context.level.developmentScenario);
            if (!impl_->dataRunner.Load(impl_->dataAsset, context))
            {
                throw std::runtime_error("Development scenario '" + context.level.developmentScenario + "': failed to resolve roles or execute setup");
            }
            impl_->kind = ScenarioKind::DataDriven;
            impl_->lastMode = context.gameplayMode;
            return;
        }
        impl_->lastMode = context.gameplayMode;
    }

    void AppDevelopmentScenarioRuntime::Update(ScenarioContext& context) noexcept
    {
        if (impl_->kind == ScenarioKind::DataDriven)
        {
            if (impl_->lastMode == rendern::GameplayRuntimeMode::Game &&
                context.gameplayMode == rendern::GameplayRuntimeMode::Editor)
            {
                impl_->dataRunner.Stop(context);
            }
            if (context.gameplayMode == rendern::GameplayRuntimeMode::Game)
            {
                impl_->dataRunner.Update(context);
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
        
        if (impl_->kind == ScenarioKind::DataDriven)
        {
            if (command == ScenarioCommand::Start)
            {
                (void)impl_->dataRunner.Start(context);
            }
            else if (command == ScenarioCommand::Stop)
            {
                impl_->dataRunner.Stop(context);
            }
            else
            {
                impl_->dataRunner.Reset(context);
            }
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
        case ScenarioKind::DataDriven:
            if (const auto* asset = impl_->dataRunner.GetAsset())
            {
                view.title = asset->title.c_str();
                view.description = asset->description.c_str();
                view.canStart = impl_->dataRunner.CanStart(context);
                view.canStop = impl_->dataRunner.IsRunning();
                view.canReset = true;
                const auto& results = impl_->dataRunner.GetResults();
                if (results.empty())
                {
                    view.statuses[0] = {"Status", impl_->dataRunner.IsRunning() ? "Running" : "Loaded"};
                    view.statusCount = 1;
                }
                else
                {
                    view.statusCount = static_cast<unsigned int>(std::min<std::size_t>(results.size(), 4));
                    for (unsigned int index = 0; index < view.statusCount; ++index)
                    {
                        view.statuses[index] = {results[index].name.c_str(),
                            results[index].diagnostic.empty() ? ToString(results[index].status)
                                : results[index].diagnostic.c_str()};
                    }
                }
            }
            break;

        default:
            break;
        }
        return view;
    }
}