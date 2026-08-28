module;

#include <cstdint>
#include <limits>
#include <optional>

export module core:gameplay_request;

import :EnTTHelpers;

export namespace rendern
{
    using EnTT_helpers::EntityHandle;
    using EnTT_helpers::kNullEntity;
    
    struct GameplayRequestHandle
    {
        using ValueType = std::uint64_t;
        
        static constexpr ValueType InvalidValue = 
            std::numeric_limits<ValueType>::max();
        
        ValueType value{InvalidValue};
        
        constexpr GameplayRequestHandle() noexcept = default;
        
        explicit constexpr GameplayRequestHandle(const ValueType inValue) noexcept 
            : value(inValue) {}
        
        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return value != InvalidValue;
        }
        
        friend constexpr bool operator==(
            const GameplayRequestHandle&, 
            const GameplayRequestHandle&) noexcept = default;
    };
    
    struct GameplayRequestContext
    {
        EntityHandle requesterEntity{kNullEntity};
        
        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return requesterEntity != kNullEntity;
        }
        
        friend constexpr bool operator==(
            const GameplayRequestContext&,
            const GameplayRequestContext&) noexcept = default;
    };
    
    enum class GameplayRequestStatus : std::uint8_t
    {
        Pending,
        Running,
        Succeeded,
        Failed,
        Cancelled
    };
    
    [[nodiscard]] constexpr bool IsGameplayRequestTerminal(
        const GameplayRequestStatus status) noexcept
    {
        switch (status)
        {
            case GameplayRequestStatus::Pending: [[fallthrough]];
            case GameplayRequestStatus::Running:
                return false;
            case GameplayRequestStatus::Succeeded: [[fallthrough]];
            case GameplayRequestStatus::Failed: [[fallthrough]];
            case GameplayRequestStatus::Cancelled:
                return true;
            default:
                return true;
        }
    }
    
    template <typename TRequestt>
    class IGameplayRequestBoundary
    {
    public:
        virtual ~IGameplayRequestBoundary() = default;
        
        [[nodiscard]] virtual std::optional<GameplayRequestHandle> Submit(
            const GameplayRequestContext& context,
            const TRequestt& request) = 0;
        
        [[nodiscard]] virtual std::optional<GameplayRequestStatus> TryGetStatus(
            GameplayRequestHandle requestHandle) const = 0;
        
        [[nodiscard]] virtual bool Cancel(
            GameplayRequestHandle requestHandle) = 0;
    };
}