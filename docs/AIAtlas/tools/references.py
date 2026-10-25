"""Реестр источников AI-атласа CoreEngine.

Каждая запись: (ID, путь от корня репозитория, ожидаемый текст строки, n-е вхождение (с 1), описание).
`resolve()` находит n-ю строку, содержащую ожидаемый текст, и ПАДАЕТ, если текста нет: номера строк никогда не
вводятся вручную. Цитаты в главах пишутся как [[ID]], выдержки кода — @@code ID [число строк]@@.
"""
import hashlib
import subprocess
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]

AI = 'src/Gameplay/AI'
GOAP = f'{AI}/GOAP'
RT = f'{AI}/Runtime'
NAV = f'{AI}/Navigation'
CMP = f'{AI}/Composition'
COMP = f'{CMP}/Components'
GNAV = 'src/Gameplay/Navigation'
TRV = 'src/Gameplay/Traversal'
GRT = 'src/Gameplay/Runtime'
APP = 'src/App'

# (ID, path, expected text, nth, description)
REFERENCES = [
    # ---------------------------------------------------------------- состояние
    ('WS-FACT-ID', f'{GOAP}/AIAgentWorldState.cppm', 'struct AIWorldFactId', 1, 'Сильный тип ID булева факта'),
    ('WS-INT-ID', f'{GOAP}/AIAgentWorldState.cppm', 'struct AIWorldIntegerFactId', 1, 'Сильный тип ID целочисленного факта'),
    ('WS-CLASS', f'{GOAP}/AIAgentWorldState.cppm', 'class AIAgentWorldState', 1, 'Контейнер фактов агента'),
    ('WS-CAPACITY', f'{GOAP}/AIAgentWorldState.cppm', 'static constexpr std::size_t FactCapacity = 128u;', 1, 'Ёмкость: 128 bool'),
    ('WS-STORAGE', f'{GOAP}/AIAgentWorldState.cppm', 'std::bitset<FactCapacity> facts_{};', 1, 'Хранение: bitset + массив int32'),
    ('WS-EQUAL', f'{GOAP}/AIAgentWorldState.cppm', 'friend bool operator==(', 1, 'Равенство по полному состоянию'),
    ('WS-HASH', f'{GOAP}/AIAgentWorldState.cppm', 'struct AIAgentWorldStateHash', 1, 'FNV-1a хеш полного состояния'),
    ('WS-SETFACT', f'{GOAP}/AIAgentWorldState.cppm', 'void SetFact(', 1, 'Запись bool-факта с проверкой индекса'),
    # ---------------------------------------------------------------- контракты
    ('DC-AIID', f'{RT}/AIActionContracts.cppm', 'struct AIId', 1, 'Шаблон сильного ID действия/контекста'),
    ('DC-ACTION-STATUS', f'{RT}/AIActionContracts.cppm', 'enum class AIActionExecutionStatus', 1, 'Статус исполнения действия'),
    ('DC-FACT-COND', f'{GOAP}/AIDecisionContracts.cppm', 'struct AIFactCondition', 1, 'Булево условие'),
    ('DC-NUM-OP', f'{GOAP}/AIDecisionContracts.cppm', 'enum class AINumericConditionOperator', 1, 'Операторы числовых условий'),
    ('DC-NUM-EFFECT-OP', f'{GOAP}/AIDecisionContracts.cppm', 'enum class AINumericEffectOperation', 1, 'Set / Add'),
    ('DC-GOAL', f'{GOAP}/AIDecisionContracts.cppm', 'struct AIGoalDefinition', 1, 'Цель: только булевы desiredFacts'),
    ('DC-ACTION', f'{GOAP}/AIDecisionContracts.cppm', 'struct AIActionDefinition', 1, 'Определение действия: условия, эффекты, стоимость, continuation'),
    ('DC-PLAN', f'{GOAP}/AIDecisionContracts.cppm', 'struct AIPlanStep', 1, 'Шаг плана и план'),
    # ---------------------------------------------------------------- операции
    ('SO-EVAL-FACT', f'{GOAP}/AIStateOperations.cppm', 'bool EvaluateFactCondition(', 1, 'Проверка bool-условия'),
    ('SO-APPLY-FACTS', f'{GOAP}/AIStateOperations.cppm', 'void ApplyFactEffects(', 1, 'Применение bool-эффектов'),
    ('SO-EVAL-NUM', f'{GOAP}/AIStateOperations.cppm', 'constexpr bool EvaluateNumericCondition(', 1, 'Сравнение через <=>'),
    ('SO-APPLY-NUM', f'{GOAP}/AIStateOperations.cppm', 'bool ApplyNumericEffects(', 1, 'Числовые эффекты на копии с контролем переполнения'),
    ('SO-OVERFLOW', f'{GOAP}/AIStateOperations.cppm', 'if ((effect.value > 0 && current > std::numeric_limits<std::int32_t>::max() - effect.value)', 1, 'Проверка переполнения Add'),
    # ---------------------------------------------------------------- планировщик
    ('PL-INPUT', f'{GOAP}/AIPlanner.cppm', 'bool IsInputValid(', 1, 'Валидация входа планировщика'),
    ('PL-CONT-SUBSET', f'{GOAP}/AIPlanner.cppm', '|| !std::ranges::all_of(action.continuationConditions,', 1, 'continuation ⊆ preconditions'),
    ('PL-UNIQUE-KEYS', f'{GOAP}/AIPlanner.cppm', 'return std::ranges::adjacent_find(actionKeys) == actionKeys.end();', 1, 'Уникальность (actionId, contextId)'),
    ('PL-NODE', f'{GOAP}/AIPlanner.cppm', 'struct SearchNode', 1, 'Узел поиска: состояние, стоимость, sequence, копия шагов'),
    ('PL-ORDER', f'{GOAP}/AIPlanner.cppm', 'struct SearchNodeGreater', 1, 'Порядок очереди: cost, затем sequence'),
    ('PL-FIND', f'{GOAP}/AIPlanner.cppm', 'std::optional<AIPlan> FindAIPlan(', 1, 'Uniform-cost поиск плана'),
    ('PL-EMPTY-PLAN', f'{GOAP}/AIPlanner.cppm', 'return AIPlan{ goal.goalId, {} };', 1, 'Пустой план для уже выполненной цели'),
    ('PL-SORT', f'{GOAP}/AIPlanner.cppm', 'std::ranges::sort(orderedActions, {}, [](const AIActionDefinition* action)', 1, 'Сортировка действий по (actionId, contextId)'),
    ('PL-STALE', f'{GOAP}/AIPlanner.cppm', 'if (currentBest == bestCosts.end() || current.cost != currentBest->second)', 1, 'Пропуск устаревших записей'),
    ('PL-GOALTEST', f'{GOAP}/AIPlanner.cppm', 'if (AreFactConditionsSatisfied(current.state, goal.desiredFacts))', 1, 'Проверка цели при извлечении'),
    ('PL-RELAX', f'{GOAP}/AIPlanner.cppm', 'if (known != bestCosts.end() && known->second <= nextCost)', 1, 'Отсечение не лучшего пути к состоянию'),
    # ---------------------------------------------------------------- выбор цели
    ('GS-CANDIDATE', f'{GOAP}/AIGoalSelectionContracts.cppm', 'struct AIGoalSelectionCandidate', 1, 'Кандидат цели: baseScore + scoreRules'),
    ('GS-SCORE', f'{GOAP}/AIGoalSelection.cppm', 'float ScoreAIGoal(', 1, 'Оценка цели'),
    ('GS-SELECT', f'{GOAP}/AIGoalSelection.cppm', 'AIGoalSelectionResult SelectAIGoal(', 1, 'Выбор цели'),
    ('GS-TIE', f'{GOAP}/AIGoalSelection.cppm', 'if (!selectedGoal.IsValid() || score > selectedGoal.score)', 1, 'Строгое сравнение: при равенстве первая цель'),
    # ---------------------------------------------------------------- исполнение плана
    ('PE-STATUS', f'{GOAP}/AIPlanExecution.cppm', 'enum class AIPlanExecutionStatus', 1, 'Состояния исполнения плана'),
    ('PE-CLASS', f'{GOAP}/AIPlanExecution.cppm', 'explicit AIPlanExecution(AIPlan plan) noexcept', 1, 'Машина состояний плана'),
    ('PE-APPLY', f'{GOAP}/AIPlanExecution.cppm', 'AIPlanExecutionStatus ApplyCurrentStepStatus(', 1, 'Переходы по статусу действия'),
    ('BR-START', f'{GOAP}/AIPlanExecutionBridge.cppm', 'static AIPlanExecutionStatus StartReadyPlanStep(', 1, 'Старт шага: binding → runtime → AISystem'),
    ('BR-SYNC', f'{GOAP}/AIPlanExecutionBridge.cppm', 'static AIPlanExecutionStatus SynchronizeRunningPlanStep(', 1, 'Синхронизация шага с AISystem'),
    ('BR-CANCEL', f'{GOAP}/AIPlanExecutionBridge.cppm', 'static void CancelPlanExecution(', 1, 'Отмена исполнения'),
    # ---------------------------------------------------------------- decision runtime
    ('DR-CLASS', f'{GOAP}/AIDecisionRuntime.cppm', 'class AIDecisionRuntime', 1, 'Monitored decision loop одного агента'),
    ('DR-PLAN', f'{GOAP}/AIDecisionRuntime.cppm', 'bool Plan(', 1, 'Явное планирование (снимает explicit cancel)'),
    ('DR-UPDATE', f'{GOAP}/AIDecisionRuntime.cppm', 'AIPlanExecutionStatus Update(', 1, 'Основной update решения'),
    ('DR-NO-GOAL', f'{GOAP}/AIDecisionRuntime.cppm', 'if (selectedGoal == nullptr)', 1, 'Нет цели → отмена и Succeeded'),
    ('DR-INVALIDATED', f'{GOAP}/AIDecisionRuntime.cppm', 'const bool bPlanInvalidated = !bGoalChanged', 1, 'Invalidation только при прежней цели'),
    ('DR-KEEP-RUNNING', f'{GOAP}/AIDecisionRuntime.cppm', '// A goal change must not destroy an otherwise valid running plan.', 1, 'Сохранение старого исполнения при смене цели'),
    ('DR-UPDATE-STEP', f'{GOAP}/AIDecisionRuntime.cppm', 'AIPlanExecutionStatus Update(', 2, 'Старт/синхронизация текущего шага'),
    ('DR-CANCEL', f'{GOAP}/AIDecisionRuntime.cppm', 'void Cancel(AISystem& aiSystem) noexcept', 1, 'Явная отмена'),
    ('DR-CONTINUATION', f'{GOAP}/AIDecisionRuntime.cppm', 'bool IsRunningStepContinuationValid_(', 1, 'Проверка continuation текущего шага'),
    ('DR-REMAINING', f'{GOAP}/AIDecisionRuntime.cppm', 'bool IsRemainingPlanValid_(', 1, 'Прогон остатка плана'),
    ('DR-INSTALL', f'{GOAP}/AIDecisionRuntime.cppm', 'void InstallPlan_(AIPlan plan, AISystem& aiSystem)', 1, 'Установка нового плана'),
    # ---------------------------------------------------------------- action runtime
    ('AR-CONTEXT', f'{RT}/AIActionRuntime.cppm', 'struct AIActionRuntimeContext', 1, 'Контекст вызова действия'),
    ('AR-RESULT', f'{RT}/AIActionRuntime.cppm', 'enum class AIActionRuntimeResult', 1, 'Running/Succeeded/Failed'),
    ('AR-IFACE', f'{RT}/AIActionRuntime.cppm', 'class IAIActionRuntime', 1, 'Интерфейс Start/Tick/Cancel'),
    ('AB-IFACE', f'{RT}/AIActionBinding.cppm', 'class IAIActionBinding', 1, 'Фабрика runtime для шага'),
    ('AB-REGISTRY', f'{RT}/AIActionBinding.cppm', 'class AIActionBindingRegistry', 1, 'Невладеющий реестр bindings по actionId'),
    ('AT-CLASS', f'{RT}/AIActionTask.cppm', 'class AIActionTask', 1, 'Владеющая обёртка runtime со статусом'),
    ('AT-CANCEL', f'{RT}/AIActionTask.cppm', 'void Cancel() noexcept', 1, 'Cancel только для Running'),
    ('AS-CLASS', f'{RT}/AISystem.cppm', 'class AISystem', 1, 'Одна задача на агента'),
    ('AS-UPDATE', f'{RT}/AISystem.cppm', 'std::size_t Update(GameplayWorld& world, const float deltaSeconds = 0.0f)', 1, 'Тик активных задач'),
    ('AS-START', f'{RT}/AISystem.cppm', 'AIActionExecutionStatus StartAction(', 1, 'Синхронный старт задачи (заменяет прежнюю)'),
    ('AS-CLEANUP', f'{RT}/AISystem.cppm', 'void CleanupInactiveTasks_(GameplayWorld& world)', 1, 'Удаление задач мёртвых агентов'),
    # ---------------------------------------------------------------- обёртка решения
    ('GD-INSTANCE-IFACE', f'{RT}/GameplayAIDecisionContracts.cppm', 'class GameplayAIDecisionInstance', 1, 'Интерфейс решения'),
    ('GD-OBS-CONTEXT', f'{RT}/GameplayAIDecisionContracts.cppm', 'struct GameplayAIObservationContext', 1, 'Мир + события + выходной буфер'),
    ('GD-CREATION', f'{RT}/GameplayAIDecisionCreationContext.cppm', 'struct GameplayAIDecisionCreationContext', 1, 'Сервисы создания решения'),
    ('GD-FACTORY', f'{RT}/GameplayAIDecision.cppm', 'class GameplayAIDecisionFactoryRegistry', 1, 'Реестр фабрик решений'),
    ('GG-DECISION', f'{GOAP}/GameplayGOAPDecision.cppm', 'class GameplayGOAPDecision', 1, 'GOAP-обёртка: факты, определение, bindings'),
    ('GG-SETUP', f'{GOAP}/GameplayGOAPDecisionSetup.cppm', 'class IGameplayGOAPContext', 1, 'Доменный контекст наблюдения'),
    ('GG-REACTION', f'{GOAP}/GameplayGOAPDecisionSetup.cppm', 'class IGameplayGOAPEventReaction', 1, 'Реакция на подтверждённые события'),
    ('GG-INSTANCE', f'{GOAP}/GameplayGOAPDecisionInstance.cppm', 'class GameplayGOAPDecisionInstance :', 1, 'Instance решения'),
    ('GG-INSTANCE-UPDATE', f'{GOAP}/GameplayGOAPDecisionInstance.cppm', 'void Update(AISystem& aiSystem, const GameplayAIObservationContext& observation) override', 1, 'Порядок проходов наблюдения и событий'),
    ('GG-FIELD-ORDER', f'{GOAP}/GameplayGOAPDecisionInstance.cppm', '// Bindings borrow the context, facts and event buffer. Context and events', 1, 'Порядок полей и lifetime'),
    ('GG-CREATE', f'{GOAP}/GameplayGOAPDecisionInstance.cppm', 'std::unique_ptr<GameplayAIDecisionInstance> CreateGameplayGOAPDecision(', 1, 'Создание instance (+path inspection)'),
    ('GG-DEBUGVM', f'{GOAP}/Debug/AIDebugViewModel.cppm', 'struct AIDebugViewModel', 1, 'Снимок для отладки'),
    ('GG-PATH-INSPECTION', f'{GOAP}/Debug/GameplayGOAPPathInspection.cppm', 'class IGameplayGOAPPlannedPathProvider', 1, 'Планируемый путь для debug'),
    # ---------------------------------------------------------------- авторинг
    ('AU-GOAP-PARSE', f'{GOAP}/Authoring/GameplayGOAPDefinitionAsset.cppm', 'ParseGameplayGOAPDefinitionAsset(const std::string_view json,', 1, 'Разбор .goap.json'),
    ('AU-COMPILE', f'{GOAP}/Authoring/GameplayGOAPDefinitionCompiler.cppm', 'GameplayGOAPCompiledDefinition CompileGameplayGOAPDefinition(', 3, 'Компиляция определения'),
    ('AU-CONTEXT-UNIQUE', f'{GOAP}/Authoring/GameplayGOAPDefinitionCompiler.cppm', '"duplicate action context");', 1, 'Контекст уникален в определении'),
    ('AU-GOAL-BUILD', f'{GOAP}/Authoring/GameplayGOAPDefinitionCompiler.cppm', 'AIGoalSelectionCandidate goal{', 1, 'Цель из ассета: только baseScore'),
    ('AU-ROOT', f'{GOAP}/Authoring/GameplayAIAssetParsing.cppm', 'JsonValue Root(const std::string_view json, const std::string_view source)', 1, 'Проверка version == 1'),
    ('AU-FIELDS', f'{GOAP}/Authoring/GameplayAIAssetParsing.cppm', 'void Fields(const JsonObject& object', 1, 'Запрет неизвестных полей'),
    ('AU-PARSERS', f'{GOAP}/Authoring/GameplayAIAssetParsing.cppm', 'class GameplayAIComponentParsers', 1, 'Типизированные парсеры компонентов (std::any)'),
    ('AU-BEHAVIOR', f'{GOAP}/Authoring/GameplayAIDecisionAsset.cppm', 'struct GameplayAIBehaviorAsset', 1, 'Behavior-ассет'),
    ('AU-BEHAVIOR-PARSE', f'{GOAP}/Authoring/GameplayAIDecisionAsset.cppm', 'GameplayAIBehaviorAsset ParseGameplayAIBehaviorAsset(std::string_view json, std::string_view source,', 1, 'Разбор behavior'),
    ('AU-ROUTES-PARSE', f'{GOAP}/Authoring/GameplayAIDecisionAsset.cppm', 'GameplayAIRouteGraphAsset ParseGameplayAIRouteGraphAsset(std::string_view json, std::string_view source)', 2, 'Разбор .routes.json'),
    # ---------------------------------------------------------------- композиция
    ('CO-ROLE', f'{CMP}/GameplayGOAPCompositionRegistry.cppm', 'struct GameplayAIResolvedRole', 1, 'Роль: имя, entity, позиция (снимок)'),
    ('CO-OBS-IFACE', f'{CMP}/GameplayGOAPCompositionRegistry.cppm', 'class IGameplayGOAPObservation', 1, 'Интерфейс наблюдения с метаданными входов/выходов'),
    ('CO-CAP-IFACE', f'{CMP}/GameplayGOAPCompositionRegistry.cppm', 'class IGameplayGOAPCapability', 1, 'Интерфейс capability-провайдера'),
    ('CO-CONTEXT', f'{CMP}/GameplayGOAPCompositionRegistry.cppm', 'struct GameplayGOAPCompositionContext', 1, 'Контекст компиляции компонентов'),
    ('CO-REGISTRY', f'{CMP}/GameplayGOAPCompositionRegistry.cppm', 'class GameplayGOAPCompositionRegistry', 1, 'Реестр компонентов'),
    ('CO-REG-CAP', f'{CMP}/GameplayGOAPCompositionRegistry.cppm', 'bool RegisterCapability(std::string_view type, AIActionId actionId,', 1, 'Регистрация capability'),
    ('CO-SEMANTIC', f'{CMP}/GameplayGOAPCompositionRegistry.cppm', 'std::vector<GameplayGOAPSemanticAction> SemanticActions() const', 1, 'Тип capability = семантическое действие'),
    ('CO-ORDER', f'{CMP}/GameplayGOAPObservationOrder.cppm', 'OrderGameplayGOAPObservations(', 1, 'Writers и топологический порядок'),
    ('CO-ORDER-PICK', f'{CMP}/GameplayGOAPObservationOrder.cppm', 'const auto next = std::ranges::find_if(observers, [&](const auto& observer)', 1, 'Выбор первого готового наблюдателя'),
    ('CO-TEMPLATE', f'{CMP}/GameplayGOAPDecisionTemplate.cppm', 'class GameplayGOAPDecisionTemplate', 1, 'Неизменяемый снимок конфигурации'),
    ('CO-INSTANCE-DEF', f'{CMP}/GameplayGOAPDecisionTemplate.cppm', 'GameplayGOAPDecisionDefinition InstanceDefinition(', 1, 'Копия определения с cost overrides'),
    ('CO-RESOLVE-ROLES', f'{CMP}/GameplayGOAPAssetComposition.cppm', 'std::vector<GameplayAIResolvedRole> ResolveRoles(', 1, 'Разрешение ролей по уровню'),
    ('CO-FROM-TEMPLATE', f'{CMP}/GameplayGOAPAssetComposition.cppm', 'std::unique_ptr<GameplayAIDecisionInstance> CreateGameplayGOAPDecisionFromTemplate(', 1, 'Сборка instance'),
    ('CO-REGISTER-ASSETS', f'{CMP}/GameplayGOAPAssetComposition.cppm', 'void RegisterGameplayAIDecisionAssets(', 1, 'Транзакционная регистрация каталога'),
    ('CO-PENDING', f'{CMP}/GameplayGOAPAssetComposition.cppm', 'auto pending = destination;', 1, 'Регистрация во временной копии'),
    ('CO-DEFAULT-FACTORIES', f'{CMP}/GameplayAIBuiltinDecisions.cppm', 'GameplayAIDecisionFactoryRegistry MakeDefaultGameplayAIDecisionFactories()', 1, 'Каталог по умолчанию'),
    ('CO-BUILTIN', f'{CMP}/GameplayGOAPBuiltinComponents.cppm', 'GameplayGOAPCompositionRegistry MakeDefaultGameplayGOAPComponents()', 1, 'Встроенные компоненты'),
    # ---------------------------------------------------------------- компоненты
    ('CP-SPATIAL', f'{COMP}/GameplayGOAPSpatialComponents.cppm', 'class SpatialObservation final : public IGameplayGOAPObservation', 1, 'target_available / within_distance'),
    ('CP-SPATIAL-OBSERVE', f'{COMP}/GameplayGOAPSpatialComponents.cppm', 'bool value = (!parameters_.requireEntity || world.IsEntityValid(target_.entity))', 1, 'Вычисление spatial-факта, latch'),
    ('CP-NEAREST', f'{COMP}/GameplayGOAPSpatialComponents.cppm', 'class NearestLocationObservation final', 1, 'nearest_location (one-hot, память)'),
    ('CP-NEAREST-TIE', f'{COMP}/GameplayGOAPSpatialComponents.cppm', 'if (candidate <= distance)', 1, 'Равенство → более поздний элемент'),
    ('CP-PICKUP-AVAIL', f'{COMP}/GameplayGOAPSpatialComponents.cppm', 'class PickupAvailabilityObservation final', 1, 'pickup_available с учётом броней'),
    ('CP-SPATIAL-REGISTER', f'{COMP}/GameplayGOAPSpatialComponents.cppm', 'void RegisterGameplayGOAPSpatialComponents(', 1, 'Регистрация spatial-компонентов'),
    ('CP-LEDGER', f'{COMP}/GameplayGOAPResourceComponents.cppm', 'class ResourceLedgerObservation final', 1, 'resource_ledger'),
    ('CP-LEDGER-OBSERVE', f'{COMP}/GameplayGOAPResourceComponents.cppm', 'if (event.type == GameplayWorldEventType::PickupCollected)', 1, 'Подтверждение событий ledger'),
    ('CP-HIDE', f'{COMP}/GameplayGOAPResourceComponents.cppm', 'class HideOnPurchaseReaction final', 1, 'hide_on_purchase'),
    ('CP-PURCHASE-RT', f'{COMP}/GameplayGOAPResourceComponents.cppm', 'class PurchaseRuntime final : public IAIActionRuntime', 1, 'Runtime покупки'),
    ('CP-PURCHASE-CAP', f'{COMP}/GameplayGOAPResourceComponents.cppm', 'class PurchaseCapability final : public IGameplayGOAPCapability', 1, 'Проверка модели покупки'),
    ('CP-PURCHASE-ID', f'{COMP}/GameplayGOAPResourceComponents.cppm', 'inline constexpr AIActionId kAIPurchaseActionId{3u};', 1, 'ID purchase = 3'),
    ('CP-MOVE-GRAPH', f'{COMP}/GameplayGOAPMoveToComponent.cppm', 'AuthoredGraph CompileGraph(const GameplayGOAPCompositionContext& context)', 1, 'Авторский граф маршрутов'),
    ('CP-MOVE-CAP', f'{COMP}/GameplayGOAPMoveToComponent.cppm', 'class MoveToCapability final : public IGameplayGOAPCapability,', 1, 'MoveTo provider'),
    ('CP-MOVE-FALLBACK', f'{COMP}/GameplayGOAPMoveToComponent.cppm', 'graph.edges = {{route.start, route.goal, 1.0f, {}}};', 1, 'Прямой fallback-граф'),
    ('CP-MOVE-BINDING', f'{COMP}/GameplayGOAPMoveToComponent.cppm', 'return std::make_unique<AIMoveToActionBinding>(world_, links_, executors_, *this, reservations_, *this);', 1, 'Binding без obstacle query/debug'),
    ('CP-MOVE-RESOLVE', f'{COMP}/GameplayGOAPMoveToComponent.cppm', 'std::optional<AIMoveToActionRequest> ResolveRequest(const AIActionRuntimeContext& context) override', 1, 'sourceRadius и запрос MoveTo'),
    ('CP-MOVE-ASSET', f'{COMP}/GameplayGOAPMoveToComponentAssets.cppm', 'struct GameplayAIMoveToAsset', 1, 'Параметры move_to и значения по умолчанию'),
    # ---------------------------------------------------------------- навигационные действия
    ('NA-MOVETO-ID', f'{NAV}/AIMoveToAction.cppm', 'inline constexpr AIActionId kAIMoveToActionId{2u};', 1, 'ID move_to = 2'),
    ('NA-MOVETO-CREATE', f'{NAV}/AIMoveToAction.cppm', 'static std::unique_ptr<IAIActionRuntime> CreateRuntime(', 1, 'MoveTo → поиск → FollowRoute runtime'),
    ('NA-MOVETO-BINDING', f'{NAV}/AIMoveToActionBinding.cppm', 'class AIMoveToActionBinding final : public IAIActionBinding', 1, 'Binding MoveTo'),
    ('NA-RESERVED', f'{NAV}/AIMoveToActionBinding.cppm', 'class ReservedAIMoveToActionRuntime final : public IAIActionRuntime', 1, 'MoveTo с бронью цели'),
    ('NA-RESERVED-START', f'{NAV}/AIMoveToActionBinding.cppm', 'if (started_ || reservations_.IsReserved(target_) ||', 1, 'Старт отклоняет любую существующую бронь'),
    ('NA-ROUTE-ID', f'{NAV}/AIFollowRouteActionRuntime.cppm', 'inline constexpr AIActionId kAIFollowRouteActionId{1u};', 1, 'ID FollowRoute = 1'),
    ('NA-ROUTE-RT', f'{NAV}/AIFollowRouteActionRuntime.cppm', 'class AIFollowRouteActionRuntime final : public IAIActionRuntime', 1, 'Runtime движения по маршруту'),
    ('NA-ROUTE-TICK', f'{NAV}/AIFollowRouteActionRuntime.cppm', 'AIActionRuntimeResult Tick(', 1, 'Tick маршрута и traversal'),
    ('NA-ROUTE-ADVANCE', f'{NAV}/AIFollowRouteActionRuntime.cppm', 'AIActionRuntimeResult AdvanceFollower_(const EntityHandle entity)', 1, 'Продвижение follower и physicallyBlocked'),
    ('NA-ROUTE-APPLY', f'{NAV}/AIFollowRouteActionRuntime.cppm', 'void ApplyFollowerMovement_(', 1, 'Avoidance → команда → facing'),
    ('NA-ROUTE-TRAVERSAL', f'{NAV}/AIFollowRouteActionRuntime.cppm', 'AIActionRuntimeResult StartTraversal_(', 1, 'Передача сегмента executor'),
    ('NA-ROUTE-STOP', f'{NAV}/AIFollowRouteActionRuntime.cppm', 'void StopMovementIfAccessible_(const EntityHandle entity) noexcept', 1, 'Остановка с сохранением вертикали'),
    ('NA-ROUTE-CREATE', f'{NAV}/AIFollowRouteAction.cppm', 'class AIFollowRouteAction', 1, 'Фабрика/старт FollowRoute'),
    ('NA-FOLLOW-SETTINGS', f'{NAV}/AIFollowTargetActionRuntime.cppm', 'struct AIFollowTargetSettings', 1, 'Seek + интервал 0.05 с'),
    ('NA-FOLLOW-RT', f'{NAV}/AIFollowTargetActionRuntime.cppm', 'class AIFollowTargetActionRuntime final : public IAIActionRuntime', 1, 'FollowTarget runtime'),
    ('NA-FOLLOW-TICK', f'{NAV}/AIFollowTargetActionRuntime.cppm', 'AIActionRuntimeResult Tick(', 1, 'Sampling + переиздание команды'),
    ('NA-FLEE-SETTINGS', f'{NAV}/AIFleeTargetActionRuntime.cppm', 'struct AIFleeTargetSettings', 1, 'trigger 2, safe 3, интервал 0.05'),
    ('NA-FLEE-CTOR', f'{NAV}/AIFleeTargetActionRuntime.cppm', 'settings_.safeRadius = std::max(settings_.safeRadius, settings_.triggerRadius);', 1, 'safe ≥ trigger'),
    ('NA-FLEE-REFRESH', f'{NAV}/AIFleeTargetActionRuntime.cppm', 'void RefreshSteering_(const EntityHandle agentEntity) noexcept', 1, 'Пересчёт Flee: дистанция, гистерезис, avoidance'),
    ('NA-FLEE-HYST', f'{NAV}/AIFleeTargetActionRuntime.cppm', 'if (isFleeing_)', 1, 'Гистерезис бегства'),
    ('NA-AGENT-AVOID', f'{NAV}/GameplayAgentObstacleAvoidance.cppm', 'GameplayMovementIntent ApplyGameplayAgentObstacleAvoidance(', 1, 'Агентный адаптер avoidance'),
    # ---------------------------------------------------------------- маршруты и steering
    ('RT-ROUTE', f'{GNAV}/GameplayRoute.cppm', 'struct GameplayRoute', 3, 'Точки + аннотации сегментов'),
    ('RT-ROUTE-VALID', f'{GNAV}/GameplayRoute.cppm', 'const std::size_t expectedSegmentCount = points.size() - 1u;', 1, 'N точек ↔ N−1 аннотаций'),
    ('RT-ANNOTATION', f'{GNAV}/GameplayRoute.cppm', 'struct GameplayRouteSegmentAnnotation', 1, 'Аннотация traversal'),
    ('RS-GRAPH', f'{GNAV}/GameplayRouteSearch.cppm', 'struct GameplayRouteGraph', 3, 'Взвешенный ориентированный граф'),
    ('RS-FIND', f'{GNAV}/GameplayRouteSearch.cppm', 'GameplayRouteSearchResult FindWeightedGameplayRoute(', 2, 'Дейкстра по узлам'),
    ('RS-SAME', f'{GNAV}/GameplayRouteSearch.cppm', 'if (resolvedStartNodeIndex == resolvedGoalNodeIndex)', 1, 'start == goal → одна точка'),
    ('RF-STATUS', f'{GNAV}/GameplayRouteFollower.cppm', 'enum class GameplayRouteFollowerStatus', 1, 'Статусы follower'),
    ('RF-SETTINGS', f'{GNAV}/GameplayRouteFollower.cppm', 'float cornerLookAheadDistance{0.75f};', 1, 'Look-ahead по умолчанию 0.75'),
    ('RF-START', f'{GNAV}/GameplayRouteFollower.cppm', 'GameplayRouteFollowerStatus Start(', 1, 'Старт follower; anchor = points[0]'),
    ('RF-TICK', f'{GNAV}/GameplayRouteFollower.cppm', 'GameplayRouteFollowerOutput Tick(', 1, 'Тик follower'),
    ('RF-SLOWING', f'{GNAV}/GameplayRouteFollower.cppm', 'segmentSteeringSettings.slowingRadius = bIsFinalSegment', 1, 'slowing = acceptance на промежуточных'),
    ('RF-LOOKAHEAD', f'{GNAV}/GameplayRouteFollower.cppm', 'bool TryBuildLookAheadTarget_(', 1, 'Виртуальная цель look-ahead'),
    ('RF-PASSED', f'{GNAV}/GameplayRouteFollower.cppm', 'bool HasPassedCurrentCorner_(', 1, 'Плоскость перехода угла'),
    ('RF-COMPLETE', f'{GNAV}/GameplayRouteFollower.cppm', 'bool CompleteTraversal(', 1, 'Подтверждение traversal'),
    ('ST-INTENT', f'{GNAV}/GameplaySteering.cppm', 'struct GameplayMovementIntent', 1, 'Намерение движения'),
    ('ST-ARRIVAL-SETTINGS', f'{GNAV}/GameplaySteering.cppm', 'struct GameplayArrivalSteeringSettings', 1, 'r=0.25, R=1.0, m_min=0.1'),
    ('ST-SEEK', f'{GNAV}/GameplaySteering.cppm', 'GameplaySteeringOutput BuildGameplaySeekSteering(', 1, 'Seek'),
    ('ST-FLEE', f'{GNAV}/GameplaySteering.cppm', 'GameplayMovementIntent BuildGameplayFleeSteering(', 1, 'Flee'),
    ('ST-ARRIVAL', f'{GNAV}/GameplaySteering.cppm', 'GameplaySteeringOutput BuildGameplayArrivalSteering(', 1, 'Arrival'),
    ('ST-APPLY', f'{GNAV}/GameplaySteering.cppm', 'void ApplyGameplayMovementIntent(', 1, 'Намерение → CharacterCommand'),
    ('MU-EPS', 'src/Core/Math/MathUtils.cppm', 'constexpr float kLengthEpsilon = 1e-6f;', 1, 'Эпсилоны длины/движения'),
    # ---------------------------------------------------------------- avoidance
    ('OA-QUERY', f'{GNAV}/GameplayObstacleAvoidance.cppm', 'class IGameplayObstacleQuery', 1, 'Интерфейс геометрических запросов'),
    ('OA-SUPPORT-DEFAULT', f'{GNAV}/GameplayObstacleAvoidance.cppm', 'virtual bool ProbeSupport(', 1, 'Синтетическая опора по умолчанию'),
    ('OA-SETTINGS', f'{GNAV}/GameplayObstacleAvoidance.cppm', 'struct GameplayObstacleAvoidanceSettings', 1, 'Параметры avoidance'),
    ('OA-WEIGHTS', f'{GNAV}/GameplayObstacleAvoidance.cppm', 'constexpr float SelectedFeelerWeight = 0.60f;', 1, 'Веса escape'),
    ('OA-SANITIZE', f'{GNAV}/GameplayObstacleAvoidance.cppm', 'SanitizedSettings Sanitize(', 1, 'Нормализация параметров, горизонт скорости'),
    ('OA-SUPPORT', f'{GNAV}/GameplayObstacleAvoidance.cppm', 'bool HasSupport(', 1, 'Проверка опоры кандидата'),
    ('OA-ESCAPE', f'{GNAV}/GameplayObstacleAvoidance.cppm', 'mathUtils::Vec3 BuildEscapeCandidate(', 1, 'Escape-кандидат'),
    ('OA-APPLY', f'{GNAV}/GameplayObstacleAvoidance.cppm', 'rendern::GameplayMovementIntent rendern::ApplyGameplayObstacleAvoidance(', 2, 'Основной алгоритм avoidance'),
    ('OA-FEELERS', f'{GNAV}/GameplayObstacleAvoidance.cppm', 'const mathUtils::Vec3 left{', 1, 'Поворот feelers'),
    ('OA-SIDE', f'{GNAV}/GameplayObstacleAvoidance.cppm', '// Equal clearance always selects left, keeping symmetric situations deterministic.', 1, 'Выбор стороны и гистерезис'),
    ('OA-PHYS-QUERY', f'{APP}/GameplayPhysicsObstacleQuery.h', 'class GameplayPhysicsObstacleQuery final : public rendern::IGameplayObstacleQuery', 1, 'Jolt-адаптер, StaticWorld по умолчанию'),
    ('OA-PHYS-PROBE', f'{APP}/GameplayPhysicsCharacterIntegration.cpp', 'bool appRuntime::GameplayPhysicsObstacleQuery::Probe(', 1, 'Sphere cast / raycast'),
    ('OA-PHYS-SUPPORT', f'{APP}/GameplayPhysicsCharacterIntegration.cpp', 'bool appRuntime::GameplayPhysicsObstacleQuery::ProbeSupport(', 1, 'Raycast вниз'),
    # ---------------------------------------------------------------- traversal и физика
    ('TV-EXECUTOR', f'{TRV}/GameplayTraversalExecutor.cppm', 'class IGameplayTraversalExecutor', 1, 'Интерфейс executor'),
    ('TV-LINK', f'{TRV}/GameplayTraversalLink.cppm', 'struct GameplayTraversalLink', 1, 'Link: handle, тип, цель, jump-данные'),
    ('TV-JUMP-DATA', f'{TRV}/GameplayTraversalLink.cppm', 'struct GameplayJumpTraversalData', 1, 'Данные прыжка'),
    ('TV-LINK-REG', f'{TRV}/GameplayTraversalLinkRegistry.cppm', 'class GameplayTraversalLinkRegistry', 1, 'Реестр links'),
    ('TV-EXEC-REG', f'{TRV}/GameplayTraversalExecutorRegistry.cppm', 'class GameplayTraversalExecutorRegistry', 1, 'Реестр executors (невладеющий)'),
    ('TV-DOOR', f'{TRV}/DoorTraversalExecutor.cppm', 'class DoorTraversalExecutor final : public IGameplayTraversalExecutor', 1, 'Door executor'),
    ('TV-DOOR-TICK', f'{TRV}/DoorTraversalExecutor.cppm', 'GameplayTraversalExecutionResult Tick(', 1, 'Подход к InteractionPoint и открытие'),
    ('TV-JUMP', f'{TRV}/JumpTraversalExecutor.cppm', 'class JumpTraversalExecutor final : public IGameplayTraversalExecutor', 1, 'Jump executor'),
    ('TV-JUMP-TICK', f'{TRV}/JumpTraversalExecutor.cppm', 'GameplayTraversalExecutionResult Tick(', 1, 'Фазы прыжка'),
    ('TV-JUMP-TIMEOUT', f'{TRV}/JumpTraversalExecutor.cppm', 'static constexpr float kTraversalTimeoutSeconds = 10.0f;', 1, 'Тайм-аут 10 с'),
    ('PH-BLOCKED', f'{APP}/GameplayPhysicsCharacterIntegration.cpp', 'constexpr float kBlockedIntentSpeed = 0.1f;', 1, 'Пороги physicallyBlocked'),
    ('PH-BLOCKED-CALC', f'{APP}/GameplayPhysicsCharacterIntegration.cpp', 'movementState->physicallyBlocked =', 1, 'Вычисление physicallyBlocked'),
    ('PH-ROOT-OFFSET', f'{APP}/GameplayPhysicsCharacterIntegration.cpp', 'transform->position = *position + binding->visualRootOffset;', 1, 'Transform = основание капсулы'),
    ('CM-MOVEMENT', 'src/Gameplay/Character/CharacterMovement.cppm', 'const mathUtils::Vec3 targetVelocity = command->moveWorld * (targetSpeed * command->moveMagnitude);', 1, 'Команда → целевая скорость'),
    ('CM-MOTOR', 'src/Gameplay/Character/GameplayCharacterComponents.cppm', 'float maxRunSpeed{ 4.5f };', 1, 'Скорости и ускорения мотора по умолчанию'),
    ('CM-PHYSICAL', 'src/Gameplay/Character/GameplayCharacterComponents.cppm', 'constexpr float GetTotalHeight() const noexcept', 1, 'Полная высота = цилиндр + 2r'),
    # ---------------------------------------------------------------- ресурсы, события, брони
    ('EV-WORLD', 'src/Gameplay/Events/GameplayWorldEvent.cppm', 'enum class GameplayWorldEventType', 1, 'Типы мировых событий'),
    ('RV-SYSTEM', 'src/Gameplay/Interaction/GameplayObjectReservationSystem.cppm', 'class GameplayObjectReservationSystem', 1, 'Брони объектов'),
    ('RV-TRY', 'src/Gameplay/Interaction/GameplayObjectReservationSystem.cppm', 'const auto [it, bInserted] = ownerByObject_.try_emplace(objectEntity, agentEntity);', 1, 'Повторная бронь владельцем — успех'),
    ('PK-SYSTEM', 'src/Gameplay/Pickup/GameplayPickupSystem.cppm', 'class GameplayPickupSystem', 1, 'Сбор pickup без учёта броней'),
    # ---------------------------------------------------------------- runtime-фасад и кадр
    ('GR-CTOR', f'{GRT}/GameplayRuntime.cppm', 'GameplayRuntime() : GameplayRuntime(GameplayAIDecisionFactoryRegistry{}) {}', 1, 'Конструктор по умолчанию: пустой каталог'),
    ('GR-MEMBERS', f'{GRT}/GameplayRuntime.cppm', 'AISystem aiSystem_{};', 1, 'Владение AISystem, решениями, реестрами'),
    ('GR-BEGIN', f'{GRT}/GameplayRuntime_PublicApi.inl', 'void GameplayRuntime::BeginFrame()', 1, 'Очистка кадровых данных'),
    ('GR-RESET-FRAME', f'{GRT}/GameplayRuntime_FrameLifecycle.inl', 'void GameplayRuntime::ResetEntityFrameState_(const EntityHandle entity)', 1, 'Сброс CharacterCommand каждый кадр'),
    ('GR-PREPHYS', f'{GRT}/GameplayRuntime_PublicApi.inl', 'void GameplayRuntime::PrePhysicsUpdate(const GameplayUpdateContext& ctx)', 1, 'PrePhysicsUpdate'),
    ('GR-AI-ORDER', f'{GRT}/GameplayRuntime_PublicApi.inl', 'UpdateActiveAIDecisions_();', 1, 'Решения перед aiSystem_.Update'),
    ('GR-AI-UPDATE', f'{GRT}/GameplayRuntime_PublicApi.inl', 'aiSystem_.Update(world_, ctx.deltaSeconds);', 1, 'Тик задач AI'),
    ('GR-POSTPHYS', f'{GRT}/GameplayRuntime_PublicApi.inl', 'void GameplayRuntime::PostPhysicsUpdate(const GameplayUpdateContext& ctx)', 1, 'PostPhysicsUpdate'),
    ('GR-START-ROUTE', f'{GRT}/GameplayRuntime_PublicApi.inl', 'AIActionExecutionStatus GameplayRuntime::StartAIFollowRoute(', 1, 'Прямой FollowRoute с query и debug'),
    ('GR-START-MOVETO', f'{GRT}/GameplayRuntime_PublicApi.inl', 'AIActionExecutionStatus GameplayRuntime::StartAIMoveTo(', 1, 'Прямой MoveTo'),
    ('GR-START-FOLLOW', f'{GRT}/GameplayRuntime_PublicApi.inl', 'AIActionExecutionStatus GameplayRuntime::StartAIFollowTarget(', 1, 'Прямой FollowTarget'),
    ('GR-START-FLEE', f'{GRT}/GameplayRuntime_PublicApi.inl', 'AIActionExecutionStatus GameplayRuntime::StartAIFleeTarget(', 1, 'Прямой FleeTarget'),
    ('GR-CREATE-BINDING', f'{GRT}/GameplayRuntime_PublicApi.inl', 'GameplayRuntime::CreateAIMoveToActionBinding(', 1, 'Программный MoveTo binding с query и debug'),
    ('GR-CANCEL-ACTION', f'{GRT}/GameplayRuntime_PublicApi.inl', 'void GameplayRuntime::CancelAIAction(const EntityHandle agentEntity)', 1, 'Отмена действия'),
    ('GR-START-DECISION', f'{GRT}/GameplayRuntime_PublicApi.inl', 'bool GameplayRuntime::StartAIDecision(', 1, 'Синхронный старт решения'),
    ('GR-CANCEL-DECISION', f'{GRT}/GameplayRuntime_PublicApi.inl', 'void GameplayRuntime::CancelAIDecision(const EntityHandle agentEntity) noexcept', 1, 'Отмена и удаление решения'),
    ('GR-UPDATE-DECISIONS', f'{GRT}/GameplayRuntime_PublicApi.inl', 'void GameplayRuntime::UpdateActiveAIDecisions_()', 1, 'Обновление решений по возрастанию handle'),
    ('GR-DEBUG-VIEWS', f'{GRT}/GameplayRuntime_PublicApi.inl', 'std::vector<GameplayAIDebugAgentView> GameplayRuntime::BuildAIDebugAgentViews() const', 1, 'Снимки отладки'),
    ('GR-MODE-EDITOR', f'{GRT}/GameplayRuntime_ModeAndBootstrap.inl', 'CancelAllAIDecisions_();', 1, 'Выход в Editor очищает AI'),
    ('AP-FACTORIES', f'{APP}/AppLifecycle.cpp', 'rendern::MakeDefaultGameplayAIDecisionFactories());', 1, 'App создаёт runtime с каталогом'),
    ('AP-SET-QUERY', f'{APP}/AppLifecycle.cpp', 'runtimeState.gameplayRuntime->SetObstacleQuery(physicsState.obstacleQuery.get());', 1, 'Подключение физического query'),
    ('AP-TICK', f'{APP}/AppLifecycle.cpp', 'const rendern::GameplayUpdateContext gameplayCtx = UpdateGameplayBeforePhysics(app, deltaSeconds);', 1, 'Порядок: gameplay → physics → post'),
    ('AP-SCENARIO', f'{APP}/AppLifecycle.cpp', 'app.developmentScenarioRuntime.Update(developmentContext);', 1, 'Сценарий обновляется до BeginFrame'),
    ('AP-BEFORE-PHYS', f'{APP}/AppLifecycleImpl/AppLifecycle_TickImpl.inl', 'static rendern::GameplayUpdateContext UpdateGameplayBeforePhysics(AppState& app, float deltaSeconds)', 1, 'BeginFrame, PrePhysics, submit velocities'),
    ('AP-PHYSICS', f'{APP}/AppLifecycleImpl/AppLifecycle_TickImpl.inl', 'static void UpdatePhysics(AppState& app, const float deltaSeconds)', 1, 'Шаг Jolt'),
    ('AP-AFTER-PHYS', f'{APP}/AppLifecycleImpl/AppLifecycle_TickImpl.inl', 'static void UpdateGameplayAfterPhysicsAndAnimation(', 1, 'Feedback, PostPhysics, анимация'),
    ('AP-NAV-COORD', f'{APP}/AppNavigationCoordinator.cpp', 'void Advance(appLifecycle::AppState& app)', 1, 'Bake NavMesh по профилям'),
    ('AP-DEBUG-DRAW', f'{APP}/AppDebugVisualization.cpp', 'if (app.graphicsState.rendererSettings.drawNavigationPathDebug && runtime.gameplayRuntime)', 1, 'Рисование маршрутов'),
    ('AP-DEBUG-PLANNED', f'{APP}/AppDebugVisualization.cpp', 'if (app.graphicsState.rendererSettings.drawAIPlannedPathDebug', 1, 'Planned path и steering telemetry'),
    # ---------------------------------------------------------------- навигация
    ('UI-GOAP-PANEL', 'src/Render/ImGui/ImGuiDebugUI_GOAP.inl', 'ImGui::Begin("AI / GOAP");', 1, 'Окно отладки GOAP'),
    ('UI-DEBUG-FLAGS', 'src/Render/ImGui/ImGuiDebugUI_RendererCore.inl', 'ImGui::Checkbox("AI planned paths", &rs.drawAIPlannedPathDebug);', 1, 'Флаги отладочной отрисовки'),
    ('NV-GEOMETRY', 'src/Navigation/Navigation.cppm', '// Triangle soup is expressed in CoreEngine world space (right-handed, Y-up).', 1, 'Правосторонняя система, +Y вверх'),
    ('NV-AGENT', 'src/Navigation/Navigation.cppm', 'struct AgentSettings', 1, 'Параметры агента NavMesh'),
    ('NV-QUERY-STATUS', 'src/Navigation/Navigation.cppm', 'enum class QueryStatus', 1, 'Статусы запроса пути'),
    ('NV-PROFILES', 'src/Navigation/Navigation.cppm', 'class ProfileRegistry final', 1, 'Bake на профиль агента'),
    ('NV-FINDPATH', 'src/Navigation/Recast/Navigation.cpp', 'PathResult World::FindPath(const PathRequest& request) const', 1, 'Detour: nearest poly, corridor, straight path'),
    ('NV-PARTIAL', 'src/Navigation/Recast/Navigation.cpp', 'if (corridor[corridorCount - 1] != goalPolygon)', 1, 'Частичный коридор = NoPath'),
    ('NV-LIMITS', 'src/Navigation/Recast/Navigation.cpp', 'constexpr int MaximumPathPolygons = 2048;', 1, 'Лимиты буферов'),
    ('NV-CONFIG', 'src/Navigation/Recast/Navigation.cpp', 'config.cs = settings.cellSize;', 1, 'Перевод метров в клетки Recast (ceil/floor)'),
    ('NV-ERODE', 'src/Navigation/Recast/Navigation.cpp', '!rcErodeWalkableArea(&context, config.walkableRadius, *compactHeightfield) ||', 1, 'Эрозия по радиусу агента'),
    ('NV-GEOM-BUILD', f'{APP}/LevelNavigation.cppm', 'GeometryResult BuildNavigationGeometry(', 1, 'Статические меши уровня в мировых координатах'),
    ('NV-BUILD-SETTINGS', 'src/Navigation/Navigation.cppm', 'struct BuildSettings', 1, 'Параметры bake по умолчанию'),
    ('NV-AGENT-MAP', f'{APP}/LevelNavigation.cppm', 'constexpr navigation::AgentSettings BuildAgentSettings(', 1, 'Физика персонажа → профиль NavMesh'),
    # ---------------------------------------------------------------- сценарии
    ('SC-REMOVE-TARGET', f'{APP}/Development/DevelopmentScenario.cpp', 'else if constexpr (std::is_same_v<T, TemporarilyRemoveTargetAtMidpointOperation>)', 2, 'Временное удаление цели (steady_clock)'),
    ('SC-START-DECISION', f'{APP}/Development/DevelopmentScenario.cpp', '!context.gameplayRuntime.StartAIDecision(entity, op.decision, &result->diagnostic))', 1, 'Сценарий запускает production API'),
    ('SC-NAV-PATH', f'{APP}/Development/DevelopmentScenario.cpp', 'else if constexpr (std::is_same_v<T, StartNavigationPathOperation>)', 2, 'Maze: NavMesh → GameplayRoute'),
    ('SC-RUNNER-UPDATE', f'{APP}/Development/DevelopmentScenario.cpp', 'void DevelopmentScenarioRunner::Update(ScenarioContext& context) noexcept', 1, 'Обновление сценария'),
    # ---------------------------------------------------------------- ассеты
    ('AS-CATALOG', 'assets/ai/decisions/catalog.json', '"decisions": [', 1, 'Каталог решений'),
    ('AS-MARKER-GOAP', 'assets/ai/goap/marker_visit.goap.json', '"id": "marker_visit",', 1, 'Marker Visit: определение'),
    ('AS-MARKER-BEHAVIOR', 'assets/ai/behaviors/marker_visit.behavior.json', '"definition": "ai/goap/marker_visit.goap.json",', 1, 'Marker Visit: behavior'),
    ('AS-MARKER-BINDINGS', 'assets/ai/bindings/marker_visit.bindings.json', '"roles": [', 1, 'Marker Visit: роли'),
    ('AS-RECOVERY-GOAP', 'assets/ai/goap/target_recovery.goap.json', '"id": "target_recovery",', 1, 'Target Recovery: определение'),
    ('AS-RECOVERY-BEHAVIOR', 'assets/ai/behaviors/target_recovery.behavior.json', '"definition": "ai/goap/target_recovery.goap.json",', 1, 'Target Recovery: behavior'),
    ('AS-ACCESS-GOAP', 'assets/ai/goap/access_key.goap.json', '"id": "access_key",', 1, 'Access Key: определение'),
    ('AS-ACCESS-BUY', 'assets/ai/goap/access_key.goap.json', '"context": "buy_key",', 1, 'Access Key: purchase'),
    ('AS-ACCESS-BEHAVIOR', 'assets/ai/behaviors/access_key.behavior.json', '"routeGraph": "ai/routes/access_key.routes.json",', 1, 'Access Key: behavior'),
    ('AS-ACCESS-ROUTES', 'assets/ai/routes/access_key.routes.json', '"nodes": [', 1, 'Access Key: граф маршрутов'),
    ('AS-SC-RECOVERY', 'assets/development/ai_goap_target_recovery.scenario.json', '"temporarilyRemoveTargetAtMidpoint"', 1, 'Сценарий Target Recovery'),
    ('AS-SC-MAZE', 'assets/development/navigation_pathfinding_maze.scenario.json', '"startNavigationPath"', 1, 'Сценарий Maze'),
    ('AS-SC-PLAYGROUND', 'assets/development/ai_steering_playground.scenario.json', '"startFleeTarget"', 1, 'Сценарий Steering Playground'),
    ('AS-SC-REPLAN', 'assets/development/ai_goap_access_key_replanning.scenario.json', '"whenPickupCollected"', 1, 'Сценарий перепланирования Access Key'),
    ('AS-SC-RESERVATION', 'assets/development/ai_goap_access_key_reservation.scenario.json', '"access_key_reserved"', 1, 'Сценарий двух агентов'),
    ('AS-SC-JUMP', 'assets/development/ai_goap_access_key_jump.scenario.json', '"registerJumpTraversalLink"', 1, 'Сценарий прыжка'),
]

