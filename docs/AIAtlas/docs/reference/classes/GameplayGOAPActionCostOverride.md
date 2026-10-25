# GameplayGOAPActionCostOverride

**struct** · домен «Авторинг» · `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm:28–32` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Авторинг».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 29 | поле | public | `std::string action{};` |
| 30 | поле | public | `std::string context{};` |
| 31 | поле | public | `float cost{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm:28–32`.

```cpp linenums="28"
struct GameplayGOAPActionCostOverride {
  std::string action{};
  std::string context{};
  float cost{};
};
```
