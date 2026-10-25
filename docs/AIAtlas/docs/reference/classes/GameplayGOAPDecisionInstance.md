# GameplayGOAPDecisionInstance

**class** · домен «Решение (instance)» · `src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm:18–143` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Instance решения одного агента: контекст наблюдений, реакции, буферы событий и GOAP-обёртка.

## Архитектурный разбор

**Владелец и время жизни.** unique_ptr в GameplayRuntime::activeAIDecisions_. Порядок полей: goap_ уничтожается первым, контекст — последним. Копирование/перемещение запрещены.

**Runtime-методы, входы и выходы.** Update(aiSystem, observation): два прохода наблюдений и реакций вокруг GOAP-update; Cancel; GetStatus; интерфейс инспекции.

**Кто вызывает и в какой фазе.** GameplayRuntime::UpdateActiveAIDecisions_ (по возрастанию handle), StartAIDecision (первый update).

**Ошибки, terminal states, отмена.** Неполные bindings → configured_ = false → CreateGameplayGOAPDecision возвращает nullptr.

**Ссылки.** [теория: 07-observations-events](../../chapters/07-observations-events.md) · [теория: 09-execution-lifetime](../../chapters/09-execution-lifetime.md) · [схема d07-instance-update](../diagrams.md#d07-instance-update) · [схема d09-ownership](../diagrams.md#d09-ownership) · [GG-INSTANCE](../source-references.md#gg-instance) · [GG-INSTANCE-UPDATE](../source-references.md#gg-instance-update) · [GG-FIELD-ORDER](../source-references.md#gg-field-order) · [GG-CREATE](../source-references.md#gg-create) · тест `TestGameplayAIAssetComposition.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 23 | конструктор | public | `GameplayGOAPDecisionInstance(const EntityHandle agent, GameplayGOAPDecisionSetup setup)` |
| 24 | метод | public | `: context_(std::move(setup.context)), reactions_(std::move(setup.eventReactions)),` |
| 25 | метод | public | `goap_(agent, std::move(setup.definition))` |
| 49 | конструктор | public | `GameplayGOAPDecisionInstance(const GameplayGOAPDecisionInstance&) = delete;` |
| 50 | поле | public | `GameplayGOAPDecisionInstance& operator=(const GameplayGOAPDecisionInstance&) = delete;` |
| 51 | конструктор | public | `GameplayGOAPDecisionInstance(GameplayGOAPDecisionInstance&&) = delete;` |
| 52 | поле | public | `GameplayGOAPDecisionInstance& operator=(GameplayGOAPDecisionInstance&&) = delete;` |
| 54 | метод | public | `void Update(AISystem& aiSystem, const GameplayAIObservationContext& observation) override` |
| 78 | метод | public | `void Cancel(AISystem& aiSystem) noexcept override` |
| 83 | метод | public | `[[nodiscard]] GameplayAIDecisionStatus GetStatus() const noexcept override` |
| 106 | метод | public | `[[nodiscard]] AIPlanExecutionStatus GetGOAPStatus() const noexcept override` |
| 111 | метод | public | `[[nodiscard]] const AIAgentWorldState& GetObservedState() const noexcept override` |
| 116 | метод | public | `[[nodiscard]] AIDebugViewModel BuildDebugViewModel() const override` |
| 121 | метод | public | `[[nodiscard]] bool IsConfigured() const noexcept` |
| 127 | метод | private | `void ReactToEvents_(const GameplayWorld& world, std::span<const GameplayWorldEvent> events)` |
| 137 | поле | private | `std::unique_ptr<IGameplayGOAPContext> context_;` |
| 138 | поле | private | `std::vector<std::unique_ptr<IGameplayGOAPEventReaction>> reactions_;` |
| 139 | поле | private | `std::vector<GameplayWorldEvent> reactionEvents_{};` |
| 140 | поле | private | `std::vector<GameplayWorldEvent> runtimeEvents_{};` |
| 141 | поле | private | `GameplayGOAPDecision goap_;` |
| 142 | поле | private | `bool configured_{};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm:18–78`.

```cpp linenums="18"
    class GameplayGOAPDecisionInstance :
        public GameplayAIDecisionInstance,
        public IGameplayGOAPInspection
    {
    public:
        GameplayGOAPDecisionInstance(const EntityHandle agent, GameplayGOAPDecisionSetup setup)
            : context_(std::move(setup.context)), reactions_(std::move(setup.eventReactions)),
              goap_(agent, std::move(setup.definition))
        {
            if (!context_)
            {
                return;
            }
            for (const auto& reaction : reactions_)
            {
                if (!reaction)
                {
                    return;
                }
            }
            for (const GameplayGOAPActionBindingSetup& binding : setup.actionBindings)
            {
                if (!binding.create || !goap_.InstallActionBinding(binding.actionId,
                    binding.create(goap_.GetObservedState(), runtimeEvents_)))
                {
                    return;
                }
            }
            configured_ = goap_.HasCompleteActionBindings();
        }

        GameplayGOAPDecisionInstance(const GameplayGOAPDecisionInstance&) = delete;
        GameplayGOAPDecisionInstance& operator=(const GameplayGOAPDecisionInstance&) = delete;
        GameplayGOAPDecisionInstance(GameplayGOAPDecisionInstance&&) = delete;
        GameplayGOAPDecisionInstance& operator=(GameplayGOAPDecisionInstance&&) = delete;

        void Update(AISystem& aiSystem, const GameplayAIObservationContext& observation) override
        {
            if (!configured_)
            {
                return;
            }
            reactionEvents_.clear();
            // Input and output may alias. Finish all input reads before forwarding events.
            context_->Observe(observation.world, observation.events, goap_.GetObservedState());
            ReactToEvents_(observation.world, observation.events);
            runtimeEvents_.clear();
            goap_.Update(aiSystem, observation.world);
            context_->ObserveActionEvents(
                observation.world, runtimeEvents_, goap_.GetObservedState());
            ReactToEvents_(observation.world, runtimeEvents_);
            if (observation.eventOutput != nullptr)
            {
                observation.eventOutput->insert(observation.eventOutput->end(),
                    runtimeEvents_.begin(), runtimeEvents_.end());
                observation.eventOutput->insert(observation.eventOutput->end(),
                    reactionEvents_.begin(), reactionEvents_.end());
            }
        }

        void Cancel(AISystem& aiSystem) noexcept override
```
