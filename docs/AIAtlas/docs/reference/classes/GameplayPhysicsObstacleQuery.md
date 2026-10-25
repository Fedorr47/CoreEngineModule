# GameplayPhysicsObstacleQuery

**class** · домен «Интеграция App» · `src/App/GameplayPhysicsObstacleQuery.h:15–34` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public rendern::IGameplayObstacleQuery`

> Адаптер Jolt для avoidance: sweep сферы / луч и луч вниз для опоры; маска StaticWorld по умолчанию.

## Архитектурный разбор

**Ссылки.** [теория: 15-obstacle-avoidance](../../chapters/15-obstacle-avoidance.md) · [схема d15-pipeline](../diagrams.md#d15-pipeline) · [OA-PHYS-QUERY](../source-references.md#oa-phys-query) · [OA-PHYS-PROBE](../source-references.md#oa-phys-probe) · [OA-PHYS-SUPPORT](../source-references.md#oa-phys-support)

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 18 | конструктор | public | `explicit GameplayPhysicsObstacleQuery(` |
| 23 | метод | public | `[[nodiscard]] bool Probe(` |
| 27 | метод | public | `[[nodiscard]] bool ProbeSupport(` |
| 32 | поле | private | `const physics::JoltPhysicsWorld& physicsWorld_;` |
| 33 | поле | private | `physics::PhysicsQueryLayerMask layerMask_;` |

## Объявление

Включено из исходника при сборке; `src/App/GameplayPhysicsObstacleQuery.h:15–34`.

```cpp linenums="15"
    class GameplayPhysicsObstacleQuery final : public rendern::IGameplayObstacleQuery
    {
    public:
        explicit GameplayPhysicsObstacleQuery(
            const physics::JoltPhysicsWorld& physicsWorld,
            physics::PhysicsQueryLayerMask layerMask =
                physics::PhysicsQueryLayerMask::StaticWorld) noexcept;

        [[nodiscard]] bool Probe(
            const rendern::GameplayObstacleProbeRequest& request,
            rendern::GameplayObstacleProbeHit& hit) const noexcept override;

        [[nodiscard]] bool ProbeSupport(
            const rendern::GameplaySupportProbeRequest& request,
            rendern::GameplaySupportProbeHit& hit) const noexcept override;

    private:
        const physics::JoltPhysicsWorld& physicsWorld_;
        physics::PhysicsQueryLayerMask layerMask_;
    };
```
