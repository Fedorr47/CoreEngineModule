module;

#include <memory>
#include <optional>

export module core:ai_move_to_action_binding;

import :gameplay;
import :ai_move_to_action;
import :gameplay_traversal_link_registry;
import :gameplay_traversal_executor_registry;
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

    // All dependencies are non-owning and must outlive runtime creation.
    class AIMoveToActionBinding final : public IAIActionBinding
    {
    public:
        AIMoveToActionBinding(
            GameplayWorld& world,
            const GameplayTraversalLinkRegistry& traversalLinkRegistry,
            const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
            IAIMoveToActionRequestProvider& requestProvider) noexcept
            : world_(world)
            , traversalLinkRegistry_(traversalLinkRegistry)
            , traversalExecutorRegistry_(traversalExecutorRegistry)
            , requestProvider_(requestProvider)
        {
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
            return AIMoveToAction::CreateRuntime(
                context,
                world_,
                traversalLinkRegistry_,
                traversalExecutorRegistry_,
                *request);
        }

    private:
        GameplayWorld& world_;
        const GameplayTraversalLinkRegistry& traversalLinkRegistry_;
        const GameplayTraversalExecutorRegistry& traversalExecutorRegistry_;
        IAIMoveToActionRequestProvider& requestProvider_;
    };
}