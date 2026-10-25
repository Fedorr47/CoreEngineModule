# GameplayGOAPAuthoredAction

**struct** · домен «Авторинг» · `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm:39–49` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Авторинг».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 40 | поле | public | `std::string action{};` |
| 41 | поле | public | `std::string context{};` |
| 42 | поле | public | `float cost{};` |
| 43 | поле | public | `std::vector<GameplayGOAPAuthoredBoolean> preconditions{};` |
| 44 | поле | public | `std::vector<GameplayGOAPAuthoredBoolean> effects{};` |
| 45 | поле | public | `std::vector<GameplayGOAPAuthoredNumeric> numericPreconditions{};` |
| 46 | поле | public | `std::vector<GameplayGOAPAuthoredNumeric> numericEffects{};` |
| 47 | поле | public | `std::vector<GameplayGOAPAuthoredBoolean> continuationConditions{};` |
| 48 | поле | public | `std::vector<GameplayGOAPAuthoredNumeric> numericContinuationConditions{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm:39–49`.

```cpp linenums="39"
  struct GameplayGOAPAuthoredAction {
    std::string action{};
    std::string context{};
    float cost{};
    std::vector<GameplayGOAPAuthoredBoolean> preconditions{};
    std::vector<GameplayGOAPAuthoredBoolean> effects{};
    std::vector<GameplayGOAPAuthoredNumeric> numericPreconditions{};
    std::vector<GameplayGOAPAuthoredNumeric> numericEffects{};
    std::vector<GameplayGOAPAuthoredBoolean> continuationConditions{};
    std::vector<GameplayGOAPAuthoredNumeric> numericContinuationConditions{};
  };
```
