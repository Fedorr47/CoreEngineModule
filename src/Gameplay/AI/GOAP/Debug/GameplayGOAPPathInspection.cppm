module;

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

export module core:gameplay_goap_path_inspection;

export import :ai_action_contracts;
import :ai_decision_contracts;
export import :gameplay_route;

export namespace rendern
{
    struct GameplayAIDebugPlannedRouteStep
    {
        std::size_t planStepIndex{};
        AIActionId actionId{};
        AIActionContextId contextId{};
        std::optional<GameplayRoute> route{};
    };

    struct GameplayAIDebugPlannedPathView
    {
        std::vector<GameplayAIDebugPlannedRouteStep> routeSteps{};
        bool complete{true};
    };

    // Optional action capability; non-navigation capabilities do not import routes.
    class IGameplayGOAPActionPathProvider
    {
    public:
        virtual ~IGameplayGOAPActionPathProvider() = default;
        [[nodiscard]] virtual std::optional<GameplayRoute> BuildDebugRoute(AIActionContextId context) const = 0;
    };

    class IGameplayGOAPPlannedPathProvider
    {
    public:
        virtual ~IGameplayGOAPPlannedPathProvider() = default;
        [[nodiscard]] virtual GameplayAIDebugPlannedPathView BuildPlannedPath(
            std::span<const AIPlanStep> selectedPlan,
            std::optional<std::size_t> currentStepIndex) const = 0;
    };

    class IGameplayGOAPPathInspection
    {
    public:
        virtual ~IGameplayGOAPPathInspection() = default;

        [[nodiscard]] virtual GameplayAIDebugPlannedPathView
            BuildPlannedPathDebugView() const = 0;
    };
}