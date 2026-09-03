module;

#include <algorithm>
#include <bitset>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module core:gameplay_goap_observation_order;
import :gameplay_goap_composition_registry;

export namespace rendern
{
    // Dependency metadata uses independent boolean and integer fact namespaces.
    [[nodiscard]] std::vector<std::unique_ptr<IGameplayGOAPObservation>> OrderGameplayGOAPObservations(
        std::vector<std::unique_ptr<IGameplayGOAPObservation>> observers,
        const GameplayGOAPCompiledDefinition& compiled, std::string_view source)
    {
        const auto fail = [&](std::string_view reason)
        {
            throw std::runtime_error("AI behavior '" + std::string(source) + "', observations: " + std::string(reason));
        };
        std::bitset<AIAgentWorldState::FactCapacity> booleanWriters;
        std::bitset<AIAgentWorldState::IntegerFactCapacity> integerWriters;
        for (const auto& observer : observers)
        {
            if (!observer)
            {
                fail("observation compiler returned no provider");
            }
            for (const auto fact : observer->BooleanOutputs())
            {
                if (fact.index >= compiled.definition.metadata.booleanFacts.size() || booleanWriters.test(fact.index))
                {
                    fail("unknown fact or multiple observation writers");
                }
                booleanWriters.set(fact.index);
            }
            for (const auto fact : observer->IntegerOutputs())
            {
                if (fact.index >= compiled.definition.metadata.integerFacts.size() || integerWriters.test(fact.index))
                {
                    fail("unknown fact or multiple observation writers");
                }
                integerWriters.set(fact.index);
            }
        }
        std::vector<std::unique_ptr<IGameplayGOAPObservation>> result;
        if (booleanWriters.count() != compiled.definition.metadata.booleanFacts.size()
            || integerWriters.count() != compiled.definition.metadata.integerFacts.size())
        {
            fail("fact has no observation writer");
        }
        // Stable dependency order makes derived observations independent of asset order.
        std::bitset<AIAgentWorldState::FactCapacity> ready;
        std::bitset<AIAgentWorldState::IntegerFactCapacity> integersReady;
        while (!observers.empty())
        {
            const auto next = std::ranges::find_if(observers, [&](const auto& observer)
            {
                return std::ranges::all_of(observer->BooleanInputs(), [&](const auto fact)
                {
                    return fact.index < ready.size() && ready.test(fact.index);
                }) && std::ranges::all_of(observer->IntegerInputs(), [&](const auto fact)
                {
                    return fact.index < integersReady.size() && integersReady.test(fact.index);
                });
            });
            if (next == observers.end())
            {
                fail("cyclic or unresolved observation dependencies");
            }
            for (const auto fact : (*next)->BooleanOutputs())
            {
                ready.set(fact.index);
            }
            for (const auto fact : (*next)->IntegerOutputs())
            {
                integersReady.set(fact.index);
            }
            result.push_back(std::move(*next));
            observers.erase(next);
        }

        return result;
    }
}
