module;

#include <memory>
#include <span>
#include <utility>
#include <vector>

export module core:gameplay_goap_decision_instance;

export import :gameplay_goap_decision_setup;
import :gameplay_goap_decision;
import :gameplay_goap_inspection;
import :gameplay_goap_path_inspection;
export import :gameplay_ai_decision_contracts;

export namespace rendern
{
    class GameplayGOAPDecisionInstance :
        public GameplayAIDecisionInstance,
        public IGameplayGOAPInspection
    {
    public:
        GameplayGOAPDecisionInstance(const EntityHandle agent, GameplayGOAPDecisionSetup setup)
            : context_(std::move(setup.context)), reactions_(std::move(setup.eventReactions)),
              goap_(agent, std::move(setup.definition))
        {
            if (!context_)
            {
                return;
            }
            for (const auto& reaction : reactions_)
            {
                if (!reaction)
                {
                    return;
                }
            }
            for (const GameplayGOAPActionBindingSetup& binding : setup.actionBindings)
            {
                if (!binding.create || !goap_.InstallActionBinding(binding.actionId,
                    binding.create(goap_.GetObservedState(), runtimeEvents_)))
                {
                    return;
                }
            }
            configured_ = goap_.HasCompleteActionBindings();
        }

        GameplayGOAPDecisionInstance(const GameplayGOAPDecisionInstance&) = delete;
        GameplayGOAPDecisionInstance& operator=(const GameplayGOAPDecisionInstance&) = delete;
        GameplayGOAPDecisionInstance(GameplayGOAPDecisionInstance&&) = delete;
        GameplayGOAPDecisionInstance& operator=(GameplayGOAPDecisionInstance&&) = delete;

        void Update(AISystem& aiSystem, const GameplayAIObservationContext& observation) override
        {
            if (!configured_)
            {
                return;
            }
            reactionEvents_.clear();
            // Input and output may alias. Finish all input reads before forwarding events.
            context_->Observe(observation.world, observation.events, goap_.GetObservedState());
            ReactToEvents_(observation.world, observation.events);
            runtimeEvents_.clear();
            goap_.Update(aiSystem, observation.world);
            context_->ObserveActionEvents(
                observation.world, runtimeEvents_, goap_.GetObservedState());
            ReactToEvents_(observation.world, runtimeEvents_);
            if (observation.eventOutput != nullptr)
            {
                observation.eventOutput->insert(observation.eventOutput->end(),
                    runtimeEvents_.begin(), runtimeEvents_.end());
                observation.eventOutput->insert(observation.eventOutput->end(),
                    reactionEvents_.begin(), reactionEvents_.end());
            }
        }

        void Cancel(AISystem& aiSystem) noexcept override
        {
            goap_.Cancel(aiSystem);
        }

        [[nodiscard]] GameplayAIDecisionStatus GetStatus() const noexcept override
        {
            if (!configured_)
            {
                return GameplayAIDecisionStatus::Failed;
            }
            switch (goap_.GetStatus())
            {
            case AIPlanExecutionStatus::NotStarted:
                return GameplayAIDecisionStatus::NotStarted;
            case AIPlanExecutionStatus::ReadyToStartStep:
            case AIPlanExecutionStatus::RunningStep:
                return GameplayAIDecisionStatus::Running;
            case AIPlanExecutionStatus::Succeeded:
                return GameplayAIDecisionStatus::Succeeded;
            case AIPlanExecutionStatus::Failed:
                return GameplayAIDecisionStatus::Failed;
            case AIPlanExecutionStatus::Cancelled:
                return GameplayAIDecisionStatus::Cancelled;
            }
            return GameplayAIDecisionStatus::Failed;
        }

        [[nodiscard]] AIPlanExecutionStatus GetGOAPStatus() const noexcept override
        {
            return goap_.GetStatus();
        }

        [[nodiscard]] const AIAgentWorldState& GetObservedState() const noexcept override
        {
            return goap_.GetObservedState();
        }

        [[nodiscard]] AIDebugViewModel BuildDebugViewModel() const override
        {
            return goap_.BuildDebugViewModel();
        }

        [[nodiscard]] bool IsConfigured() const noexcept
        {
            return configured_;
        }

    private:
        void ReactToEvents_(const GameplayWorld& world, std::span<const GameplayWorldEvent> events)
        {
            for (const auto& reaction : reactions_)
            {
                reaction->React(world, events, goap_.GetObservedState(), reactionEvents_);
            }
        }

        // Bindings borrow the context, facts and event buffer. Context and events
        // outlive goap_; the decision owner must cancel active AISystem tasks first.
        std::unique_ptr<IGameplayGOAPContext> context_;
        std::vector<std::unique_ptr<IGameplayGOAPEventReaction>> reactions_;
        std::vector<GameplayWorldEvent> reactionEvents_{};
        std::vector<GameplayWorldEvent> runtimeEvents_{};
        GameplayGOAPDecision goap_;
        bool configured_{};
    };

    namespace gameplay_goap_detail
    {
        // Preserve capability discovery: a decision without a path provider does
        // not advertise IGameplayGOAPPathInspection.
        class DecisionWithPathInspection final :
            public GameplayGOAPDecisionInstance,
            public IGameplayGOAPPathInspection
        {
        public:
            DecisionWithPathInspection(const EntityHandle agent, GameplayGOAPDecisionSetup setup,
                const IGameplayGOAPPlannedPathProvider& paths)
                : GameplayGOAPDecisionInstance(agent, std::move(setup)), paths_(paths)
            {
            }

            [[nodiscard]] GameplayAIDebugPlannedPathView BuildPlannedPathDebugView() const override
            {
                const AIDebugViewModel plan = BuildDebugViewModel();
                std::vector<AIPlanStep> steps;
                steps.reserve(plan.selectedPlan.size());
                for (const AIDebugPlanStepView& step : plan.selectedPlan)
                {
                    steps.push_back({step.actionId, step.contextId});
                }
                return paths_.BuildPlannedPath(steps, plan.currentStepIndex);
            }

        private:
            const IGameplayGOAPPlannedPathProvider& paths_;
        };
    }

    [[nodiscard]] std::unique_ptr<GameplayAIDecisionInstance> CreateGameplayGOAPDecision(
        const EntityHandle agent, GameplayGOAPDecisionSetup setup)
    {
        const auto* paths = dynamic_cast<const IGameplayGOAPPlannedPathProvider*>(setup.context.get());
        std::unique_ptr<GameplayGOAPDecisionInstance> decision;
        if (paths != nullptr)
        {
            decision = std::make_unique<gameplay_goap_detail::DecisionWithPathInspection>(
                agent, std::move(setup), *paths);
        }
        else
        {
            decision = std::make_unique<GameplayGOAPDecisionInstance>(agent, std::move(setup));
        }
        if (!decision->IsConfigured())
        {
            return nullptr;
        }
        return decision;
    }
}
