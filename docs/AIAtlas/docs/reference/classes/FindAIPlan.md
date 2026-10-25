# FindAIPlan

**function** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIPlanner.cppm:141–141` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Uniform-cost search плана от наблюдаемого состояния к цели; без эвристики A*.

## Архитектурный разбор

**Владелец и время жизни.** Свободная функция без состояния; узлы поиска — локальные (копия состояния и шагов).

**Runtime-методы, входы и выходы.** (initialState, goal, actions) → optional<AIPlan>; пустой план, если цель уже выполнена; nullopt при невалидном входе или недостижимости.

**Кто вызывает и в какой фазе.** AIDecisionRuntime::Update (смена цели, инвалидация, нет/terminal исполнения), AIDecisionRuntime::Plan.

**Что вызывает сама сущность.** IsInputValid, AreFactConditionsSatisfied, AreNumericConditionsSatisfied, ApplyFactEffects, ApplyNumericEffects.

**Ошибки, terminal states, отмена.** Нет лимита раскрытий и бюджета; при бесконечном достижимом пространстве и недостижимой цели не завершится.

**Ссылки.** [теория: 05-planner](../../chapters/05-planner.md) · [теория: 02-goap-strips](../../chapters/02-goap-strips.md) · [схема d05-search-loop](../diagrams.md#d05-search-loop) · [схема g05-access-key-map](../diagrams.md#g05-access-key-map) · [PL-FIND](../source-references.md#pl-find) · [PL-INPUT](../source-references.md#pl-input) · [PL-RELAX](../source-references.md#pl-relax) · [PL-STALE](../source-references.md#pl-stale) · тест `TestAIPlanner.cpp`

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIPlanner.cppm:141–141`.

```cpp linenums="141"
    [[nodiscard]] std::optional<AIPlan> FindAIPlan(
```
