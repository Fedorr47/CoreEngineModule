# GameplayRouteFollower

**class** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRouteFollower.cppm:41–336` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Автомат следования по ломаной: arrival промежуточных/финальной точек, look-ahead, плоскость перехода, traversal.

## Архитектурный разбор

**Владелец и время жизни.** Член AIFollowRouteActionRuntime; владеет маршрутом.

**Runtime-методы, входы и выходы.** Start, Tick(position) → намерение/статус, CompleteTraversal(handle), Reset.

**Ссылки.** [теория: 12-route-follower](../../chapters/12-route-follower.md) · [схема d12-tick-flow](../diagrams.md#d12-tick-flow) · [схема d12-follower-states](../diagrams.md#d12-follower-states) · [схема g12-lookahead](../diagrams.md#g12-lookahead) · [схема g12-corner-plane](../diagrams.md#g12-corner-plane) · [RF-TICK](../source-references.md#rf-tick) · [RF-LOOKAHEAD](../source-references.md#rf-lookahead) · [RF-PASSED](../source-references.md#rf-passed) · [RF-COMPLETE](../source-references.md#rf-complete) · тест `TestGameplayRouteFollower.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 46 | метод | public | `[[nodiscard]] GameplayRouteFollowerStatus Start(` |
| 82 | метод | public | `[[nodiscard]] GameplayRouteFollowerOutput Tick(` |
| 179 | метод | public | `[[nodiscard]] bool CompleteTraversal(` |
| 203 | метод | public | `void Reset() noexcept` |
| 212 | метод | public | `[[nodiscard]] GameplayRouteFollowerStatus GetStatus() const noexcept` |
| 218 | метод | private | `[[nodiscard]] bool TryBuildLookAheadTarget_(` |
| 261 | метод | private | `[[nodiscard]] bool HasPassedCurrentCorner_(` |
| 321 | метод | private | `[[nodiscard]] std::optional<GameplayTraversalLinkHandle> GetCurrentTraversalLink() const noexcept` |
| 331 | поле | private | `GameplayArrivalSteeringSettings steeringSettings_{};` |
| 332 | поле | private | `GameplayRouteFollowerSettings followerSettings_{};` |
| 333 | поле | private | `GameplayRouteFollowerStatus status_{GameplayRouteFollowerStatus::NotStarted};` |
| 334 | поле | private | `GameplayRoute route_{};` |
| 335 | поле | private | `std::size_t currentSegmentIndex_{0u};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRouteFollower.cppm:41–101`.

```cpp linenums="41"
    class GameplayRouteFollower
    {
    public:
        // The supplied route is owned by value. points[0] is the route start
        // anchor; the follower never prepends a segment from currentPosition.
        [[nodiscard]] GameplayRouteFollowerStatus Start(
            GameplayRoute route,
            const GameplayArrivalSteeringSettings& steeringSettings = {},
            const GameplayRouteFollowerSettings& followerSettings = {})
        {
            if (status_ != GameplayRouteFollowerStatus::NotStarted)
            {
                return status_;
            }

            route_ = std::move(route);
            steeringSettings_ = steeringSettings;
            followerSettings_ = followerSettings;
            if (!std::isfinite(followerSettings_.cornerLookAheadDistance) ||
                followerSettings_.cornerLookAheadDistance < 0.0f)
            {
                followerSettings_.cornerLookAheadDistance = 0.0f;
            }
            currentSegmentIndex_ = 0u;

            if (!route_.IsValid())
            {
                status_ = GameplayRouteFollowerStatus::InvalidRoute;
                return status_;
            }

            if (route_.segmentAnnotations.empty())
            {
                status_ = GameplayRouteFollowerStatus::Succeeded;
                return status_;
            }

            status_ = GameplayRouteFollowerStatus::Following;
            return status_;
        }

        [[nodiscard]] GameplayRouteFollowerOutput Tick(
            const mathUtils::Vec3& currentPosition)
        {
            GameplayRouteFollowerOutput output{};
            output.status = status_;

            if (status_ == GameplayRouteFollowerStatus::TraversalRequired)
            {
                output.requiredTraversalLink = GetCurrentTraversalLink();
                return output;
            }

            if (status_ != GameplayRouteFollowerStatus::Following)
            {
                return output;
            }

            bool bAdvancedByLookAhead = false;

            while (currentSegmentIndex_ < route_.segmentAnnotations.size())
```
