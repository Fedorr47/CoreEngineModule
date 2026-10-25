# GameplayGOAPAuthoredGoal

**struct** · домен «Авторинг» · `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm:34–38` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Авторинг».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 35 | поле | public | `std::string name{};` |
| 36 | поле | public | `float score{};` |
| 37 | поле | public | `std::vector<GameplayGOAPAuthoredBoolean> facts{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm:34–38`.

```cpp linenums="34"
  struct GameplayGOAPAuthoredGoal {
    std::string name{};
    float score{};
    std::vector<GameplayGOAPAuthoredBoolean> facts{};
  };
```
