# GameplayRequestStatus

**enum** · домен «Диагностика» · `src/Gameplay/Navigation/GameplayRequest.cppm:54–61` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRequest.cppm:54–61`.

```cpp linenums="54"
    enum class GameplayRequestStatus : std::uint8_t
    {
        Pending,
        Running,
        Succeeded,
        Failed,
        Cancelled
    };
```
