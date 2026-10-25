# Демонстрация: пошаговый GOAP-поиск

!!! warning "Учебная модель"
    Это JavaScript-модель контракта `FindAIPlan` (глава [05](../chapters/05-planner.md)): uniform-cost search,
    порядок действий `(actionId, contextId)`, равные стоимости — по порядку вставки (`seq`), отсечение
    `known ≤ nextCost`, проверка цели при извлечении. **Не** запуск движка: числа в float64 (в движке — float32),
    нет наблюдений, мира, исполнения и кадров. Определения и стоимости — из реальных ассетов
    (`assets/ai/goap/*.goap.json`, route costs Access Key посчитаны по графу и позициям уровня при сборке атласа).

<div class="demo" data-demo="goap" markdown="0">
  <div class="demo-grid">
    <div>
      <label>Определение <select data-demo-domain></select></label>
      <label>Стартовое состояние <select data-demo-preset></select></label>
      <p class="demo-note">Факты стартового состояния можно менять — поиск сбросится и пересчитается.</p>
      <div data-demo-facts style="columns: 2; font-size: .72rem"></div>
      <p>
        <button type="button" data-demo-action="step">Шаг</button>
        <button type="button" data-demo-action="run">До конца</button>
        <button type="button" data-demo-action="reset">Сброс</button>
      </p>
      <p data-demo-summary></p>
      <details><summary>Стоимости действий</summary>
        <table><thead><tr><th>Действие</th><th>Стоимость</th></tr></thead><tbody data-demo-costs></tbody></table>
      </details>
    </div>
    <div class="demo-out" data-demo-output></div>
  </div>
</div>

## Что попробовать

1. **Access Key, «Старт сценария», «До конца»** — план `start_to_coin_c → coin_c_to_coin_a →
   coin_a_to_access_key_shop → buy_key → access_key_shop_to_goal`, стоимость 42.0854, 22 раскрытых состояния
   (тот же результат даёт `tools/goap_model.py`, проверка в `tools/verify_examples.py`).
2. **Шаги 1–3**: первым извлекается старт, в очередь попадают три монеты (6.09, 6.72, 7.82); вторым — монета B,
   магазин отклонён причиной `coins >= 2 (сейчас 1)`.
3. **«У монеты C, монета A исчезла»** — перепланирование: `coin_c_to_coin_b → …`, 40.0852.
4. **«Нет доступных монет»** — очередь пустеет, «плана нет»: так решение получает `Failed`.
5. **Target Recovery**: снимите `goalAvailable` — плана нет; отметьте `atDestination` — пустой план.
6. Отметьте в Access Key `atStart` и `coinBCollected` вручную — увидите, как поиск обходит уже «собранную» монету.

## Отличия от runtime

| Модель | CoreEngine |
|---|---|
| поиск запускается кнопкой | `AIDecisionRuntime::Update` при смене цели/инвалидации/отсутствии плана |
| факты вводятся вручную | факты пишут наблюдения (глава [07](../chapters/07-observations-events.md)) |
| float64 | `float` стоимости |
| нет проверки `continuation ⊆ preconditions` | `IsInputValid` отвергает такие определения |
| только первая цель определения | цель выбирает `SelectAIGoal` |
