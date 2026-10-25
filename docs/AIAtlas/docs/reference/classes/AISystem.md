# AISystem

**class** · домен «Action runtime» · `src/Gameplay/AI/Runtime/AISystem.cppm:17–161` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Владелец задач действий: не более одной задачи на агента; синхронный старт и покадровый тик.

## Архитектурный разбор

**Владелец и время жизни.** Член GameplayRuntime по значению; хранит unordered_map<EntityHandle, AIActionTask>.

**Runtime-методы, входы и выходы.** StartAction (отменяет и заменяет прежнюю задачу, вызывает Start), Update(world, dt) (очистка задач мёртвых агентов, Tick Running), CancelAction, ClearAction, Reset, GetActionStatus.

**Кто вызывает и в какой фазе.** Мост решений (старт/статус); GameplayRuntime::PrePhysicsUpdate (Update после решений); прямые StartAI*.

**Ошибки, terminal states, отмена.** Terminal-задача остаётся до замены или смерти агента — её статус читают решения.

**Ссылки.** [теория: 09-execution-lifetime](../../chapters/09-execution-lifetime.md) · [теория: 10-frame-order](../../chapters/10-frame-order.md) · [схема d09-action-lifecycle](../diagrams.md#d09-action-lifecycle) · [схема d09-task-states](../diagrams.md#d09-task-states) · [AS-CLASS](../source-references.md#as-class) · [AS-START](../source-references.md#as-start) · [AS-UPDATE](../source-references.md#as-update) · [AS-CLEANUP](../source-references.md#as-cleanup) · тест `TestAISystem.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 22 | метод | public | `std::size_t Update(GameplayWorld& world, const float deltaSeconds = 0.0f)` |
| 50 | метод | public | `[[nodiscard]] AIActionExecutionStatus StartAction(` |
| 74 | метод | public | `void CancelAction(const EntityHandle agentEntity) noexcept` |
| 87 | метод | public | `void ClearAction(const EntityHandle agentEntity) noexcept` |
| 93 | метод | public | `void Reset() noexcept` |
| 107 | метод | public | `[[nodiscard]] AIActionExecutionStatus GetActionStatus(const EntityHandle agentEntity) const noexcept` |
| 117 | метод | public | `[[nodiscard]] AIActionExecutionStatus GetActionStatus(` |
| 130 | метод | public | `[[nodiscard]] bool HasActiveAction(const EntityHandle agentEntity) const noexcept` |
| 138 | метод | private | `void CleanupInactiveTasks_(GameplayWorld& world)` |
| 159 | поле | private | `std::vector<EntityHandle> aiEntitiesScratch_{};` |
| 160 | поле | private | `std::unordered_map<EntityHandle, AIActionTask> tasksByEntity_{};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/Runtime/AISystem.cppm:17–77`.

```cpp linenums="17"
    class AISystem
    {
    public:
        // Runs the synchronous AI update boundary, advances active actions, and
        // returns the number of agents that remained valid for processing.
        std::size_t Update(GameplayWorld& world, const float deltaSeconds = 0.0f)
        {
            world.CollectAIEntities(aiEntitiesScratch_);
            CleanupInactiveTasks_(world);

            for (const EntityHandle entity : aiEntitiesScratch_)
            {
                auto taskIt = tasksByEntity_.find(entity);
                if (taskIt == tasksByEntity_.end())
                {
                    continue;
                }

                AIActionTask& task = taskIt->second;
                if (task.GetStatus() == AIActionExecutionStatus::NotStarted)
                {
                    [[maybe_unused]] const AIActionExecutionStatus status = task.Start();
                }
                if (task.IsRunning())
                {
                    [[maybe_unused]] const AIActionExecutionStatus status =
                        task.Tick(deltaSeconds);
                }
            }

            return aiEntitiesScratch_.size();
        }

        [[nodiscard]] AIActionExecutionStatus StartAction(
            const GameplayWorld& world,
            const AIActionRuntimeContext context,
            std::unique_ptr<IAIActionRuntime> runtime)
        {
            const bool bHasValidContext = context.IsValid();
            const bool bHasRuntime = runtime != nullptr;
            const bool bHasValidEntity = bHasValidContext && world.IsEntityValid(context.agentEntity);
            const bool bIsAIAgent = bHasValidEntity && world.HasAI(context.agentEntity);

            if (!bHasValidContext || !bHasRuntime || !bHasValidEntity || !bIsAIAgent)
            {
                return AIActionExecutionStatus::Failed;
            }

            CancelAction(context.agentEntity);
            tasksByEntity_.erase(context.agentEntity);
            auto taskEntry = tasksByEntity_.emplace(
                context.agentEntity,
                AIActionTask(context, std::move(runtime)));

            return taskEntry.first->second.Start();
        }

        void CancelAction(const EntityHandle agentEntity) noexcept
        {
            auto taskIt = tasksByEntity_.find(agentEntity);
            if (taskIt == tasksByEntity_.end())
```
