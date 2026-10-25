# FindWeightedGameplayRoute

**function** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRouteSearch.cppm:94–94` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Дейкстра по явному ориентированному графу с восстановлением пути по предшественникам.

## Архитектурный разбор

**Кто вызывает и в какой фазе.** MoveToCapability (route costs при создании instance), AIMoveToAction::CreateRuntime (маршрут шага).

**Ошибки, terminal states, отмена.** InvalidGraph, InvalidRequest, NoRoute; start == goal → маршрут из одной точки.

**Ссылки.** [теория: 11-spatial-routes](../../chapters/11-spatial-routes.md) · [RS-FIND](../source-references.md#rs-find) · [RS-SAME](../source-references.md#rs-same) · тест `TestGameplayRouteSearch.cpp`

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRouteSearch.cppm:94–94`.

```cpp linenums="94"
    [[nodiscard]] GameplayRouteSearchResult FindWeightedGameplayRoute(
```
