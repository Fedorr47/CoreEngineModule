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
            
            return aiEntitiesScratch_.size();
        }

    private:
        // Reused between frames to avoid allocating a temporary agent list on
        // every synchronous AI update.
        std::vector<EntityHandle> aiEntitiesScratch_{};
    };
}
