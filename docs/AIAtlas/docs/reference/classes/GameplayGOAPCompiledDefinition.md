# GameplayGOAPCompiledDefinition

**class** · домен «Авторинг» · `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm:34–64` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Авторинг».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 36 | поле | public | `GameplayGOAPDecisionDefinition definition{};` |
| 38 | метод | public | `FindBooleanFact(const std::string_view name) const` |
| 44 | метод | public | `FindIntegerFact(const std::string_view name) const` |
| 50 | метод | public | `FindActionContext(const std::string_view name) const` |
| 57 | метод | private | `friend GameplayGOAPCompiledDefinition CompileGameplayGOAPDefinition(` |
| 61 | поле | private | `std::unordered_map<std::string, AIWorldFactId> booleanFacts_{};` |
| 62 | поле | private | `std::unordered_map<std::string, AIWorldIntegerFactId> integerFacts_{};` |
| 63 | поле | private | `std::unordered_map<std::string, AIActionContextId> contexts_{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm:34–64`.

```cpp linenums="34"
class GameplayGOAPCompiledDefinition {
public:
  GameplayGOAPDecisionDefinition definition{};
  [[nodiscard]] std::optional<AIWorldFactId>
  FindBooleanFact(const std::string_view name) const {
    const auto found = booleanFacts_.find(std::string(name));
    return found == booleanFacts_.end() ? std::nullopt
                                        : std::optional{found->second};
  }
  [[nodiscard]] std::optional<AIWorldIntegerFactId>
  FindIntegerFact(const std::string_view name) const {
    const auto found = integerFacts_.find(std::string(name));
    return found == integerFacts_.end() ? std::nullopt
                                        : std::optional{found->second};
  }
  [[nodiscard]] std::optional<AIActionContextId>
  FindActionContext(const std::string_view name) const {
    const auto found = contexts_.find(std::string(name));
    return found == contexts_.end() ? std::nullopt
                                    : std::optional{found->second};
  }

private:
  friend GameplayGOAPCompiledDefinition CompileGameplayGOAPDefinition(
      const GameplayGOAPDefinitionAsset &,
      std::span<const GameplayGOAPSemanticAction>,
      std::span<const GameplayGOAPActionCostOverride>);
  std::unordered_map<std::string, AIWorldFactId> booleanFacts_{};
  std::unordered_map<std::string, AIWorldIntegerFactId> integerFacts_{};
  std::unordered_map<std::string, AIActionContextId> contexts_{};
};
```
