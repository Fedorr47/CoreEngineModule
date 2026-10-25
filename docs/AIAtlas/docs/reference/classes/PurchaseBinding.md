# PurchaseBinding

**class** · домен «Компоненты» · `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:256–281` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IAIActionBinding`

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Компоненты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 259 | конструктор | public | `PurchaseBinding(AIAgentWorldState& facts, const GameplayWorld& world,` |
| 261 | метод | public | `: facts_(facts), world_(world), events_(events), contexts_(std::move(contexts)) {}` |
| 262 | метод | public | `std::unique_ptr<IAIActionRuntime> CreateRuntime(const AIActionRuntimeContext& context) override` |
| 277 | поле | private | `AIAgentWorldState& facts_;` |
| 278 | поле | private | `const GameplayWorld& world_;` |
| 279 | поле | private | `std::vector<GameplayWorldEvent>& events_;` |
| 280 | поле | private | `std::vector<PurchaseContext> contexts_;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:256–281`.

```cpp linenums="256"
    class PurchaseBinding final : public IAIActionBinding
    {
    public:
        PurchaseBinding(AIAgentWorldState& facts, const GameplayWorld& world,
            std::vector<GameplayWorldEvent>& events, std::vector<PurchaseContext> contexts)
            : facts_(facts), world_(world), events_(events), contexts_(std::move(contexts)) {}
        std::unique_ptr<IAIActionRuntime> CreateRuntime(const AIActionRuntimeContext& context) override
        {
            if (context.actionId == kAIPurchaseActionId)
            {
                for (const auto& entry : contexts_)
                {
                    if (entry.id == context.contextId)
                    {
                        return std::make_unique<PurchaseRuntime>(facts_, world_, events_, entry.transaction, entry.radius);
                    }
                }
            }
            return nullptr;
        }
    private:
        AIAgentWorldState& facts_;
        const GameplayWorld& world_;
        std::vector<GameplayWorldEvent>& events_;
        std::vector<PurchaseContext> contexts_;
    };
```
