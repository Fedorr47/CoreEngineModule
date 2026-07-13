module;

#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>

export module core:ai_agent_world_state;

export namespace rendern
{
    struct AIWorldFactId
    {
        std::uint16_t index{};

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

    private:
        [[nodiscard]] static constexpr bool IsFactValid_(
            const AIWorldFactId factId) noexcept
        {
            return factId.index < FactCapacity;
        }

        std::bitset<FactCapacity> facts_{};
    };
}