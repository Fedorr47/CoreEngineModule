module;

#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

export module core:ai_agent_world_state;

export namespace rendern
{
    struct AIWorldFactId
    {
        using ValueType = std::uint16_t;

        static constexpr ValueType InvalidIndex =
            std::numeric_limits<ValueType>::max();

        ValueType index{ InvalidIndex };

        friend constexpr bool operator==(
            const AIWorldFactId&,
            const AIWorldFactId&) noexcept = default;
    };

    class AIAgentWorldState
    {
    public:
        static constexpr std::size_t FactCapacity = 128u;

        [[nodiscard]] bool IsFactSet(
            const AIWorldFactId factId) const noexcept
        {
            const bool bIsValidFact = IsFactValid_(factId);
            assert(bIsValidFact);

            if (!bIsValidFact)
            {
                return false;
            }

            return facts_.test(factId.index);
        }

        void SetFact(
            const AIWorldFactId factId,
            const bool bValue = true) noexcept
        {
            const bool bIsValidFact = IsFactValid_(factId);
            assert(bIsValidFact);

            if (!bIsValidFact)
            {
                return;
            }

            facts_.set(factId.index, bValue);
        }

        void ClearFact(const AIWorldFactId factId) noexcept
        {
            SetFact(factId, false);
        }

        void Clear() noexcept
        {
            facts_.reset();
        }
        
        friend bool operator==(
                    const AIAgentWorldState&,
                    const AIAgentWorldState&) noexcept = default;

    private:
        [[nodiscard]] static constexpr bool IsFactValid_(
            const AIWorldFactId factId) noexcept
        {
            return factId.index < FactCapacity;
        }

        std::bitset<FactCapacity> facts_{};
    };
    
    struct AIAgentWorldStateHash
    {
        [[nodiscard]] std::size_t operator()(
            const AIAgentWorldState& state) const noexcept
        {
            // FNV-1a over every logical fact keeps hashing independent of the
            // bitset representation and covers the complete state identity.
            std::size_t hash = sizeof(std::size_t) == 8u
                ? static_cast<std::size_t>(14695981039346656037ull)
                : static_cast<std::size_t>(2166136261u);
            constexpr std::size_t prime = sizeof(std::size_t) == 8u
                ? static_cast<std::size_t>(1099511628211ull)
                : static_cast<std::size_t>(16777619u);

            for (std::size_t index = 0u; index < AIAgentWorldState::FactCapacity; ++index)
            {
                hash ^= state.IsFactSet(
                    AIWorldFactId{ static_cast<AIWorldFactId::ValueType>(index) })
                    ? 1u
                    : 0u;
                hash *= prime;
            }

            return hash;
        }
    };
}