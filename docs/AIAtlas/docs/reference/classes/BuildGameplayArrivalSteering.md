# BuildGameplayArrivalSteering

**function** · домен «Steering» · `src/Gameplay/Navigation/GameplaySteering.cppm:110–110` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Arrival в XZ: нулевая команда в r + 0.001, линейная шкала до R с нижней границей m_min.

## Архитектурный разбор

**Кто вызывает и в какой фазе.** GameplayRouteFollower, Door/Jump executors.

**Ссылки.** [теория: 13-steering](../../chapters/13-steering.md) · [схема g13-arrival-profile](../diagrams.md#g13-arrival-profile) · [ST-ARRIVAL](../source-references.md#st-arrival) · тест `TestGameplaySteering.cpp`

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplaySteering.cppm:110–110`.

```cpp linenums="110"
    [[nodiscard]] GameplaySteeringOutput BuildGameplayArrivalSteering(
```
