export module core:gameplay_ai_decision_creation_context;

import :EnTTHelpers;

export namespace rendern
{
    using namespace EnTT_helpers;

    class GameplayWorld;
    struct LevelAsset;
    class GameplayTraversalLinkRegistry;
    class GameplayTraversalExecutorRegistry;
    class GameplayObjectReservationSystem;

    // The context itself is transient. Referenced services are non-owning and must
    // outlive any decision/action that borrows them during Create().
    struct GameplayAIDecisionCreationContext
    {
        EntityHandle agent{kNullEntity};
        LevelAsset& level;
        GameplayWorld& world;
        const GameplayTraversalLinkRegistry& traversalLinkRegistry;
        const GameplayTraversalExecutorRegistry& traversalExecutorRegistry;
        GameplayObjectReservationSystem& reservationSystem;
    };

}
