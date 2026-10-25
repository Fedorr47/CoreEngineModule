# CreateGameplayGOAPDecisionFromTemplate

**function** · домен «Композиция» · `src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:149–149` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Связывание template с уровнем и создание instance: роли, наблюдения, реакции, providers, cost overrides.

## Архитектурный разбор

**Кто вызывает и в какой фазе.** Фабрика каталога из GameplayRuntime::StartAIDecision.

**Ошибки, terminal states, отмена.** Исключение → фабрика возвращает nullptr и пишет diagnostic.

**Ссылки.** [теория: 06-assets-composition](../../chapters/06-assets-composition.md) · [схема d06-template-instance](../diagrams.md#d06-template-instance) · [CO-FROM-TEMPLATE](../source-references.md#co-from-template) · [CO-RESOLVE-ROLES](../source-references.md#co-resolve-roles)

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:149–149`.

```cpp linenums="149"
    [[nodiscard]] std::unique_ptr<GameplayAIDecisionInstance> CreateGameplayGOAPDecisionFromTemplate(
```
