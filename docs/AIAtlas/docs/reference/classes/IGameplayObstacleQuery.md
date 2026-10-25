# IGameplayObstacleQuery

**class** · домен «Obstacle avoidance» · `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:79–100` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Интерфейс геометрических запросов avoidance: Probe (sweep) и ProbeSupport (опора; по умолчанию синтетическая).

## Архитектурный разбор

**Владелец и время жизни.** Реализация GameplayPhysicsObstacleQuery принадлежит App (AppPhysicsState); runtime хранит невладеющий указатель.

**Ссылки.** [теория: 15-obstacle-avoidance](../../chapters/15-obstacle-avoidance.md) · [схема d15-pipeline](../diagrams.md#d15-pipeline) · [OA-QUERY](../source-references.md#oa-query) · [OA-SUPPORT-DEFAULT](../source-references.md#oa-support-default) · [OA-PHYS-QUERY](../source-references.md#oa-phys-query)

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 82 | метод | public | `virtual ~IGameplayObstacleQuery() = default;` |
| 84 | метод | public | `[[nodiscard]] virtual bool Probe(` |
| 89 | метод | public | `[[nodiscard]] virtual bool ProbeSupport(` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:79–100`.

```cpp linenums="79"
    class IGameplayObstacleQuery
    {
    public:
        virtual ~IGameplayObstacleQuery() = default;

        [[nodiscard]] virtual bool Probe(
            const GameplayObstacleProbeRequest& request,
            GameplayObstacleProbeHit& hit) const noexcept = 0;

        // Queries may retain the legacy, support-unaware behavior by not overriding this.
        [[nodiscard]] virtual bool ProbeSupport(
        const GameplaySupportProbeRequest& request,
         GameplaySupportProbeHit& hit) const noexcept
        {
            hit = {
                .distance = 0.0f,
                .position = request.origin,
                .normal = {0.0f, 1.0f, 0.0f}
            };
            return true;
        }
    };
```
