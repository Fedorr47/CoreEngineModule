module;

#include <cstddef>
#include <optional>
#include <vector>

export module core:gameplay_goap_path_inspection;

export import :ai_action_contracts;
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

    class IGameplayGOAPPathInspection
    {
    public:
        virtual ~IGameplayGOAPPathInspection() = default;

        [[nodiscard]] virtual GameplayAIDebugPlannedPathView
            BuildPlannedPathDebugView() const = 0;
    };
}