module;

#include <cstddef>
#include <vector>

export module core:ai_system;

import :gameplay;

export namespace rendern
{
    class AISystem
    {
    public:
        // Runs the synchronous AI update boundary and returns the number of
        // agents that remained valid for processing during this update.
        std::size_t Update(GameplayWorld& world)
        {
            world.CollectAIEntities(aiEntitiesScratch_);

            std::size_t processedAgentCount = 0;
            for ([[maybe_unused]] const EntityHandle entity : aiEntitiesScratch_)
            {
                // AIComponent is intentionally a marker in CR-336. This loop
                // establishes the deterministic per-agent update boundary for
                // the runtime behavior added by later AI tasks.
                ++processedAgentCount;
            }

            return processedAgentCount;
        }

    private:
        // Reused between frames to avoid allocating a temporary agent list on
        // every synchronous AI update.
        std::vector<EntityHandle> aiEntitiesScratch_{};
    };
}
