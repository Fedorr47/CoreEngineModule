# GameplayGOAPDecisionTemplate

**class** · домен «Композиция» · `src/Gameplay/AI/Composition/GameplayGOAPDecisionTemplate.cppm:31–154` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Неизменяемый снимок конфигурации решения, независимый от мира и агента.

## Архитектурный разбор

**Владелец и время жизни.** shared_ptr<const> захвачен фабрикой каталога; разделяется всеми instance.

**Runtime-методы, входы и выходы.** Конструктор (model, routeGraph, компиляция, типы, покрытие контекстов), InstanceDefinition(overrides), аксессоры.

**Кто вызывает и в какой фазе.** RegisterGameplayAIDecisionAssets (создание), CreateGameplayGOAPDecisionFromTemplate (использование).

**Ошибки, terminal states, отмена.** Исключение с префиксом «AI behavior <source>» при любой ошибке валидации.

**Ссылки.** [теория: 06-assets-composition](../../chapters/06-assets-composition.md) · [схема d06-template-instance](../diagrams.md#d06-template-instance) · [схема d06-composition-classes](../diagrams.md#d06-composition-classes) · [CO-TEMPLATE](../source-references.md#co-template) · [CO-INSTANCE-DEF](../source-references.md#co-instance-def) · тест `TestGameplayGOAPDecisionTemplate.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 34 | конструктор | public | `GameplayGOAPDecisionTemplate(GameplayAIBehaviorAsset behavior, GameplayAILevelBindingsAsset bindings,` |
| 37 | метод | public | `: behavior_(std::move(behavior)), bindings_(std::move(bindings)), graph_(std::move(graph)),` |
| 38 | метод | public | `components_(std::move(components)), definitionSource_(definition.source)` |
| 96 | метод | public | `[[nodiscard]] const GameplayAIBehaviorAsset& Behavior() const noexcept { return behavior_; }` |
| 97 | метод | public | `[[nodiscard]] const GameplayAILevelBindingsAsset& Bindings() const noexcept { return bindings_; }` |
| 98 | метод | public | `[[nodiscard]] const GameplayAIRouteGraphAsset* RouteGraph() const noexcept { return graph_ ? &*graph_ : nullptr; }` |
| 99 | метод | public | `[[nodiscard]] const GameplayGOAPCompiledDefinition& Compiled() const noexcept { return compiled_; }` |
| 100 | метод | public | `[[nodiscard]] const GameplayGOAPCompositionRegistry& Components() const noexcept { return components_; }` |
| 101 | метод | public | `[[nodiscard]] const std::vector<GameplayGOAPPreparedCapabilityGroup>& Capabilities() const noexcept { return capabilities_; }` |
| 105 | метод | public | `[[nodiscard]] GameplayGOAPDecisionDefinition InstanceDefinition(` |
| 147 | поле | private | `GameplayAIBehaviorAsset behavior_;` |
| 148 | поле | private | `GameplayAILevelBindingsAsset bindings_;` |
| 149 | поле | private | `std::optional<GameplayAIRouteGraphAsset> graph_;` |
| 150 | поле | private | `GameplayGOAPCompositionRegistry components_;` |
| 151 | поле | private | `GameplayGOAPCompiledDefinition compiled_;` |
| 152 | поле | private | `std::string definitionSource_;` |
| 153 | поле | private | `std::vector<GameplayGOAPPreparedCapabilityGroup> capabilities_;` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/Composition/GameplayGOAPDecisionTemplate.cppm:31–91`.

```cpp linenums="31"
    class GameplayGOAPDecisionTemplate
    {
    public:
        GameplayGOAPDecisionTemplate(GameplayAIBehaviorAsset behavior, GameplayAILevelBindingsAsset bindings,
            const GameplayGOAPDefinitionAsset& definition, GameplayGOAPCompositionRegistry components,
            std::optional<GameplayAIRouteGraphAsset> graph = {})
            : behavior_(std::move(behavior)), bindings_(std::move(bindings)), graph_(std::move(graph)),
              components_(std::move(components)), definitionSource_(definition.source)
        {
            if (behavior_.model != "goap")
            {
                Fail_("unknown decision model '" + behavior_.model + "'");
            }
            if (!behavior_.routeGraph.empty() && !graph_)
            {
                Fail_("missing authored route graph input");
            }
            compiled_ = CompileGameplayGOAPDefinition(definition, components_.SemanticActions());
            for (const auto& observer : behavior_.observations)
            {
                if (components_.Observation(observer.type) == nullptr)
                {
                    Fail_("unknown observation type '" + observer.type + "'");
                }
            }
            for (const auto& reaction : behavior_.reactions)
            {
                if (components_.Reaction(reaction.type) == nullptr)
                {
                    Fail_("unknown reaction type '" + reaction.type + "'");
                }
            }
            std::set<std::string> boundContexts;
            std::map<std::string, std::vector<GameplayAICapabilityAsset>> groups;
            for (const auto& asset : behavior_.capabilities)
            {
                if (components_.Capability(asset.type) == nullptr)
                {
                    Fail_("unknown capability '" + asset.type + "'");
                }
                if (!boundContexts.insert(asset.context).second)
                {
                    Fail_("multiple capability bindings for '" + asset.context + "'");
                }
                const auto authored = std::ranges::find(definition.actions, asset.context,
                    &GameplayGOAPAuthoredAction::context);
                if (authored == definition.actions.end() || authored->action != asset.type)
                {
                    Fail_("capability does not match a definition action/context '" + asset.context + "'");
                }
                groups[asset.type].push_back(asset);
            }
            for (const auto& action : definition.actions)
            {
                if (!boundContexts.contains(action.context))
                {
                    Fail_("action context '" + action.context + "' has no capability binding");
                }
            }
            for (auto& [type, assets] : groups)
            {
```
