# GameplayGOAPCompositionContext

**struct** · домен «Композиция» · `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:61–118` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Композиция».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 63 | поле | public | `const GameplayAIDecisionCreationContext& services;` |
| 64 | поле | public | `const GameplayGOAPCompiledDefinition& definition;` |
| 65 | поле | public | `std::span<const GameplayAIResolvedRole> roles;` |
| 66 | поле | public | `std::string_view source;` |
| 67 | поле | public | `std::span<const GameplayAIObservationAsset> observations{};` |
| 68 | поле | public | `const GameplayAIRouteGraphAsset* routeGraph{};` |
| 69 | поле | public | `const GameplayAILevelBindingsAsset* levelBindings{};` |
| 77 | метод | public | `[[nodiscard]] const GameplayAIResolvedRole& Role(std::string_view name) const` |
| 89 | метод | public | `[[nodiscard]] AIWorldIntegerFactId IntegerFact(std::string_view name) const` |
| 99 | метод | public | `[[nodiscard]] const T& Parameters(const TAsset& asset) const` |
| 109 | метод | public | `[[nodiscard]] AIWorldFactId BooleanFact(std::string_view name) const` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:61–118`.

```cpp linenums="61"
    struct GameplayGOAPCompositionContext
    {
        const GameplayAIDecisionCreationContext& services;
        const GameplayGOAPCompiledDefinition& definition;
        std::span<const GameplayAIResolvedRole> roles;
        std::string_view source;
        std::span<const GameplayAIObservationAsset> observations{};
        const GameplayAIRouteGraphAsset* routeGraph{};
        const GameplayAILevelBindingsAsset* levelBindings{};

        [[noreturn]] void Fail(std::string_view component, std::string_view reason) const
        {
            throw std::runtime_error("AI behavior '" + std::string(source) + "', "
                + std::string(component) + ": " + std::string(reason));
        }

        [[nodiscard]] const GameplayAIResolvedRole& Role(std::string_view name) const
        {
            for (const auto& role : roles)
            {
                if (role.role == name)
                {
                    return role;
                }
            }
            Fail("role '" + std::string(name) + "'", "no level binding");
        }

        [[nodiscard]] AIWorldIntegerFactId IntegerFact(std::string_view name) const
        {
            const auto fact = definition.FindIntegerFact(name);
            if (!fact)
            {
                Fail(name, "unknown fact or expected int");
            }
            return *fact;
        }
        template<class T, class TAsset>
        [[nodiscard]] const T& Parameters(const TAsset& asset) const
        {
            const auto* parameters = std::any_cast<T>(&asset.parameters);
            if (parameters == nullptr)
            {
                Fail(asset.type, "incorrect typed parameters");
            }
            return *parameters;
        }

        [[nodiscard]] AIWorldFactId BooleanFact(std::string_view name) const
        {
            const auto fact = definition.FindBooleanFact(name);
            if (!fact)
            {
                Fail("fact '" + std::string(name) + "'", "unknown fact or expected bool");
            }
            return *fact;
        }
    };
```