_CACHE = {}


def _git(*args):
    try:
        return subprocess.run(['git', *args], cwd=REPO, capture_output=True, text=True, check=True).stdout.strip()
    except Exception:  # noqa: BLE001 — git может отсутствовать; это не ошибка сборки
        return ''


def snapshot():
    if 'snapshot' not in _CACHE:
        dirty = {line[3:].strip().strip('"') for line in _git('status', '--porcelain').splitlines() if line}
        _CACHE['snapshot'] = dict(head=_git('rev-parse', 'HEAD'), branch=_git('rev-parse', '--abbrev-ref', 'HEAD'),
                                  dirty=dirty)
    return _CACHE['snapshot']


def read_lines(path):
    key = ('lines', path)
    if key not in _CACHE:
        file_path = REPO / path
        _CACHE[key] = file_path.read_text(encoding='utf-8-sig', errors='replace').splitlines() if file_path.is_file() else None
    return _CACHE[key]


def resolve():
    """Возвращает (resolved, failures). resolved — список словарей с line, occurrences, sha256 файла."""
    if 'resolved' in _CACHE:
        return _CACHE['resolved']
    snap = snapshot()
    resolved, failures, seen = [], [], set()
    for identifier, path, expected, nth, description in REFERENCES:
        if identifier in seen:
            failures.append(f'{identifier}: duplicate id')
            continue
        seen.add(identifier)
        lines = read_lines(path)
        if lines is None:
            failures.append(f'{identifier}: missing file {path}')
            continue
        hits = [number for number, line in enumerate(lines, 1) if expected in line]
        if len(hits) < nth:
            failures.append(f'{identifier}: expected text not found ({len(hits)} hits, need {nth}): {expected}')
            continue
        data = (REPO / path).read_bytes()
        resolved.append(dict(id=identifier, path=path, line=hits[nth - 1], expected=expected, nth=nth,
                             occurrences=len(hits), description=description,
                             file_sha256=hashlib.sha256(data).hexdigest(),
                             state='working-tree' if path in snap['dirty'] else 'HEAD'))
    _CACHE['resolved'] = (resolved, failures)
    return resolved, failures


def by_id():
    resolved, _ = resolve()
    return {entry['id']: entry for entry in resolved}


def excerpt(identifier, count):
    entry = by_id()[identifier]
    lines = read_lines(entry['path'])
    start = entry['line']
    end = min(len(lines), start + count - 1)
    return start, end, lines[start - 1:end]


if __name__ == '__main__':
    ok, bad = resolve()
    print(len(ok), 'resolved;', len(bad), 'failures')
    for failure in bad:
        print('  ', failure)
