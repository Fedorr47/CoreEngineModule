module;

#include <memory>
#include <optional>

export module core:ai_move_to_action_binding;

import :gameplay;
import :ai_move_to_action;
import :gameplay_traversal_link_registry;
import :gameplay_traversal_executor_registry;
import :gameplay_object_reservation_system;
import :gameplay_obstacle_avoidance;
export import :ai_action_binding;

export namespace rendern
{
    class IAIMoveToActionRequestProvider
    {
    public:
        virtual ~IAIMoveToActionRequestProvider() = default;

        [[nodiscard]] virtual std::optional<AIMoveToActionRequest> ResolveRequest(
            const AIActionRuntimeContext& context) = 0;
    };
    
     class IAIActionReservationTargetProvider
    {
    public:
        virtual ~IAIActionReservationTargetProvider() = default;
        [[nodiscard]] virtual EntityHandle ResolveReservationTarget(
            const AIActionRuntimeContext& context) = 0;
    };

    class ReservedAIMoveToActionRuntime final : public IAIActionRuntime
    {
    public:
        ReservedAIMoveToActionRuntime(GameplayWorld& world,
            const GameplayTraversalLinkRegistry& links,
            const GameplayTraversalExecutorRegistry& executors,
            GameplayObjectReservationSystem& reservations,
            AIMoveToActionRequest request, const EntityHandle target,
            const IGameplayObstacleQuery* obstacleQuery) noexcept
            : world_(world), links_(links), executors_(executors), reservations_(reservations),
            request_(request), target_(target), obstacleQuery_(obstacleQuery)
        {
        }

        [[nodiscard]] AIActionRuntimeResult Start(const AIActionRuntimeContext& context) override
        {
            if (started_ || reservations_.IsReserved(target_) ||
                !reservations_.TryReserve(world_, target_, context.agentEntity))
            {
                return AIActionRuntimeResult::Failed;
            }
            ownsReservation_ = true;
            runtime_ = AIMoveToAction::CreateRuntime(
                context, world_, links_, executors_, request_, obstacleQuery_);
            if (runtime_ == nullptr)
            {
                Release_(context.agentEntity);
                return AIActionRuntimeResult::Failed;
            }
            started_ = true;
            const AIActionRuntimeResult result = runtime_->Start(context);
            if (result != AIActionRuntimeResult::Running)
            {
                Release_(context.agentEntity);
                started_ = false;
            }
            return result;
        }

        [[nodiscard]] AIActionRuntimeResult Tick(
            const AIActionRuntimeContext& context, const float deltaSeconds) override
        {
            if (!started_ || runtime_ == nullptr)
            {
                return AIActionRuntimeResult::Failed;
            }
            if (!ownsReservation_ || !reservations_.IsReservedBy(target_, context.agentEntity))
            {
                runtime_->Cancel(context);
                ownsReservation_ = false;
                started_ = false;
                return AIActionRuntimeResult::Failed;
            }
            const AIActionRuntimeResult result = runtime_->Tick(context, deltaSeconds);
            if (result != AIActionRuntimeResult::Running)
            {
                Release_(context.agentEntity);
                started_ = false;
            }
            return result;
        }

        void Cancel(const AIActionRuntimeContext& context) noexcept override
        {
            if (started_ && runtime_ != nullptr)
            {
                runtime_->Cancel(context);
            }
            Release_(context.agentEntity);
            started_ = false;
        }

    private:
        void Release_(const EntityHandle agent) noexcept
        {
            if (ownsReservation_ && reservations_.IsReservedBy(target_, agent))
            {
                (void)reservations_.Release(target_, agent);
            }
            ownsReservation_ = false;
        }

        GameplayWorld& world_;
        const GameplayTraversalLinkRegistry& links_;
        const GameplayTraversalExecutorRegistry& executors_;
        GameplayObjectReservationSystem& reservations_;
        AIMoveToActionRequest request_{};
        EntityHandle target_{kNullEntity};
        const IGameplayObstacleQuery* obstacleQuery_{nullptr};
        std::unique_ptr<IAIActionRuntime> runtime_{};
        bool ownsReservation_{};
        bool started_{};
    };

    // All dependencies are non-owning and must outlive runtime creation.
    class AIMoveToActionBinding final : public IAIActionBinding
    {
    public:
        AIMoveToActionBinding(
            GameplayWorld& world,
            const GameplayTraversalLinkRegistry& traversalLinkRegistry,
            const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
            IAIMoveToActionRequestProvider& requestProvider,
            const IGameplayObstacleQuery* obstacleQuery = nullptr) noexcept
            : world_(world)
            , traversalLinkRegistry_(traversalLinkRegistry)
            , traversalExecutorRegistry_(traversalExecutorRegistry)
            , requestProvider_(requestProvider)
            , obstacleQuery_(obstacleQuery)
        {
        }
        
        AIMoveToActionBinding(GameplayWorld& world,
            const GameplayTraversalLinkRegistry& traversalLinkRegistry,
            const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
            IAIMoveToActionRequestProvider& requestProvider,
            GameplayObjectReservationSystem& reservationSystem,
            IAIActionReservationTargetProvider& reservationTargetProvider,
            const IGameplayObstacleQuery* obstacleQuery = nullptr) noexcept
            : AIMoveToActionBinding(world, traversalLinkRegistry, traversalExecutorRegistry,
                requestProvider, obstacleQuery)
        {
            reservationSystem_ = &reservationSystem;
            reservationTargetProvider_ = &reservationTargetProvider;
        }


        [[nodiscard]] std::unique_ptr<IAIActionRuntime> CreateRuntime(
            const AIActionRuntimeContext& context) override
        {
            if (context.actionId != kAIMoveToActionId)
            {
                return nullptr;
            }
            const std::optional<AIMoveToActionRequest> request =
                requestProvider_.ResolveRequest(context);
            if (!request)
            {
                return nullptr;
            }
            if (reservationSystem_ != nullptr && reservationTargetProvider_ != nullptr)
            {
                const EntityHandle target = reservationTargetProvider_->ResolveReservationTarget(context);
                if (target != kNullEntity)
                {
                    return std::make_unique<ReservedAIMoveToActionRuntime>(world_,
                        traversalLinkRegistry_, traversalExecutorRegistry_, *reservationSystem_,
                        *request, target, obstacleQuery_);
                }
            }
            return AIMoveToAction::CreateRuntime(
                context,
                world_,
                traversalLinkRegistry_,
                traversalExecutorRegistry_,
                *request,
                obstacleQuery_);
        }

    private:
        GameplayWorld& world_;
        const GameplayTraversalLinkRegistry& traversalLinkRegistry_;
        const GameplayTraversalExecutorRegistry& traversalExecutorRegistry_;
        IAIMoveToActionRequestProvider& requestProvider_;
        const IGameplayObstacleQuery* obstacleQuery_{nullptr};
        GameplayObjectReservationSystem* reservationSystem_{};
        IAIActionReservationTargetProvider* reservationTargetProvider_{};
    };
}