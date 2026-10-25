# GameplayGOAPCompositionRegistry

**class** · домен «Композиция» · `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:139–242` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Реестр типов компонентов: parser (JSON → T) + compiler (T + контекст → объект) для наблюдений, capabilities и реакций.

## Архитектурный разбор

**Владелец и время жизни.** Значение; копия хранится в каждом template.

**Runtime-методы, входы и выходы.** RegisterObservation/Capability/Reaction<T>, SemanticActions (тип capability = имя действия), AssetParsers.

**Ошибки, terminal states, отмена.** Регистрация отвергается при пустом типе, повторе или повторном actionId.

**Ссылки.** [теория: 06-assets-composition](../../chapters/06-assets-composition.md) · [CO-REGISTRY](../source-references.md#co-registry) · [CO-REG-CAP](../source-references.md#co-reg-cap) · [CO-SEMANTIC](../source-references.md#co-semantic) · тест `TestGameplayGOAPDecisionTemplate.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 143 | метод | public | `[[nodiscard]] bool RegisterObservation(std::string_view type,` |
| 162 | метод | public | `[[nodiscard]] bool RegisterReaction(std::string_view type,` |
| 180 | метод | public | `[[nodiscard]] const GameplayGOAPReactionCompiler* Reaction(std::string_view type) const` |
| 187 | метод | public | `[[nodiscard]] bool RegisterCapability(std::string_view type, AIActionId actionId,` |
| 213 | метод | public | `[[nodiscard]] const GameplayAIComponentParsers& AssetParsers() const noexcept { return parsers_; }` |
| 215 | метод | public | `[[nodiscard]] const GameplayGOAPObservationCompiler* Observation(std::string_view type) const` |
| 221 | метод | public | `[[nodiscard]] const GameplayGOAPCapabilityRegistration* Capability(std::string_view type) const` |
| 227 | метод | public | `[[nodiscard]] std::vector<GameplayGOAPSemanticAction> SemanticActions() const` |
| 238 | поле | private | `GameplayAIComponentParsers parsers_;` |
| 239 | поле | private | `std::map<std::string, GameplayGOAPObservationCompiler, std::less<>> observations_;` |
| 240 | поле | private | `std::map<std::string, GameplayGOAPReactionCompiler, std::less<>> reactions_;` |
| 241 | поле | private | `std::map<std::string, GameplayGOAPCapabilityRegistration, std::less<>> capabilities_;` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:139–199`.

```cpp linenums="139"
    class GameplayGOAPCompositionRegistry
    {
    public:
        template<class T>
        [[nodiscard]] bool RegisterObservation(std::string_view type,
            std::function<T(const GameplayAIComponentParseContext&)> parse,
            GameplayGOAPObservationCompiler compiler)
        {
            if (type.empty() || !parse || !compiler || observations_.contains(type))
            {
                return false;
            }
            auto parsers = parsers_;
            if (!parsers.Register<T>(GameplayAIComponentKind::Observation, type, std::move(parse)))
            {
                return false;
            }
            observations_.emplace(std::string(type), std::move(compiler));
            parsers_ = std::move(parsers);
            return true;
        }

        template<class T>
        [[nodiscard]] bool RegisterReaction(std::string_view type,
            std::function<T(const GameplayAIComponentParseContext&)> parse,
            GameplayGOAPReactionCompiler compiler)
        {
            if (type.empty() || !parse || !compiler || reactions_.contains(type))
            {
                return false;
            }
            auto parsers = parsers_;
            if (!parsers.Register<T>(GameplayAIComponentKind::Reaction, type, std::move(parse)))
            {
                return false;
            }
            reactions_.emplace(std::string(type), std::move(compiler));
            parsers_ = std::move(parsers);
            return true;
        }

        [[nodiscard]] const GameplayGOAPReactionCompiler* Reaction(std::string_view type) const
        {
            const auto found = reactions_.find(type);
            return found == reactions_.end() ? nullptr : &found->second;
        }

        template<class T>
        [[nodiscard]] bool RegisterCapability(std::string_view type, AIActionId actionId,
            std::function<T(const GameplayAIComponentParseContext&)> parse,
            GameplayGOAPCapabilityCompiler compiler)
        {
            if (type.empty() || !actionId.IsValid() || !parse || !compiler || capabilities_.contains(type))
            {
                return false;
            }
            for (const auto& [name, entry] : capabilities_)
            {
                if (entry.actionId == actionId)
                {
                    return false;
```
