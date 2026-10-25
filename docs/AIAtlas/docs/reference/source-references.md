# Реестр источников

Каждая ссылка вида `[ID]` в атласе ведёт сюда. Строки **вычислены при сборке** инструментом `tools/references.py`: для каждой записи найдено n-е вхождение ожидаемого текста в файле текущего рабочего дерева. Отсутствующий текст ломает сборку, поэтому номера строк соответствуют снимку `Deferred` @ `8c6c0dc37ab3` на момент сборки. Колонка «состояние» показывает, был ли файл изменён относительно HEAD (`working-tree`) — для таких файлов ссылка GitHub не является доказательством.

Ссылка «файл» открывает исходник из рабочего дерева рядом с атласом (работает из `file://`), «GitHub» — дополнительная навигация к тому же коммиту.

## Ядро GOAP и выбор цели

#### DC-ACTION {#dc-action}

`src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:76` · Определение действия: условия, эффекты, стоимость, continuation · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionContracts.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionContracts.cppm#L76">GitHub</a>

Якорь: `` struct AIActionDefinition ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="76"
        struct AIActionDefinition
        {
            AIActionId actionId{};
            std::vector<AIFactCondition> preconditions{};
            std::vector<AIFactEffect> effects{};
            AIActionContextId contextId{};
            float baseCost{ 1.0f };
            std::vector<AINumericCondition> numericPreconditions{};
    ```

#### DC-ACTION-STATUS {#dc-action-status}

`src/Gameplay/AI/Runtime/AIActionContracts.cppm:53` · Статус исполнения действия · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/AIActionContracts.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/AIActionContracts.cppm#L53">GitHub</a>

Якорь: `` enum class AIActionExecutionStatus ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="53"
        enum class AIActionExecutionStatus : std::uint8_t
        {
            NotStarted,
            Running,
            Succeeded,
            Failed,
            Cancelled
        };
    ```

#### DC-AIID {#dc-aiid}

`src/Gameplay/AI/Runtime/AIActionContracts.cppm:22` · Шаблон сильного ID действия/контекста · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/AIActionContracts.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/AIActionContracts.cppm#L22">GitHub</a>

Якорь: `` struct AIId ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="22"
        struct AIId
        {
            using ValueType = std::uint16_t;

            static constexpr ValueType InvalidValue =
                std::numeric_limits<ValueType>::max();

            ValueType value{ InvalidValue };
    ```

#### DC-FACT-COND {#dc-fact-cond}

`src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:24` · Булево условие · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionContracts.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionContracts.cppm#L24">GitHub</a>

Якорь: `` struct AIFactCondition ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="24"
        struct AIFactCondition
        {
            AIWorldFactId factId{};
            bool bExpectedValue{ true };

            friend constexpr bool operator==(const AIFactCondition&, const AIFactCondition&) noexcept = default;
        };

    ```

#### DC-GOAL {#dc-goal}

`src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:70` · Цель: только булевы desiredFacts · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionContracts.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionContracts.cppm#L70">GitHub</a>

Якорь: `` struct AIGoalDefinition ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="70"
        struct AIGoalDefinition
        {
            AIGoalId goalId{};
            std::vector<AIFactCondition> desiredFacts{};
        };

        struct AIActionDefinition
        {
    ```

#### DC-NUM-EFFECT-OP {#dc-num-effect-op}

`src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:57` · Set / Add · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionContracts.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionContracts.cppm#L57">GitHub</a>

Якорь: `` enum class AINumericEffectOperation ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="57"
        enum class AINumericEffectOperation : std::uint8_t
        {
            Set,
            Add
        };

        struct AINumericEffect
        {
    ```

#### DC-NUM-OP {#dc-num-op}

`src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:40` · Операторы числовых условий · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionContracts.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionContracts.cppm#L40">GitHub</a>

Якорь: `` enum class AINumericConditionOperator ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="40"
        enum class AINumericConditionOperator : std::uint8_t
        {
            Equal,
            NotEqual,
            Less,
            LessOrEqual,
            Greater,
            GreaterOrEqual
    ```

#### DC-PLAN {#dc-plan}

`src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:91` · Шаг плана и план · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionContracts.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionContracts.cppm#L91">GitHub</a>

Якорь: `` struct AIPlanStep ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="91"
        struct AIPlanStep
        {
            AIActionId actionId{};
            AIActionContextId contextId{};

            friend constexpr bool operator==(const AIPlanStep&, const AIPlanStep&) noexcept = default;
        };

    ```

#### GS-CANDIDATE {#gs-candidate}

`src/Gameplay/AI/GOAP/AIGoalSelectionContracts.cppm:17` · Кандидат цели: baseScore + scoreRules · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIGoalSelectionContracts.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIGoalSelectionContracts.cppm#L17">GitHub</a>

Якорь: `` struct AIGoalSelectionCandidate ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="17"
        struct AIGoalSelectionCandidate
        {
            AIGoalDefinition goal{};
            float baseScore{};
            std::vector<AIGoalScoreRule> scoreRules{};
        };

    }
    ```

#### GS-SCORE {#gs-score}

`src/Gameplay/AI/GOAP/AIGoalSelection.cppm:27` · Оценка цели · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIGoalSelection.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIGoalSelection.cppm#L27">GitHub</a>

Якорь: `` float ScoreAIGoal( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="27"
        [[nodiscard]] float ScoreAIGoal(
            const AIAgentWorldState& worldState,
            const AIGoalSelectionCandidate& candidate) noexcept
        {
            float score = candidate.baseScore;
            for (const AIGoalScoreRule& rule : candidate.scoreRules)
            {
                if (rule.condition.factId.index < AIAgentWorldState::FactCapacity
    ```

#### GS-SELECT {#gs-select}

`src/Gameplay/AI/GOAP/AIGoalSelection.cppm:44` · Выбор цели · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIGoalSelection.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIGoalSelection.cppm#L44">GitHub</a>

Якорь: `` AIGoalSelectionResult SelectAIGoal( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="44"
        [[nodiscard]] AIGoalSelectionResult SelectAIGoal(
            const AIAgentWorldState& worldState,
            const std::span<const AIGoalSelectionCandidate> candidates) noexcept
        {
            AIGoalSelectionResult selectedGoal{};
            for (const AIGoalSelectionCandidate& candidate : candidates)
            {
                const bool bValidGoal = candidate.goal.goalId.IsValid()
    ```

#### GS-TIE {#gs-tie}

`src/Gameplay/AI/GOAP/AIGoalSelection.cppm:74` · Строгое сравнение: при равенстве первая цель · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIGoalSelection.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIGoalSelection.cppm#L74">GitHub</a>

Якорь: `` if (!selectedGoal.IsValid() || score > selectedGoal.score) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="74"
                if (!selectedGoal.IsValid() || score > selectedGoal.score)
                {
                    selectedGoal = AIGoalSelectionResult{
                        .goalId=candidate.goal.goalId,
                        .score=score};
                }
            }
            return selectedGoal;
    ```

#### PL-CONT-SUBSET {#pl-cont-subset}

`src/Gameplay/AI/GOAP/AIPlanner.cppm:87` · continuation ⊆ preconditions · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanner.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanner.cppm#L87">GitHub</a>

Якорь: `` || !std::ranges::all_of(action.continuationConditions, ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="87"
                       || !std::ranges::all_of(action.continuationConditions,
                           [&](const AIFactCondition& continuation)
                           {
                               return IsFactIdValid(continuation.factId)
                                   && std::ranges::find(action.preconditions, continuation)
                                       != action.preconditions.end();
                           })
                       || !std::ranges::all_of(action.numericContinuationConditions,
    ```

#### PL-EMPTY-PLAN {#pl-empty-plan}

`src/Gameplay/AI/GOAP/AIPlanner.cppm:152` · Пустой план для уже выполненной цели · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanner.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanner.cppm#L152">GitHub</a>

Якорь: `` return AIPlan{ goal.goalId, {} }; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="152"
                return AIPlan{ goal.goalId, {} };
            }

            std::vector<const AIActionDefinition*> orderedActions;
            orderedActions.reserve(actions.size());
            for (const AIActionDefinition& action : actions)
            {
                orderedActions.push_back(&action);
    ```

#### PL-FIND {#pl-find}

`src/Gameplay/AI/GOAP/AIPlanner.cppm:141` · Uniform-cost поиск плана · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanner.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanner.cppm#L141">GitHub</a>

Якорь: `` std::optional<AIPlan> FindAIPlan( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="141"
        [[nodiscard]] std::optional<AIPlan> FindAIPlan(
            const AIAgentWorldState& initialState,
            const AIGoalDefinition& goal,
            const std::span<const AIActionDefinition> actions)
        {
            if (!IsInputValid(goal, actions))
            {
                return std::nullopt;
    ```

#### PL-GOALTEST {#pl-goaltest}

`src/Gameplay/AI/GOAP/AIPlanner.cppm:181` · Проверка цели при извлечении · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanner.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanner.cppm#L181">GitHub</a>

Якорь: `` if (AreFactConditionsSatisfied(current.state, goal.desiredFacts)) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="181"
                if (AreFactConditionsSatisfied(current.state, goal.desiredFacts))
                {
                    return AIPlan{ goal.goalId, std::move(current.steps) };
                }

                for (const AIActionDefinition* action : orderedActions)
                {
                    if (!AreFactConditionsSatisfied(current.state, action->preconditions))
    ```

#### PL-INPUT {#pl-input}

`src/Gameplay/AI/GOAP/AIPlanner.cppm:50` · Валидация входа планировщика · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanner.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanner.cppm#L50">GitHub</a>

Якорь: `` bool IsInputValid( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="50"
            [[nodiscard]] bool IsInputValid(
                const AIGoalDefinition& goal,
                const std::span<const AIActionDefinition> actions)
            {
                if (!goal.goalId.IsValid()
                    || !std::ranges::all_of(goal.desiredFacts, [](const AIFactCondition& condition)
                    {
                        return IsFactIdValid(condition.factId);
    ```

#### PL-NODE {#pl-node}

`src/Gameplay/AI/GOAP/AIPlanner.cppm:117` · Узел поиска: состояние, стоимость, sequence, копия шагов · состояние: `HEAD` · вхождение 1 из 2 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanner.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanner.cppm#L117">GitHub</a>

Якорь: `` struct SearchNode ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="117"
            struct SearchNode
            {
                AIAgentWorldState state{};
                float cost{};
                std::uint64_t sequence{};
                std::vector<AIPlanStep> steps{};
            };

    ```

#### PL-ORDER {#pl-order}

`src/Gameplay/AI/GOAP/AIPlanner.cppm:125` · Порядок очереди: cost, затем sequence · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanner.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanner.cppm#L125">GitHub</a>

Якорь: `` struct SearchNodeGreater ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="125"
            struct SearchNodeGreater
            {
                [[nodiscard]] bool operator()(const SearchNode& left, const SearchNode& right) const noexcept
                {
                    if (left.cost != right.cost)
                    {
                        return left.cost > right.cost;
                    }
    ```

#### PL-RELAX {#pl-relax}

`src/Gameplay/AI/GOAP/AIPlanner.cppm:210` · Отсечение не лучшего пути к состоянию · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanner.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanner.cppm#L210">GitHub</a>

Якорь: `` if (known != bestCosts.end() && known->second <= nextCost) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="210"
                    if (known != bestCosts.end() && known->second <= nextCost)
                    {
                        continue;
                    }

                    std::vector<AIPlanStep> nextSteps = current.steps;
                    nextSteps.push_back(AIPlanStep{action->actionId, action->contextId });
                    bestCosts.insert_or_assign(nextState, nextCost);
    ```

#### PL-SORT {#pl-sort}

`src/Gameplay/AI/GOAP/AIPlanner.cppm:161` · Сортировка действий по (actionId, contextId) · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanner.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanner.cppm#L161">GitHub</a>

Якорь: `` std::ranges::sort(orderedActions, {}, [](const AIActionDefinition* action) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="161"
            std::ranges::sort(orderedActions, {}, [](const AIActionDefinition* action)
            {
                return std::pair{action->actionId.value, action->contextId.value};
            });

            std::priority_queue<SearchNode, std::vector<SearchNode>, SearchNodeGreater> frontier;
            std::unordered_map<AIAgentWorldState, float, AIAgentWorldStateHash> bestCosts;
            std::uint64_t nextSequence = 0u;
    ```

#### PL-STALE {#pl-stale}

`src/Gameplay/AI/GOAP/AIPlanner.cppm:177` · Пропуск устаревших записей · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanner.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanner.cppm#L177">GitHub</a>

Якорь: `` if (currentBest == bestCosts.end() || current.cost != currentBest->second) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="177"
                if (currentBest == bestCosts.end() || current.cost != currentBest->second)
                {
                    continue;
                }
                if (AreFactConditionsSatisfied(current.state, goal.desiredFacts))
                {
                    return AIPlan{ goal.goalId, std::move(current.steps) };
                }
    ```

#### PL-UNIQUE-KEYS {#pl-unique-keys}

`src/Gameplay/AI/GOAP/AIPlanner.cppm:114` · Уникальность (actionId, contextId) · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanner.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanner.cppm#L114">GitHub</a>

Якорь: `` return std::ranges::adjacent_find(actionKeys) == actionKeys.end(); ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="114"
                return std::ranges::adjacent_find(actionKeys) == actionKeys.end();
            }

            struct SearchNode
            {
                AIAgentWorldState state{};
                float cost{};
                std::uint64_t sequence{};
    ```

#### SO-APPLY-FACTS {#so-apply-facts}

`src/Gameplay/AI/GOAP/AIStateOperations.cppm:36` · Применение bool-эффектов · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIStateOperations.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIStateOperations.cppm#L36">GitHub</a>

Якорь: `` void ApplyFactEffects( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="36"
        void ApplyFactEffects(
            AIAgentWorldState& worldState,
            std::span<const AIFactEffect> effects)
        {
            for (const AIFactEffect& effect : effects)
            {
                worldState.SetFact(effect.factId, effect.bValue);
            }
    ```

#### SO-APPLY-NUM {#so-apply-num}

`src/Gameplay/AI/GOAP/AIStateOperations.cppm:99` · Числовые эффекты на копии с контролем переполнения · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIStateOperations.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIStateOperations.cppm#L99">GitHub</a>

Якорь: `` bool ApplyNumericEffects( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="99"
        [[nodiscard]] bool ApplyNumericEffects(
            AIAgentWorldState& worldState,
            std::span<const AINumericEffect> effects) noexcept
        {
            AIAgentWorldState result = worldState;
            for (const AINumericEffect& effect : effects)
            {
                switch (effect.operation)
    ```

#### SO-EVAL-FACT {#so-eval-fact}

`src/Gameplay/AI/GOAP/AIStateOperations.cppm:14` · Проверка bool-условия · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIStateOperations.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIStateOperations.cppm#L14">GitHub</a>

Якорь: `` bool EvaluateFactCondition( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="14"
        [[nodiscard]] bool EvaluateFactCondition(
            const AIAgentWorldState& worldState,
            const AIFactCondition& condition) noexcept
        {
            return worldState.IsFactSet(condition.factId) == condition.bExpectedValue;
        }

        [[nodiscard]] bool AreFactConditionsSatisfied(
    ```

#### SO-EVAL-NUM {#so-eval-num}

`src/Gameplay/AI/GOAP/AIStateOperations.cppm:46` · Сравнение через <=> · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIStateOperations.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIStateOperations.cppm#L46">GitHub</a>

Якорь: `` constexpr bool EvaluateNumericCondition( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="46"
        [[nodiscard]] constexpr bool EvaluateNumericCondition(
        const std::int32_t actual,
        const AINumericConditionOperator op,
        const std::int32_t expected) noexcept
        {
            const auto ordering = actual <=> expected;

            switch (op)
    ```

#### SO-OVERFLOW {#so-overflow}

`src/Gameplay/AI/GOAP/AIStateOperations.cppm:114` · Проверка переполнения Add · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIStateOperations.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIStateOperations.cppm#L114">GitHub</a>

Якорь: `` if ((effect.value > 0 && current > std::numeric_limits<std::int32_t>::max() - effect.value) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="114"
                        if ((effect.value > 0 && current > std::numeric_limits<std::int32_t>::max() - effect.value)
                            || (effect.value < 0 && current < std::numeric_limits<std::int32_t>::min() - effect.value))
                        {
                            return false;
                        }
                        result.SetIntegerFact(effect.factId, current + effect.value);
                        break;
                    }
    ```

#### WS-CAPACITY {#ws-capacity}

`src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:45` · Ёмкость: 128 bool · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIAgentWorldState.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIAgentWorldState.cppm#L45">GitHub</a>

Якорь: `` static constexpr std::size_t FactCapacity = 128u; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="45"
            static constexpr std::size_t FactCapacity = 128u;
            static constexpr std::size_t IntegerFactCapacity = 128u;

            [[nodiscard]] bool IsFactSet(
                const AIWorldFactId factId) const noexcept
            {
                const bool bIsValidFact = IsFactValid_(factId);
                assert(bIsValidFact);
    ```

#### WS-CLASS {#ws-class}

`src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:42` · Контейнер фактов агента · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIAgentWorldState.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIAgentWorldState.cppm#L42">GitHub</a>

Якорь: `` class AIAgentWorldState ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="42"
        class AIAgentWorldState
        {
        public:
            static constexpr std::size_t FactCapacity = 128u;
            static constexpr std::size_t IntegerFactCapacity = 128u;

            [[nodiscard]] bool IsFactSet(
                const AIWorldFactId factId) const noexcept
    ```

#### WS-EQUAL {#ws-equal}

`src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:117` · Равенство по полному состоянию · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIAgentWorldState.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIAgentWorldState.cppm#L117">GitHub</a>

Якорь: `` friend bool operator==( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="117"
            friend bool operator==(
                const AIAgentWorldState& left,
                const AIAgentWorldState& right) noexcept
            {
                return left.facts_ == right.facts_
                    && left.integerFacts_ == right.integerFacts_;
            }

    ```

#### WS-FACT-ID {#ws-fact-id}

`src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:14` · Сильный тип ID булева факта · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIAgentWorldState.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIAgentWorldState.cppm#L14">GitHub</a>

Якорь: `` struct AIWorldFactId ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="14"
        struct AIWorldFactId
        {
            using ValueType = std::uint16_t;

            static constexpr ValueType InvalidIndex =
                std::numeric_limits<ValueType>::max();

            ValueType index{ InvalidIndex };
    ```

#### WS-HASH {#ws-hash}

`src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:142` · FNV-1a хеш полного состояния · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIAgentWorldState.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIAgentWorldState.cppm#L142">GitHub</a>

Якорь: `` struct AIAgentWorldStateHash ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="142"
        struct AIAgentWorldStateHash
        {
            [[nodiscard]] std::size_t operator()(
                const AIAgentWorldState& state) const noexcept
            {
                // FNV-1a over every logical fact keeps hashing independent of the
                // bitset representation and covers the complete state identity.
                std::size_t hash = sizeof(std::size_t) == 8u
    ```

#### WS-INT-ID {#ws-int-id}

`src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:28` · Сильный тип ID целочисленного факта · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIAgentWorldState.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIAgentWorldState.cppm#L28">GitHub</a>

Якорь: `` struct AIWorldIntegerFactId ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="28"
        struct AIWorldIntegerFactId
        {
            using ValueType = std::uint16_t;

            static constexpr ValueType InvalidIndex =
                std::numeric_limits<ValueType>::max();

            ValueType index{ InvalidIndex };
    ```

#### WS-SETFACT {#ws-setfact}

`src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:62` · Запись bool-факта с проверкой индекса · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIAgentWorldState.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIAgentWorldState.cppm#L62">GitHub</a>

Якорь: `` void SetFact( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="62"
            void SetFact(
                const AIWorldFactId factId,
                const bool bValue = true) noexcept
            {
                const bool bIsValidFact = IsFactValid_(factId);
                assert(bIsValidFact);

                if (!bIsValidFact)
    ```

#### WS-STORAGE {#ws-storage}

`src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:138` · Хранение: bitset + массив int32 · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIAgentWorldState.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIAgentWorldState.cppm#L138">GitHub</a>

Якорь: `` std::bitset<FactCapacity> facts_{}; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="138"
            std::bitset<FactCapacity> facts_{};
            std::array<std::int32_t, IntegerFactCapacity> integerFacts_{};
        };

        struct AIAgentWorldStateHash
        {
            [[nodiscard]] std::size_t operator()(
                const AIAgentWorldState& state) const noexcept
    ```

## Исполнение плана и decision runtime

#### BR-CANCEL {#br-cancel}

`src/Gameplay/AI/GOAP/AIPlanExecutionBridge.cppm:101` · Отмена исполнения · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanExecutionBridge.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanExecutionBridge.cppm#L101">GitHub</a>

Якорь: `` static void CancelPlanExecution( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="101"
            static void CancelPlanExecution(
                AIPlanExecution& execution,
                AISystem& aiSystem,
                const EntityHandle agentEntity) noexcept
            {
                if (execution.IsRunningStep())
                {
                    const AIPlanStep* step = execution.GetCurrentStep();
    ```

#### BR-START {#br-start}

`src/Gameplay/AI/GOAP/AIPlanExecutionBridge.cppm:19` · Старт шага: binding → runtime → AISystem · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanExecutionBridge.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanExecutionBridge.cppm#L19">GitHub</a>

Якорь: `` static AIPlanExecutionStatus StartReadyPlanStep( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="19"
            [[nodiscard]] static AIPlanExecutionStatus StartReadyPlanStep(
                AIPlanExecution& execution,
                const AIActionBindingRegistry& bindings,
                AISystem& aiSystem,
                const GameplayWorld& world,
                const EntityHandle agentEntity)
            {
                if (!execution.IsReadyToStartStep())
    ```

#### BR-SYNC {#br-sync}

`src/Gameplay/AI/GOAP/AIPlanExecutionBridge.cppm:76` · Синхронизация шага с AISystem · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanExecutionBridge.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanExecutionBridge.cppm#L76">GitHub</a>

Якорь: `` static AIPlanExecutionStatus SynchronizeRunningPlanStep( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="76"
            [[nodiscard]] static AIPlanExecutionStatus SynchronizeRunningPlanStep(
                AIPlanExecution& execution,
                const AISystem& aiSystem,
                const EntityHandle agentEntity) noexcept
            {
                if (!execution.IsRunningStep())
                {
                    return execution.GetStatus();
    ```

#### DR-CANCEL {#dr-cancel}

`src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm:168` · Явная отмена · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm#L168">GitHub</a>

Якорь: `` void Cancel(AISystem& aiSystem) noexcept ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="168"
            void Cancel(AISystem& aiSystem) noexcept
            {
                bExplicitlyCancelled_ = true;
                if (execution_.has_value())
                {
                    AIPlanExecutionBridge::CancelPlanExecution(*execution_, aiSystem, agentEntity_);
                    status_ = execution_->GetStatus();
                    return;
    ```

#### DR-CLASS {#dr-class}

`src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm:22` · Monitored decision loop одного агента · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm#L22">GitHub</a>

Якорь: `` class AIDecisionRuntime ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="22"
        class AIDecisionRuntime
        {
        public:
            explicit AIDecisionRuntime(const EntityHandle agentEntity) noexcept
                : agentEntity_(agentEntity)
            {
            }

    ```

#### DR-CONTINUATION {#dr-continuation}

`src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm:225` · Проверка continuation текущего шага · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm#L225">GitHub</a>

Якорь: `` bool IsRunningStepContinuationValid_( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="225"
            [[nodiscard]] bool IsRunningStepContinuationValid_(
                const AIAgentWorldState& observedState,
                const std::span<const AIActionDefinition> actions) const
            {
                if (!execution_.has_value() || !execution_->IsRunningStep())
                {
                    return false;
                }
    ```

#### DR-INSTALL {#dr-install}

`src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm:291` · Установка нового плана · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm#L291">GitHub</a>

Якорь: `` void InstallPlan_(AIPlan plan, AISystem& aiSystem) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="291"
            void InstallPlan_(AIPlan plan, AISystem& aiSystem)
            {
                CancelActiveExecution_(aiSystem);
                execution_.emplace(std::move(plan));
                status_ = execution_->Start();
            }

            void CancelActiveExecution_(AISystem& aiSystem) noexcept
    ```

#### DR-INVALIDATED {#dr-invalidated}

`src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm:92` · Invalidation только при прежней цели · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm#L92">GitHub</a>

Якорь: `` const bool bPlanInvalidated = !bGoalChanged ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="92"
                const bool bPlanInvalidated = !bGoalChanged
                    && execution_.has_value()
                    && ((execution_->IsReadyToStartStep()
                    && !IsRemainingPlanValid_(observedState, *selectedGoal, actions))
                    || (execution_->IsRunningStep()
                    && !IsRunningStepContinuationValid_(observedState, actions)));

                if (!bGoalChanged && !bPlanInvalidated && HasValidExecution_())
    ```

#### DR-KEEP-RUNNING {#dr-keep-running}

`src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm:118` · Сохранение старого исполнения при смене цели · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm#L118">GitHub</a>

Якорь: `` // A goal change must not destroy an otherwise valid running plan. ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="118"
                        // A goal change must not destroy an otherwise valid running plan.
                        if (!bPlanInvalidated && HasValidExecution_())
                        {
                            if (bSynchronizedRunningExecution)
                            {
                                return status_;
                            }

    ```

#### DR-NO-GOAL {#dr-no-goal}

`src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm:75` · Нет цели → отмена и Succeeded · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm#L75">GitHub</a>

Якорь: `` if (selectedGoal == nullptr) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="75"
                if (selectedGoal == nullptr)
                {
                    CancelActiveExecution_(aiSystem);
                    execution_.reset();
                    selectedGoalId_.reset();
                    status_ = AIPlanExecutionStatus::Succeeded;
                    return status_;
                }
    ```

#### DR-PLAN {#dr-plan}

`src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm:30` · Явное планирование (снимает explicit cancel) · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm#L30">GitHub</a>

Якорь: `` bool Plan( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="30"
            [[nodiscard]] bool Plan(
                const AIAgentWorldState& observedState,
                const AIGoalDefinition& goal,
                const std::span<const AIActionDefinition> actions,
                AISystem& aiSystem)
            {
                std::optional<AIPlan> plan = FindAIPlan(observedState, goal, actions);
                if (!plan.has_value())
    ```

#### DR-REMAINING {#dr-remaining}

`src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm:252` · Прогон остатка плана · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm#L252">GitHub</a>

Якорь: `` bool IsRemainingPlanValid_( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="252"
            [[nodiscard]] bool IsRemainingPlanValid_(
                const AIAgentWorldState& observedState,
                const AIGoalDefinition& goal,
                const std::span<const AIActionDefinition> actions) const
            {
                if (!execution_.has_value())
                {
                    return false;
    ```

#### DR-UPDATE {#dr-update}

`src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm:54` · Основной update решения · состояние: `HEAD` · вхождение 1 из 2 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm#L54">GitHub</a>

Якорь: `` AIPlanExecutionStatus Update( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="54"
            [[nodiscard]] AIPlanExecutionStatus Update(
                const AIAgentWorldState& observedState,
                const std::span<const AIGoalSelectionCandidate> candidates,
                const std::span<const AIActionDefinition> actions,
                const AIActionBindingRegistry& bindings,
                AISystem& aiSystem,
                const GameplayWorld& world)
            {
    ```

#### DR-UPDATE-STEP {#dr-update-step}

`src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm:144` · Старт/синхронизация текущего шага · состояние: `HEAD` · вхождение 2 из 2 · <a href="../../../../src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm#L144">GitHub</a>

Якорь: `` AIPlanExecutionStatus Update( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="144"
            [[nodiscard]] AIPlanExecutionStatus Update(
                const AIActionBindingRegistry& bindings,
                AISystem& aiSystem,
                const GameplayWorld& world)
            {
                if (!execution_.has_value() || execution_->IsTerminal())
                {
                    return status_;
    ```

#### PE-APPLY {#pe-apply}

`src/Gameplay/AI/GOAP/AIPlanExecution.cppm:98` · Переходы по статусу действия · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanExecution.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanExecution.cppm#L98">GitHub</a>

Якорь: `` AIPlanExecutionStatus ApplyCurrentStepStatus( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="98"
            AIPlanExecutionStatus ApplyCurrentStepStatus(const AIActionExecutionStatus actionStatus) noexcept
            {
                if (status_ != AIPlanExecutionStatus::RunningStep)
                {
                    return status_;
                }

                switch (actionStatus)
    ```

#### PE-CLASS {#pe-class}

`src/Gameplay/AI/GOAP/AIPlanExecution.cppm:28` · Машина состояний плана · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanExecution.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanExecution.cppm#L28">GitHub</a>

Якорь: `` explicit AIPlanExecution(AIPlan plan) noexcept ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="28"
            explicit AIPlanExecution(AIPlan plan) noexcept
                : plan_(std::move(plan))
            {
                if (!plan_.goalId.IsValid()
                    || !std::ranges::all_of(plan_.steps, [](const AIPlanStep& step)
                    {
                        return step.actionId.IsValid();
                    }))
    ```

#### PE-STATUS {#pe-status}

`src/Gameplay/AI/GOAP/AIPlanExecution.cppm:15` · Состояния исполнения плана · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/AIPlanExecution.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/AIPlanExecution.cppm#L15">GitHub</a>

Якорь: `` enum class AIPlanExecutionStatus ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="15"
        enum class AIPlanExecutionStatus : std::uint8_t
        {
            NotStarted,
            ReadyToStartStep,
            RunningStep,
            Succeeded,
            Failed,
            Cancelled
    ```

## Action runtime и AISystem

#### AB-IFACE {#ab-iface}

`src/Gameplay/AI/Runtime/AIActionBinding.cppm:14` · Фабрика runtime для шага · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/AIActionBinding.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/AIActionBinding.cppm#L14">GitHub</a>

Якорь: `` class IAIActionBinding ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="14"
        class IAIActionBinding
        {
        public:
            virtual ~IAIActionBinding() = default;

            [[nodiscard]] virtual std::unique_ptr<IAIActionRuntime> CreateRuntime(
                const AIActionRuntimeContext& context) = 0;
        };
    ```

#### AB-REGISTRY {#ab-registry}

`src/Gameplay/AI/Runtime/AIActionBinding.cppm:24` · Невладеющий реестр bindings по actionId · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/AIActionBinding.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/AIActionBinding.cppm#L24">GitHub</a>

Якорь: `` class AIActionBindingRegistry ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="24"
        class AIActionBindingRegistry
        {
        public:
            [[nodiscard]] bool Register(const AIActionId actionId, IAIActionBinding& binding) noexcept
            {
                if (!actionId.IsValid())
                {
                    return false;
    ```

#### AR-CONTEXT {#ar-context}

`src/Gameplay/AI/Runtime/AIActionRuntime.cppm:15` · Контекст вызова действия · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/AIActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/AIActionRuntime.cppm#L15">GitHub</a>

Якорь: `` struct AIActionRuntimeContext ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="15"
        struct AIActionRuntimeContext
        {
            EntityHandle agentEntity{kNullEntity};
            AIActionId actionId{};
            AIActionContextId contextId{};

            [[nodiscard]] constexpr bool IsValid() const noexcept
            {
    ```

#### AR-IFACE {#ar-iface}

`src/Gameplay/AI/Runtime/AIActionRuntime.cppm:41` · Интерфейс Start/Tick/Cancel · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/AIActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/AIActionRuntime.cppm#L41">GitHub</a>

Якорь: `` class IAIActionRuntime ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="41"
        class IAIActionRuntime
        {
        public:
            virtual ~IAIActionRuntime() = default;

            // Starts one action invocation for the supplied agent/action pair. The
            // caller owns lifecycle ordering and must observe only Running,
            // Succeeded, or Failed as valid start results; NotStarted is reserved
    ```

#### AR-RESULT {#ar-result}

`src/Gameplay/AI/Runtime/AIActionRuntime.cppm:34` · Running/Succeeded/Failed · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/AIActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/AIActionRuntime.cppm#L34">GitHub</a>

Якорь: `` enum class AIActionRuntimeResult ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="34"
        enum class AIActionRuntimeResult : std::uint8_t
        {
            Running,
            Succeeded,
            Failed
        };

        class IAIActionRuntime
    ```

#### AS-CLASS {#as-class}

`src/Gameplay/AI/Runtime/AISystem.cppm:17` · Одна задача на агента · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/AISystem.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/AISystem.cppm#L17">GitHub</a>

Якорь: `` class AISystem ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="17"
        class AISystem
        {
        public:
            // Runs the synchronous AI update boundary, advances active actions, and
            // returns the number of agents that remained valid for processing.
            std::size_t Update(GameplayWorld& world, const float deltaSeconds = 0.0f)
            {
                world.CollectAIEntities(aiEntitiesScratch_);
    ```

#### AS-CLEANUP {#as-cleanup}

`src/Gameplay/AI/Runtime/AISystem.cppm:138` · Удаление задач мёртвых агентов · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/AISystem.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/AISystem.cppm#L138">GitHub</a>

Якорь: `` void CleanupInactiveTasks_(GameplayWorld& world) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="138"
            void CleanupInactiveTasks_(GameplayWorld& world)
            {
                for (auto it = tasksByEntity_.begin(); it != tasksByEntity_.end();)
                {
                    const EntityHandle entity = it->first;
                    const bool bHasLiveAgent = world.IsEntityValid(entity) && world.HasAI(entity);
                    if (!bHasLiveAgent)
                    {
    ```

#### AS-START {#as-start}

`src/Gameplay/AI/Runtime/AISystem.cppm:50` · Синхронный старт задачи (заменяет прежнюю) · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/AISystem.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/AISystem.cppm#L50">GitHub</a>

Якорь: `` AIActionExecutionStatus StartAction( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="50"
            [[nodiscard]] AIActionExecutionStatus StartAction(
                const GameplayWorld& world,
                const AIActionRuntimeContext context,
                std::unique_ptr<IAIActionRuntime> runtime)
            {
                const bool bHasValidContext = context.IsValid();
                const bool bHasRuntime = runtime != nullptr;
                const bool bHasValidEntity = bHasValidContext && world.IsEntityValid(context.agentEntity);
    ```

#### AS-UPDATE {#as-update}

`src/Gameplay/AI/Runtime/AISystem.cppm:22` · Тик активных задач · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/AISystem.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/AISystem.cppm#L22">GitHub</a>

Якорь: `` std::size_t Update(GameplayWorld& world, const float deltaSeconds = 0.0f) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="22"
            std::size_t Update(GameplayWorld& world, const float deltaSeconds = 0.0f)
            {
                world.CollectAIEntities(aiEntitiesScratch_);
                CleanupInactiveTasks_(world);

                for (const EntityHandle entity : aiEntitiesScratch_)
                {
                    auto taskIt = tasksByEntity_.find(entity);
    ```

#### AT-CANCEL {#at-cancel}

`src/Gameplay/AI/Runtime/AIActionTask.cppm:75` · Cancel только для Running · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/AIActionTask.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/AIActionTask.cppm#L75">GitHub</a>

Якорь: `` void Cancel() noexcept ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="75"
            void Cancel() noexcept
            {
                if (status_ != AIActionExecutionStatus::Running || runtime_ == nullptr)
                {
                    return;
                }

                runtime_->Cancel(context_);
    ```

#### AT-CLASS {#at-class}

`src/Gameplay/AI/Runtime/AIActionTask.cppm:14` · Владеющая обёртка runtime со статусом · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/AIActionTask.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/AIActionTask.cppm#L14">GitHub</a>

Якорь: `` class AIActionTask ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="14"
        class AIActionTask
        {
        public:
            AIActionTask(
                AIActionRuntimeContext context,
                std::unique_ptr<IAIActionRuntime> runtime) noexcept
            : context_(context)
            , runtime_(std::move(runtime))
    ```

## Решение, instance и отладка

#### GD-CREATION {#gd-creation}

`src/Gameplay/AI/Runtime/GameplayAIDecisionCreationContext.cppm:21` · Сервисы создания решения · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/GameplayAIDecisionCreationContext.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/GameplayAIDecisionCreationContext.cppm#L21">GitHub</a>

Якорь: `` struct GameplayAIDecisionCreationContext ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="21"
        struct GameplayAIDecisionCreationContext
        {
            EntityHandle agent{kNullEntity};
            LevelAsset& level;
            GameplayWorld& world;
            const GameplayTraversalLinkRegistry& traversalLinkRegistry;
            const GameplayTraversalExecutorRegistry& traversalExecutorRegistry;
            GameplayObjectReservationSystem& reservationSystem;
    ```

#### GD-FACTORY {#gd-factory}

`src/Gameplay/AI/Runtime/GameplayAIDecision.cppm:23` · Реестр фабрик решений · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/GameplayAIDecision.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/GameplayAIDecision.cppm#L23">GitHub</a>

Якорь: `` class GameplayAIDecisionFactoryRegistry ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="23"
        class GameplayAIDecisionFactoryRegistry
        {
        public:
            [[nodiscard]] bool Register(
                const std::string_view definitionId, GameplayAIDecisionFactory factory)
            {
                if (definitionId.empty() || !factory)
                {
    ```

#### GD-INSTANCE-IFACE {#gd-instance-iface}

`src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm:31` · Интерфейс решения · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm#L31">GitHub</a>

Якорь: `` class GameplayAIDecisionInstance ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="31"
        class GameplayAIDecisionInstance
        {
        public:
            virtual ~GameplayAIDecisionInstance() = default;
            virtual void Update(AISystem& aiSystem, const GameplayAIObservationContext& observation) = 0;
            virtual void Cancel(AISystem& aiSystem) noexcept = 0;
            [[nodiscard]] virtual GameplayAIDecisionStatus GetStatus() const noexcept = 0;
        };
    ```

#### GD-OBS-CONTEXT {#gd-obs-context}

`src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm:24` · Мир + события + выходной буфер · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm#L24">GitHub</a>

Якорь: `` struct GameplayAIObservationContext ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="24"
        struct GameplayAIObservationContext
        {
            const GameplayWorld& world;
            std::span<const GameplayWorldEvent> events;
            std::vector<GameplayWorldEvent>* eventOutput{};
        };

        class GameplayAIDecisionInstance
    ```

#### GG-CREATE {#gg-create}

`src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm:177` · Создание instance (+path inspection) · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm#L177">GitHub</a>

Якорь: `` std::unique_ptr<GameplayAIDecisionInstance> CreateGameplayGOAPDecision( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="177"
        [[nodiscard]] std::unique_ptr<GameplayAIDecisionInstance> CreateGameplayGOAPDecision(
            const EntityHandle agent, GameplayGOAPDecisionSetup setup)
        {
            const auto* paths = dynamic_cast<const IGameplayGOAPPlannedPathProvider*>(setup.context.get());
            std::unique_ptr<GameplayGOAPDecisionInstance> decision;
            if (paths != nullptr)
            {
                decision = std::make_unique<gameplay_goap_detail::DecisionWithPathInspection>(
    ```

#### GG-DEBUGVM {#gg-debugvm}

`src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:73` · Снимок для отладки · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm#L73">GitHub</a>

Якорь: `` struct AIDebugViewModel ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="73"
        struct AIDebugViewModel
        {
            std::vector<AIDebugBooleanFactView> booleanFacts{};
            std::vector<AIDebugIntegerFactView> integerFacts{};
            std::optional<AIGoalId> selectedGoalId{};
            std::string selectedGoalName{};
            std::vector<AIDebugPlanStepView> selectedPlan{};
            float totalPlanCost{};
    ```

#### GG-DECISION {#gg-decision}

`src/Gameplay/AI/GOAP/GameplayGOAPDecision.cppm:22` · GOAP-обёртка: факты, определение, bindings · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/GameplayGOAPDecision.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/GameplayGOAPDecision.cppm#L22">GitHub</a>

Якорь: `` class GameplayGOAPDecision ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="22"
        class GameplayGOAPDecision
        {
        public:
            GameplayGOAPDecision(const EntityHandle agent,
                GameplayGOAPDecisionDefinition definition)
                : decision_(agent), definition_(std::move(definition))
            {
            }
    ```

#### GG-FIELD-ORDER {#gg-field-order}

`src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm:135` · Порядок полей и lifetime · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm#L135">GitHub</a>

Якорь: `` // Bindings borrow the context, facts and event buffer. Context and events ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="135"
            // Bindings borrow the context, facts and event buffer. Context and events
            // outlive goap_; the decision owner must cancel active AISystem tasks first.
            std::unique_ptr<IGameplayGOAPContext> context_;
            std::vector<std::unique_ptr<IGameplayGOAPEventReaction>> reactions_;
            std::vector<GameplayWorldEvent> reactionEvents_{};
            std::vector<GameplayWorldEvent> runtimeEvents_{};
            GameplayGOAPDecision goap_;
            bool configured_{};
    ```

#### GG-INSTANCE {#gg-instance}

`src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm:18` · Instance решения · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm#L18">GitHub</a>

Якорь: `` class GameplayGOAPDecisionInstance : ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="18"
        class GameplayGOAPDecisionInstance :
            public GameplayAIDecisionInstance,
            public IGameplayGOAPInspection
        {
        public:
            GameplayGOAPDecisionInstance(const EntityHandle agent, GameplayGOAPDecisionSetup setup)
                : context_(std::move(setup.context)), reactions_(std::move(setup.eventReactions)),
                  goap_(agent, std::move(setup.definition))
    ```

#### GG-INSTANCE-UPDATE {#gg-instance-update}

`src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm:54` · Порядок проходов наблюдения и событий · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm#L54">GitHub</a>

Якорь: `` void Update(AISystem& aiSystem, const GameplayAIObservationContext& observation) override ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="54"
            void Update(AISystem& aiSystem, const GameplayAIObservationContext& observation) override
            {
                if (!configured_)
                {
                    return;
                }
                reactionEvents_.clear();
                // Input and output may alias. Finish all input reads before forwarding events.
    ```

#### GG-PATH-INSPECTION {#gg-path-inspection}

`src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:38` · Планируемый путь для debug · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm#L38">GitHub</a>

Якорь: `` class IGameplayGOAPPlannedPathProvider ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="38"
        class IGameplayGOAPPlannedPathProvider
        {
        public:
            virtual ~IGameplayGOAPPlannedPathProvider() = default;
            [[nodiscard]] virtual GameplayAIDebugPlannedPathView BuildPlannedPath(
                std::span<const AIPlanStep> selectedPlan,
                std::optional<std::size_t> currentStepIndex) const = 0;
        };
    ```

#### GG-REACTION {#gg-reaction}

`src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm:35` · Реакция на подтверждённые события · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm#L35">GitHub</a>

Якорь: `` class IGameplayGOAPEventReaction ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="35"
        class IGameplayGOAPEventReaction
        {
        public:
            virtual ~IGameplayGOAPEventReaction() = default;
            virtual void React(const GameplayWorld& world, std::span<const GameplayWorldEvent> events,
                const AIAgentWorldState& facts, std::vector<GameplayWorldEvent>& output) = 0;
        };

    ```

#### GG-SETUP {#gg-setup}

`src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm:18` · Доменный контекст наблюдения · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm#L18">GitHub</a>

Якорь: `` class IGameplayGOAPContext ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="18"
        class IGameplayGOAPContext
        {
        public:
            virtual ~IGameplayGOAPContext() = default;

            virtual void Observe(const GameplayWorld& world,
                std::span<const GameplayWorldEvent> events, AIAgentWorldState& facts) = 0;

    ```

## Авторинг

#### AU-BEHAVIOR {#au-behavior}

`src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:57` · Behavior-ассет · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm#L57">GitHub</a>

Якорь: `` struct GameplayAIBehaviorAsset ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="57"
        struct GameplayAIBehaviorAsset
        {
            std::string id{};
            std::string source{};
            std::string model{};
            std::string definition{};
            std::vector<GameplayAIObservationAsset> observations{};
            std::vector<GameplayAICapabilityAsset> capabilities{};
    ```

#### AU-BEHAVIOR-PARSE {#au-behavior-parse}

`src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:112` · Разбор behavior · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm#L112">GitHub</a>

Якорь: `` GameplayAIBehaviorAsset ParseGameplayAIBehaviorAsset(std::string_view json, std::string_view source, ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="112"
        GameplayAIBehaviorAsset ParseGameplayAIBehaviorAsset(std::string_view json, std::string_view source,
            const GameplayAIComponentParsers& parsers)
        {
            using namespace ai_asset_detail;
            const auto value = Root(json, source);
            const auto& root = value.AsObject();
            Fields(root, {"version", "id", "model", "definition", "observations", "capabilities", "routeGraph", "inspectPath", "reactions"}, source, "root");
            GameplayAIBehaviorAsset result{
    ```

#### AU-COMPILE {#au-compile}

`src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm:147` · Компиляция определения · состояние: `HEAD` · вхождение 3 из 3 · <a href="../../../../src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm#L147">GitHub</a>

Якорь: `` GameplayGOAPCompiledDefinition CompileGameplayGOAPDefinition( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="147"
      GameplayGOAPCompiledDefinition CompileGameplayGOAPDefinition(
          const GameplayGOAPDefinitionAsset &asset,
          const std::span<const GameplayGOAPSemanticAction> semanticActions,
          const std::span<const GameplayGOAPActionCostOverride> costOverrides) {
        if (asset.id.empty()) {
          Invalid(asset.source, "root", {}, "missing id");
        }
        ValidateSymbol(asset.source, "root", asset.id);
    ```

#### AU-CONTEXT-UNIQUE {#au-context-unique}

`src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm:294` · Контекст уникален в определении · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm#L294">GitHub</a>

Якорь: `` "duplicate action context"); ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="294"
                    "duplicate action context");
          }
          auto [context, inserted] = compiled.contexts_.emplace(
              authored.context, AIActionContextId{nextContext});
          if (inserted) {
            if (nextContext == AIActionContextId::InvalidValue) {
              Invalid(asset.source, "actions", authored.context,
                      "context id capacity exceeded");
    ```

#### AU-FIELDS {#au-fields}

`src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm:42` · Запрет неизвестных полей · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm#L42">GitHub</a>

Якорь: `` void Fields(const JsonObject& object ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="42"
        void Fields(const JsonObject& object, const std::initializer_list<std::string_view> allowed,
            const std::string_view source, const std::string_view location)
        {
            for (const auto& [name, value] : object)
            {
                if (std::ranges::find(allowed, name) == allowed.end())
                {
                    Invalid(source, location, "unknown field '" + name + "'");
    ```

#### AU-GOAL-BUILD {#au-goal-build}

`src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm:229` · Цель из ассета: только baseScore · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm#L229">GitHub</a>

Якорь: `` AIGoalSelectionCandidate goal{ ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="229"
          AIGoalSelectionCandidate goal{
            .goal =
                AIGoalDefinition{AIGoalId{static_cast<AIGoalId::ValueType>(index)},
                                 {}},
            .baseScore = authored.score};
          for (const auto &condition : authored.facts) {
            goal.goal.desiredFacts.push_back(
                {booleanId(condition.fact, "goals", authored.name), condition.value});
    ```

#### AU-GOAP-PARSE {#au-goap-parse}

`src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm:170` · Разбор .goap.json · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm#L170">GitHub</a>

Якорь: `` ParseGameplayGOAPDefinitionAsset(const std::string_view json, ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="170"
      ParseGameplayGOAPDefinitionAsset(const std::string_view json,
                                       const std::string_view source) {
        const JsonValue rootValue = jsonUtils::JsonParser(json).Parse();
        if (!rootValue.IsObject()) {
          Invalid(source, "root", {}, "root must be an object");
        }
        const JsonObject &root = rootValue.AsObject();
        GameplayGOAPDefinitionAsset asset{};
    ```

#### AU-PARSERS {#au-parsers}

`src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm:183` · Типизированные парсеры компонентов (std::any) · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm#L183">GitHub</a>

Якорь: `` class GameplayAIComponentParsers ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="183"
        class GameplayAIComponentParsers
        {
        public:
            template<class T>
            [[nodiscard]] bool Register(GameplayAIComponentKind kind, std::string_view type,
                std::function<T(const GameplayAIComponentParseContext&)> parse)
            {
                if (type.empty() || !parse)
    ```

#### AU-ROOT {#au-root}

`src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm:151` · Проверка version == 1 · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm#L151">GitHub</a>

Якорь: `` JsonValue Root(const std::string_view json, const std::string_view source) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="151"
        JsonValue Root(const std::string_view json, const std::string_view source)
        {
            JsonValue root;
            try
            {
                root = jsonUtils::JsonParser(json).Parse();
            }
            catch (const std::exception& error)
    ```

#### AU-ROUTES-PARSE {#au-routes-parse}

`src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:229` · Разбор .routes.json · состояние: `HEAD` · вхождение 2 из 2 · <a href="../../../../src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm#L229">GitHub</a>

Якорь: `` GameplayAIRouteGraphAsset ParseGameplayAIRouteGraphAsset(std::string_view json, std::string_view source) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="229"
        GameplayAIRouteGraphAsset ParseGameplayAIRouteGraphAsset(std::string_view json, std::string_view source)
        {
            using namespace ai_asset_detail;
            const auto value = Root(json, source);
            const auto& root = value.AsObject();
            Fields(root, {"version", "nodes", "edges"}, source, "root");
            GameplayAIRouteGraphAsset result{.source = std::string(source), .nodes = Strings(root, "nodes", source, "nodes")};
            for (const auto& value : Array(root, "edges", source, "edges"))
    ```

## Композиция

#### CO-BUILTIN {#co-builtin}

`src/Gameplay/AI/Composition/GameplayGOAPBuiltinComponents.cppm:13` · Встроенные компоненты · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPBuiltinComponents.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPBuiltinComponents.cppm#L13">GitHub</a>

Якорь: `` GameplayGOAPCompositionRegistry MakeDefaultGameplayGOAPComponents() ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="13"
        [[nodiscard]] GameplayGOAPCompositionRegistry MakeDefaultGameplayGOAPComponents()
        {
            GameplayGOAPCompositionRegistry registry;
            RegisterGameplayGOAPSpatialComponents(registry);
            RegisterGameplayGOAPResourceComponents(registry);
            RegisterGameplayGOAPMoveToComponent(registry);
            return registry;
        }
    ```

#### CO-CAP-IFACE {#co-cap-iface}

`src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:52` · Интерфейс capability-провайдера · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm#L52">GitHub</a>

Якорь: `` class IGameplayGOAPCapability ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="52"
        class IGameplayGOAPCapability
        {
        public:
            virtual ~IGameplayGOAPCapability() = default;
            [[nodiscard]] virtual std::vector<GameplayGOAPActionCostOverride> CostOverrides() const { return {}; }
            [[nodiscard]] virtual std::unique_ptr<IAIActionBinding> CreateBinding(
                AIAgentWorldState& facts, std::vector<GameplayWorldEvent>& events) = 0;
        };
    ```

#### CO-CONTEXT {#co-context}

`src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:61` · Контекст компиляции компонентов · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm#L61">GitHub</a>

Якорь: `` struct GameplayGOAPCompositionContext ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="61"
        struct GameplayGOAPCompositionContext
        {
            const GameplayAIDecisionCreationContext& services;
            const GameplayGOAPCompiledDefinition& definition;
            std::span<const GameplayAIResolvedRole> roles;
            std::string_view source;
            std::span<const GameplayAIObservationAsset> observations{};
            const GameplayAIRouteGraphAsset* routeGraph{};
    ```

#### CO-DEFAULT-FACTORIES {#co-default-factories}

`src/Gameplay/AI/Composition/GameplayAIBuiltinDecisions.cppm:9` · Каталог по умолчанию · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayAIBuiltinDecisions.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayAIBuiltinDecisions.cppm#L9">GitHub</a>

Якорь: `` GameplayAIDecisionFactoryRegistry MakeDefaultGameplayAIDecisionFactories() ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="9"
        [[nodiscard]] GameplayAIDecisionFactoryRegistry MakeDefaultGameplayAIDecisionFactories()
        {
            GameplayAIDecisionFactoryRegistry registry;
            RegisterGameplayAIDecisionAssets(registry,
                LoadGameplayAIDecisionCatalogAsset("ai/decisions/catalog.json"),
                MakeDefaultGameplayGOAPComponents());
            return registry;
        }
    ```

#### CO-FROM-TEMPLATE {#co-from-template}

`src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:149` · Сборка instance · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm#L149">GitHub</a>

Якорь: `` std::unique_ptr<GameplayAIDecisionInstance> CreateGameplayGOAPDecisionFromTemplate( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="149"
        [[nodiscard]] std::unique_ptr<GameplayAIDecisionInstance> CreateGameplayGOAPDecisionFromTemplate(
            const GameplayAIDecisionCreationContext& services, const GameplayGOAPDecisionTemplate& prepared)
        {
            using namespace goap_asset_composition_detail;
            const auto& behavior = prepared.Behavior();
            const auto& bindings = prepared.Bindings();
            const auto& registry = prepared.Components();
            const auto& compiled = prepared.Compiled();
    ```

#### CO-INSTANCE-DEF {#co-instance-def}

`src/Gameplay/AI/Composition/GameplayGOAPDecisionTemplate.cppm:105` · Копия определения с cost overrides · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPDecisionTemplate.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPDecisionTemplate.cppm#L105">GitHub</a>

Якорь: `` GameplayGOAPDecisionDefinition InstanceDefinition( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="105"
            [[nodiscard]] GameplayGOAPDecisionDefinition InstanceDefinition(
                std::span<const GameplayGOAPActionCostOverride> overrides) const
            {
                auto result = compiled_.definition;
                std::set<std::pair<std::string, std::string>> keys;
                for (const auto& value : overrides)
                {
                    const auto invalid = [&](std::string_view reason)
    ```

#### CO-OBS-IFACE {#co-obs-iface}

`src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:35` · Интерфейс наблюдения с метаданными входов/выходов · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm#L35">GitHub</a>

Якорь: `` class IGameplayGOAPObservation ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="35"
        class IGameplayGOAPObservation
        {
        public:
            virtual ~IGameplayGOAPObservation() = default;
            [[nodiscard]] virtual std::vector<AIWorldFactId> BooleanOutputs() const = 0;
            [[nodiscard]] virtual std::vector<AIWorldIntegerFactId> IntegerOutputs() const { return {}; }
            [[nodiscard]] virtual std::vector<AIWorldFactId> BooleanInputs() const { return {}; }
            [[nodiscard]] virtual std::vector<AIWorldIntegerFactId> IntegerInputs() const { return {}; }
    ```

#### CO-ORDER {#co-order}

`src/Gameplay/AI/Composition/GameplayGOAPObservationOrder.cppm:18` · Writers и топологический порядок · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPObservationOrder.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPObservationOrder.cppm#L18">GitHub</a>

Якорь: `` OrderGameplayGOAPObservations( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="18"
        [[nodiscard]] std::vector<std::unique_ptr<IGameplayGOAPObservation>> OrderGameplayGOAPObservations(
            std::vector<std::unique_ptr<IGameplayGOAPObservation>> observers,
            const GameplayGOAPCompiledDefinition& compiled, std::string_view source)
        {
            const auto fail = [&](std::string_view reason)
            {
                throw std::runtime_error("AI behavior '" + std::string(source) + "', observations: " + std::string(reason));
            };
    ```

#### CO-ORDER-PICK {#co-order-pick}

`src/Gameplay/AI/Composition/GameplayGOAPObservationOrder.cppm:62` · Выбор первого готового наблюдателя · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPObservationOrder.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPObservationOrder.cppm#L62">GitHub</a>

Якорь: `` const auto next = std::ranges::find_if(observers, [&](const auto& observer) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="62"
                const auto next = std::ranges::find_if(observers, [&](const auto& observer)
                {
                    return std::ranges::all_of(observer->BooleanInputs(), [&](const auto fact)
                    {
                        return fact.index < ready.size() && ready.test(fact.index);
                    }) && std::ranges::all_of(observer->IntegerInputs(), [&](const auto fact)
                    {
                        return fact.index < integersReady.size() && integersReady.test(fact.index);
    ```

#### CO-PENDING {#co-pending}

`src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:267` · Регистрация во временной копии · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm#L267">GitHub</a>

Якорь: `` auto pending = destination; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="267"
            auto pending = destination;
            for (const auto& reference : catalog.decisions)
            {
                const auto diagnosticPrefix = "Decision '" + reference.id + "' (behavior '"
                    + reference.behavior + "', bindings '" + reference.bindings + "'): ";
                std::shared_ptr<const GameplayGOAPDecisionTemplate> prepared;
                try
                {
    ```

#### CO-REG-CAP {#co-reg-cap}

`src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:187` · Регистрация capability · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm#L187">GitHub</a>

Якорь: `` bool RegisterCapability(std::string_view type, AIActionId actionId, ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="187"
            [[nodiscard]] bool RegisterCapability(std::string_view type, AIActionId actionId,
                std::function<T(const GameplayAIComponentParseContext&)> parse,
                GameplayGOAPCapabilityCompiler compiler)
            {
                if (type.empty() || !actionId.IsValid() || !parse || !compiler || capabilities_.contains(type))
                {
                    return false;
                }
    ```

#### CO-REGISTER-ASSETS {#co-register-assets}

`src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:252` · Транзакционная регистрация каталога · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm#L252">GitHub</a>

Якорь: `` void RegisterGameplayAIDecisionAssets( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="252"
        void RegisterGameplayAIDecisionAssets(GameplayAIDecisionFactoryRegistry& destination,
            const GameplayAIDecisionCatalogAsset& catalog, const GameplayGOAPCompositionRegistry& components)
        {
            std::set<std::string> ids;
            for (const auto& reference : catalog.decisions)
            {
                if (reference.id.empty() || destination.Contains(reference.id) || !ids.insert(reference.id).second)
                {
    ```

#### CO-REGISTRY {#co-registry}

`src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:139` · Реестр компонентов · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm#L139">GitHub</a>

Якорь: `` class GameplayGOAPCompositionRegistry ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="139"
        class GameplayGOAPCompositionRegistry
        {
        public:
            template<class T>
            [[nodiscard]] bool RegisterObservation(std::string_view type,
                std::function<T(const GameplayAIComponentParseContext&)> parse,
                GameplayGOAPObservationCompiler compiler)
            {
    ```

#### CO-RESOLVE-ROLES {#co-resolve-roles}

`src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:35` · Разрешение ролей по уровню · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm#L35">GitHub</a>

Якорь: `` std::vector<GameplayAIResolvedRole> ResolveRoles( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="35"
        std::vector<GameplayAIResolvedRole> ResolveRoles(const GameplayAILevelBindingsAsset& bindings,
            const GameplayAIDecisionCreationContext& context)
        {
            std::vector<GameplayAIResolvedRole> result;
            std::set<std::string> names;
            std::vector<EntityHandle> entities;
            context.world.CollectNodeLinkEntities(entities);
            for (const auto& binding : bindings.roles)
    ```

#### CO-ROLE {#co-role}

`src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:26` · Роль: имя, entity, позиция (снимок) · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm#L26">GitHub</a>

Якорь: `` struct GameplayAIResolvedRole ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="26"
        struct GameplayAIResolvedRole
        {
            std::string role;
            EntityHandle entity{kNullEntity};
            mathUtils::Vec3 position{};
        };

        // Observers and capability providers belong to a single decision. Compile inputs
    ```

#### CO-SEMANTIC {#co-semantic}

`src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:227` · Тип capability = семантическое действие · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm#L227">GitHub</a>

Якорь: `` std::vector<GameplayGOAPSemanticAction> SemanticActions() const ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="227"
            [[nodiscard]] std::vector<GameplayGOAPSemanticAction> SemanticActions() const
            {
                std::vector<GameplayGOAPSemanticAction> result;
                for (const auto& [name, entry] : capabilities_)
                {
                    result.push_back({name, entry.actionId});
                }
                return result;
    ```

#### CO-TEMPLATE {#co-template}

`src/Gameplay/AI/Composition/GameplayGOAPDecisionTemplate.cppm:31` · Неизменяемый снимок конфигурации · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/GameplayGOAPDecisionTemplate.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/GameplayGOAPDecisionTemplate.cppm#L31">GitHub</a>

Якорь: `` class GameplayGOAPDecisionTemplate ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="31"
        class GameplayGOAPDecisionTemplate
        {
        public:
            GameplayGOAPDecisionTemplate(GameplayAIBehaviorAsset behavior, GameplayAILevelBindingsAsset bindings,
                const GameplayGOAPDefinitionAsset& definition, GameplayGOAPCompositionRegistry components,
                std::optional<GameplayAIRouteGraphAsset> graph = {})
                : behavior_(std::move(behavior)), bindings_(std::move(bindings)), graph_(std::move(graph)),
                  components_(std::move(components)), definitionSource_(definition.source)
    ```

## Встроенные компоненты

#### CP-HIDE {#cp-hide}

`src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:172` · hide_on_purchase · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm#L172">GitHub</a>

Якорь: `` class HideOnPurchaseReaction final ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="172"
        class HideOnPurchaseReaction final : public IGameplayGOAPEventReaction
        {
        public:
            HideOnPurchaseReaction(const GameplayAIReactionAsset& asset, const GameplayGOAPCompositionContext& context)
                : agent_(context.services.agent)
            {
                const auto& parameters = context.Parameters<GameplayAIHideOnPurchaseAsset>(asset);
                transaction_ = ResolveReceipt(parameters.receipt, context);
    ```

#### CP-LEDGER {#cp-ledger}

`src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:81` · resource_ledger · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm#L81">GitHub</a>

Якорь: `` class ResourceLedgerObservation final ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="81"
        class ResourceLedgerObservation final : public IGameplayGOAPObservation
        {
        public:
            ResourceLedgerObservation(const GameplayAIObservationAsset& asset, const GameplayGOAPCompositionContext& context)
            {
                const auto& ledger = context.Parameters<GameplayAIResourceLedgerAsset>(asset);
                resource_ = context.IntegerFact(ledger.fact);
                std::set<EntityHandle> targets;
    ```

#### CP-LEDGER-OBSERVE {#cp-ledger-observe}

`src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:126` · Подтверждение событий ledger · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm#L126">GitHub</a>

Якорь: `` if (event.type == GameplayWorldEventType::PickupCollected) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="126"
                    if (event.type == GameplayWorldEventType::PickupCollected)
                    {
                        for (const auto& pickup : pickups_)
                        {
                            if (event.subject != pickup.target || facts.IsFactSet(pickup.fact))
                            {
                                continue;
                            }
    ```

#### CP-MOVE-ASSET {#cp-move-asset}

`src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponentAssets.cppm:13` · Параметры move_to и значения по умолчанию · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponentAssets.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponentAssets.cppm#L13">GitHub</a>

Якорь: `` struct GameplayAIMoveToAsset ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="13"
        struct GameplayAIMoveToAsset
        {
            std::string source, target;
            float acceptanceRadius{0.4f}, slowingRadius{1.5f};
            bool wantsRun{};
            float sourceRadius{};
            bool reserveTarget{}, routeCost{};
        };
    ```

#### CP-MOVE-BINDING {#cp-move-binding}

`src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm:173` · Binding без obstacle query/debug · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm#L173">GitHub</a>

Якорь: `` return std::make_unique<AIMoveToActionBinding>(world_, links_, executors_, *this, reservations_, *this); ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="173"
                return std::make_unique<AIMoveToActionBinding>(world_, links_, executors_, *this, reservations_, *this);
            }
            std::vector<GameplayGOAPActionCostOverride> CostOverrides() const override { return costs_; }
            std::optional<GameplayRoute> BuildDebugRoute(AIActionContextId id) const override
            {
                const auto* route = Find(id);
                if (route == nullptr)
                {
    ```

#### CP-MOVE-CAP {#cp-move-cap}

`src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm:92` · MoveTo provider · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm#L92">GitHub</a>

Якорь: `` class MoveToCapability final : public IGameplayGOAPCapability, ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="92"
        class MoveToCapability final : public IGameplayGOAPCapability,
            public IAIMoveToActionRequestProvider, public IAIActionReservationTargetProvider,
            public IGameplayGOAPActionPathProvider
        {
        public:
            MoveToCapability(std::span<const GameplayAICapabilityAsset> assets, const GameplayGOAPCompositionContext& context)
                : world_(context.services.world), links_(context.services.traversalLinkRegistry),
                  executors_(context.services.traversalExecutorRegistry), reservations_(context.services.reservationSystem)
    ```

#### CP-MOVE-FALLBACK {#cp-move-fallback}

`src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm:141` · Прямой fallback-граф · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm#L141">GitHub</a>

Якорь: `` graph.edges = {{route.start, route.goal, 1.0f, {}}}; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="141"
                        graph.edges = {{route.start, route.goal, 1.0f, {}}};
                        graphs_.push_back(std::move(graph));
                        if (parameters.routeCost)
                        {
                            context.Fail(asset.context, "routeCost requires an authored route graph");
                        }
                    }
                    const auto planned = FindWeightedGameplayRoute(graphs_[route.graphIndex], route.start, route.goal);
    ```

#### CP-MOVE-GRAPH {#cp-move-graph}

`src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm:36` · Авторский граф маршрутов · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm#L36">GitHub</a>

Якорь: `` AuthoredGraph CompileGraph(const GameplayGOAPCompositionContext& context) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="36"
        AuthoredGraph CompileGraph(const GameplayGOAPCompositionContext& context)
        {
            AuthoredGraph result;
            const auto& asset = *context.routeGraph;
            for (const auto& name : asset.nodes)
            {
                const GameplayRouteNodeId id{result.nodes.size() + 1u};
                if (!result.nodes.emplace(name, id).second)
    ```

#### CP-MOVE-RESOLVE {#cp-move-resolve}

`src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm:191` · sourceRadius и запрос MoveTo · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm#L191">GitHub</a>

Якорь: `` std::optional<AIMoveToActionRequest> ResolveRequest(const AIActionRuntimeContext& context) override ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="191"
            std::optional<AIMoveToActionRequest> ResolveRequest(const AIActionRuntimeContext& context) override
            {
                const auto* route = Find(context.contextId);
                if (route == nullptr)
                {
                    return {};
                }
                const auto& graph = graphs_[route->graphIndex];
    ```

#### CP-NEAREST {#cp-nearest}

`src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:90` · nearest_location (one-hot, память) · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm#L90">GitHub</a>

Якорь: `` class NearestLocationObservation final ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="90"
        class NearestLocationObservation final : public IGameplayGOAPObservation
        {
        public:
            NearestLocationObservation(const GameplayAIObservationAsset& asset, const GameplayGOAPCompositionContext& context)
            {
                const auto& parameters = context.Parameters<GameplayAINearestLocationAsset>(asset);
                radius_ = parameters.radius;
                if (!(radius_ > 0) || !std::isfinite(radius_ * radius_) || parameters.locations.empty())
    ```

#### CP-NEAREST-TIE {#cp-nearest-tie}

`src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:122` · Равенство → более поздний элемент · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm#L122">GitHub</a>

Якорь: `` if (candidate <= distance) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="122"
                    if (candidate <= distance)
                    {
                        nearest = index;
                        distance = candidate;
                    }
                }
                // Symbolic location remains the last confirmed node while in transit.
                if (nearest)
    ```

#### CP-PICKUP-AVAIL {#cp-pickup-avail}

`src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:148` · pickup_available с учётом броней · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm#L148">GitHub</a>

Якорь: `` class PickupAvailabilityObservation final ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="148"
        class PickupAvailabilityObservation final : public IGameplayGOAPObservation
        {
        public:
            PickupAvailabilityObservation(const GameplayAIObservationAsset& asset, const GameplayGOAPCompositionContext& context)
                : reservations_(context.services.reservationSystem)
            {
                const auto& parameters = context.Parameters<GameplayAIPickupAvailabilityAsset>(asset);
                target_ = context.Role(parameters.target).entity;
    ```

#### CP-PURCHASE-CAP {#cp-purchase-cap}

`src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:283` · Проверка модели покупки · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm#L283">GitHub</a>

Якорь: `` class PurchaseCapability final : public IGameplayGOAPCapability ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="283"
        class PurchaseCapability final : public IGameplayGOAPCapability
        {
        public:
            PurchaseCapability(std::span<const GameplayAICapabilityAsset> assets, const GameplayGOAPCompositionContext& context)
                : world_(context.services.world)
            {
                for (const auto& asset : assets)
                {
    ```

#### CP-PURCHASE-ID {#cp-purchase-id}

`src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:26` · ID purchase = 3 · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm#L26">GitHub</a>

Якорь: `` inline constexpr AIActionId kAIPurchaseActionId{3u}; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="26"
        inline constexpr AIActionId kAIPurchaseActionId{3u};
    }

    namespace rendern::goap_resource_detail
    {
        struct Transaction
        {
            EntityHandle target{kNullEntity};
    ```

#### CP-PURCHASE-RT {#cp-purchase-rt}

`src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:212` · Runtime покупки · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm#L212">GitHub</a>

Якорь: `` class PurchaseRuntime final : public IAIActionRuntime ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="212"
        class PurchaseRuntime final : public IAIActionRuntime
        {
        public:
            PurchaseRuntime(AIAgentWorldState& facts, const GameplayWorld& world,
                std::vector<GameplayWorldEvent>& events, Transaction transaction, float radius)
                : facts_(facts), world_(world), events_(events), transaction_(transaction), radius_(radius) {}
            AIActionRuntimeResult Start(const AIActionRuntimeContext& context) override
            {
    ```

#### CP-SPATIAL {#cp-spatial}

`src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:20` · target_available / within_distance · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm#L20">GitHub</a>

Якорь: `` class SpatialObservation final : public IGameplayGOAPObservation ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="20"
        class SpatialObservation final : public IGameplayGOAPObservation
        {
        public:
            SpatialObservation(const GameplayAIObservationAsset& asset, const GameplayGOAPCompositionContext& context)
            {
                parameters_ = context.Parameters<GameplayAISpatialObservationAsset>(asset);
                target_ = context.Role(parameters_.target);
                fact_ = context.BooleanFact(parameters_.fact);
    ```

#### CP-SPATIAL-OBSERVE {#cp-spatial-observe}

`src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:56` · Вычисление spatial-факта, latch · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm#L56">GitHub</a>

Якорь: `` bool value = (!parameters_.requireEntity || world.IsEntityValid(target_.entity)) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="56"
                bool value = (!parameters_.requireEntity || world.IsEntityValid(target_.entity))
                    && (!parameters_.requireInteractionPoint || world.HasInteractionPoint(target_.entity));
                for (const auto input : inputs_)
                {
                    value = value && facts.IsFactSet(input);
                }
                if (distance_)
                {
    ```

#### CP-SPATIAL-REGISTER {#cp-spatial-register}

`src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:183` · Регистрация spatial-компонентов · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm#L183">GitHub</a>

Якорь: `` void RegisterGameplayGOAPSpatialComponents( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="183"
        void RegisterGameplayGOAPSpatialComponents(GameplayGOAPCompositionRegistry& registry)
        {
            const auto spatial = [](const auto& asset, const auto& context)
            {
                return std::make_unique<goap_spatial_detail::SpatialObservation>(asset, context);
            };
            const bool available = registry.RegisterObservation<GameplayAISpatialObservationAsset>("target_available", ParseGameplayAISpatialObservation, spatial);
            const bool distance = registry.RegisterObservation<GameplayAISpatialObservationAsset>("within_distance", ParseGameplayAISpatialObservation, spatial);
    ```

## Навигационные действия AI

#### NA-AGENT-AVOID {#na-agent-avoid}

`src/Gameplay/AI/Navigation/GameplayAgentObstacleAvoidance.cppm:10` · Агентный адаптер avoidance · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/GameplayAgentObstacleAvoidance.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/GameplayAgentObstacleAvoidance.cppm#L10">GitHub</a>

Якорь: `` GameplayMovementIntent ApplyGameplayAgentObstacleAvoidance( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="10"
        [[nodiscard]] GameplayMovementIntent ApplyGameplayAgentObstacleAvoidance(
            const GameplayWorld& world,
            const EntityHandle entity,
            const GameplayMovementIntent& baseMovement,
            const IGameplayObstacleQuery& query,
            const GameplayObstacleAvoidanceSettings& settings,
            GameplayObstacleAvoidanceState& state,
            GameplayObstacleAvoidanceDebugSnapshot* debugOut = nullptr) noexcept
    ```

#### NA-FLEE-CTOR {#na-flee-ctor}

`src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm:48` · safe ≥ trigger · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm#L48">GitHub</a>

Якорь: `` settings_.safeRadius = std::max(settings_.safeRadius, settings_.triggerRadius); ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="48"
                settings_.safeRadius = std::max(settings_.safeRadius, settings_.triggerRadius);
                settings_.steeringUpdateIntervalSeconds =
                    std::max(settings_.steeringUpdateIntervalSeconds, 0.0f);
            }

            [[nodiscard]] AIActionRuntimeResult Start(
                const AIActionRuntimeContext& context) override
            {
    ```

#### NA-FLEE-HYST {#na-flee-hyst}

`src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm:135` · Гистерезис бегства · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm#L135">GitHub</a>

Якорь: `` if (isFleeing_) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="135"
                if (isFleeing_)
                {
                    isFleeing_ = distance < settings_.safeRadius;
                }
                else
                {
                    isFleeing_ = distance <= settings_.triggerRadius;
                }
    ```

#### NA-FLEE-REFRESH {#na-flee-refresh}

`src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm:127` · Пересчёт Flee: дистанция, гистерезис, avoidance · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm#L127">GitHub</a>

Якорь: `` void RefreshSteering_(const EntityHandle agentEntity) noexcept ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="127"
            void RefreshSteering_(const EntityHandle agentEntity) noexcept
            {
                const mathUtils::Vec3 agentPosition = world_.TryGetTransform(agentEntity)->position;
                const mathUtils::Vec3 targetPosition = world_.TryGetTransform(targetEntity_)->position;
                mathUtils::Vec3 planarDelta = targetPosition - agentPosition;
                planarDelta.y = 0.0f;
                const float distance = mathUtils::Length(planarDelta);

    ```

#### NA-FLEE-SETTINGS {#na-flee-settings}

`src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm:22` · trigger 2, safe 3, интервал 0.05 · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm#L22">GitHub</a>

Якорь: `` struct AIFleeTargetSettings ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="22"
        struct AIFleeTargetSettings
        {
            GameplayFleeSteeringSettings steering{};
            float triggerRadius{2.0f};
            float safeRadius{3.0f};
            float steeringUpdateIntervalSeconds{0.05f};
        };

    ```

#### NA-FOLLOW-RT {#na-follow-rt}

`src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm:27` · FollowTarget runtime · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm#L27">GitHub</a>

Якорь: `` class AIFollowTargetActionRuntime final : public IAIActionRuntime ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="27"
        class AIFollowTargetActionRuntime final : public IAIActionRuntime
        {
        public:
            AIFollowTargetActionRuntime(
                GameplayWorld& world,
                const EntityHandle targetEntity,
                AIFollowTargetSettings settings = {},
                const IGameplayObstacleQuery* obstacleQuery = nullptr,
    ```

#### NA-FOLLOW-SETTINGS {#na-follow-settings}

`src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm:21` · Seek + интервал 0.05 с · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm#L21">GitHub</a>

Якорь: `` struct AIFollowTargetSettings ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="21"
        struct AIFollowTargetSettings
        {
            GameplaySeekSteeringSettings steering{};
            float steeringUpdateIntervalSeconds{0.05f};
        };

        class AIFollowTargetActionRuntime final : public IAIActionRuntime
        {
    ```

#### NA-FOLLOW-TICK {#na-follow-tick}

`src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm:64` · Sampling + переиздание команды · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm#L64">GitHub</a>

Якорь: `` AIActionRuntimeResult Tick( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="64"
            [[nodiscard]] AIActionRuntimeResult Tick(
                const AIActionRuntimeContext& context,
                const float deltaSeconds) override
            {
                if (!Validate_(context))
                {
                    ClearMovementIfAccessible_(context.agentEntity);
                    return AIActionRuntimeResult::Failed;
    ```

#### NA-MOVETO-BINDING {#na-moveto-binding}

`src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm:138` · Binding MoveTo · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm#L138">GitHub</a>

Якорь: `` class AIMoveToActionBinding final : public IAIActionBinding ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="138"
        class AIMoveToActionBinding final : public IAIActionBinding
        {
        public:
            AIMoveToActionBinding(
                GameplayWorld& world,
                const GameplayTraversalLinkRegistry& traversalLinkRegistry,
                const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
                IAIMoveToActionRequestProvider& requestProvider,
    ```

#### NA-MOVETO-CREATE {#na-moveto-create}

`src/Gameplay/AI/Navigation/AIMoveToAction.cppm:38` · MoveTo → поиск → FollowRoute runtime · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIMoveToAction.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIMoveToAction.cppm#L38">GitHub</a>

Якорь: `` static std::unique_ptr<IAIActionRuntime> CreateRuntime( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="38"
            [[nodiscard]] static std::unique_ptr<IAIActionRuntime> CreateRuntime(
                const AIActionRuntimeContext& context,
                GameplayWorld& world,
                const GameplayTraversalLinkRegistry& traversalLinkRegistry,
                const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
                const AIMoveToActionRequest& request,
                const IGameplayObstacleQuery* obstacleQuery = nullptr,
                const GameplayObstacleAvoidanceSettings& obstacleSettings = {},
    ```

#### NA-MOVETO-ID {#na-moveto-id}

`src/Gameplay/AI/Navigation/AIMoveToAction.cppm:23` · ID move_to = 2 · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIMoveToAction.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIMoveToAction.cppm#L23">GitHub</a>

Якорь: `` inline constexpr AIActionId kAIMoveToActionId{2u}; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="23"
        inline constexpr AIActionId kAIMoveToActionId{2u};

        struct AIMoveToActionRequest
        {
            // Non-owning. The graph must remain alive only through synchronous
            // runtime creation; the resolved GameplayRoute is runtime-owned.
            const GameplayRouteGraph* routeGraph{nullptr};
            GameplayRouteNodeId startNodeId{};
    ```

#### NA-RESERVED {#na-reserved}

`src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm:37` · MoveTo с бронью цели · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm#L37">GitHub</a>

Якорь: `` class ReservedAIMoveToActionRuntime final : public IAIActionRuntime ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="37"
        class ReservedAIMoveToActionRuntime final : public IAIActionRuntime
        {
        public:
            ReservedAIMoveToActionRuntime(GameplayWorld& world,
                const GameplayTraversalLinkRegistry& links,
                const GameplayTraversalExecutorRegistry& executors,
                GameplayObjectReservationSystem& reservations,
                AIMoveToActionRequest request, const EntityHandle target,
    ```

#### NA-RESERVED-START {#na-reserved-start}

`src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm:56` · Старт отклоняет любую существующую бронь · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm#L56">GitHub</a>

Якорь: `` if (started_ || reservations_.IsReserved(target_) || ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="56"
                if (started_ || reservations_.IsReserved(target_) ||
                    !reservations_.TryReserve(world_, target_, context.agentEntity))
                {
                    return AIActionRuntimeResult::Failed;
                }
                ownsReservation_ = true;
                runtime_ = AIMoveToAction::CreateRuntime(
                    context, world_, links_, executors_, request_, obstacleQuery_, {}, debugRegistry_,
    ```

#### NA-ROUTE-ADVANCE {#na-route-advance}

`src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm:156` · Продвижение follower и physicallyBlocked · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm#L156">GitHub</a>

Якорь: `` AIActionRuntimeResult AdvanceFollower_(const EntityHandle entity) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="156"
            [[nodiscard]] AIActionRuntimeResult AdvanceFollower_(const EntityHandle entity)
            {
                const GameplayTransformComponent* transform = world_.TryGetTransform(entity);
                const GameplayCharacterMovementStateComponent* movementState =
                    world_.TryGetCharacterMovementState(entity);
                physicallyBlocked_ = movementState->physicallyBlocked;
                const GameplayRouteFollowerOutput output = follower_.Tick(transform->position);
                switch (output.status)
    ```

#### NA-ROUTE-APPLY {#na-route-apply}

`src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm:185` · Avoidance → команда → facing · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm#L185">GitHub</a>

Якорь: `` void ApplyFollowerMovement_( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="185"
            void ApplyFollowerMovement_(
                const EntityHandle entity,
                const GameplayMovementIntent& movement) noexcept
            {
                GameplayCharacterCommandComponent* command = world_.TryGetCharacterCommand(entity);
                GameplayCharacterMotorComponent* motor = world_.TryGetCharacterMotor(entity);
                GameplayCharacterMovementStateComponent* movementState =
                    world_.TryGetCharacterMovementState(entity);
    ```

#### NA-ROUTE-CREATE {#na-route-create}

`src/Gameplay/AI/Navigation/AIFollowRouteAction.cppm:24` · Фабрика/старт FollowRoute · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIFollowRouteAction.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIFollowRouteAction.cppm#L24">GitHub</a>

Якорь: `` class AIFollowRouteAction ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="24"
        class AIFollowRouteAction
        {
        public:
            [[nodiscard]] static std::unique_ptr<IAIActionRuntime> CreateRuntime(
                GameplayWorld& world,
                const GameplayTraversalLinkRegistry& traversalLinkRegistry,
                const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
                const EntityHandle agentEntity,
    ```

#### NA-ROUTE-ID {#na-route-id}

`src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm:25` · ID FollowRoute = 1 · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm#L25">GitHub</a>

Якорь: `` inline constexpr AIActionId kAIFollowRouteActionId{1u}; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="25"
        inline constexpr AIActionId kAIFollowRouteActionId{1u};

        class AIFollowRouteActionRuntime final : public IAIActionRuntime
        {
        public:
            AIFollowRouteActionRuntime(
                GameplayWorld& world,
                const GameplayTraversalLinkRegistry& traversalLinkRegistry,
    ```

#### NA-ROUTE-RT {#na-route-rt}

`src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm:27` · Runtime движения по маршруту · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm#L27">GitHub</a>

Якорь: `` class AIFollowRouteActionRuntime final : public IAIActionRuntime ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="27"
        class AIFollowRouteActionRuntime final : public IAIActionRuntime
        {
        public:
            AIFollowRouteActionRuntime(
                GameplayWorld& world,
                const GameplayTraversalLinkRegistry& traversalLinkRegistry,
                const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
                GameplayRoute route,
    ```

#### NA-ROUTE-STOP {#na-route-stop}

`src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm:330` · Остановка с сохранением вертикали · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm#L330">GitHub</a>

Якорь: `` void StopMovementIfAccessible_(const EntityHandle entity) noexcept ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="330"
            void StopMovementIfAccessible_(const EntityHandle entity) noexcept
            {
                obstacleAvoidanceState_ = {};
                if (debugRegistry_ != nullptr)
                {
                    debugRegistry_->Clear(entity);
                }
                ClearMovementIntentIfAccessible_(entity);
    ```

#### NA-ROUTE-TICK {#na-route-tick}

`src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm:83` · Tick маршрута и traversal · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm#L83">GitHub</a>

Якорь: `` AIActionRuntimeResult Tick( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="83"
            [[nodiscard]] AIActionRuntimeResult Tick(
                const AIActionRuntimeContext& context,
                const float deltaSeconds) override
            {
                if (!ValidateContextAndAgent_(context))
                {
                    StopMovementIfAccessible_(context.agentEntity);
                    CancelActiveTraversal_();
    ```

#### NA-ROUTE-TRAVERSAL {#na-route-traversal}

`src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm:223` · Передача сегмента executor · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm#L223">GitHub</a>

Якорь: `` AIActionRuntimeResult StartTraversal_( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="223"
            [[nodiscard]] AIActionRuntimeResult StartTraversal_(
                const EntityHandle entity,
                const std::optional<GameplayTraversalLinkHandle>& requiredLink)
            {
                obstacleAvoidanceState_ = {};
                if (debugRegistry_ != nullptr)
                {
                    debugRegistry_->Clear(entity);
    ```

## Маршруты и steering

#### MU-EPS {#mu-eps}

`src/Core/Math/MathUtils.cppm:29` · Эпсилоны длины/движения · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Core/Math/MathUtils.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Core/Math/MathUtils.cppm#L29">GitHub</a>

Якорь: `` constexpr float kLengthEpsilon = 1e-6f; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="29"
        constexpr float kLengthEpsilon = 1e-6f;
        constexpr float kLengthEpsilonSq = kLengthEpsilon * kLengthEpsilon;
        constexpr float kMoveEpsilon = 1e-4f;
        constexpr float kMoveEpsilonSq = kMoveEpsilon * kMoveEpsilon;

        struct Vec2
        {
            float x{ 0 }, y{ 0 };
    ```

#### RF-COMPLETE {#rf-complete}

`src/Gameplay/Navigation/GameplayRouteFollower.cppm:179` · Подтверждение traversal · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayRouteFollower.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayRouteFollower.cppm#L179">GitHub</a>

Якорь: `` bool CompleteTraversal( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="179"
            [[nodiscard]] bool CompleteTraversal(
                GameplayTraversalLinkHandle completedTraversalLink) noexcept
            {
                const std::optional<GameplayTraversalLinkHandle> pendingTraversalLink =
                    GetCurrentTraversalLink();
                const bool bCnaCompleteTraversal =
                    status_ == GameplayRouteFollowerStatus::TraversalRequired &&
                    pendingTraversalLink.has_value() &&
    ```

#### RF-LOOKAHEAD {#rf-lookahead}

`src/Gameplay/Navigation/GameplayRouteFollower.cppm:218` · Виртуальная цель look-ahead · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayRouteFollower.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayRouteFollower.cppm#L218">GitHub</a>

Якорь: `` bool TryBuildLookAheadTarget_( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="218"
             [[nodiscard]] bool TryBuildLookAheadTarget_(
                const mathUtils::Vec3& currentPosition,
                mathUtils::Vec3& steeringTarget) const noexcept
            {
                const float lookAheadDistance = followerSettings_.cornerLookAheadDistance;
                const std::size_t nextSegmentIndex = currentSegmentIndex_ + 1u;
                if (lookAheadDistance <= 0.0f ||
                    nextSegmentIndex >= route_.segmentAnnotations.size() ||
    ```

#### RF-PASSED {#rf-passed}

`src/Gameplay/Navigation/GameplayRouteFollower.cppm:261` · Плоскость перехода угла · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayRouteFollower.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayRouteFollower.cppm#L261">GitHub</a>

Якорь: `` bool HasPassedCurrentCorner_( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="261"
            [[nodiscard]] bool HasPassedCurrentCorner_(
                const mathUtils::Vec3& currentPosition) const noexcept
             {
                 const std::size_t nextSegmentIndex = currentSegmentIndex_ + 1u;
                 if (nextSegmentIndex >= route_.segmentAnnotations.size())
                 {
                     return false;
                 }
    ```

#### RF-SETTINGS {#rf-settings}

`src/Gameplay/Navigation/GameplayRouteFollower.cppm:38` · Look-ahead по умолчанию 0.75 · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayRouteFollower.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayRouteFollower.cppm#L38">GitHub</a>

Якорь: `` float cornerLookAheadDistance{0.75f}; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="38"
            float cornerLookAheadDistance{0.75f};
        };

        class GameplayRouteFollower
        {
        public:
            // The supplied route is owned by value. points[0] is the route start
            // anchor; the follower never prepends a segment from currentPosition.
    ```

#### RF-SLOWING {#rf-slowing}

`src/Gameplay/Navigation/GameplayRouteFollower.cppm:117` · slowing = acceptance на промежуточных · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayRouteFollower.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayRouteFollower.cppm#L117">GitHub</a>

Якорь: `` segmentSteeringSettings.slowingRadius = bIsFinalSegment ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="117"
                    segmentSteeringSettings.slowingRadius = bIsFinalSegment
                        ? steeringSettings_.slowingRadius
                        : steeringSettings_.acceptanceRadius;

                    const mathUtils::Vec3& cornerPosition = route_.points[currentSegmentIndex_ + 1u].worldPosition;
                    const GameplaySteeringOutput cornerSteering = BuildGameplayArrivalSteering(
                        currentPosition,
                        cornerPosition,
    ```

#### RF-START {#rf-start}

`src/Gameplay/Navigation/GameplayRouteFollower.cppm:46` · Старт follower; anchor = points[0] · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayRouteFollower.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayRouteFollower.cppm#L46">GitHub</a>

Якорь: `` GameplayRouteFollowerStatus Start( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="46"
            [[nodiscard]] GameplayRouteFollowerStatus Start(
                GameplayRoute route,
                const GameplayArrivalSteeringSettings& steeringSettings = {},
                const GameplayRouteFollowerSettings& followerSettings = {})
            {
                if (status_ != GameplayRouteFollowerStatus::NotStarted)
                {
                    return status_;
    ```

#### RF-STATUS {#rf-status}

`src/Gameplay/Navigation/GameplayRouteFollower.cppm:19` · Статусы follower · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayRouteFollower.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayRouteFollower.cppm#L19">GitHub</a>

Якорь: `` enum class GameplayRouteFollowerStatus ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="19"
        enum class GameplayRouteFollowerStatus : std::uint8_t
        {
            NotStarted,
            Following,
            TraversalRequired,
            Succeeded,
            InvalidRoute
        };
    ```

#### RF-TICK {#rf-tick}

`src/Gameplay/Navigation/GameplayRouteFollower.cppm:82` · Тик follower · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayRouteFollower.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayRouteFollower.cppm#L82">GitHub</a>

Якорь: `` GameplayRouteFollowerOutput Tick( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="82"
            [[nodiscard]] GameplayRouteFollowerOutput Tick(
                const mathUtils::Vec3& currentPosition)
            {
                GameplayRouteFollowerOutput output{};
                output.status = status_;

                if (status_ == GameplayRouteFollowerStatus::TraversalRequired)
                {
    ```

#### RS-FIND {#rs-find}

`src/Gameplay/Navigation/GameplayRouteSearch.cppm:215` · Дейкстра по узлам · состояние: `HEAD` · вхождение 2 из 2 · <a href="../../../../src/Gameplay/Navigation/GameplayRouteSearch.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayRouteSearch.cppm#L215">GitHub</a>

Якорь: `` GameplayRouteSearchResult FindWeightedGameplayRoute( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="215"
        GameplayRouteSearchResult FindWeightedGameplayRoute(
            const GameplayRouteGraph& graph,
            const GameplayRouteNodeId startNodeId,
            const GameplayRouteNodeId goalNodeId)
        {
            if (!graph.IsValid())
            {
                return GameplayRouteSearchResult{.status = GameplayRouteSearchStatus::InvalidGraph};
    ```

#### RS-GRAPH {#rs-graph}

`src/Gameplay/Navigation/GameplayRouteSearch.cppm:64` · Взвешенный ориентированный граф · состояние: `HEAD` · вхождение 3 из 3 · <a href="../../../../src/Gameplay/Navigation/GameplayRouteSearch.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayRouteSearch.cppm#L64">GitHub</a>

Якорь: `` struct GameplayRouteGraph ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="64"
        struct GameplayRouteGraph
        {
            std::vector<GameplayRouteGraphNode> nodes{};
            std::vector<GameplayRouteGraphEdge> edges{};

            [[nodiscard]] bool IsValid() const;
        };

    ```

#### RS-SAME {#rs-same}

`src/Gameplay/Navigation/GameplayRouteSearch.cppm:242` · start == goal → одна точка · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayRouteSearch.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayRouteSearch.cppm#L242">GitHub</a>

Якорь: `` if (resolvedStartNodeIndex == resolvedGoalNodeIndex) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="242"
            if (resolvedStartNodeIndex == resolvedGoalNodeIndex)
            {
                GameplayRoute route{ .points =
                    {GameplayRoutePoint
                        {.worldPosition = graph.nodes[resolvedStartNodeIndex].worldPosition}
                    }
                };

    ```

#### RT-ANNOTATION {#rt-annotation}

`src/Gameplay/Navigation/GameplayRoute.cppm:42` · Аннотация traversal · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayRoute.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayRoute.cppm#L42">GitHub</a>

Якорь: `` struct GameplayRouteSegmentAnnotation ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="42"
        struct GameplayRouteSegmentAnnotation
        {
            // Empty means the segment is traversed through ordinary steering.
            // A valid link delegates special traversal to an external system.
            std::optional<GameplayTraversalLinkHandle> traversalLink{};

            [[nodiscard]] constexpr bool IsValid() const noexcept
            {
    ```

#### RT-ROUTE {#rt-route}

`src/Gameplay/Navigation/GameplayRoute.cppm:56` · Точки + аннотации сегментов · состояние: `HEAD` · вхождение 3 из 3 · <a href="../../../../src/Gameplay/Navigation/GameplayRoute.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayRoute.cppm#L56">GitHub</a>

Якорь: `` struct GameplayRoute ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="56"
        struct GameplayRoute
        {
            // Ordered world-space points. Segment annotation i describes travel
            // from points[i] to points[i + 1].
            std::vector<GameplayRoutePoint> points{};

           // Contains exactly one annotation for every pair of adjacent points.
           // An annotation without a traversal link represents ordinary movement.
    ```

#### RT-ROUTE-VALID {#rt-route-valid}

`src/Gameplay/Navigation/GameplayRoute.cppm:78` · N точек ↔ N−1 аннотаций · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayRoute.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayRoute.cppm#L78">GitHub</a>

Якорь: `` const std::size_t expectedSegmentCount = points.size() - 1u; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="78"
                const std::size_t expectedSegmentCount = points.size() - 1u;
                const bool bHasExpectedSegmentCount = segmentAnnotations.size() == expectedSegmentCount;

                if (!bHasExpectedSegmentCount)
                {
                    return false;
                }

    ```

#### ST-APPLY {#st-apply}

`src/Gameplay/Navigation/GameplaySteering.cppm:161` · Намерение → CharacterCommand · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplaySteering.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplaySteering.cppm#L161">GitHub</a>

Якорь: `` void ApplyGameplayMovementIntent( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="161"
        void ApplyGameplayMovementIntent(
            const GameplayMovementIntent& intent,
            GameplayCharacterCommandComponent& command) noexcept
        {
            mathUtils::Vec3 planarDirection = intent.moveWorld;
            planarDirection.y = 0.0f;

            const float directionLengthSquare = mathUtils::Dot(planarDirection, planarDirection);
    ```

#### ST-ARRIVAL {#st-arrival}

`src/Gameplay/Navigation/GameplaySteering.cppm:110` · Arrival · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplaySteering.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplaySteering.cppm#L110">GitHub</a>

Якорь: `` GameplaySteeringOutput BuildGameplayArrivalSteering( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="110"
        [[nodiscard]] GameplaySteeringOutput BuildGameplayArrivalSteering(
            const mathUtils::Vec3& currentPosition,
            const mathUtils::Vec3& targetPosition,
            const GameplayArrivalSteeringSettings& settings = {}) noexcept
        {
            constexpr float kArrivalTolerance = 0.001f;

            mathUtils::Vec3 planarDelta = targetPosition - currentPosition;
    ```

#### ST-ARRIVAL-SETTINGS {#st-arrival-settings}

`src/Gameplay/Navigation/GameplaySteering.cppm:33` · r=0.25, R=1.0, m_min=0.1 · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplaySteering.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplaySteering.cppm#L33">GitHub</a>

Якорь: `` struct GameplayArrivalSteeringSettings ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="33"
        struct GameplayArrivalSteeringSettings
        {
            float acceptanceRadius{0.25f};
            float slowingRadius{1.0f};
            float minimumMoveMagnitude{0.1f};
            bool wantsRun{false};
        };

    ```

#### ST-FLEE {#st-flee}

`src/Gameplay/Navigation/GameplaySteering.cppm:88` · Flee · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplaySteering.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplaySteering.cppm#L88">GitHub</a>

Якорь: `` GameplayMovementIntent BuildGameplayFleeSteering( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="88"
        [[nodiscard]] GameplayMovementIntent BuildGameplayFleeSteering(
            const mathUtils::Vec3& currentPosition,
            const mathUtils::Vec3& threatPosition,
            const GameplayFleeSteeringSettings& settings = {}) noexcept
        {
            mathUtils::Vec3 planarDelta = currentPosition - threatPosition;
            planarDelta.y = 0.0f;

    ```

#### ST-INTENT {#st-intent}

`src/Gameplay/Navigation/GameplaySteering.cppm:14` · Намерение движения · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplaySteering.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplaySteering.cppm#L14">GitHub</a>

Якорь: `` struct GameplayMovementIntent ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="14"
        struct GameplayMovementIntent
        {
            mathUtils::Vec3 moveWorld{0.0f, 0.0f, 0.0f};
            float moveMagnitude{0.0f};
            bool wantsRun{false};

            [[nodiscard]] bool IsMoving() const noexcept
            {
    ```

#### ST-SEEK {#st-seek}

`src/Gameplay/Navigation/GameplaySteering.cppm:59` · Seek · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplaySteering.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplaySteering.cppm#L59">GitHub</a>

Якорь: `` GameplaySteeringOutput BuildGameplaySeekSteering( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="59"
        [[nodiscard]] GameplaySteeringOutput BuildGameplaySeekSteering(
            const mathUtils::Vec3& currentPosition,
            const mathUtils::Vec3& targetPosition,
            const GameplaySeekSteeringSettings& settings = {}) noexcept
        {
            constexpr float kAcceptanceTolerance = 0.001f;

            mathUtils::Vec3 planarDelta = targetPosition - currentPosition;
    ```

## Obstacle avoidance

#### OA-APPLY {#oa-apply}

`src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:388` · Основной алгоритм avoidance · состояние: `HEAD` · вхождение 2 из 4 · <a href="../../../../src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm#L388">GitHub</a>

Якорь: `` rendern::GameplayMovementIntent rendern::ApplyGameplayObstacleAvoidance( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="388"
    rendern::GameplayMovementIntent rendern::ApplyGameplayObstacleAvoidance(
        const GameplayObstacleAvoidanceInput& input,
        const IGameplayObstacleQuery& query,
        const GameplayObstacleAvoidanceSettings& settings,
        GameplayObstacleAvoidanceState& state,
        GameplayObstacleAvoidanceDebugSnapshot* debugOut) noexcept
    {
        const GameplayMovementIntent& baseMovement = input.baseMovement;
    ```

#### OA-ESCAPE {#oa-escape}

`src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:323` · Escape-кандидат · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm#L323">GitHub</a>

Якорь: `` mathUtils::Vec3 BuildEscapeCandidate( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="323"
        [[nodiscard]] mathUtils::Vec3 BuildEscapeCandidate(
            const mathUtils::Vec3& selectedFeeler,
            const mathUtils::Vec3& forward,
            const rendern::GameplayObstacleAvoidanceSide side,
            const mathUtils::Vec3& forwardHitNormal) noexcept
        {
            mathUtils::Vec3 planarNormal{
                forwardHitNormal.x, 0.0f, forwardHitNormal.z};
    ```

#### OA-FEELERS {#oa-feelers}

`src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:424` · Поворот feelers · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm#L424">GitHub</a>

Якорь: `` const mathUtils::Vec3 left{ ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="424"
        const mathUtils::Vec3 left{
            (forward.x * cosine) + (forward.z * sine),
            0.0f,
            (forward.z * cosine) - (forward.x * sine)
        };
        const mathUtils::Vec3 right{
            (forward.x * cosine) - (forward.z * sine),
            0.0f,
    ```

#### OA-PHYS-PROBE {#oa-phys-probe}

`src/App/GameplayPhysicsCharacterIntegration.cpp:37` · Sphere cast / raycast · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/GameplayPhysicsCharacterIntegration.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/GameplayPhysicsCharacterIntegration.cpp#L37">GitHub</a>

Якорь: `` bool appRuntime::GameplayPhysicsObstacleQuery::Probe( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="37"
    bool appRuntime::GameplayPhysicsObstacleQuery::Probe(
        const rendern::GameplayObstacleProbeRequest& request,
        rendern::GameplayObstacleProbeHit& hit) const noexcept
    {
        const auto physicsHit = request.clearanceRadius > 0.0f &&
            std::isfinite(request.clearanceRadius)
            ? physicsWorld_.ShapeCastClosest({
                .shape = physics::SphereShapeDescriptor{.radius = request.clearanceRadius},
    ```

#### OA-PHYS-QUERY {#oa-phys-query}

`src/App/GameplayPhysicsObstacleQuery.h:15` · Jolt-адаптер, StaticWorld по умолчанию · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/GameplayPhysicsObstacleQuery.h">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/GameplayPhysicsObstacleQuery.h#L15">GitHub</a>

Якорь: `` class GameplayPhysicsObstacleQuery final : public rendern::IGameplayObstacleQuery ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="15"
        class GameplayPhysicsObstacleQuery final : public rendern::IGameplayObstacleQuery
        {
        public:
            explicit GameplayPhysicsObstacleQuery(
                const physics::JoltPhysicsWorld& physicsWorld,
                physics::PhysicsQueryLayerMask layerMask =
                    physics::PhysicsQueryLayerMask::StaticWorld) noexcept;

    ```

#### OA-PHYS-SUPPORT {#oa-phys-support}

`src/App/GameplayPhysicsCharacterIntegration.cpp:66` · Raycast вниз · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/GameplayPhysicsCharacterIntegration.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/GameplayPhysicsCharacterIntegration.cpp#L66">GitHub</a>

Якорь: `` bool appRuntime::GameplayPhysicsObstacleQuery::ProbeSupport( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="66"
    bool appRuntime::GameplayPhysicsObstacleQuery::ProbeSupport(
        const rendern::GameplaySupportProbeRequest& request,
        rendern::GameplaySupportProbeHit& hit) const noexcept
    {
        const auto physicsHit = physicsWorld_.RayCastClosest({
            .origin = request.origin,
            .direction = {0.0f, -1.0f, 0.0f},
            .maxDistance = request.maximumDistance,
    ```

#### OA-QUERY {#oa-query}

`src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:79` · Интерфейс геометрических запросов · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm#L79">GitHub</a>

Якорь: `` class IGameplayObstacleQuery ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="79"
        class IGameplayObstacleQuery
        {
        public:
            virtual ~IGameplayObstacleQuery() = default;

            [[nodiscard]] virtual bool Probe(
                const GameplayObstacleProbeRequest& request,
                GameplayObstacleProbeHit& hit) const noexcept = 0;
    ```

#### OA-SANITIZE {#oa-sanitize}

`src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:189` · Нормализация параметров, горизонт скорости · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm#L189">GitHub</a>

Якорь: `` SanitizedSettings Sanitize( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="189"
        [[nodiscard]] SanitizedSettings Sanitize(
            const rendern::GameplayObstacleAvoidanceInput& input,
            const rendern::GameplayObstacleAvoidanceSettings& settings) noexcept
        {
            const float angleDegrees = std::isfinite(settings.sideProbeAngleDegrees)
                ? std::clamp(settings.sideProbeAngleDegrees, 0.0f, 90.0f)
                : 0.0f;
            const float characterRadius = FiniteOrZero(input.characterRadius);
    ```

#### OA-SETTINGS {#oa-settings}

`src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:102` · Параметры avoidance · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm#L102">GitHub</a>

Якорь: `` struct GameplayObstacleAvoidanceSettings ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="102"
        struct GameplayObstacleAvoidanceSettings
        {
            float forwardProbeDistance{3.0f};
            float sideProbeDistance{2.0f};
            float sideProbeAngleDegrees{30.0f};
            float sideSwitchClearanceAdvantage{0.15f};
            float clearanceMargin{0.02f};
            float supportProbeForwardDistance{0.75f};
    ```

#### OA-SIDE {#oa-side}

`src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:487` · Выбор стороны и гистерезис · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm#L487">GitHub</a>

Якорь: `` // Equal clearance always selects left, keeping symmetric situations deterministic. ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="487"
        // Equal clearance always selects left, keeping symmetric situations deterministic.
        const GameplayObstacleAvoidanceSide preferredSide =
            leftResult.clearance >= rightResult.clearance
            ? GameplayObstacleAvoidanceSide::Left
            : GameplayObstacleAvoidanceSide::Right;
        GameplayObstacleAvoidanceSide chosenSide = preferredSide;
        const GameplayObstacleAvoidanceSide previousSide = state.committedSide;
        if (leftSupported != rightSupported)
    ```

#### OA-SUPPORT {#oa-support}

`src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:281` · Проверка опоры кандидата · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm#L281">GitHub</a>

Якорь: `` bool HasSupport( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="281"
        [[nodiscard]] bool HasSupport(
            const rendern::IGameplayObstacleQuery& query,
            const mathUtils::Vec3& probeOrigin,
            const mathUtils::Vec3& direction,
            const SanitizedSettings& settings) noexcept
        {
            const float maximumDistance = settings.supportUpOffset +
                settings.maximumSupportDropDistance;
    ```

#### OA-SUPPORT-DEFAULT {#oa-support-default}

`src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:89` · Синтетическая опора по умолчанию · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm#L89">GitHub</a>

Якорь: `` virtual bool ProbeSupport( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="89"
            [[nodiscard]] virtual bool ProbeSupport(
            const GameplaySupportProbeRequest& request,
             GameplaySupportProbeHit& hit) const noexcept
            {
                hit = {
                    .distance = 0.0f,
                    .position = request.origin,
                    .normal = {0.0f, 1.0f, 0.0f}
    ```

#### OA-WEIGHTS {#oa-weights}

`src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:157` · Веса escape · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm#L157">GitHub</a>

Якорь: `` constexpr float SelectedFeelerWeight = 0.60f; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="157"
        constexpr float SelectedFeelerWeight = 0.60f;
        constexpr float SurfaceTangentWeight = 0.40f;
        constexpr float ObstacleSeparationBias = 0.30f;

        struct SanitizedSettings
        {
            float forwardDistance{0.0f};
            float sideDistance{0.0f};
    ```

## Traversal и физика

#### CM-MOTOR {#cm-motor}

`src/Gameplay/Character/GameplayCharacterComponents.cppm:46` · Скорости и ускорения мотора по умолчанию · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Character/GameplayCharacterComponents.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Character/GameplayCharacterComponents.cppm#L46">GitHub</a>

Якорь: `` float maxRunSpeed{ 4.5f }; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="46"
            float maxRunSpeed{ 4.5f };
            float acceleration{ 12.0f };
            float deceleration{ 16.0f };
            float backwardSpeedScale{ 0.72f };
            float airDeceleration{ 2.5f };
            float jumpVerticalSpeed{ 5.5f };
        };

    ```

#### CM-MOVEMENT {#cm-movement}

`src/Gameplay/Character/CharacterMovement.cppm:136` · Команда → целевая скорость · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Character/CharacterMovement.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Character/CharacterMovement.cppm#L136">GitHub</a>

Якорь: `` const mathUtils::Vec3 targetVelocity = command->moveWorld * (targetSpeed * command->moveMagnitude); ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="136"
                const mathUtils::Vec3 targetVelocity = command->moveWorld * (targetSpeed * command->moveMagnitude);

                if (movementState != nullptr &&
                    jumpRequested &&
                    movementState->jumpPhase == GameplayJumpPhase::None)
                {
                    // Capture the actual planar momentum at jump preparation start.
                    // Ground movement continues until the explicit physical attempt.
    ```

#### CM-PHYSICAL {#cm-physical}

`src/Gameplay/Character/GameplayCharacterComponents.cppm:70` · Полная высота = цилиндр + 2r · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Character/GameplayCharacterComponents.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Character/GameplayCharacterComponents.cppm#L70">GitHub</a>

Якорь: `` constexpr float GetTotalHeight() const noexcept ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="70"
            [[nodiscard]] constexpr float GetTotalHeight() const noexcept
            {
                return cylinderHeight + 2.0f * radius;
            }

            [[nodiscard]] constexpr physics::PhysicsCharacterDescriptor BuildPhysicsCharacterDescriptor(
                const mathUtils::Vec3 position,
                const float maximumSpeed) const noexcept
    ```

#### PH-BLOCKED {#ph-blocked}

`src/App/GameplayPhysicsCharacterIntegration.cpp:251` · Пороги physicallyBlocked · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/GameplayPhysicsCharacterIntegration.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/GameplayPhysicsCharacterIntegration.cpp#L251">GitHub</a>

Якорь: `` constexpr float kBlockedIntentSpeed = 0.1f; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="251"
        constexpr float kBlockedIntentSpeed = 0.1f;
        constexpr float kBlockedProgressRatio = 0.1f;
        constexpr float kBlockedPersistenceSeconds = 0.25f;
        constexpr float kFixedDeltaSeconds = 1.0f / 60.0f;

        rendern::GameplayWorld& world = gameplayRuntime.GetWorld();

        for (const rendern::EntityHandle entity : gameplayRuntime.GetNodeBoundEntities())
    ```

#### PH-BLOCKED-CALC {#ph-blocked-calc}

`src/App/GameplayPhysicsCharacterIntegration.cpp:327` · Вычисление physicallyBlocked · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/GameplayPhysicsCharacterIntegration.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/GameplayPhysicsCharacterIntegration.cpp#L327">GitHub</a>

Якорь: `` movementState->physicallyBlocked = ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="327"
            movementState->physicallyBlocked =
                movementState->physicalBlockedSeconds >= kBlockedPersistenceSeconds;
        }

        return true;
    }

    bool appRuntime::DestroyGameplayPhysicsCharacters(
    ```

#### PH-ROOT-OFFSET {#ph-root-offset}

`src/App/GameplayPhysicsCharacterIntegration.cpp:292` · Transform = основание капсулы · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/GameplayPhysicsCharacterIntegration.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/GameplayPhysicsCharacterIntegration.cpp#L292">GitHub</a>

Якорь: `` transform->position = *position + binding->visualRootOffset; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="292"
            transform->position = *position + binding->visualRootOffset;
            motor->velocity = *velocity;
            movementState->grounded = ground->bIsWalkable;
            movementState->falling = !ground->bIsSupported && velocity->y < 0.0f;
            movementState->jumping = movementState->jumpPhase == rendern::GameplayJumpPhase::Airborne;
            if (movementState->jumpPhase == rendern::GameplayJumpPhase::Airborne &&
                !ground->bIsSupported)
            {
    ```

#### TV-DOOR {#tv-door}

`src/Gameplay/Traversal/DoorTraversalExecutor.cppm:17` · Door executor · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Traversal/DoorTraversalExecutor.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Traversal/DoorTraversalExecutor.cppm#L17">GitHub</a>

Якорь: `` class DoorTraversalExecutor final : public IGameplayTraversalExecutor ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="17"
        class DoorTraversalExecutor final : public IGameplayTraversalExecutor
        {
        public:
            static constexpr GameplayTraversalTypeId kTypeId{kDoorTraversalTypeId};

            DoorTraversalExecutor(
                GameplayWorld& world,
                GameplayObjectReservationSystem& reservationSystem,
    ```

#### TV-DOOR-TICK {#tv-door-tick}

`src/Gameplay/Traversal/DoorTraversalExecutor.cppm:74` · Подход к InteractionPoint и открытие · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Traversal/DoorTraversalExecutor.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Traversal/DoorTraversalExecutor.cppm#L74">GitHub</a>

Якорь: `` GameplayTraversalExecutionResult Tick( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="74"
            [[nodiscard]] GameplayTraversalExecutionResult Tick(
                const GameplayTraversalExecutionContext& context, float) override
            {
                auto it = activeByAgent_.find(context.agentEntity);
                if (it == activeByAgent_.end() || !ContextsMatch_(it->second.context, context))
                {
                    return GameplayTraversalExecutionResult::Failed;
                }
    ```

#### TV-EXEC-REG {#tv-exec-reg}

`src/Gameplay/Traversal/GameplayTraversalExecutorRegistry.cppm:14` · Реестр executors (невладеющий) · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Traversal/GameplayTraversalExecutorRegistry.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Traversal/GameplayTraversalExecutorRegistry.cppm#L14">GitHub</a>

Якорь: `` class GameplayTraversalExecutorRegistry ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="14"
        class GameplayTraversalExecutorRegistry
        {
        public:
            [[nodiscard]] bool Register(
                const GameplayTraversalTypeId typeId,
                IGameplayTraversalExecutor& executor)
            {
                return Register_(typeId, executor, true);
    ```

#### TV-EXECUTOR {#tv-executor}

`src/Gameplay/Traversal/GameplayTraversalExecutor.cppm:36` · Интерфейс executor · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Traversal/GameplayTraversalExecutor.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Traversal/GameplayTraversalExecutor.cppm#L36">GitHub</a>

Якорь: `` class IGameplayTraversalExecutor ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="36"
        class IGameplayTraversalExecutor
        {
        public:
            virtual ~IGameplayTraversalExecutor() = default;
            [[nodiscard]] virtual GameplayTraversalExecutionResult Start(
                const GameplayTraversalExecutionContext& context) = 0;
            [[nodiscard]] virtual GameplayTraversalExecutionResult Tick(
                const GameplayTraversalExecutionContext& context, float deltaSeconds) = 0;
    ```

#### TV-JUMP {#tv-jump}

`src/Gameplay/Traversal/JumpTraversalExecutor.cppm:18` · Jump executor · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Traversal/JumpTraversalExecutor.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Traversal/JumpTraversalExecutor.cppm#L18">GitHub</a>

Якорь: `` class JumpTraversalExecutor final : public IGameplayTraversalExecutor ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="18"
        class JumpTraversalExecutor final : public IGameplayTraversalExecutor
        {
        public:
            static constexpr GameplayTraversalTypeId kTypeId{kJumpTraversalTypeId};

            explicit JumpTraversalExecutor(
                GameplayWorld& world,
                const GameplayTraversalLinkRegistry& linkRegistry,
    ```

#### TV-JUMP-DATA {#tv-jump-data}

`src/Gameplay/Traversal/GameplayTraversalLink.cppm:42` · Данные прыжка · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Traversal/GameplayTraversalLink.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Traversal/GameplayTraversalLink.cppm#L42">GitHub</a>

Якорь: `` struct GameplayJumpTraversalData ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="42"
        struct GameplayJumpTraversalData
        {
            mathUtils::Vec3 takeoffPosition{};
            mathUtils::Vec3 landingPosition{};
            float verticalSpeed{0.0f};
            float takeoffTolerance{0.0f};
            float landingHorizontalTolerance{0.0f};
            float landingVerticalTolerance{0.0f};
    ```

#### TV-JUMP-TICK {#tv-jump-tick}

`src/Gameplay/Traversal/JumpTraversalExecutor.cppm:49` · Фазы прыжка · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Traversal/JumpTraversalExecutor.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Traversal/JumpTraversalExecutor.cppm#L49">GitHub</a>

Якорь: `` GameplayTraversalExecutionResult Tick( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="49"
            [[nodiscard]] GameplayTraversalExecutionResult Tick(
                const GameplayTraversalExecutionContext& context, const float deltaSeconds) override
            {
                auto it = activeByAgent_.find(context.agentEntity);
                if (it == activeByAgent_.end() || !ContextsMatch_(it->second.context, context) ||
                    !ResolveExpectedLink_(context) || !HasRequiredAgentState_(context.agentEntity))
                {
                    return Fail_(it, context.agentEntity);
    ```

#### TV-JUMP-TIMEOUT {#tv-jump-timeout}

`src/Gameplay/Traversal/JumpTraversalExecutor.cppm:172` · Тайм-аут 10 с · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Traversal/JumpTraversalExecutor.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Traversal/JumpTraversalExecutor.cppm#L172">GitHub</a>

Якорь: `` static constexpr float kTraversalTimeoutSeconds = 10.0f; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="172"
            static constexpr float kTraversalTimeoutSeconds = 10.0f;

            [[nodiscard]] std::optional<GameplayTraversalLink> ResolveExpectedLink_(
                const GameplayTraversalExecutionContext& context) const noexcept
            {
                const auto link = linkRegistry_.Find(context.traversalLink);
                if (!link || context.traversalTypeId != kJumpTraversalTypeId ||
                    link->traversalTypeId != kJumpTraversalTypeId ||
    ```

#### TV-LINK {#tv-link}

`src/Gameplay/Traversal/GameplayTraversalLink.cppm:61` · Link: handle, тип, цель, jump-данные · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Traversal/GameplayTraversalLink.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Traversal/GameplayTraversalLink.cppm#L61">GitHub</a>

Якорь: `` struct GameplayTraversalLink ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="61"
        struct GameplayTraversalLink
        {
            GameplayTraversalLinkHandle handle{};
            GameplayTraversalTypeId traversalTypeId{};
            EntityHandle targetEntity{kNullEntity};
            GameplayJumpTraversalData jump{};

            [[nodiscard]] bool IsValid() const noexcept
    ```

#### TV-LINK-REG {#tv-link-reg}

`src/Gameplay/Traversal/GameplayTraversalLinkRegistry.cppm:23` · Реестр links · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Traversal/GameplayTraversalLinkRegistry.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Traversal/GameplayTraversalLinkRegistry.cppm#L23">GitHub</a>

Якорь: `` class GameplayTraversalLinkRegistry ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="23"
        class GameplayTraversalLinkRegistry
        {
        public:
            [[nodiscard]] bool Register(GameplayTraversalLink link)
            {
                if (!link.IsValid())
                {
                    return false;
    ```

## Ресурсы, события, брони

#### EV-WORLD {#ev-world}

`src/Gameplay/Events/GameplayWorldEvent.cppm:14` · Типы мировых событий · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Events/GameplayWorldEvent.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Events/GameplayWorldEvent.cppm#L14">GitHub</a>

Якорь: `` enum class GameplayWorldEventType ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="14"
        enum class GameplayWorldEventType : std::uint8_t
        {
            PickupCollected,
            ResourcePurchased,
            HideEntityRequested
        };

        struct GameplayWorldEvent
    ```

#### PK-SYSTEM {#pk-system}

`src/Gameplay/Pickup/GameplayPickupSystem.cppm:14` · Сбор pickup без учёта броней · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Pickup/GameplayPickupSystem.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Pickup/GameplayPickupSystem.cppm#L14">GitHub</a>

Якорь: `` class GameplayPickupSystem ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="14"
        class GameplayPickupSystem
        {
        public:
            void Update(GameplayWorld& world, std::span<const EntityHandle> collectors,
                std::vector<GameplayWorldEvent>& outEvents) const
            {
                std::vector<EntityHandle> pickups;
                world.CollectPickupEntities(pickups);
    ```

#### RV-SYSTEM {#rv-system}

`src/Gameplay/Interaction/GameplayObjectReservationSystem.cppm:12` · Брони объектов · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Interaction/GameplayObjectReservationSystem.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Interaction/GameplayObjectReservationSystem.cppm#L12">GitHub</a>

Якорь: `` class GameplayObjectReservationSystem ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="12"
        class GameplayObjectReservationSystem
        {
        public:
            [[nodiscard]] bool TryReserve(
                const GameplayWorld& world,
                const EntityHandle objectEntity,
                const EntityHandle agentEntity)
            {
    ```

#### RV-TRY {#rv-try}

`src/Gameplay/Interaction/GameplayObjectReservationSystem.cppm:34` · Повторная бронь владельцем — успех · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Interaction/GameplayObjectReservationSystem.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Interaction/GameplayObjectReservationSystem.cppm#L34">GitHub</a>

Якорь: `` const auto [it, bInserted] = ownerByObject_.try_emplace(objectEntity, agentEntity); ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="34"
                const auto [it, bInserted] = ownerByObject_.try_emplace(objectEntity, agentEntity);
                return bInserted || it->second == agentEntity;
            }

            [[nodiscard]] bool Release(const EntityHandle objectEntity, const EntityHandle agentEntity) noexcept
            {
                const auto it = ownerByObject_.find(objectEntity);
                if (it == ownerByObject_.end()
    ```

## Runtime-фасад и порядок кадра

#### AP-AFTER-PHYS {#ap-after-phys}

`src/App/AppLifecycleImpl/AppLifecycle_TickImpl.inl:246` · Feedback, PostPhysics, анимация · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/AppLifecycleImpl/AppLifecycle_TickImpl.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/AppLifecycleImpl/AppLifecycle_TickImpl.inl#L246">GitHub</a>

Якорь: `` static void UpdateGameplayAfterPhysicsAndAnimation( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="246"
    static void UpdateGameplayAfterPhysicsAndAnimation(
        AppState& app, const rendern::GameplayUpdateContext& gameplayCtx, const float deltaSeconds)
    {
        if (app.runtimeState.gameplayRuntime)
        {
            auto* physicsWorld = app.physicsState.joltPhysicsWorld.get();
            if (gameplayCtx.mode == rendern::GameplayRuntimeMode::Game && physicsWorld != nullptr &&
                !appRuntime::ApplyGameplayPhysicsCharacterFeedback(
    ```

#### AP-BEFORE-PHYS {#ap-before-phys}

`src/App/AppLifecycleImpl/AppLifecycle_TickImpl.inl:208` · BeginFrame, PrePhysics, submit velocities · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/AppLifecycleImpl/AppLifecycle_TickImpl.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/AppLifecycleImpl/AppLifecycle_TickImpl.inl#L208">GitHub</a>

Якорь: `` static rendern::GameplayUpdateContext UpdateGameplayBeforePhysics(AppState& app, float deltaSeconds) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="208"
    static rendern::GameplayUpdateContext UpdateGameplayBeforePhysics(AppState& app, float deltaSeconds)
    {
        CORE_ASSERT_RUNTIME_THREAD();

        const rendern::GameplayUpdateContext gameplayCtx = BuildGameplayUpdateContext(app, deltaSeconds);
        auto* gameplayRuntime = app.runtimeState.gameplayRuntime.get();

        if (gameplayRuntime)
    ```

#### AP-DEBUG-DRAW {#ap-debug-draw}

`src/App/AppDebugVisualization.cpp:387` · Рисование маршрутов · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/AppDebugVisualization.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/AppDebugVisualization.cpp#L387">GitHub</a>

Якорь: `` if (app.graphicsState.rendererSettings.drawNavigationPathDebug && runtime.gameplayRuntime) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="387"
            if (app.graphicsState.rendererSettings.drawNavigationPathDebug && runtime.gameplayRuntime)
            {
                constexpr std::uint32_t navigationPathColor = 0xffff40a6u;
                constexpr float navigationPathHalfWidth = 0.03f;
                for (const auto& entry : runtime.gameplayRuntime->GetNavigationDebugRegistry().Routes())
                {
                    const rendern::GameplayRoute& route = entry.second;
                    for (std::size_t index = 0; index + 1u < route.points.size(); ++index)
    ```

#### AP-DEBUG-PLANNED {#ap-debug-planned}

`src/App/AppDebugVisualization.cpp:435` · Planned path и steering telemetry · состояние: `HEAD` · вхождение 1 из 2 · <a href="../../../../src/App/AppDebugVisualization.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/AppDebugVisualization.cpp#L435">GitHub</a>

Якорь: `` if (app.graphicsState.rendererSettings.drawAIPlannedPathDebug ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="435"
            if (app.graphicsState.rendererSettings.drawAIPlannedPathDebug
                   && runtime.gameplayRuntime)
            {
                constexpr std::uint32_t ordinaryColor = 0xff33ccffu;
                constexpr std::uint32_t traversalColor = 0xffff66ffu;
                for (const rendern::GameplayAIPlannedPathDebugAgentView& agent :
                       runtime.gameplayRuntime->BuildAIPlannedPathDebugAgentViews())
                {
    ```

#### AP-FACTORIES {#ap-factories}

`src/App/AppLifecycle.cpp:232` · App создаёт runtime с каталогом · состояние: `HEAD` · вхождение 1 из 2 · <a href="../../../../src/App/AppLifecycle.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/AppLifecycle.cpp#L232">GitHub</a>

Якорь: `` rendern::MakeDefaultGameplayAIDecisionFactories()); ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="232"
                rendern::MakeDefaultGameplayAIDecisionFactories());
            runtimeState.gameplayRuntime->SetObstacleQuery(physicsState.obstacleQuery.get());
            runtimeState.gameplayRuntime->Initialize(
                *contentState.levelAsset, *runtimeState.levelInstance, runtimeState.scene);
            appDevelopment::ScenarioContext developmentContext{
                *runtimeState.gameplayRuntime, *contentState.levelAsset,
                *runtimeState.levelInstance, runtimeState.scene, runtimeState.gameplayMode,
                physicsState.joltPhysicsWorld.get(), runtimeState.navigationProfiles.get()};
    ```

#### AP-NAV-COORD {#ap-nav-coord}

`src/App/AppNavigationCoordinator.cpp:18` · Bake NavMesh по профилям · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/AppNavigationCoordinator.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/AppNavigationCoordinator.cpp#L18">GitHub</a>

Якорь: `` void Advance(appLifecycle::AppState& app) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="18"
        void Advance(appLifecycle::AppState& app)
        {
            auto& runtime = app.runtimeState;
            if (runtime.navigationState == appLifecycle::AppRuntimeState::NavigationState::Pending)
            {
                const app::navigationRuntime::GeometryResult geometry =
                    app::navigationRuntime::BuildLevelNavigationGeometry(*runtime.levelInstance);
                if (geometry.status == app::navigationRuntime::GeometryStatus::WaitingForMeshes)
    ```

#### AP-PHYSICS {#ap-physics}

`src/App/AppLifecycleImpl/AppLifecycle_TickImpl.inl:272` · Шаг Jolt · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/AppLifecycleImpl/AppLifecycle_TickImpl.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/AppLifecycleImpl/AppLifecycle_TickImpl.inl#L272">GitHub</a>

Якорь: `` static void UpdatePhysics(AppState& app, const float deltaSeconds) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="272"
    static void UpdatePhysics(AppState& app, const float deltaSeconds)
    {
        CORE_ASSERT_PHYSICS_THREAD();

        auto* physicsWorld = app.physicsState.joltPhysicsWorld.get();
        if (physicsWorld == nullptr
            || app.runtimeState.gameplayMode != rendern::GameplayRuntimeMode::Game)
        {
    ```

#### AP-SCENARIO {#ap-scenario}

`src/App/AppLifecycle.cpp:529` · Сценарий обновляется до BeginFrame · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/AppLifecycle.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/AppLifecycle.cpp#L529">GitHub</a>

Якорь: `` app.developmentScenarioRuntime.Update(developmentContext); ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="529"
            app.developmentScenarioRuntime.Update(developmentContext);
            const auto streamingEnd = Clock::now();

            const auto timingStart = Clock::now();
            const float deltaSeconds = UpdateFrameTimingAndLoadingOverlay(app);
            const auto timingEnd = Clock::now();

            const auto inputStart = Clock::now();
    ```

#### AP-SET-QUERY {#ap-set-query}

`src/App/AppLifecycle.cpp:233` · Подключение физического query · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/AppLifecycle.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/AppLifecycle.cpp#L233">GitHub</a>

Якорь: `` runtimeState.gameplayRuntime->SetObstacleQuery(physicsState.obstacleQuery.get()); ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="233"
            runtimeState.gameplayRuntime->SetObstacleQuery(physicsState.obstacleQuery.get());
            runtimeState.gameplayRuntime->Initialize(
                *contentState.levelAsset, *runtimeState.levelInstance, runtimeState.scene);
            appDevelopment::ScenarioContext developmentContext{
                *runtimeState.gameplayRuntime, *contentState.levelAsset,
                *runtimeState.levelInstance, runtimeState.scene, runtimeState.gameplayMode,
                physicsState.joltPhysicsWorld.get(), runtimeState.navigationProfiles.get()};
            app.developmentScenarioRuntime.OnLevelLoaded(developmentContext);
    ```

#### AP-TICK {#ap-tick}

`src/App/AppLifecycle.cpp:546` · Порядок: gameplay → physics → post · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/AppLifecycle.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/AppLifecycle.cpp#L546">GitHub</a>

Якорь: `` const rendern::GameplayUpdateContext gameplayCtx = UpdateGameplayBeforePhysics(app, deltaSeconds); ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="546"
            const rendern::GameplayUpdateContext gameplayCtx = UpdateGameplayBeforePhysics(app, deltaSeconds);
            UpdatePhysics(app, deltaSeconds);
            UpdateGameplayAfterPhysicsAndAnimation(app, gameplayCtx, deltaSeconds);
            appDebugVisualization::ComposeExternalDebugGeometry(app);
            const auto gameplayEnd = Clock::now();

            const auto imguiStart = Clock::now();
            const void* imguiDrawData = appUi::BuildImGuiFrameIfEnabled(
    ```

#### GR-AI-ORDER {#gr-ai-order}

`src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl:281` · Решения перед aiSystem_.Update · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl#L281">GitHub</a>

Якорь: `` UpdateActiveAIDecisions_(); ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="281"
                UpdateActiveAIDecisions_();
                hideWorldEventSubjects(eventCountBeforeAIDecisions);
                aiSystem_.Update(world_, ctx.deltaSeconds);
                UpdateGameplayCombatRequests(world_, nodeBoundEntities_, actionDefinitions_);
                UpdateGameplayInteractionRequests(world_, nodeBoundEntities_, actionDefinitions_);
                ExecuteGameplayGraphs_(ctx);
                UpdateGameplayCharacterMovement(world_, nodeBoundEntities_, ctx.deltaSeconds);
            }
    ```

#### GR-AI-UPDATE {#gr-ai-update}

`src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl:283` · Тик задач AI · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl#L283">GitHub</a>

Якорь: `` aiSystem_.Update(world_, ctx.deltaSeconds); ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="283"
                aiSystem_.Update(world_, ctx.deltaSeconds);
                UpdateGameplayCombatRequests(world_, nodeBoundEntities_, actionDefinitions_);
                UpdateGameplayInteractionRequests(world_, nodeBoundEntities_, actionDefinitions_);
                ExecuteGameplayGraphs_(ctx);
                UpdateGameplayCharacterMovement(world_, nodeBoundEntities_, ctx.deltaSeconds);
            }

            void GameplayRuntime::PostPhysicsUpdate(const GameplayUpdateContext& ctx)
    ```

#### GR-BEGIN {#gr-begin}

`src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl:218` · Очистка кадровых данных · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl#L218">GitHub</a>

Якорь: `` void GameplayRuntime::BeginFrame() ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="218"
            void GameplayRuntime::BeginFrame()
            {
                CORE_ASSERT_RUNTIME_THREAD();

                recentNotifyEvents_.clear();
                recentGameplayEvents_.clear();
                currentWorldEvents_.clear();
                CompactTrackedState_();
    ```

#### GR-CANCEL-ACTION {#gr-cancel-action}

`src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl:561` · Отмена действия · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl#L561">GitHub</a>

Якорь: `` void GameplayRuntime::CancelAIAction(const EntityHandle agentEntity) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="561"
            void GameplayRuntime::CancelAIAction(const EntityHandle agentEntity)
            {
                CORE_ASSERT_RUNTIME_THREAD();
                aiSystem_.CancelAction(agentEntity);
                steeringDebugRegistry_.Clear(agentEntity);
                navigationDebugRegistry_.Clear(agentEntity);
            }

    ```

#### GR-CANCEL-DECISION {#gr-cancel-decision}

`src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl:666` · Отмена и удаление решения · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl#L666">GitHub</a>

Якорь: `` void GameplayRuntime::CancelAIDecision(const EntityHandle agentEntity) noexcept ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="666"
            void GameplayRuntime::CancelAIDecision(const EntityHandle agentEntity) noexcept
            {
                CORE_ASSERT_RUNTIME_THREAD();
                const auto found = activeAIDecisions_.find(agentEntity);
                if (found == activeAIDecisions_.end())
                {
                    return;
                }
    ```

#### GR-CREATE-BINDING {#gr-create-binding}

`src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl:584` · Программный MoveTo binding с query и debug · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl#L584">GitHub</a>

Якорь: `` GameplayRuntime::CreateAIMoveToActionBinding( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="584"
                GameplayRuntime::CreateAIMoveToActionBinding(
                IAIMoveToActionRequestProvider& requestProvider)
            {
                CORE_ASSERT_RUNTIME_THREAD();
                return std::make_unique<AIMoveToActionBinding>(
                    world_, traversalLinkRegistry_, traversalExecutorRegistry_, requestProvider,
                    obstacleQuery_, &steeringDebugRegistry_, &navigationDebugRegistry_);
            }
    ```

#### GR-CTOR {#gr-ctor}

`src/Gameplay/Runtime/GameplayRuntime.cppm:91` · Конструктор по умолчанию: пустой каталог · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime.cppm#L91">GitHub</a>

Якорь: `` GameplayRuntime() : GameplayRuntime(GameplayAIDecisionFactoryRegistry{}) {} ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="91"
            GameplayRuntime() : GameplayRuntime(GameplayAIDecisionFactoryRegistry{}) {}

            explicit GameplayRuntime(GameplayAIDecisionFactoryRegistry decisionFactories)
                : aiDecisionFactoryRegistry_(std::move(decisionFactories))
            {
                const bool bRegisteredDoor = traversalExecutorRegistry_.RegisterRuntimeOwned(
                    DoorTraversalExecutor::kTypeId, doorTraversalExecutor_);
                const bool bRegisteredJump = traversalExecutorRegistry_.RegisterRuntimeOwned(
    ```

#### GR-DEBUG-VIEWS {#gr-debug-views}

`src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl:704` · Снимки отладки · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl#L704">GitHub</a>

Якорь: `` std::vector<GameplayAIDebugAgentView> GameplayRuntime::BuildAIDebugAgentViews() const ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="704"
            std::vector<GameplayAIDebugAgentView> GameplayRuntime::BuildAIDebugAgentViews() const
            {
                std::vector<GameplayAIDebugAgentView> result;
                result.reserve(activeAIDecisions_.size());
                for (const auto& [agent, decision] : activeAIDecisions_)
                {
                    const auto* inspection =
                        dynamic_cast<const IGameplayGOAPInspection*>(decision.get());
    ```

#### GR-MEMBERS {#gr-members}

`src/Gameplay/Runtime/GameplayRuntime.cppm:262` · Владение AISystem, решениями, реестрами · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime.cppm#L262">GitHub</a>

Якорь: `` AISystem aiSystem_{}; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="262"
            AISystem aiSystem_{};
            GameplayAIDecisionFactoryRegistry aiDecisionFactoryRegistry_{};
            std::unordered_map<EntityHandle, std::unique_ptr<GameplayAIDecisionInstance>> activeAIDecisions_{};
            GameplayTraversalLinkRegistry traversalLinkRegistry_{};
            GameplayTraversalExecutorRegistry traversalExecutorRegistry_{};
            GameplayUnsupportedTraversalExecutor unsupportedTraversalExecutor_{};
            GameplayObjectReservationSystem objectReservationSystem_{};
            GameplayPickupSystem pickupSystem_{};
    ```

#### GR-MODE-EDITOR {#gr-mode-editor}

`src/Gameplay/Runtime/GameplayRuntime_ModeAndBootstrap.inl:142` · Выход в Editor очищает AI · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_ModeAndBootstrap.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_ModeAndBootstrap.inl#L142">GitHub</a>

Якорь: `` CancelAllAIDecisions_(); ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="142"
                    CancelAllAIDecisions_();
                    aiSystem_.Reset();
                    objectReservationSystem_.Reset();
                    // External traversal services belong to one simulation session.
                    traversalLinkRegistry_.Reset();
                    traversalExecutorRegistry_.ResetExternalRegistrations();
                    ResetSimulationState_();
                    if (ctx.scene != nullptr && ctx.levelInstance != nullptr)
    ```

#### GR-POSTPHYS {#gr-postphys}

`src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl:290` · PostPhysicsUpdate · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl#L290">GitHub</a>

Якорь: `` void GameplayRuntime::PostPhysicsUpdate(const GameplayUpdateContext& ctx) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="290"
            void GameplayRuntime::PostPhysicsUpdate(const GameplayUpdateContext& ctx)
            {
                CORE_ASSERT_RUNTIME_THREAD();
                if (ctx.mode != GameplayRuntimeMode::Game)
                {
                    return;
                }

    ```

#### GR-PREPHYS {#gr-prephys}

`src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl:233` · PrePhysicsUpdate · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl#L233">GitHub</a>

Якорь: `` void GameplayRuntime::PrePhysicsUpdate(const GameplayUpdateContext& ctx) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="233"
            void GameplayRuntime::PrePhysicsUpdate(const GameplayUpdateContext& ctx)
            {
                CORE_ASSERT_RUNTIME_THREAD();

                RemoveDeadNodeBoundEntities_(ctx);
                EnsureBootstrapEntity_(ctx);

                if (ctx.mode != lastMode_)
    ```

#### GR-RESET-FRAME {#gr-reset-frame}

`src/Gameplay/Runtime/GameplayRuntime_FrameLifecycle.inl:1` · Сброс CharacterCommand каждый кадр · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_FrameLifecycle.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_FrameLifecycle.inl#L1">GitHub</a>

Якорь: `` void GameplayRuntime::ResetEntityFrameState_(const EntityHandle entity) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="1"
            void GameplayRuntime::ResetEntityFrameState_(const EntityHandle entity)
            {
                if (GameplayInputIntentComponent* intent = world_.TryGetInputIntent(entity))
                {
                    *intent = {};
                }

                if (GameplayCharacterCommandComponent* command = world_.TryGetCharacterCommand(entity))
    ```

#### GR-START-DECISION {#gr-start-decision}

`src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl:610` · Синхронный старт решения · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl#L610">GitHub</a>

Якорь: `` bool GameplayRuntime::StartAIDecision( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="610"
            bool GameplayRuntime::StartAIDecision(
                const EntityHandle agentEntity, const std::string_view definitionId, std::string* diagnostic)
            {
                CORE_ASSERT_RUNTIME_THREAD();
                if (diagnostic != nullptr)
                {
                    diagnostic->clear();
                }
    ```

#### GR-START-FLEE {#gr-start-flee}

`src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl:509` · Прямой FleeTarget · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl#L509">GitHub</a>

Якорь: `` AIActionExecutionStatus GameplayRuntime::StartAIFleeTarget( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="509"
            [[nodiscard]] AIActionExecutionStatus GameplayRuntime::StartAIFleeTarget(
                const EntityHandle agentEntity, const EntityHandle targetEntity,
                const AIFleeTargetSettings& settings)
            {
                CORE_ASSERT_RUNTIME_THREAD();
                return AIFleeTargetAction::Start(
                    aiSystem_, world_, agentEntity, targetEntity, settings, obstacleQuery_, {},
                    &steeringDebugRegistry_);
    ```

#### GR-START-FOLLOW {#gr-start-follow}

`src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl:499` · Прямой FollowTarget · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl#L499">GitHub</a>

Якорь: `` AIActionExecutionStatus GameplayRuntime::StartAIFollowTarget( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="499"
            [[nodiscard]] AIActionExecutionStatus GameplayRuntime::StartAIFollowTarget(
                const EntityHandle agentEntity, const EntityHandle targetEntity,
                const AIFollowTargetSettings& settings)
            {
                CORE_ASSERT_RUNTIME_THREAD();
                return AIFollowTargetAction::Start(
                    aiSystem_, world_, agentEntity, targetEntity, settings, obstacleQuery_, {},
                    &steeringDebugRegistry_);
    ```

#### GR-START-MOVETO {#gr-start-moveto}

`src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl:478` · Прямой MoveTo · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl#L478">GitHub</a>

Якорь: `` AIActionExecutionStatus GameplayRuntime::StartAIMoveTo( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="478"
            [[nodiscard]] AIActionExecutionStatus GameplayRuntime::StartAIMoveTo(
                const EntityHandle agentEntity,
                const GameplayRouteGraph& routeGraph,
                const GameplayRouteNodeId startNodeId,
                const GameplayRouteNodeId goalNodeId,
                const GameplayArrivalSteeringSettings& steeringSettings)
            {
                CORE_ASSERT_RUNTIME_THREAD();
    ```

#### GR-START-ROUTE {#gr-start-route}

`src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl:461` · Прямой FollowRoute с query и debug · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl#L461">GitHub</a>

Якорь: `` AIActionExecutionStatus GameplayRuntime::StartAIFollowRoute( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="461"
            [[nodiscard]] AIActionExecutionStatus GameplayRuntime::StartAIFollowRoute(
                const EntityHandle agentEntity,
                GameplayRoute route,
                const GameplayArrivalSteeringSettings& steeringSettings)
            {
                CORE_ASSERT_RUNTIME_THREAD();
                return AIFollowRouteAction::Start(
                    aiSystem_,
    ```

#### GR-UPDATE-DECISIONS {#gr-update-decisions}

`src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl:744` · Обновление решений по возрастанию handle · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Gameplay/Runtime/GameplayRuntime_PublicApi.inl#L744">GitHub</a>

Якорь: `` void GameplayRuntime::UpdateActiveAIDecisions_() ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="744"
            void GameplayRuntime::UpdateActiveAIDecisions_()
            {
                std::vector<EntityHandle> orderedAgents;
                orderedAgents.reserve(activeAIDecisions_.size());
                for (const auto& [entity, decision] : activeAIDecisions_)
                {
                    (void)decision;
                    if (world_.IsEntityValid(entity))
    ```

## NavMesh

#### NV-AGENT {#nv-agent}

`src/Navigation/Navigation.cppm:22` · Параметры агента NavMesh · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Navigation/Navigation.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Navigation/Navigation.cppm#L22">GitHub</a>

Якорь: `` struct AgentSettings ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="22"
        struct AgentSettings
        {
            float radius{ 0.4f };
            float height{ 1.8f };
            float maximumStepHeight{ 0.4f };
            float maximumSlopeAngleDegrees{ 45.0f };

            [[nodiscard]] bool IsValid() const noexcept;
    ```

#### NV-AGENT-MAP {#nv-agent-map}

`src/App/LevelNavigation.cppm:30` · Физика персонажа → профиль NavMesh · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/LevelNavigation.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/LevelNavigation.cppm#L30">GitHub</a>

Якорь: `` constexpr navigation::AgentSettings BuildAgentSettings( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="30"
        [[nodiscard]] constexpr navigation::AgentSettings BuildAgentSettings(
            const rendern::GameplayCharacterPhysicalSettingsComponent& settings) noexcept
        {
            return {
                .radius = settings.radius,
                .height = settings.GetTotalHeight(),
                .maximumStepHeight = settings.maximumStepHeight,
                .maximumSlopeAngleDegrees = settings.maximumSlopeAngleDegrees
    ```

#### NV-BUILD-SETTINGS {#nv-build-settings}

`src/Navigation/Navigation.cppm:32` · Параметры bake по умолчанию · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Navigation/Navigation.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Navigation/Navigation.cppm#L32">GitHub</a>

Якорь: `` struct BuildSettings ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="32"
        struct BuildSettings
        {
            AgentSettings agent{};
            float cellSize{ 0.2f };
            float cellHeight{ 0.1f };
            float regionMinSize{ 2.0f };
            float regionMergeSize{ 8.0f };
            float edgeMaxLength{ 12.0f };
    ```

#### NV-CONFIG {#nv-config}

`src/Navigation/Recast/Navigation.cpp:290` · Перевод метров в клетки Recast (ceil/floor) · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Navigation/Recast/Navigation.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Navigation/Recast/Navigation.cpp#L290">GitHub</a>

Якорь: `` config.cs = settings.cellSize; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="290"
            config.cs = settings.cellSize;
            config.ch = settings.cellHeight;
            config.walkableSlopeAngle = settings.agent.maximumSlopeAngleDegrees;
            if (!mathUtils::TryConvertToInt(std::ceil(settings.agent.height / config.ch), config.walkableHeight) ||
                !mathUtils::TryConvertToInt(std::floor(settings.agent.maximumStepHeight / config.ch), config.walkableClimb) ||
                !mathUtils::TryConvertToInt(std::ceil(settings.agent.radius / config.cs), config.walkableRadius) ||
                !mathUtils::TryConvertToInt(settings.edgeMaxLength / config.cs, config.maxEdgeLen) ||
                !mathUtils::TryConvertToInt(settings.regionMinSize * settings.regionMinSize, config.minRegionArea) ||
    ```

#### NV-ERODE {#nv-erode}

`src/Navigation/Recast/Navigation.cpp:375` · Эрозия по радиусу агента · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Navigation/Recast/Navigation.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Navigation/Recast/Navigation.cpp#L375">GitHub</a>

Якорь: `` !rcErodeWalkableArea(&context, config.walkableRadius, *compactHeightfield) || ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="375"
                !rcErodeWalkableArea(&context, config.walkableRadius, *compactHeightfield) ||
                !rcBuildDistanceField(&context, *compactHeightfield) ||
                !rcBuildRegions(
                    &context,
                    *compactHeightfield,
                    0,
                    config.minRegionArea,
                    config.mergeRegionArea))
    ```

#### NV-FINDPATH {#nv-findpath}

`src/Navigation/Recast/Navigation.cpp:522` · Detour: nearest poly, corridor, straight path · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Navigation/Recast/Navigation.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Navigation/Recast/Navigation.cpp#L522">GitHub</a>

Якорь: `` PathResult World::FindPath(const PathRequest& request) const ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="522"
        PathResult World::FindPath(const PathRequest& request) const
        {
            if (!IsInitialized())
            {
                return {};
            }

            if (!IsFinite(request.start) || !IsFinite(request.goal) || !IsFinite(request.searchExtents) ||
    ```

#### NV-GEOM-BUILD {#nv-geom-build}

`src/App/LevelNavigation.cppm:49` · Статические меши уровня в мировых координатах · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/LevelNavigation.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/LevelNavigation.cppm#L49">GitHub</a>

Якорь: `` GeometryResult BuildNavigationGeometry( ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="49"
        [[nodiscard]] GeometryResult BuildNavigationGeometry(
            std::span<const rendern::LevelStaticMeshSource> sources)
        {
            GeometryResult result{};
            std::size_t totalVertices{};
            std::size_t totalIndices{};
            bool waiting = false;

    ```

#### NV-GEOMETRY {#nv-geometry}

`src/Navigation/Navigation.cppm:15` · Правосторонняя система, +Y вверх · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Navigation/Navigation.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Navigation/Navigation.cppm#L15">GitHub</a>

Якорь: `` // Triangle soup is expressed in CoreEngine world space (right-handed, Y-up). ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="15"
        // Triangle soup is expressed in CoreEngine world space (right-handed, Y-up).
        struct Geometry
        {
            std::vector<mathUtils::Vec3> vertices;
            std::vector<std::uint32_t> indices;
        };

        struct AgentSettings
    ```

#### NV-LIMITS {#nv-limits}

`src/Navigation/Recast/Navigation.cpp:137` · Лимиты буферов · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Navigation/Recast/Navigation.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Navigation/Recast/Navigation.cpp#L137">GitHub</a>

Якорь: `` constexpr int MaximumPathPolygons = 2048; ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="137"
            constexpr int MaximumPathPolygons = 2048;
            constexpr int MaximumStraightPathPoints = 2048;

            bool AreBuildSettingsValid(const BuildSettings& settings)
            {
                return settings.agent.IsValid() &&
                    std::isfinite(settings.cellSize) &&
                    settings.cellSize > 0.0f &&
    ```

#### NV-PARTIAL {#nv-partial}

`src/Navigation/Recast/Navigation.cpp:592` · Частичный коридор = NoPath · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Navigation/Recast/Navigation.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Navigation/Recast/Navigation.cpp#L592">GitHub</a>

Якорь: `` if (corridor[corridorCount - 1] != goalPolygon) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="592"
            if (corridor[corridorCount - 1] != goalPolygon)
            {
                return { QueryStatus::NoPath, {} };
            }

            float straightPath[MaximumStraightPathPoints * 3]{};
            unsigned char straightPathFlags[MaximumStraightPathPoints]{};
            dtPolyRef straightPathPolygons[MaximumStraightPathPoints]{};
    ```

#### NV-PROFILES {#nv-profiles}

`src/Navigation/Navigation.cppm:158` · Bake на профиль агента · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Navigation/Navigation.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Navigation/Navigation.cppm#L158">GitHub</a>

Якорь: `` class ProfileRegistry final ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="158"
        class ProfileRegistry final
        {
        public:
            ProfileRegistry();
            ~ProfileRegistry();
            ProfileRegistry(ProfileRegistry&&) noexcept;
            ProfileRegistry& operator=(ProfileRegistry&&) noexcept;
            ProfileRegistry(const ProfileRegistry&) = delete;
    ```

#### NV-QUERY-STATUS {#nv-query-status}

`src/Navigation/Navigation.cppm:54` · Статусы запроса пути · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Navigation/Navigation.cppm">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Navigation/Navigation.cppm#L54">GitHub</a>

Якорь: `` enum class QueryStatus ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="54"
        enum class QueryStatus
        {
            Succeeded,
            NotInitialized,
            InvalidRequest,
            StartNotOnNavMesh,
            GoalNotOnNavMesh,
            NoPath,
    ```

## Development-сценарии

#### SC-NAV-PATH {#sc-nav-path}

`src/App/Development/DevelopmentScenario.cpp:1009` · Maze: NavMesh → GameplayRoute · состояние: `HEAD` · вхождение 2 из 2 · <a href="../../../../src/App/Development/DevelopmentScenario.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/Development/DevelopmentScenario.cpp#L1009">GitHub</a>

Якорь: `` else if constexpr (std::is_same_v<T, StartNavigationPathOperation>) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="1009"
                        else if constexpr (std::is_same_v<T, StartNavigationPathOperation>)
                        {
                            auto result = std::ranges::find(instance.results, op.result,
                                &ScenarioOperationResult::name);
                            if (result == instance.results.end() || context.navigationProfiles == nullptr)
                            {
                                return false;
                            }
    ```

#### SC-REMOVE-TARGET {#sc-remove-target}

`src/App/Development/DevelopmentScenario.cpp:706` · Временное удаление цели (steady_clock) · состояние: `HEAD` · вхождение 2 из 2 · <a href="../../../../src/App/Development/DevelopmentScenario.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/Development/DevelopmentScenario.cpp#L706">GitHub</a>

Якорь: `` else if constexpr (std::is_same_v<T, TemporarilyRemoveTargetAtMidpointOperation>) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="706"
                     else if constexpr (std::is_same_v<T, TemporarilyRemoveTargetAtMidpointOperation>)
                    {
                        const auto agentNode = instance.nodes.find(op.agent);
                        if (agentNode == instance.nodes.end())
                        {
                            return false;
                        }
                        const EntityHandle agent = ResolveEntity(agentNode->second, context);
    ```

#### SC-RUNNER-UPDATE {#sc-runner-update}

`src/App/Development/DevelopmentScenario.cpp:1241` · Обновление сценария · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/Development/DevelopmentScenario.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/Development/DevelopmentScenario.cpp#L1241">GitHub</a>

Якорь: `` void DevelopmentScenarioRunner::Update(ScenarioContext& context) noexcept ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="1241"
        void DevelopmentScenarioRunner::Update(ScenarioContext& context) noexcept
        {
            if (impl_->running && !DevelopmentScenarioOperationExecutor::ExecuteAll(impl_->asset->update, *this, context)) Stop(context);
            for (ScenarioOperationResult& result : impl_->results)
            {
                if (result.status != ScenarioOperationResultStatus::Running)
                {
                    continue;
    ```

#### SC-START-DECISION {#sc-start-decision}

`src/App/Development/DevelopmentScenario.cpp:868` · Сценарий запускает production API · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/App/Development/DevelopmentScenario.cpp">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/App/Development/DevelopmentScenario.cpp#L868">GitHub</a>

Якорь: `` !context.gameplayRuntime.StartAIDecision(entity, op.decision, &result->diagnostic)) ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="868"
                                !context.gameplayRuntime.StartAIDecision(entity, op.decision, &result->diagnostic))
                            {
                                if (result != instance.results.end())
                                {
                                    result->status = ScenarioOperationResultStatus::Failed;
                                }
                                return false;
                            }
    ```

## Ассеты

#### AS-ACCESS-BEHAVIOR {#as-access-behavior}

`assets/ai/behaviors/access_key.behavior.json:6` · Access Key: behavior · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../assets/ai/behaviors/access_key.behavior.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/ai/behaviors/access_key.behavior.json#L6">GitHub</a>

Якорь: `` "routeGraph": "ai/routes/access_key.routes.json", ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="6"
      "routeGraph": "ai/routes/access_key.routes.json",
      "inspectPath": true,
      "observations": [
        {
          "type": "resource_ledger",
          "fact": "coins",
          "pickups": [
            {
    ```

#### AS-ACCESS-BUY {#as-access-buy}

`assets/ai/goap/access_key.goap.json:569` · Access Key: purchase · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../assets/ai/goap/access_key.goap.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/ai/goap/access_key.goap.json#L569">GitHub</a>

Якорь: `` "context": "buy_key", ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="569"
          "context": "buy_key",
          "cost": 1.0,
          "preconditions": [
            {
              "fact": "hasAccessKey",
              "value": false
            },
            {
    ```

#### AS-ACCESS-GOAP {#as-access-goap}

`assets/ai/goap/access_key.goap.json:2` · Access Key: определение · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../assets/ai/goap/access_key.goap.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/ai/goap/access_key.goap.json#L2">GitHub</a>

Якорь: `` "id": "access_key", ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="2"
      "id": "access_key",
      "facts": [
        {
          "name": "hasAccessKey",
          "type": "bool"
        },
        {
          "name": "atDestination",
    ```

#### AS-ACCESS-ROUTES {#as-access-routes}

`assets/ai/routes/access_key.routes.json:3` · Access Key: граф маршрутов · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../assets/ai/routes/access_key.routes.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/ai/routes/access_key.routes.json#L3">GitHub</a>

Якорь: `` "nodes": [ ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="3"
      "nodes": [
        "start",
        "coinA",
        "coinB",
        "coinC",
        "shop",
        "goal"
      ],
    ```

#### AS-CATALOG {#as-catalog}

`assets/ai/decisions/catalog.json:3` · Каталог решений · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../assets/ai/decisions/catalog.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/ai/decisions/catalog.json#L3">GitHub</a>

Якорь: `` "decisions": [ ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="3"
      "decisions": [
        {
          "id": "target_recovery",
          "behavior": "ai/behaviors/target_recovery.behavior.json",
          "bindings": "ai/bindings/target_recovery.bindings.json"
        },
        {
          "id": "marker_visit",
    ```

#### AS-MARKER-BEHAVIOR {#as-marker-behavior}

`assets/ai/behaviors/marker_visit.behavior.json:5` · Marker Visit: behavior · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../assets/ai/behaviors/marker_visit.behavior.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/ai/behaviors/marker_visit.behavior.json#L5">GitHub</a>

Якорь: `` "definition": "ai/goap/marker_visit.goap.json", ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="5"
      "definition": "ai/goap/marker_visit.goap.json",
      "observations": [
        {
          "type": "target_available",
          "target": "destination",
          "fact": "markerPresent",
          "requireInteractionPoint": true
        },
    ```

#### AS-MARKER-BINDINGS {#as-marker-bindings}

`assets/ai/bindings/marker_visit.bindings.json:3` · Marker Visit: роли · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../assets/ai/bindings/marker_visit.bindings.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/ai/bindings/marker_visit.bindings.json#L3">GitHub</a>

Якорь: `` "roles": [ ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="3"
      "roles": [
        {
          "role": "origin",
          "node": "GOAP_Marker_Start"
        },
        {
          "role": "destination",
          "node": "GOAP_Marker_Goal"
    ```

#### AS-MARKER-GOAP {#as-marker-goap}

`assets/ai/goap/marker_visit.goap.json:2` · Marker Visit: определение · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../assets/ai/goap/marker_visit.goap.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/ai/goap/marker_visit.goap.json#L2">GitHub</a>

Якорь: `` "id": "marker_visit", ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="2"
      "id": "marker_visit",
      "facts": [
        {
          "name": "markerReached",
          "type": "bool"
        },
        {
          "name": "markerPresent",
    ```

#### AS-RECOVERY-BEHAVIOR {#as-recovery-behavior}

`assets/ai/behaviors/target_recovery.behavior.json:5` · Target Recovery: behavior · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../assets/ai/behaviors/target_recovery.behavior.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/ai/behaviors/target_recovery.behavior.json#L5">GitHub</a>

Якорь: `` "definition": "ai/goap/target_recovery.goap.json", ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="5"
      "definition": "ai/goap/target_recovery.goap.json",
      "observations": [
        {
          "type": "target_available",
          "target": "destination",
          "fact": "goalAvailable",
          "requireInteractionPoint": true
        },
    ```

#### AS-RECOVERY-GOAP {#as-recovery-goap}

`assets/ai/goap/target_recovery.goap.json:2` · Target Recovery: определение · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../assets/ai/goap/target_recovery.goap.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/ai/goap/target_recovery.goap.json#L2">GitHub</a>

Якорь: `` "id": "target_recovery", ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="2"
      "id": "target_recovery",
      "facts": [
        {
          "name": "atDestination",
          "type": "bool"
        },
        {
          "name": "goalAvailable",
    ```

#### AS-SC-JUMP {#as-sc-jump}

`assets/development/ai_goap_access_key_jump.scenario.json:39` · Сценарий прыжка · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../assets/development/ai_goap_access_key_jump.scenario.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/development/ai_goap_access_key_jump.scenario.json#L39">GitHub</a>

Якорь: `` "registerJumpTraversalLink" ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="39"
        { "op": "registerJumpTraversalLink", "entity": "landing", "handle": 9470001,
          "takeoff": "takeoff", "landing": "landing", "verticalSpeed": 5.5,
          "takeoffTolerance": 0.20, "landingHorizontalTolerance": 0.55,
          "landingVerticalTolerance": 0.30 },
        { "op": "startAIDecision", "entity": "agent", "decision": "access_key_jump", "result": "GOAP decision" }
      ],
      "stop": [
        { "op": "cancelAIDecision", "entity": "agent" }, { "op": "cancelAI", "entity": "agent" },
    ```

#### AS-SC-MAZE {#as-sc-maze}

`assets/development/navigation_pathfinding_maze.scenario.json:53` · Сценарий Maze · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../assets/development/navigation_pathfinding_maze.scenario.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/development/navigation_pathfinding_maze.scenario.json#L53">GitHub</a>

Якорь: `` "startNavigationPath" ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="53"
          "op": "startNavigationPath",
          "entity": "agent",
          "target": "goal",
          "searchExtents": [
            1.0,
            2.0,
            1.0
          ],
    ```

#### AS-SC-PLAYGROUND {#as-sc-playground}

`assets/development/ai_steering_playground.scenario.json:25` · Сценарий Steering Playground · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../assets/development/ai_steering_playground.scenario.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/development/ai_steering_playground.scenario.json#L25">GitHub</a>

Якорь: `` "startFleeTarget" ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="25"
        {"op":"startFleeTarget","entity":"fleeAgent","target":"threat","triggerRadius":5.0,"safeRadius":7.0,"wantsRun":false},
        {"op":"startMoveTo","entity":"routeAgent","nodes":["routeStart","routeCorner","routeGoal"],"edges":[{"from":"routeStart","to":"routeCorner","cost":1},{"from":"routeCorner","to":"routeGoal","cost":1}],"start":"routeStart","goal":"routeGoal","acceptanceRadius":0.2,"slowingRadius":2.0,"wantsRun":false}
      ],
      "update":[],
      "stop":[{"op":"cancelAI","entity":"followAgent"},{"op":"cancelAI","entity":"fleeAgent"},{"op":"cancelAI","entity":"routeAgent"}],
      "reset":[
        {"op":"cancelAI","entity":"followAgent"},{"op":"cancelAI","entity":"fleeAgent"},{"op":"cancelAI","entity":"routeAgent"},
        {"op":"restoreTransform","entity":"followAgent","slot":"follow"},{"op":"restoreTransform","entity":"followTarget","slot":"target"},
    ```

#### AS-SC-RECOVERY {#as-sc-recovery}

`assets/development/ai_goap_target_recovery.scenario.json:27` · Сценарий Target Recovery · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../assets/development/ai_goap_target_recovery.scenario.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/development/ai_goap_target_recovery.scenario.json#L27">GitHub</a>

Якорь: `` "temporarilyRemoveTargetAtMidpoint" ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="27"
        {"op":"temporarilyRemoveTargetAtMidpoint","entity":"goal","agent":"agent","distanceFraction":0.5,"unavailableSeconds":5.0}
      ],
      "stop": [
        {"op":"cancelAIDecision","entity":"agent"},
        {"op":"cancelAI","entity":"agent"}
      ],
      "reset": [
        {"op":"cancelAIDecision","entity":"agent"},
    ```

#### AS-SC-REPLAN {#as-sc-replan}

`assets/development/ai_goap_access_key_replanning.scenario.json:191` · Сценарий перепланирования Access Key · состояние: `HEAD` · вхождение 1 из 2 · <a href="../../../../assets/development/ai_goap_access_key_replanning.scenario.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/development/ai_goap_access_key_replanning.scenario.json#L191">GitHub</a>

Якорь: `` "whenPickupCollected" ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="191"
          "whenPickupCollected": "coinC"
        },
        {
          "op": "setRuntimeVisibility",
          "entity": "coinA",
          "visible": false,
          "whenPickupCollected": "coinC"
        }
    ```

#### AS-SC-RESERVATION {#as-sc-reservation}

`assets/development/ai_goap_access_key_reservation.scenario.json:51` · Сценарий двух агентов · состояние: `HEAD` · вхождение 1 из 2 · <a href="../../../../assets/development/ai_goap_access_key_reservation.scenario.json">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/assets/development/ai_goap_access_key_reservation.scenario.json#L51">GitHub</a>

Якорь: `` "access_key_reserved" ``

??? note "Фрагмент (8 строк от якоря)"

    ```json linenums="51"
        { "op": "startAIDecision", "entity": "agentB", "decision": "access_key_reserved", "result": "GOAP decision B" },
        { "op": "startAIDecision", "entity": "agentA", "decision": "access_key_reserved", "result": "GOAP decision A" }
      ],
      "update": [],
      "stop": [
        { "op": "cancelAIDecision", "entity": "agentA" },
        { "op": "cancelAI", "entity": "agentA" },
        { "op": "cancelAIDecision", "entity": "agentB" },
    ```

## Прочее

#### UI-DEBUG-FLAGS {#ui-debug-flags}

`src/Render/ImGui/ImGuiDebugUI_RendererCore.inl:392` · Флаги отладочной отрисовки · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Render/ImGui/ImGuiDebugUI_RendererCore.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Render/ImGui/ImGuiDebugUI_RendererCore.inl#L392">GitHub</a>

Якорь: `` ImGui::Checkbox("AI planned paths", &rs.drawAIPlannedPathDebug); ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="392"
            ImGui::Checkbox("AI planned paths", &rs.drawAIPlannedPathDebug);
            ImGui::Checkbox("Performance panel", &rs.showPerformancePanel);
            ImGui::Checkbox("Log CPU frame timings", &rs.logCpuFrameTimings);
            ImGui::Checkbox("VSync", &rs.enableVSync);
            ImGui::Checkbox("Render Debug Window Swapchain", &rs.enableDebugWindowRender);
            ImGui::TextDisabled("F7: toggle separate debug window swapchain rendering.");

            if (rs.drawGameplayMovementDebug)
    ```

#### UI-GOAP-PANEL {#ui-goap-panel}

`src/Render/ImGui/ImGuiDebugUI_GOAP.inl:69` · Окно отладки GOAP · состояние: `HEAD` · вхождение 1 из 1 · <a href="../../../../src/Render/ImGui/ImGuiDebugUI_GOAP.inl">файл</a> · <a href="https://github.com/Fedorr47/CoreEngineModule/blob/8c6c0dc37ab369e201d1b3252448f96d1e81cd37/src/Render/ImGui/ImGuiDebugUI_GOAP.inl#L69">GitHub</a>

Якорь: `` ImGui::Begin("AI / GOAP"); ``

??? note "Фрагмент (8 строк от якоря)"

    ```cpp linenums="69"
            ImGui::Begin("AI / GOAP");
            if (gameplayRuntime == nullptr)
            {
                ImGui::TextDisabled("No GOAP agent selected");
                ImGui::End();
                return;
            }

    ```
