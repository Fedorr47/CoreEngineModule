# ApplyGameplayObstacleAvoidance

**function** · домен «Obstacle avoidance» · `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:125–125` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Локальная коррекция намерения: три sweep-пробы, escape-кандидаты, опора, выбор стороны с памятью.

## Архитектурный разбор

**Кто вызывает и в какой фазе.** ApplyGameplayAgentObstacleAvoidance из runtimes маршрута и Follow/Flee при наличии query.

**Ошибки, terminal states, отмена.** Нет опоры у обоих кандидатов → нулевая команда.

**Ссылки.** [теория: 15-obstacle-avoidance](../../chapters/15-obstacle-avoidance.md) · [схема g15-feelers](../diagrams.md#g15-feelers) · [схема g15-support](../diagrams.md#g15-support) · [схема d15-side-selection](../diagrams.md#d15-side-selection) · [OA-APPLY](../source-references.md#oa-apply) · [OA-ESCAPE](../source-references.md#oa-escape) · [OA-SUPPORT](../source-references.md#oa-support) · [OA-SIDE](../source-references.md#oa-side) · тест `TestGameplayObstacleAvoidance.cpp`

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:125–125`.

```cpp linenums="125"
    [[nodiscard]] GameplayMovementIntent ApplyGameplayObstacleAvoidance(
```
