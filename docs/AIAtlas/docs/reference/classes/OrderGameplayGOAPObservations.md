# OrderGameplayGOAPObservations

**function** · домен «Композиция» · `src/Gameplay/AI/Composition/GameplayGOAPObservationOrder.cppm:18–18` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Проверка единственного writer и топологический порядок наблюдений по метаданным входов/выходов.

## Архитектурный разбор

**Кто вызывает и в какой фазе.** CreateGameplayGOAPDecisionFromTemplate.

**Ошибки, terminal states, отмена.** «unknown fact or multiple observation writers», «fact has no observation writer», «cyclic or unresolved observation dependencies».

**Ссылки.** [теория: 07-observations-events](../../chapters/07-observations-events.md) · [схема d07-observation-deps](../diagrams.md#d07-observation-deps) · [CO-ORDER](../source-references.md#co-order) · [CO-ORDER-PICK](../source-references.md#co-order-pick) · тест `TestGameplayAIAssetComposition.cpp`

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/GameplayGOAPObservationOrder.cppm:18–18`.

```cpp linenums="18"
    [[nodiscard]] std::vector<std::unique_ptr<IGameplayGOAPObservation>> OrderGameplayGOAPObservations(
```
