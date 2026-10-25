# GameplayGOAPDecision

**class** · домен «Решение (instance)» · `src/Gameplay/AI/GOAP/GameplayGOAPDecision.cppm:22–166` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> GOAP-обёртка: AIDecisionRuntime + неизменяемое определение + факты агента + bindings.

## Архитектурный разбор

**Владелец и время жизни.** Член GameplayGOAPDecisionInstance по значению; владеет bindings.

**Runtime-методы, входы и выходы.** InstallActionBinding, HasCompleteActionBindings, Update, Cancel, BuildDebugViewModel (имена из metadata).

**Ссылки.** [теория: 08-decision-runtime](../../chapters/08-decision-runtime.md) · [GG-DECISION](../source-references.md#gg-decision) · тест `TestGameplayAIDecision.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 25 | конструктор | public | `GameplayGOAPDecision(const EntityHandle agent,` |
| 27 | метод | public | `: decision_(agent), definition_(std::move(definition))` |
| 31 | метод | public | `[[nodiscard]] bool InstallActionBinding(const AIActionId actionId,` |
| 42 | метод | public | `[[nodiscard]] bool HasCompleteActionBindings() const noexcept` |
| 54 | метод | public | `void Update(AISystem& aiSystem, const GameplayWorld& world)` |
| 60 | метод | public | `void Cancel(AISystem& aiSystem) noexcept` |
| 65 | метод | public | `[[nodiscard]] AIPlanExecutionStatus GetStatus() const noexcept` |
| 70 | метод | public | `[[nodiscard]] const AIAgentWorldState& GetObservedState() const noexcept` |
| 75 | метод | public | `[[nodiscard]] AIAgentWorldState& GetObservedState() noexcept` |
| 80 | метод | public | `[[nodiscard]] AIDebugViewModel BuildDebugViewModel(` |
| 88 | метод | public | `[[nodiscard]] AIDebugViewModel BuildDebugViewModel() const` |
| 161 | поле | private | `AIDecisionRuntime decision_;` |
| 162 | поле | private | `const GameplayGOAPDecisionDefinition definition_;` |
| 163 | поле | private | `AIAgentWorldState facts_{};` |
| 164 | поле | private | `AIActionBindingRegistry bindings_{};` |
| 165 | поле | private | `std::vector<std::unique_ptr<IAIActionBinding>> ownedBindings_{};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/GameplayGOAPDecision.cppm:22–82`.

```cpp linenums="22"
    class GameplayGOAPDecision
    {
    public:
        GameplayGOAPDecision(const EntityHandle agent,
            GameplayGOAPDecisionDefinition definition)
            : decision_(agent), definition_(std::move(definition))
        {
        }

        [[nodiscard]] bool InstallActionBinding(const AIActionId actionId,
            std::unique_ptr<IAIActionBinding> binding)
        {
            if (!binding || !bindings_.Register(actionId, *binding))
            {
                return false;
            }
            ownedBindings_.push_back(std::move(binding));
            return true;
        }

        [[nodiscard]] bool HasCompleteActionBindings() const noexcept
        {
            for (const AIActionDefinition& action : definition_.actions)
            {
                if (action.actionId.IsValid() && !bindings_.Contains(action.actionId))
                {
                    return false;
                }
            }
            return true;
        }

        void Update(AISystem& aiSystem, const GameplayWorld& world)
        {
            (void)decision_.Update(facts_, definition_.goals, definition_.actions,
                bindings_, aiSystem, world);
        }

        void Cancel(AISystem& aiSystem) noexcept
        {
            decision_.Cancel(aiSystem);
        }

        [[nodiscard]] AIPlanExecutionStatus GetStatus() const noexcept
        {
            return decision_.GetStatus();
        }

        [[nodiscard]] const AIAgentWorldState& GetObservedState() const noexcept
        {
            return facts_;
        }

        [[nodiscard]] AIAgentWorldState& GetObservedState() noexcept
        {
            return facts_;
        }

        [[nodiscard]] AIDebugViewModel BuildDebugViewModel(
            const std::span<const AIWorldFactId> booleanFactIds,
            const std::span<const AIWorldIntegerFactId> integerFactIds) const
```
