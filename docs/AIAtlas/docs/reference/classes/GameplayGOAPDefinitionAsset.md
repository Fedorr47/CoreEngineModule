# GameplayGOAPDefinitionAsset

**struct** · домен «Авторинг» · `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm:50–56` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Авторинг».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 51 | поле | public | `std::string id{};` |
| 52 | поле | public | `std::string source{};` |
| 53 | поле | public | `std::vector<GameplayGOAPAuthoredFact> facts{};` |
| 54 | поле | public | `std::vector<GameplayGOAPAuthoredGoal> goals{};` |
| 55 | поле | public | `std::vector<GameplayGOAPAuthoredAction> actions{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm:50–56`.

```cpp linenums="50"
  struct GameplayGOAPDefinitionAsset {
    std::string id{};
    std::string source{};
    std::vector<GameplayGOAPAuthoredFact> facts{};
    std::vector<GameplayGOAPAuthoredGoal> goals{};
    std::vector<GameplayGOAPAuthoredAction> actions{};
  };
```
