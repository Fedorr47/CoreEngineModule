# PurchaseCapability

**class** · домен «Компоненты» · `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:283–333` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IGameplayGOAPCapability`

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Компоненты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 286 | конструктор | public | `PurchaseCapability(std::span<const GameplayAICapabilityAsset> assets, const GameplayGOAPCompositionContext& context)` |
| 287 | метод | public | `: world_(context.services.world)` |
| 325 | метод | public | `std::unique_ptr<IAIActionBinding> CreateBinding(AIAgentWorldState& facts,` |
| 331 | поле | private | `const GameplayWorld& world_;` |
| 332 | поле | private | `std::vector<PurchaseContext> contexts_;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:283–333`.

```cpp linenums="283"
    class PurchaseCapability final : public IGameplayGOAPCapability
    {
    public:
        PurchaseCapability(std::span<const GameplayAICapabilityAsset> assets, const GameplayGOAPCompositionContext& context)
            : world_(context.services.world)
        {
            for (const auto& asset : assets)
            {
                const auto& parameters = context.Parameters<GameplayAIPurchaseAsset>(asset);
                if (!(parameters.radius > 0) || !std::isfinite(parameters.radius * parameters.radius))
                {
                    context.Fail(asset.context, "purchase radius must be positive and finite");
                }
                const auto transaction = ResolveReceipt(parameters.receipt, context);
                const auto id = context.definition.FindActionContext(asset.context);
                if (!id)
                {
                    context.Fail(asset.context, "unknown purchase context");
                }
                const auto action = std::ranges::find(context.definition.definition.actions, *id, &AIActionDefinition::contextId);
                if (action == context.definition.definition.actions.end()
                    || action->effects.size() != 1 || action->effects.front().factId != transaction.result
                    || !action->effects.front().bValue || action->numericEffects.size() != 1
                    || action->numericEffects.front().factId != transaction.resource
                    || action->numericEffects.front().operation != AINumericEffectOperation::Add
                    || action->numericEffects.front().value != -transaction.price)
                {
                    context.Fail(asset.context, "purchase definition effects do not match the receipt/price");
                }
                const bool fundsGuard = std::ranges::any_of(action->numericPreconditions, [&](const auto& condition)
                {
                    return condition.factId == transaction.resource
                        && condition.comparison == AINumericConditionOperator::GreaterOrEqual
                        && condition.value == transaction.price;
                });
                if (!fundsGuard)
                {
                    context.Fail(asset.context, "purchase requires a matching resource >= price precondition");
                }
                contexts_.push_back({*id, transaction, parameters.radius});
            }
        }
        std::unique_ptr<IAIActionBinding> CreateBinding(AIAgentWorldState& facts,
            std::vector<GameplayWorldEvent>& events) override
        {
            return std::make_unique<PurchaseBinding>(facts, world_, events, contexts_);
        }
    private:
        const GameplayWorld& world_;
        std::vector<PurchaseContext> contexts_;
    };
```
