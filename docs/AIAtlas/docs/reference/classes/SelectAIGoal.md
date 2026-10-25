# SelectAIGoal

**function** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIGoalSelection.cppm:44–44` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Выбор невыполненной цели с максимальной оценкой; при равенстве — первая.

## Архитектурный разбор

**Runtime-методы, входы и выходы.** (state, candidates) → AIGoalSelectionResult; оценка = baseScore + Σ scoreDelta выполненных правил.

**Кто вызывает и в какой фазе.** AIDecisionRuntime::Update каждый update.

**Ошибки, terminal states, отмена.** Невалидные цели и неконечные оценки пропускаются; «нет цели» → решение Succeeded.

**Ссылки.** [теория: 04-goals-actions](../../chapters/04-goals-actions.md) · [схема d04-goal-selection](../diagrams.md#d04-goal-selection) · [GS-SELECT](../source-references.md#gs-select) · [GS-TIE](../source-references.md#gs-tie) · [AU-GOAL-BUILD](../source-references.md#au-goal-build) · тест `TestAIGoalSelection.cpp`

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIGoalSelection.cppm:44–44`.

```cpp linenums="44"
    [[nodiscard]] AIGoalSelectionResult SelectAIGoal(
```
