# HideOnPurchaseReaction

**class** · домен «Компоненты» · `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:172–210` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IGameplayGOAPEventReaction`

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Компоненты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 175 | конструктор | public | `HideOnPurchaseReaction(const GameplayAIReactionAsset& asset, const GameplayGOAPCompositionContext& context)` |
| 176 | метод | public | `: agent_(context.services.agent)` |
| 187 | метод | public | `void React(const GameplayWorld& world, std::span<const GameplayWorldEvent> events,` |
| 206 | поле | private | `EntityHandle agent_{kNullEntity};` |
| 207 | поле | private | `EntityHandle target_{kNullEntity};` |
| 208 | поле | private | `Transaction transaction_{};` |
| 209 | поле | private | `bool emitted_{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:172–210`.

```cpp linenums="172"
    class HideOnPurchaseReaction final : public IGameplayGOAPEventReaction
    {
    public:
        HideOnPurchaseReaction(const GameplayAIReactionAsset& asset, const GameplayGOAPCompositionContext& context)
            : agent_(context.services.agent)
        {
            const auto& parameters = context.Parameters<GameplayAIHideOnPurchaseAsset>(asset);
            transaction_ = ResolveReceipt(parameters.receipt, context);
            target_ = context.Role(parameters.target).entity;
            const auto* node = context.services.world.TryGetNodeLink(target_);
            if (!context.services.world.IsEntityValid(target_) || node == nullptr || node->nodeIndex < 0)
            {
                context.Fail(asset.type, "visibility target requires a live node-bound entity");
            }
        }
        void React(const GameplayWorld& world, std::span<const GameplayWorldEvent> events,
            const AIAgentWorldState& facts, std::vector<GameplayWorldEvent>& output) override
        {
            if (emitted_ || !world.IsEntityValid(target_) || !facts.IsFactSet(transaction_.result))
            {
                return;
            }
            for (const auto& event : events)
            {
                if (event.type == GameplayWorldEventType::ResourcePurchased && event.instigator == agent_
                    && event.subject == transaction_.target && event.receiptId == transaction_.receiptId)
                {
                    output.push_back({GameplayWorldEventType::HideEntityRequested, agent_, target_});
                    emitted_ = true;
                    return;
                }
            }
        }
    private:
        EntityHandle agent_{kNullEntity};
        EntityHandle target_{kNullEntity};
        Transaction transaction_{};
        bool emitted_{};
    };
```
