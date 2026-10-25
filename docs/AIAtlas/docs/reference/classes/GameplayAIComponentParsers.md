# GameplayAIComponentParsers

**class** · домен «Авторинг» · `src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm:183–222` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Авторинг».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 187 | метод | public | `[[nodiscard]] bool Register(GameplayAIComponentKind kind, std::string_view type,` |
| 201 | метод | public | `[[nodiscard]] std::any Parse(GameplayAIComponentKind kind, std::string_view type,` |
| 221 | поле | private | `std::map<GameplayAIComponentKind, std::map<std::string, Parser, std::less<>>> parsers_;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm:183–222`.

```cpp linenums="183"
    class GameplayAIComponentParsers
    {
    public:
        template<class T>
        [[nodiscard]] bool Register(GameplayAIComponentKind kind, std::string_view type,
            std::function<T(const GameplayAIComponentParseContext&)> parse)
        {
            if (type.empty() || !parse)
            {
                return false;
            }
            return parsers_[kind].emplace(std::string(type),
                [parse = std::move(parse)](const GameplayAIComponentParseContext& context) -> std::any
                {
                    return parse(context);
                }).second;
        }

        [[nodiscard]] std::any Parse(GameplayAIComponentKind kind, std::string_view type,
            const GameplayAIComponentParseContext& context) const
        {
            const auto category = parsers_.find(kind);
            if (category != parsers_.end())
            {
                const auto found = category->second.find(type);
                if (found != category->second.end())
                {
                    return found->second(context);
                }
            }
            const std::string categoryName = kind == GameplayAIComponentKind::Observation ? "observation"
                : kind == GameplayAIComponentKind::Capability ? "capability" : "reaction";
            ai_asset_detail::Invalid(context.source, context.location,
                "unknown " + categoryName + " type '" + std::string(type) + "'");
        }

    private:
        using Parser = std::function<std::any(const GameplayAIComponentParseContext&)>;
        std::map<GameplayAIComponentKind, std::map<std::string, Parser, std::less<>>> parsers_;
    };
```
