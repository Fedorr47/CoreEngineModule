#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <type_traits>

import core;

using namespace rendern;

namespace
{
    struct TestMoveRequest
    {
        int destinationId{};

        friend constexpr bool operator==(
            const TestMoveRequest&,
            const TestMoveRequest&) = default;
    };

    struct TestCraftRequest
    {
        int recipeId{};
    };

    class RecordingGameplayRequestBoundary final
        : public IGameplayRequestBoundary<TestMoveRequest>
    {
    public:
        explicit RecordingGameplayRequestBoundary(bool* destroyedFlag = nullptr) noexcept
            : destroyedFlag_{ destroyedFlag }
        {
        }

        ~RecordingGameplayRequestBoundary() override
        {
            if (destroyedFlag_ != nullptr)
            {
                *destroyedFlag_ = true;
            }
        }

        std::optional<GameplayRequestHandle> submitResult{ GameplayRequestHandle{ 7u } };
        std::optional<GameplayRequestStatus> statusResult{ GameplayRequestStatus::Running };
        bool cancelResult{ true };
        GameplayRequestContext lastSubmitContext{};
        TestMoveRequest lastSubmitRequest{};
        mutable GameplayRequestHandle lastStatusHandle{};
        GameplayRequestHandle lastCancelHandle{};
        int submitCallCount{ 0 };
        mutable int statusCallCount{ 0 };
        int cancelCallCount{ 0 };

        [[nodiscard]] std::optional<GameplayRequestHandle> Submit(
            const GameplayRequestContext& context,
            const TestMoveRequest& request) override
        {
            ++submitCallCount;
            lastSubmitContext = context;
            lastSubmitRequest = request;
            return submitResult;
        }

        [[nodiscard]] std::optional<GameplayRequestStatus> TryGetStatus(
            const GameplayRequestHandle handle) const override
        {
            ++statusCallCount;
            lastStatusHandle = handle;
            return statusResult;
        }

        [[nodiscard]] bool Cancel(const GameplayRequestHandle handle) override
        {
            ++cancelCallCount;
            lastCancelHandle = handle;
            return cancelResult;
        }

    private:
        bool* destroyedFlag_{ nullptr };
    };
}

static_assert(!std::is_convertible_v<GameplayRequestHandle::ValueType, GameplayRequestHandle>);
static_assert(!std::is_same_v<
    IGameplayRequestBoundary<TestMoveRequest>,
    IGameplayRequestBoundary<TestCraftRequest>>);
static_assert(!std::is_convertible_v<
    IGameplayRequestBoundary<TestCraftRequest>*,
    IGameplayRequestBoundary<TestMoveRequest>*>);
static_assert(!std::is_assignable_v<
    IGameplayRequestBoundary<TestMoveRequest>&,
    IGameplayRequestBoundary<TestCraftRequest>&>);
static_assert(std::has_virtual_destructor_v<IGameplayRequestBoundary<TestMoveRequest>>);

// Protects safe default construction so an unassigned request handle cannot
// accidentally refer to a live gameplay request after future storage is added.
TEST(GameplayRequest, DefaultRequestHandleIsInvalid)
{
    constexpr GameplayRequestHandle defaultHandle{};
    constexpr GameplayRequestHandle zeroHandle{ 0u };
    constexpr GameplayRequestHandle matchingHandle{ 42u };
    constexpr GameplayRequestHandle sameMatchingHandle{ 42u };
    constexpr GameplayRequestHandle differentHandle{ 43u };

    EXPECT_FALSE(defaultHandle.IsValid());
    EXPECT_TRUE(zeroHandle.IsValid());
    EXPECT_EQ(matchingHandle, sameMatchingHandle);
    EXPECT_NE(matchingHandle, differentHandle);
}

// Protects the ownership boundary so requests cannot identify kNullEntity as a
// valid requester, preventing future subsystems from accepting ownerless work.
TEST(GameplayRequest, DefaultRequestContextIsInvalid)
{
    constexpr GameplayRequestContext defaultContext{};
    constexpr GameplayRequestContext validContext{ .requesterEntity = 123u };
    constexpr GameplayRequestContext matchingContext{ .requesterEntity = 123u };
    constexpr GameplayRequestContext differentContext{ .requesterEntity = 124u };

    EXPECT_FALSE(defaultContext.IsValid());
    EXPECT_TRUE(validContext.IsValid());
    EXPECT_EQ(validContext, matchingContext);
    EXPECT_NE(validContext, differentContext);
}

// Protects lifecycle interpretation shared by future action runtimes, avoiding
// regressions where active statuses are treated as completed operations.
TEST(GameplayRequest, RequestTerminalStatusClassificationIsComplete)
{
    EXPECT_FALSE(IsGameplayRequestTerminal(GameplayRequestStatus::Pending));
    EXPECT_FALSE(IsGameplayRequestTerminal(GameplayRequestStatus::Running));
    EXPECT_TRUE(IsGameplayRequestTerminal(GameplayRequestStatus::Succeeded));
    EXPECT_TRUE(IsGameplayRequestTerminal(GameplayRequestStatus::Failed));
    EXPECT_TRUE(IsGameplayRequestTerminal(GameplayRequestStatus::Cancelled));
}

