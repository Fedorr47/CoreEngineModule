# RegisterGameplayAIDecisionAssets

**function** · домен «Композиция» · `src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:252–252` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Транзакционная регистрация каталога решений: все templates или ничего.

## Архитектурный разбор

**Кто вызывает и в какой фазе.** MakeDefaultGameplayAIDecisionFactories (App при создании GameplayRuntime).

**Ошибки, terminal states, отмена.** Ошибка любой записи → исключение до публикации реестра.

**Ссылки.** [теория: 06-assets-composition](../../chapters/06-assets-composition.md) · [CO-REGISTER-ASSETS](../source-references.md#co-register-assets) · [CO-PENDING](../source-references.md#co-pending) · тест `TestGameplayAIAssetComposition.cpp`

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:252–252`.

```cpp linenums="252"
    void RegisterGameplayAIDecisionAssets(GameplayAIDecisionFactoryRegistry& destination,
```
