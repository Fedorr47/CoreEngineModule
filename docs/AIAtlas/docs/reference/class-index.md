# Индекс сущностей

Автоматический индекс собран `tools/build_class_index.py` эвристическим парсером из 69 файлов AI, навигации, traversal и связанных систем (список — в конце страницы). Колонка «разбор» отделяет **ручной** архитектурный разбор (`tools/class_notes.py`: владелец, lifetime, вызовы, теория, тесты) от **автоматической** карточки (объявление, члены, строки). Тривиальные значения enum не получают отдельных карточек.

Путь чтения: сущность → карточка (метод/контракт) → глава с теорией → [реестр источников](source-references.md) → фрагмент кода.

<div data-filter-table markdown>

| Сущность | Вид | Домен | Экспорт | Объявление | Разбор | Назначение |
|---|---|---|---|---|---|---|
| [AIActionBindingRegistry](classes/AIActionBindingRegistry.md) | class | Action runtime | да | `src/Gameplay/AI/Runtime/AIActionBinding.cppm:24` | авто |  |
| [AIActionContextId](classes/AIActionContextId.md) | alias | Action runtime | да | `src/Gameplay/AI/Runtime/AIActionContracts.cppm:51` | авто |  |
| [AIActionContextIdTag](classes/AIActionContextIdTag.md) | struct | Action runtime | нет | `src/Gameplay/AI/Runtime/AIActionContracts.cppm:16` | авто |  |
| [AIActionExecutionStatus](classes/AIActionExecutionStatus.md) | enum | Action runtime | да | `src/Gameplay/AI/Runtime/AIActionContracts.cppm:53` | авто |  |
| [AIActionId](classes/AIActionId.md) | alias | Action runtime | да | `src/Gameplay/AI/Runtime/AIActionContracts.cppm:48` | авто |  |
| [AIActionIdTag](classes/AIActionIdTag.md) | struct | Action runtime | нет | `src/Gameplay/AI/Runtime/AIActionContracts.cppm:12` | авто |  |
| [AIActionRuntimeContext](classes/AIActionRuntimeContext.md) | struct | Action runtime | да | `src/Gameplay/AI/Runtime/AIActionRuntime.cppm:15` | авто |  |
| [AIActionRuntimeResult](classes/AIActionRuntimeResult.md) | enum | Action runtime | да | `src/Gameplay/AI/Runtime/AIActionRuntime.cppm:34` | авто |  |
| [AIActionTask](classes/AIActionTask.md) | class | Action runtime | да | `src/Gameplay/AI/Runtime/AIActionTask.cppm:14` | ручной | Владеющая обёртка IAIActionRuntime со статусом и порядком протокола Start/Tick/Cancel. |
| [AIComponent](classes/AIComponent.md) | struct | Action runtime | да | `src/Gameplay/AI/Runtime/GameplayAIComponents.cppm:7` | авто |  |
| [AIId](classes/AIId.md) | struct | Action runtime | да | `src/Gameplay/AI/Runtime/AIActionContracts.cppm:22` | авто |  |
| [AISystem](classes/AISystem.md) | class | Action runtime | да | `src/Gameplay/AI/Runtime/AISystem.cppm:17` | ручной | Владелец задач действий: не более одной задачи на агента; синхронный старт и покадровый тик. |
| [IAIActionBinding](classes/IAIActionBinding.md) | class | Action runtime | да | `src/Gameplay/AI/Runtime/AIActionBinding.cppm:14` | ручной | Фабрика runtime для шага плана по контексту (позднее связывание семантики и исполнителя). |
| [IAIActionRuntime](classes/IAIActionRuntime.md) | class | Action runtime | да | `src/Gameplay/AI/Runtime/AIActionRuntime.cppm:41` | ручной | Протокол исполнения шага: Start → Running/Succeeded/Failed, Tick, Cancel (без применения эффектов). |
| [AgentSettings](classes/AgentSettings.md) | struct | NavMesh | да | `src/Navigation/Navigation.cppm:22` | авто |  |
| [AppendPathDebugGeometry](classes/AppendPathDebugGeometry.md) | free function | NavMesh | да | `src/Navigation/Navigation.cppm:108` | авто |  |
| [BuildSettings](classes/BuildSettings.md) | struct | NavMesh | да | `src/Navigation/Navigation.cppm:32` | авто |  |
| [BuildStatus](classes/BuildStatus.md) | enum | NavMesh | да | `src/Navigation/Navigation.cppm:46` | авто |  |
| [DebugGeometry](classes/DebugGeometry.md) | struct | NavMesh | да | `src/Navigation/Navigation.cppm:101` | авто |  |
| [DebugLine](classes/DebugLine.md) | struct | NavMesh | да | `src/Navigation/Navigation.cppm:84` | авто |  |
| [DebugTriangle](classes/DebugTriangle.md) | struct | NavMesh | да | `src/Navigation/Navigation.cppm:91` | авто |  |
| [Geometry](classes/Geometry.md) | struct | NavMesh | да | `src/Navigation/Navigation.cppm:16` | авто |  |
| [PathRequest](classes/PathRequest.md) | struct | NavMesh | да | `src/Navigation/Navigation.cppm:71` | авто |  |
| [PathResult](classes/PathResult.md) | struct | NavMesh | да | `src/Navigation/Navigation.cppm:78` | авто |  |
| [ProfileHandle](classes/ProfileHandle.md) | struct | NavMesh | да | `src/Navigation/Navigation.cppm:140` | авто |  |
| [ProfileRegistry](classes/ProfileRegistry.md) | class | NavMesh | да | `src/Navigation/Navigation.cppm:158` | авто |  |
| [ProfileResolution](classes/ProfileResolution.md) | struct | NavMesh | да | `src/Navigation/Navigation.cppm:151` | авто |  |
| [ProjectionResult](classes/ProjectionResult.md) | struct | NavMesh | да | `src/Navigation/Navigation.cppm:65` | авто |  |
| [QueryStatus](classes/QueryStatus.md) | enum | NavMesh | да | `src/Navigation/Navigation.cppm:54` | авто |  |
| [World](classes/World.md) | class | NavMesh | да | `src/Navigation/Navigation.cppm:113` | ручной | NavMesh одного профиля агента: bake Recast и запросы Detour. |
| [ApplyGameplayObstacleAvoidance](classes/ApplyGameplayObstacleAvoidance.md) | free function | Obstacle avoidance | да | `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:125` | ручной | Локальная коррекция намерения: три sweep-пробы, escape-кандидаты, опора, выбор стороны с памятью. |
| [GameplayObstacleAvoidanceDebugSnapshot](classes/GameplayObstacleAvoidanceDebugSnapshot.md) | struct | Obstacle avoidance | да | `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:59` | авто |  |
| [GameplayObstacleAvoidanceInput](classes/GameplayObstacleAvoidanceInput.md) | struct | Obstacle avoidance | да | `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:115` | авто |  |
| [GameplayObstacleAvoidanceSettings](classes/GameplayObstacleAvoidanceSettings.md) | struct | Obstacle avoidance | да | `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:102` | авто |  |
| [GameplayObstacleAvoidanceState](classes/GameplayObstacleAvoidanceState.md) | struct | Obstacle avoidance | да | `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:44` | авто |  |
| [GameplayObstacleProbeDebugState](classes/GameplayObstacleProbeDebugState.md) | struct | Obstacle avoidance | да | `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:49` | авто |  |
| [GameplayObstacleProbeHit](classes/GameplayObstacleProbeHit.md) | struct | Obstacle avoidance | да | `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:22` | авто |  |
| [GameplayObstacleProbeRequest](classes/GameplayObstacleProbeRequest.md) | struct | Obstacle avoidance | да | `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:14` | авто |  |
| [GameplaySupportProbeHit](classes/GameplaySupportProbeHit.md) | struct | Obstacle avoidance | да | `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:35` | авто |  |
| [GameplaySupportProbeRequest](classes/GameplaySupportProbeRequest.md) | struct | Obstacle avoidance | да | `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:29` | авто |  |
| [IGameplayObstacleQuery](classes/IGameplayObstacleQuery.md) | class | Obstacle avoidance | да | `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:79` | ручной | Интерфейс геометрических запросов avoidance: Probe (sweep) и ProbeSupport (опора; по умолчанию синтетическая). |
| [ProbeResult](classes/ProbeResult.md) | struct | Obstacle avoidance | нет | `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:176` | авто |  |
| [SanitizedSettings](classes/SanitizedSettings.md) | struct | Obstacle avoidance | нет | `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:161` | авто |  |
| [GameplayRuntime](classes/GameplayRuntime.md) | class | Runtime-фасад | да | `src/Gameplay/Runtime/GameplayRuntime.cppm:88` | ручной | Фасад gameplay: владеет AISystem, решениями, реестрами traversal, бронями; задаёт порядок кадра. |
| [ApplyGameplayMovementIntent](classes/ApplyGameplayMovementIntent.md) | free function | Steering | да | `src/Gameplay/Navigation/GameplaySteering.cppm:161` | авто |  |
| [BuildGameplayArrivalSteering](classes/BuildGameplayArrivalSteering.md) | free function | Steering | да | `src/Gameplay/Navigation/GameplaySteering.cppm:110` | ручной | Arrival в XZ: нулевая команда в r + 0.001, линейная шкала до R с нижней границей m_min. |
| [BuildGameplayFleeSteering](classes/BuildGameplayFleeSteering.md) | free function | Steering | да | `src/Gameplay/Navigation/GameplaySteering.cppm:88` | авто |  |
| [BuildGameplaySeekSteering](classes/BuildGameplaySeekSteering.md) | free function | Steering | да | `src/Gameplay/Navigation/GameplaySteering.cppm:59` | авто |  |
| [GameplayArrivalSteeringSettings](classes/GameplayArrivalSteeringSettings.md) | struct | Steering | да | `src/Gameplay/Navigation/GameplaySteering.cppm:33` | авто |  |
| [GameplayFleeSteeringSettings](classes/GameplayFleeSteeringSettings.md) | struct | Steering | да | `src/Gameplay/Navigation/GameplaySteering.cppm:47` | авто |  |
| [GameplayMovementIntent](classes/GameplayMovementIntent.md) | struct | Steering | да | `src/Gameplay/Navigation/GameplaySteering.cppm:14` | авто |  |
| [GameplaySeekSteeringSettings](classes/GameplaySeekSteeringSettings.md) | struct | Steering | да | `src/Gameplay/Navigation/GameplaySteering.cppm:41` | авто |  |
| [GameplaySteeringOutput](classes/GameplaySteeringOutput.md) | struct | Steering | да | `src/Gameplay/Navigation/GameplaySteering.cppm:52` | авто |  |
| [GameplaySteeringStatus](classes/GameplaySteeringStatus.md) | enum | Steering | да | `src/Gameplay/Navigation/GameplaySteering.cppm:27` | авто |  |
| [DoorTraversalExecutor](classes/DoorTraversalExecutor.md) | class | Traversal | да | `src/Gameplay/Traversal/DoorTraversalExecutor.cppm:17` | ручной | Дверь по link: бронь, подход к InteractionPoint, логическое открытие, освобождение. |
| [GameplayJumpTraversalData](classes/GameplayJumpTraversalData.md) | struct | Traversal | да | `src/Gameplay/Traversal/GameplayTraversalLink.cppm:42` | авто |  |
| [GameplayTraversalExecutionContext](classes/GameplayTraversalExecutionContext.md) | struct | Traversal | да | `src/Gameplay/Traversal/GameplayTraversalExecutor.cppm:20` | авто |  |
| [GameplayTraversalExecutionResult](classes/GameplayTraversalExecutionResult.md) | enum | Traversal | да | `src/Gameplay/Traversal/GameplayTraversalExecutor.cppm:13` | авто |  |
| [GameplayTraversalExecutorRegistry](classes/GameplayTraversalExecutorRegistry.md) | class | Traversal | да | `src/Gameplay/Traversal/GameplayTraversalExecutorRegistry.cppm:14` | авто |  |
| [GameplayTraversalLink](classes/GameplayTraversalLink.md) | struct | Traversal | да | `src/Gameplay/Traversal/GameplayTraversalLink.cppm:61` | авто |  |
| [GameplayTraversalLinkHandleHasher](classes/GameplayTraversalLinkHandleHasher.md) | struct | Traversal | да | `src/Gameplay/Traversal/GameplayTraversalLinkRegistry.cppm:15` | авто |  |
| [GameplayTraversalLinkRegistry](classes/GameplayTraversalLinkRegistry.md) | class | Traversal | да | `src/Gameplay/Traversal/GameplayTraversalLinkRegistry.cppm:23` | авто |  |
| [GameplayTraversalTypeId](classes/GameplayTraversalTypeId.md) | struct | Traversal | да | `src/Gameplay/Traversal/GameplayTraversalLink.cppm:15` | авто |  |
| [GameplayTraversalTypeIdHasher](classes/GameplayTraversalTypeIdHasher.md) | struct | Traversal | да | `src/Gameplay/Traversal/GameplayTraversalLink.cppm:31` | авто |  |
| [GameplayUnsupportedTraversalExecutor](classes/GameplayUnsupportedTraversalExecutor.md) | class | Traversal | да | `src/Gameplay/Traversal/GameplayTraversalExecutor.cppm:47` | авто |  |
| [IGameplayTraversalExecutor](classes/IGameplayTraversalExecutor.md) | class | Traversal | да | `src/Gameplay/Traversal/GameplayTraversalExecutor.cppm:36` | авто |  |
| [JumpTraversalExecutor](classes/JumpTraversalExecutor.md) | class | Traversal | да | `src/Gameplay/Traversal/JumpTraversalExecutor.cppm:18` | ручной | Прыжок по link: подход к takeoff, запрос kGameplayActionJump, ожидание взлёта, приземление в допусках, тайм-аут 10 с. |
| [CompileGameplayGOAPDefinition](classes/CompileGameplayGOAPDefinition.md) | free function | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm:66` | авто |  |
| [GameplayAIBehaviorAsset](classes/GameplayAIBehaviorAsset.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:57` | авто |  |
| [GameplayAICapabilityAsset](classes/GameplayAICapabilityAsset.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:30` | авто |  |
| [GameplayAIComponentParseContext](classes/GameplayAIComponentParseContext.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm:176` | авто |  |
| [GameplayAIComponentParsers](classes/GameplayAIComponentParsers.md) | class | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm:183` | авто |  |
| [GameplayAIDecisionAssetReference](classes/GameplayAIDecisionAssetReference.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:83` | авто |  |
| [GameplayAIDecisionCatalogAsset](classes/GameplayAIDecisionCatalogAsset.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:90` | авто |  |
| [GameplayAILevelBindingsAsset](classes/GameplayAILevelBindingsAsset.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:76` | авто |  |
| [GameplayAIObservationAsset](classes/GameplayAIObservationAsset.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:25` | авто |  |
| [GameplayAIReactionAsset](classes/GameplayAIReactionAsset.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:35` | авто |  |
| [GameplayAIRoleBindingAsset](classes/GameplayAIRoleBindingAsset.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:70` | авто |  |
| [GameplayAIRouteEdgeAsset](classes/GameplayAIRouteEdgeAsset.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:40` | авто |  |
| [GameplayAIRouteGraphAsset](classes/GameplayAIRouteGraphAsset.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:45` | авто |  |
| [GameplayAITraversalBindingAsset](classes/GameplayAITraversalBindingAsset.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:51` | авто |  |
| [GameplayGOAPActionCostOverride](classes/GameplayGOAPActionCostOverride.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm:28` | авто |  |
| [GameplayGOAPAuthoredAction](classes/GameplayGOAPAuthoredAction.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm:39` | авто |  |
| [GameplayGOAPAuthoredBoolean](classes/GameplayGOAPAuthoredBoolean.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm:25` | авто |  |
| [GameplayGOAPAuthoredFact](classes/GameplayGOAPAuthoredFact.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm:21` | авто |  |
| [GameplayGOAPAuthoredGoal](classes/GameplayGOAPAuthoredGoal.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm:34` | авто |  |
| [GameplayGOAPAuthoredNumeric](classes/GameplayGOAPAuthoredNumeric.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm:29` | авто |  |
| [GameplayGOAPCompiledDefinition](classes/GameplayGOAPCompiledDefinition.md) | class | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm:34` | авто |  |
| [GameplayGOAPDefinitionAsset](classes/GameplayGOAPDefinitionAsset.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm:50` | авто |  |
| [GameplayGOAPSemanticAction](classes/GameplayGOAPSemanticAction.md) | struct | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm:24` | авто |  |
| [LoadGameplayAIBehaviorAsset](classes/LoadGameplayAIBehaviorAsset.md) | free function | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:104` | авто |  |
| [LoadGameplayAIDecisionCatalogAsset](classes/LoadGameplayAIDecisionCatalogAsset.md) | free function | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:106` | авто |  |
| [LoadGameplayAILevelBindingsAsset](classes/LoadGameplayAILevelBindingsAsset.md) | free function | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:105` | авто |  |
| [LoadGameplayAIRouteGraphAsset](classes/LoadGameplayAIRouteGraphAsset.md) | free function | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:97` | авто |  |
| [LoadGameplayGOAPDefinitionAsset](classes/LoadGameplayGOAPDefinitionAsset.md) | free function | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm:62` | авто |  |
| [ParseGameplayAIBehaviorAsset](classes/ParseGameplayAIBehaviorAsset.md) | free function | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:98` | авто |  |
| [ParseGameplayAIDecisionCatalogAsset](classes/ParseGameplayAIDecisionCatalogAsset.md) | free function | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:102` | авто |  |
| [ParseGameplayAILevelBindingsAsset](classes/ParseGameplayAILevelBindingsAsset.md) | free function | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:100` | авто |  |
| [ParseGameplayAIRouteGraphAsset](classes/ParseGameplayAIRouteGraphAsset.md) | free function | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm:96` | авто |  |
| [ParseGameplayGOAPDefinitionAsset](classes/ParseGameplayGOAPDefinitionAsset.md) | free function | Авторинг | да | `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm:59` | авто |  |
| [AIDebugActionApplicabilityView](classes/AIDebugActionApplicabilityView.md) | struct | Диагностика | да | `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:62` | авто |  |
| [AIDebugBooleanFactView](classes/AIDebugBooleanFactView.md) | struct | Диагностика | да | `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:21` | авто |  |
| [AIDebugFailedBooleanConditionView](classes/AIDebugFailedBooleanConditionView.md) | struct | Диагностика | да | `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:45` | авто |  |
| [AIDebugFailedNumericConditionView](classes/AIDebugFailedNumericConditionView.md) | struct | Диагностика | да | `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:53` | авто |  |
| [AIDebugIntegerFactView](classes/AIDebugIntegerFactView.md) | struct | Диагностика | да | `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:28` | авто |  |
| [AIDebugPlanStepView](classes/AIDebugPlanStepView.md) | struct | Диагностика | да | `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:35` | авто |  |
| [AIDebugViewModel](classes/AIDebugViewModel.md) | struct | Диагностика | да | `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:73` | авто |  |
| [BuildAIDebugViewModel](classes/BuildAIDebugViewModel.md) | free function | Диагностика | да | `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:88` | авто |  |
| [GameplayAIDebugPlannedPathView](classes/GameplayAIDebugPlannedPathView.md) | struct | Диагностика | да | `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:24` | авто |  |
| [GameplayAIDebugPlannedRouteStep](classes/GameplayAIDebugPlannedRouteStep.md) | struct | Диагностика | да | `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:16` | авто |  |
| [GameplayNavigationDebugRegistry](classes/GameplayNavigationDebugRegistry.md) | class | Диагностика | да | `src/Gameplay/Navigation/GameplayNavigationDebug.cppm:15` | авто |  |
| [GameplayRequestContext](classes/GameplayRequestContext.md) | struct | Диагностика | да | `src/Gameplay/Navigation/GameplayRequest.cppm:40` | авто |  |
| [GameplayRequestHandle](classes/GameplayRequestHandle.md) | struct | Диагностика | да | `src/Gameplay/Navigation/GameplayRequest.cppm:16` | авто |  |
| [GameplayRequestStatus](classes/GameplayRequestStatus.md) | enum | Диагностика | да | `src/Gameplay/Navigation/GameplayRequest.cppm:54` | авто |  |
| [GameplaySteeringDebugMode](classes/GameplaySteeringDebugMode.md) | enum | Диагностика | да | `src/Gameplay/Navigation/GameplaySteeringDebug.cppm:14` | авто |  |
| [GameplaySteeringDebugRegistry](classes/GameplaySteeringDebugRegistry.md) | class | Диагностика | да | `src/Gameplay/Navigation/GameplaySteeringDebug.cppm:28` | авто |  |
| [GameplaySteeringDebugState](classes/GameplaySteeringDebugState.md) | struct | Диагностика | да | `src/Gameplay/Navigation/GameplaySteeringDebug.cppm:22` | авто |  |
| [IGameplayGOAPActionPathProvider](classes/IGameplayGOAPActionPathProvider.md) | class | Диагностика | да | `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:31` | авто |  |
| [IGameplayGOAPInspection](classes/IGameplayGOAPInspection.md) | class | Диагностика | да | `src/Gameplay/AI/GOAP/Debug/GameplayGOAPInspection.cppm:11` | авто |  |
| [IGameplayGOAPPathInspection](classes/IGameplayGOAPPathInspection.md) | class | Диагностика | да | `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:47` | авто |  |
| [IGameplayGOAPPlannedPathProvider](classes/IGameplayGOAPPlannedPathProvider.md) | class | Диагностика | да | `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:38` | авто |  |
| [IGameplayRequestBoundary](classes/IGameplayRequestBoundary.md) | class | Диагностика | да | `src/Gameplay/Navigation/GameplayRequest.cppm:81` | авто |  |
| [IsGameplayRequestTerminal](classes/IsGameplayRequestTerminal.md) | free function | Диагностика | да | `src/Gameplay/Navigation/GameplayRequest.cppm:63` | авто |  |
| [GameplayPhysicsObstacleQuery](classes/GameplayPhysicsObstacleQuery.md) | class | Интеграция App | нет | `src/App/GameplayPhysicsObstacleQuery.h:15` | ручной | Адаптер Jolt для avoidance: sweep сферы / луч и луч вниз для опоры; маска StaticWorld по умолчанию. |
| [AIDecisionRuntime](classes/AIDecisionRuntime.md) | class | Исполнение плана | да | `src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm:22` | ручной | Мониторинговый контур одного агента: синхронизация шага, выбор цели, проверка плана, перепланирование, старт шагов. |
| [AIPlanExecution](classes/AIPlanExecution.md) | class | Исполнение плана | да | `src/Gameplay/AI/GOAP/AIPlanExecution.cppm:25` | ручной | Конечный автомат исполнения плана по индексу шага. |
| [AIPlanExecutionBridge](classes/AIPlanExecutionBridge.md) | class | Исполнение плана | да | `src/Gameplay/AI/GOAP/AIPlanExecutionBridge.cppm:16` | ручной | Мост от семантического шага к задаче AISystem: binding → runtime → StartAction; синхронизация и отмена. |
| [AIPlanExecutionStatus](classes/AIPlanExecutionStatus.md) | enum | Исполнение плана | да | `src/Gameplay/AI/GOAP/AIPlanExecution.cppm:15` | авто |  |
| [ComposedContext](classes/ComposedContext.md) | class | Композиция | нет | `src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:88` | авто |  |
| [CreateGameplayGOAPDecisionFromAssets](classes/CreateGameplayGOAPDecisionFromAssets.md) | free function | Композиция | да | `src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:239` | авто |  |
| [CreateGameplayGOAPDecisionFromTemplate](classes/CreateGameplayGOAPDecisionFromTemplate.md) | free function | Композиция | да | `src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:149` | ручной | Связывание template с уровнем и создание instance: роли, наблюдения, реакции, providers, cost overrides. |
| [GameplayAIResolvedRole](classes/GameplayAIResolvedRole.md) | struct | Композиция | да | `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:26` | авто |  |
| [GameplayGOAPCapabilityRegistration](classes/GameplayGOAPCapabilityRegistration.md) | struct | Композиция | да | `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:131` | авто |  |
| [GameplayGOAPCompositionContext](classes/GameplayGOAPCompositionContext.md) | struct | Композиция | да | `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:61` | авто |  |
| [GameplayGOAPCompositionRegistry](classes/GameplayGOAPCompositionRegistry.md) | class | Композиция | да | `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:139` | ручной | Реестр типов компонентов: parser (JSON → T) + compiler (T + контекст → объект) для наблюдений, capabilities и реакций. |
| [GameplayGOAPDecisionTemplate](classes/GameplayGOAPDecisionTemplate.md) | class | Композиция | да | `src/Gameplay/AI/Composition/GameplayGOAPDecisionTemplate.cppm:31` | ручной | Неизменяемый снимок конфигурации решения, независимый от мира и агента. |
| [GameplayGOAPPreparedCapabilityGroup](classes/GameplayGOAPPreparedCapabilityGroup.md) | struct | Композиция | да | `src/Gameplay/AI/Composition/GameplayGOAPDecisionTemplate.cppm:22` | авто |  |
| [IGameplayGOAPCapability](classes/IGameplayGOAPCapability.md) | class | Композиция | да | `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:52` | авто |  |
| [IGameplayGOAPObservation](classes/IGameplayGOAPObservation.md) | class | Композиция | да | `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:35` | авто |  |
| [InspectedContext](classes/InspectedContext.md) | class | Композиция | нет | `src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:117` | авто |  |
| [MakeDefaultGameplayAIDecisionFactories](classes/MakeDefaultGameplayAIDecisionFactories.md) | free function | Композиция | да | `src/Gameplay/AI/Composition/GameplayAIBuiltinDecisions.cppm:9` | авто |  |
| [MakeDefaultGameplayGOAPComponents](classes/MakeDefaultGameplayGOAPComponents.md) | free function | Композиция | да | `src/Gameplay/AI/Composition/GameplayGOAPBuiltinComponents.cppm:13` | авто |  |
| [OrderGameplayGOAPObservations](classes/OrderGameplayGOAPObservations.md) | free function | Композиция | да | `src/Gameplay/AI/Composition/GameplayGOAPObservationOrder.cppm:18` | ручной | Проверка единственного writer и топологический порядок наблюдений по метаданным входов/выходов. |
| [RegisterGameplayAIDecisionAssets](classes/RegisterGameplayAIDecisionAssets.md) | free function | Композиция | да | `src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:252` | ручной | Транзакционная регистрация каталога решений: все templates или ничего. |
| [AuthoredGraph](classes/AuthoredGraph.md) | struct | Компоненты | нет | `src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm:31` | авто |  |
| [GameplayAIHideOnPurchaseAsset](classes/GameplayAIHideOnPurchaseAsset.md) | struct | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponentAssets.cppm:34` | авто |  |
| [GameplayAIMoveToAsset](classes/GameplayAIMoveToAsset.md) | struct | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponentAssets.cppm:13` | авто |  |
| [GameplayAINearestLocationAsset](classes/GameplayAINearestLocationAsset.md) | struct | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponentAssets.cppm:28` | авто |  |
| [GameplayAIPickupAvailabilityAsset](classes/GameplayAIPickupAvailabilityAsset.md) | struct | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponentAssets.cppm:22` | авто |  |
| [GameplayAIPurchaseAsset](classes/GameplayAIPurchaseAsset.md) | struct | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponentAssets.cppm:29` | авто |  |
| [GameplayAIResourceLedgerAsset](classes/GameplayAIResourceLedgerAsset.md) | struct | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponentAssets.cppm:23` | авто |  |
| [GameplayAIResourcePickupAsset](classes/GameplayAIResourcePickupAsset.md) | struct | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponentAssets.cppm:13` | авто |  |
| [GameplayAIResourceReceiptAsset](classes/GameplayAIResourceReceiptAsset.md) | struct | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponentAssets.cppm:18` | авто |  |
| [GameplayAISpatialObservationAsset](classes/GameplayAISpatialObservationAsset.md) | struct | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponentAssets.cppm:13` | авто |  |
| [HideOnPurchaseReaction](classes/HideOnPurchaseReaction.md) | class | Компоненты | нет | `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:172` | авто |  |
| [MoveToCapability](classes/MoveToCapability.md) | class | Компоненты | нет | `src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm:92` | ручной | Provider move_to: компиляция графа маршрутов, route costs, запросы MoveTo, бронь цели, debug-маршрут. |
| [NearestLocationObservation](classes/NearestLocationObservation.md) | class | Компоненты | нет | `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:90` | авто |  |
| [ParseGameplayAIHideOnPurchase](classes/ParseGameplayAIHideOnPurchase.md) | free function | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponentAssets.cppm:73` | авто |  |
| [ParseGameplayAIMoveTo](classes/ParseGameplayAIMoveTo.md) | free function | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponentAssets.cppm:22` | авто |  |
| [ParseGameplayAINearestLocation](classes/ParseGameplayAINearestLocation.md) | free function | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponentAssets.cppm:62` | авто |  |
| [ParseGameplayAIPickupAvailability](classes/ParseGameplayAIPickupAvailability.md) | free function | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponentAssets.cppm:52` | авто |  |
| [ParseGameplayAIPurchase](classes/ParseGameplayAIPurchase.md) | free function | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponentAssets.cppm:64` | авто |  |
| [ParseGameplayAIResourceLedger](classes/ParseGameplayAIResourceLedger.md) | free function | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponentAssets.cppm:39` | авто |  |
| [ParseGameplayAISpatialObservation](classes/ParseGameplayAISpatialObservation.md) | free function | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponentAssets.cppm:34` | авто |  |
| [PickupAvailabilityObservation](classes/PickupAvailabilityObservation.md) | class | Компоненты | нет | `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:148` | авто |  |
| [PurchaseBinding](classes/PurchaseBinding.md) | class | Компоненты | нет | `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:256` | авто |  |
| [PurchaseCapability](classes/PurchaseCapability.md) | class | Компоненты | нет | `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:283` | авто |  |
| [PurchaseRuntime](classes/PurchaseRuntime.md) | class | Компоненты | нет | `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:212` | ручной | Мгновенная покупка: проверки и событие ResourcePurchased; факты не меняет. |
| [RegisterGameplayGOAPMoveToComponent](classes/RegisterGameplayGOAPMoveToComponent.md) | free function | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm:252` | авто |  |
| [RegisterGameplayGOAPResourceComponents](classes/RegisterGameplayGOAPResourceComponents.md) | free function | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:338` | авто |  |
| [RegisterGameplayGOAPSpatialComponents](classes/RegisterGameplayGOAPSpatialComponents.md) | free function | Компоненты | да | `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:183` | авто |  |
| [ResourceLedgerObservation](classes/ResourceLedgerObservation.md) | class | Компоненты | нет | `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:81` | ручной | Бухгалтерия ресурса по подтверждённым событиям сбора и покупки своего агента. |
| [SpatialObservation](classes/SpatialObservation.md) | class | Компоненты | нет | `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm:20` | авто |  |
| [Transaction](classes/Transaction.md) | struct | Компоненты | нет | `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:31` | авто |  |
| [FindWeightedGameplayRoute](classes/FindWeightedGameplayRoute.md) | free function | Маршруты | да | `src/Gameplay/Navigation/GameplayRouteSearch.cppm:94` | ручной | Дейкстра по явному ориентированному графу с восстановлением пути по предшественникам. |
| [GameplayRoute](classes/GameplayRoute.md) | struct | Маршруты | да | `src/Gameplay/Navigation/GameplayRoute.cppm:56` | авто |  |
| [GameplayRouteFollower](classes/GameplayRouteFollower.md) | class | Маршруты | да | `src/Gameplay/Navigation/GameplayRouteFollower.cppm:41` | ручной | Автомат следования по ломаной: arrival промежуточных/финальной точек, look-ahead, плоскость перехода, traversal. |
| [GameplayRouteFollowerOutput](classes/GameplayRouteFollowerOutput.md) | struct | Маршруты | да | `src/Gameplay/Navigation/GameplayRouteFollower.cppm:28` | авто |  |
| [GameplayRouteFollowerSettings](classes/GameplayRouteFollowerSettings.md) | struct | Маршруты | да | `src/Gameplay/Navigation/GameplayRouteFollower.cppm:35` | авто |  |
| [GameplayRouteFollowerStatus](classes/GameplayRouteFollowerStatus.md) | enum | Маршруты | да | `src/Gameplay/Navigation/GameplayRouteFollower.cppm:19` | авто |  |
| [GameplayRouteGraph](classes/GameplayRouteGraph.md) | struct | Маршруты | да | `src/Gameplay/Navigation/GameplayRouteSearch.cppm:64` | авто |  |
| [GameplayRouteGraphEdge](classes/GameplayRouteGraphEdge.md) | struct | Маршруты | да | `src/Gameplay/Navigation/GameplayRouteSearch.cppm:51` | авто |  |
| [GameplayRouteGraphNode](classes/GameplayRouteGraphNode.md) | struct | Маршруты | да | `src/Gameplay/Navigation/GameplayRouteSearch.cppm:44` | авто |  |
| [GameplayRouteNodeId](classes/GameplayRouteNodeId.md) | struct | Маршруты | да | `src/Gameplay/Navigation/GameplayRouteSearch.cppm:21` | авто |  |
| [GameplayRoutePoint](classes/GameplayRoutePoint.md) | struct | Маршруты | да | `src/Gameplay/Navigation/GameplayRoute.cppm:35` | авто |  |
| [GameplayRouteSearchQueueEntry](classes/GameplayRouteSearchQueueEntry.md) | struct | Маршруты | нет | `src/Gameplay/Navigation/GameplayRouteSearch.cppm:108` | авто |  |
| [GameplayRouteSearchQueueEntryGreater](classes/GameplayRouteSearchQueueEntryGreater.md) | struct | Маршруты | нет | `src/Gameplay/Navigation/GameplayRouteSearch.cppm:115` | авто |  |
| [GameplayRouteSearchResult](classes/GameplayRouteSearchResult.md) | struct | Маршруты | да | `src/Gameplay/Navigation/GameplayRouteSearch.cppm:80` | авто |  |
| [GameplayRouteSearchStatus](classes/GameplayRouteSearchStatus.md) | enum | Маршруты | да | `src/Gameplay/Navigation/GameplayRouteSearch.cppm:72` | авто |  |
| [GameplayRouteSegmentAnnotation](classes/GameplayRouteSegmentAnnotation.md) | struct | Маршруты | да | `src/Gameplay/Navigation/GameplayRoute.cppm:42` | авто |  |
| [GameplayTraversalLinkHandle](classes/GameplayTraversalLinkHandle.md) | struct | Маршруты | да | `src/Gameplay/Navigation/GameplayRoute.cppm:14` | авто |  |
| [AIFleeTargetAction](classes/AIFleeTargetAction.md) | class | Навигационные действия | да | `src/Gameplay/AI/Navigation/AIFleeTargetAction.cppm:18` | авто |  |
| [AIFleeTargetActionRuntime](classes/AIFleeTargetActionRuntime.md) | class | Навигационные действия | да | `src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm:30` | ручной | Бегство от живой цели с гистерезисом trigger/safe и выборкой. |
| [AIFleeTargetSettings](classes/AIFleeTargetSettings.md) | struct | Навигационные действия | да | `src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm:22` | авто |  |
| [AIFollowRouteAction](classes/AIFollowRouteAction.md) | class | Навигационные действия | да | `src/Gameplay/AI/Navigation/AIFollowRouteAction.cppm:24` | авто |  |
| [AIFollowRouteActionRuntime](classes/AIFollowRouteActionRuntime.md) | class | Навигационные действия | да | `src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm:27` | ручной | Исполнение маршрута: follower, avoidance (если есть query), команда и facing, traversal handoff, physicallyBlocked. |
| [AIFollowTargetAction](classes/AIFollowTargetAction.md) | class | Навигационные действия | да | `src/Gameplay/AI/Navigation/AIFollowTargetAction.cppm:18` | авто |  |
| [AIFollowTargetActionRuntime](classes/AIFollowTargetActionRuntime.md) | class | Навигационные действия | да | `src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm:27` | ручной | Следование за живой целью: Seek с выборкой и переизданием команды; бесконечно Running. |
| [AIFollowTargetSettings](classes/AIFollowTargetSettings.md) | struct | Навигационные действия | да | `src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm:21` | авто |  |
| [AIMoveToAction](classes/AIMoveToAction.md) | class | Навигационные действия | да | `src/Gameplay/AI/Navigation/AIMoveToAction.cppm:35` | авто |  |
| [AIMoveToActionBinding](classes/AIMoveToActionBinding.md) | class | Навигационные действия | да | `src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm:138` | ручной | Binding MoveTo: запрос у провайдера → обычный или бронирующий runtime. |
| [AIMoveToActionRequest](classes/AIMoveToActionRequest.md) | struct | Навигационные действия | да | `src/Gameplay/AI/Navigation/AIMoveToAction.cppm:25` | авто |  |
| [ApplyGameplayAgentObstacleAvoidance](classes/ApplyGameplayAgentObstacleAvoidance.md) | free function | Навигационные действия | да | `src/Gameplay/AI/Navigation/GameplayAgentObstacleAvoidance.cppm:10` | авто |  |
| [IAIActionReservationTargetProvider](classes/IAIActionReservationTargetProvider.md) | class | Навигационные действия | да | `src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm:29` | авто |  |
| [IAIMoveToActionRequestProvider](classes/IAIMoveToActionRequestProvider.md) | class | Навигационные действия | да | `src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm:20` | авто |  |
| [ReservedAIMoveToActionRuntime](classes/ReservedAIMoveToActionRuntime.md) | class | Навигационные действия | да | `src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm:37` | ручной | MoveTo с бронью InteractionPoint цели на время шага. |
| [GameplayObjectReservationSystem](classes/GameplayObjectReservationSystem.md) | class | Ресурсы и события | да | `src/Gameplay/Interaction/GameplayObjectReservationSystem.cppm:12` | ручной | Необязательные брони объектов с InteractionPoint агентами AI. |
| [GameplayPickupSystem](classes/GameplayPickupSystem.md) | class | Ресурсы и события | да | `src/Gameplay/Pickup/GameplayPickupSystem.cppm:14` | авто |  |
| [GameplayWorldEvent](classes/GameplayWorldEvent.md) | struct | Ресурсы и события | да | `src/Gameplay/Events/GameplayWorldEvent.cppm:21` | авто |  |
| [GameplayWorldEventType](classes/GameplayWorldEventType.md) | enum | Ресурсы и события | да | `src/Gameplay/Events/GameplayWorldEvent.cppm:14` | авто |  |
| [AIDefinitionMetadata](classes/AIDefinitionMetadata.md) | struct | Решение (instance) | да | `src/Gameplay/AI/GOAP/GameplayGOAPDefinitionContracts.cppm:23` | авто |  |
| [AINamedAction](classes/AINamedAction.md) | struct | Решение (instance) | да | `src/Gameplay/AI/GOAP/GameplayGOAPDefinitionContracts.cppm:16` | авто |  |
| [CreateGameplayGOAPDecision](classes/CreateGameplayGOAPDecision.md) | free function | Решение (instance) | да | `src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm:177` | авто |  |
| [DecisionWithPathInspection](classes/DecisionWithPathInspection.md) | class | Решение (instance) | нет | `src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm:149` | авто |  |
| [GameplayAIDecisionCreationContext](classes/GameplayAIDecisionCreationContext.md) | struct | Решение (instance) | да | `src/Gameplay/AI/Runtime/GameplayAIDecisionCreationContext.cppm:21` | авто |  |
| [GameplayAIDecisionFactoryRegistry](classes/GameplayAIDecisionFactoryRegistry.md) | class | Решение (instance) | да | `src/Gameplay/AI/Runtime/GameplayAIDecision.cppm:23` | авто |  |
| [GameplayAIDecisionInstance](classes/GameplayAIDecisionInstance.md) | class | Решение (instance) | да | `src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm:31` | авто |  |
| [GameplayAIDecisionStatus](classes/GameplayAIDecisionStatus.md) | enum | Решение (instance) | да | `src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm:15` | авто |  |
| [GameplayAIObservationContext](classes/GameplayAIObservationContext.md) | struct | Решение (instance) | да | `src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm:24` | авто |  |
| [GameplayGOAPActionBindingSetup](classes/GameplayGOAPActionBindingSetup.md) | struct | Решение (instance) | да | `src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm:43` | авто |  |
| [GameplayGOAPDecision](classes/GameplayGOAPDecision.md) | class | Решение (instance) | да | `src/Gameplay/AI/GOAP/GameplayGOAPDecision.cppm:22` | ручной | GOAP-обёртка: AIDecisionRuntime + неизменяемое определение + факты агента + bindings. |
| [GameplayGOAPDecisionDefinition](classes/GameplayGOAPDecisionDefinition.md) | struct | Решение (instance) | да | `src/Gameplay/AI/GOAP/GameplayGOAPDefinitionContracts.cppm:31` | авто |  |
| [GameplayGOAPDecisionInstance](classes/GameplayGOAPDecisionInstance.md) | class | Решение (instance) | да | `src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm:18` | ручной | Instance решения одного агента: контекст наблюдений, реакции, буферы событий и GOAP-обёртка. |
| [GameplayGOAPDecisionSetup](classes/GameplayGOAPDecisionSetup.md) | struct | Решение (instance) | да | `src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm:52` | авто |  |
| [IGameplayGOAPContext](classes/IGameplayGOAPContext.md) | class | Решение (instance) | да | `src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm:18` | авто |  |
| [IGameplayGOAPEventReaction](classes/IGameplayGOAPEventReaction.md) | class | Решение (instance) | да | `src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm:35` | авто |  |
| [AIActionDefinition](classes/AIActionDefinition.md) | struct | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:76` | авто |  |
| [AIAgentWorldState](classes/AIAgentWorldState.md) | class | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:42` | ручной | Вектор знания агента: 128 bool (bitset) и 128 int32; единица учёта состояний в поиске. |
| [AIAgentWorldStateHash](classes/AIAgentWorldStateHash.md) | struct | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:142` | авто |  |
| [AIFactCondition](classes/AIFactCondition.md) | struct | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:24` | авто |  |
| [AIFactEffect](classes/AIFactEffect.md) | struct | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:32` | авто |  |
| [AIGoalDefinition](classes/AIGoalDefinition.md) | struct | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:70` | авто |  |
| [AIGoalId](classes/AIGoalId.md) | alias | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:22` | авто |  |
| [AIGoalIdTag](classes/AIGoalIdTag.md) | struct | Ядро GOAP | нет | `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:16` | авто |  |
| [AIGoalScoreRule](classes/AIGoalScoreRule.md) | struct | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIGoalSelectionContracts.cppm:11` | авто |  |
| [AIGoalSelectionCandidate](classes/AIGoalSelectionCandidate.md) | struct | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIGoalSelectionContracts.cppm:17` | авто |  |
| [AIGoalSelectionResult](classes/AIGoalSelectionResult.md) | struct | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIGoalSelection.cppm:16` | авто |  |
| [AINumericCondition](classes/AINumericCondition.md) | struct | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:50` | авто |  |
| [AINumericConditionOperator](classes/AINumericConditionOperator.md) | enum | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:40` | авто |  |
| [AINumericEffect](classes/AINumericEffect.md) | struct | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:63` | авто |  |
| [AINumericEffectOperation](classes/AINumericEffectOperation.md) | enum | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:57` | авто |  |
| [AIPlan](classes/AIPlan.md) | struct | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:99` | авто |  |
| [AIPlanStep](classes/AIPlanStep.md) | struct | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:91` | авто |  |
| [AIWorldFactId](classes/AIWorldFactId.md) | struct | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:14` | авто |  |
| [AIWorldIntegerFactId](classes/AIWorldIntegerFactId.md) | struct | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:28` | авто |  |
| [ApplyFactEffects](classes/ApplyFactEffects.md) | free function | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIStateOperations.cppm:36` | авто |  |
| [ApplyNumericEffects](classes/ApplyNumericEffects.md) | free function | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIStateOperations.cppm:99` | авто |  |
| [AreFactConditionsSatisfied](classes/AreFactConditionsSatisfied.md) | free function | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIStateOperations.cppm:21` | авто |  |
| [AreNumericConditionsSatisfied](classes/AreNumericConditionsSatisfied.md) | free function | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIStateOperations.cppm:78` | авто |  |
| [EvaluateFactCondition](classes/EvaluateFactCondition.md) | free function | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIStateOperations.cppm:14` | авто |  |
| [EvaluateNumericCondition](classes/EvaluateNumericCondition.md) | free function | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIStateOperations.cppm:46` | авто |  |
| [FindAIPlan](classes/FindAIPlan.md) | free function | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIPlanner.cppm:141` | ручной | Uniform-cost search плана от наблюдаемого состояния к цели; без эвристики A*. |
| [ScoreAIGoal](classes/ScoreAIGoal.md) | free function | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIGoalSelection.cppm:27` | авто |  |
| [SearchNode](classes/SearchNode.md) | struct | Ядро GOAP | нет | `src/Gameplay/AI/GOAP/AIPlanner.cppm:117` | авто |  |
| [SearchNodeGreater](classes/SearchNodeGreater.md) | struct | Ядро GOAP | нет | `src/Gameplay/AI/GOAP/AIPlanner.cppm:125` | авто |  |
| [SelectAIGoal](classes/SelectAIGoal.md) | free function | Ядро GOAP | да | `src/Gameplay/AI/GOAP/AIGoalSelection.cppm:44` | ручной | Выбор невыполненной цели с максимальной оценкой; при равенстве — первая. |

</div>

## Покрытие автоматического индекса

Сущностей: 256 (из них с ручным разбором: 36).

??? note "Просканированные файлы"

    - `src/App/GameplayPhysicsObstacleQuery.h`
    - `src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm`
    - `src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponentAssets.cppm`
    - `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponentAssets.cppm`
    - `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm`
    - `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponentAssets.cppm`
    - `src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponents.cppm`
    - `src/Gameplay/AI/Composition/GameplayAIBuiltinDecisions.cppm`
    - `src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm`
    - `src/Gameplay/AI/Composition/GameplayGOAPBuiltinComponents.cppm`
    - `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm`
    - `src/Gameplay/AI/Composition/GameplayGOAPDecisionTemplate.cppm`
    - `src/Gameplay/AI/Composition/GameplayGOAPObservationOrder.cppm`
    - `src/Gameplay/AI/GOAP/AIAgentWorldState.cppm`
    - `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm`
    - `src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm`
    - `src/Gameplay/AI/GOAP/AIGoalSelection.cppm`
    - `src/Gameplay/AI/GOAP/AIGoalSelectionContracts.cppm`
    - `src/Gameplay/AI/GOAP/AIPlanExecution.cppm`
    - `src/Gameplay/AI/GOAP/AIPlanExecutionBridge.cppm`
    - `src/Gameplay/AI/GOAP/AIPlanner.cppm`
    - `src/Gameplay/AI/GOAP/AIStateOperations.cppm`
    - `src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm`
    - `src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm`
    - `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm`
    - `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm`
    - `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm`
    - `src/Gameplay/AI/GOAP/Debug/GameplayGOAPInspection.cppm`
    - `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm`
    - `src/Gameplay/AI/GOAP/GameplayGOAPDecision.cppm`
    - `src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm`
    - `src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm`
    - `src/Gameplay/AI/GOAP/GameplayGOAPDefinitionContracts.cppm`
    - `src/Gameplay/AI/Navigation/AIFleeTargetAction.cppm`
    - `src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm`
    - `src/Gameplay/AI/Navigation/AIFollowRouteAction.cppm`
    - `src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm`
    - `src/Gameplay/AI/Navigation/AIFollowTargetAction.cppm`
    - `src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm`
    - `src/Gameplay/AI/Navigation/AIMoveToAction.cppm`
    - `src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm`
    - `src/Gameplay/AI/Navigation/GameplayAgentObstacleAvoidance.cppm`
    - `src/Gameplay/AI/Runtime/AIActionBinding.cppm`
    - `src/Gameplay/AI/Runtime/AIActionContracts.cppm`
    - `src/Gameplay/AI/Runtime/AIActionRuntime.cppm`
    - `src/Gameplay/AI/Runtime/AIActionTask.cppm`
    - `src/Gameplay/AI/Runtime/AISystem.cppm`
    - `src/Gameplay/AI/Runtime/GameplayAIComponents.cppm`
    - `src/Gameplay/AI/Runtime/GameplayAIDecision.cppm`
    - `src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm`
    - `src/Gameplay/AI/Runtime/GameplayAIDecisionCreationContext.cppm`
    - `src/Gameplay/Events/GameplayWorldEvent.cppm`
    - `src/Gameplay/Interaction/GameplayObjectReservationSystem.cppm`
    - `src/Gameplay/Navigation/GameplayNavigationDebug.cppm`
    - `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm`
    - `src/Gameplay/Navigation/GameplayRequest.cppm`
    - `src/Gameplay/Navigation/GameplayRoute.cppm`
    - `src/Gameplay/Navigation/GameplayRouteFollower.cppm`
    - `src/Gameplay/Navigation/GameplayRouteSearch.cppm`
    - `src/Gameplay/Navigation/GameplaySteering.cppm`
    - `src/Gameplay/Navigation/GameplaySteeringDebug.cppm`
    - `src/Gameplay/Pickup/GameplayPickupSystem.cppm`
    - `src/Gameplay/Traversal/DoorTraversalExecutor.cppm`
    - `src/Gameplay/Traversal/GameplayTraversalExecutor.cppm`
    - `src/Gameplay/Traversal/GameplayTraversalExecutorRegistry.cppm`
    - `src/Gameplay/Traversal/GameplayTraversalLink.cppm`
    - `src/Gameplay/Traversal/GameplayTraversalLinkRegistry.cppm`
    - `src/Gameplay/Traversal/JumpTraversalExecutor.cppm`
    - `src/Navigation/Navigation.cppm`