// Protects typed dispatch and forwarding so future gameplay subsystems receive
// the original requester context and payload rather than an erased container.
TEST(GameplayRequest, TypedBoundarySubmitsContextAndPayload)
{
    RecordingGameplayRequestBoundary boundary;
    IGameplayRequestBoundary<TestMoveRequest>& interface = boundary;
    const GameplayRequestContext context{ .requesterEntity = 55u };
    const TestMoveRequest request{ .destinationId = 99 };
    const GameplayRequestHandle acceptedHandle{ 12u };
    boundary.submitResult = acceptedHandle;

    const std::optional<GameplayRequestHandle> result = interface.Submit(context, request);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, acceptedHandle);
    EXPECT_TRUE(result->IsValid());
    EXPECT_EQ(boundary.lastSubmitContext, context);
    EXPECT_EQ(boundary.lastSubmitRequest, request);
    EXPECT_EQ(boundary.submitCallCount, 1);
}

// Protects the rejection contract so callers observe absence instead of a
// fabricated invalid handle that could be mistaken for accepted work.
TEST(GameplayRequest, SubmissionCanBeRejectedWithoutFabricatingHandle)
{
    RecordingGameplayRequestBoundary boundary;
    IGameplayRequestBoundary<TestMoveRequest>& interface = boundary;
    boundary.submitResult = std::nullopt;

    const std::optional<GameplayRequestHandle> result = interface.Submit(
        GameplayRequestContext{ .requesterEntity = 56u },
        TestMoveRequest{ .destinationId = 100 });

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(boundary.submitCallCount, 1);
}

// Protects status polling so known request handles are forwarded unchanged and
// can report active as well as terminal lifecycle states deterministically.
TEST(GameplayRequest, StatusLookupReturnsKnownStatus)
{
    RecordingGameplayRequestBoundary boundary;
    IGameplayRequestBoundary<TestMoveRequest>& interface = boundary;
    const GameplayRequestHandle handle{ 44u };

    boundary.statusResult = GameplayRequestStatus::Pending;
    EXPECT_EQ(interface.TryGetStatus(handle), GameplayRequestStatus::Pending);
    boundary.statusResult = GameplayRequestStatus::Running;
    EXPECT_EQ(interface.TryGetStatus(handle), GameplayRequestStatus::Running);
    boundary.statusResult = GameplayRequestStatus::Succeeded;
    EXPECT_EQ(interface.TryGetStatus(handle), GameplayRequestStatus::Succeeded);

    EXPECT_EQ(boundary.lastStatusHandle, handle);
    EXPECT_EQ(boundary.statusCallCount, 3);
}

// Protects the distinction between unknown and failed requests so future
// runtimes do not convert missing handles into fabricated gameplay failures.
TEST(GameplayRequest, StatusLookupCanReportUnknownHandle)
{
    RecordingGameplayRequestBoundary boundary;
    IGameplayRequestBoundary<TestMoveRequest>& interface = boundary;
    boundary.statusResult = std::nullopt;

    const std::optional<GameplayRequestStatus> result =
        interface.TryGetStatus(GameplayRequestHandle{ 45u });

    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.value_or(GameplayRequestStatus::Cancelled), GameplayRequestStatus::Failed);
    EXPECT_EQ(boundary.statusCallCount, 1);
}

// Protects explicit cancellation dispatch so future subsystems, not AI tasks,
// decide whether a known request can accept interruption.
TEST(GameplayRequest, CancelForwardsHandleAndReturnsDecision)
{
    RecordingGameplayRequestBoundary boundary;
    IGameplayRequestBoundary<TestMoveRequest>& interface = boundary;
    const GameplayRequestHandle handle{ 46u };
    boundary.cancelResult = true;

    EXPECT_TRUE(interface.Cancel(handle));
    EXPECT_EQ(boundary.lastCancelHandle, handle);
    EXPECT_EQ(boundary.cancelCallCount, 1);

    boundary.cancelResult = false;
    EXPECT_FALSE(interface.Cancel(handle));
    EXPECT_EQ(boundary.cancelCallCount, 2);
}

// Protects compile-time payload separation so unrelated gameplay request types
// cannot be accidentally exchanged through one universal request boundary.
TEST(GameplayRequest, RequestBoundaryRemainsPayloadTyped)
{
    EXPECT_FALSE((std::is_same_v<
        IGameplayRequestBoundary<TestMoveRequest>,
        IGameplayRequestBoundary<TestCraftRequest>>));
    EXPECT_FALSE((std::is_convertible_v<
        IGameplayRequestBoundary<TestCraftRequest>*,
        IGameplayRequestBoundary<TestMoveRequest>*>));
}

// Protects polymorphic ownership safety so future concrete gameplay services
// can be destroyed through their specialized typed request interface.
TEST(GameplayRequest, BoundaryCanBeDestroyedThroughTypedInterface)
{
    bool destroyed = false;
    {
        std::unique_ptr<IGameplayRequestBoundary<TestMoveRequest>> boundary =
            std::make_unique<RecordingGameplayRequestBoundary>(&destroyed);
    }

    EXPECT_TRUE(destroyed);
}