module;

#include <cstdint>
#include <limits>

export module core:ai_action_contracts;

export namespace rendern
{
    namespace details
    {
        struct AIActionIdTag
        {
        };

        struct AIActionContextIdTag
        {
        };
    }

    template <typename TTag>
    struct AIId
    {
        using ValueType = std::uint16_t;

        static constexpr ValueType InvalidValue =
            std::numeric_limits<ValueType>::max();

        ValueType value{ InvalidValue };

        constexpr AIId() noexcept = default;

        explicit constexpr AIId(const ValueType inValue) noexcept
            : value{ inValue }
        {
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return value != InvalidValue;
        }

        friend constexpr bool operator==(
            const AIId&,
            const AIId&) noexcept = default;
    };

    using AIActionId = AIId<details::AIActionIdTag>;
    // Distinguishes semantic invocations of the same action (for example MoveTo
    // against two different targets) without creating scenario-specific actions.
    using AIActionContextId = AIId<details::AIActionContextIdTag>;

    enum class AIActionExecutionStatus : std::uint8_t
    {
        NotStarted,
        Running,
        Succeeded,
        Failed,
        Cancelled
    };
}