# GameplayObjectReservationSystem

**class** · домен «Ресурсы и события» · `src/Gameplay/Interaction/GameplayObjectReservationSystem.cppm:12–102` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Необязательные брони объектов с InteractionPoint агентами AI.

## Архитектурный разбор

**Владелец и время жизни.** Член GameplayRuntime; сбрасывается при переходе в Editor.

**Ссылки.** [теория: 17-resources-multiagent](../../chapters/17-resources-multiagent.md) · [схема d17-contention](../diagrams.md#d17-contention) · [RV-SYSTEM](../source-references.md#rv-system) · [RV-TRY](../source-references.md#rv-try) · тест `TestGameplayObjectReservationSystem.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 15 | метод | public | `[[nodiscard]] bool TryReserve(` |
| 38 | метод | public | `[[nodiscard]] bool Release(const EntityHandle objectEntity, const EntityHandle agentEntity) noexcept` |
| 51 | метод | public | `[[nodiscard]] bool IsReserved(const EntityHandle objectEntity) const noexcept` |
| 56 | метод | public | `[[nodiscard]] bool IsReservedBy(` |
| 63 | метод | public | `[[nodiscard]] EntityHandle GetReservationOwner(const EntityHandle objectEntity) const noexcept` |
| 73 | метод | public | `std::size_t CleanupInvalidReservations(const GameplayWorld& world) noexcept` |
| 98 | метод | public | `void Reset() noexcept { ownerByObject_.clear(); }` |
| 101 | поле | private | `std::unordered_map<EntityHandle, EntityHandle> ownerByObject_{};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/Interaction/GameplayObjectReservationSystem.cppm:12–72`.

```cpp linenums="12"
    class GameplayObjectReservationSystem
    {
    public:
        [[nodiscard]] bool TryReserve(
            const GameplayWorld& world,
            const EntityHandle objectEntity,
            const EntityHandle agentEntity)
        {
            const bool bHasValidObject = world.IsEntityValid(objectEntity);
            const bool bHasValidAgent = world.IsEntityValid(agentEntity);
            const bool bIsDifferentEntity = objectEntity != agentEntity;
            const bool bIsAIAgent = bHasValidAgent && world.HasAI(agentEntity);
            const bool bHasInteractionPoint = bHasValidObject && world.HasInteractionPoint(objectEntity);
            if (!bHasValidObject
                || !bHasValidAgent
                || !bIsDifferentEntity
                || !bIsAIAgent
                || !bHasInteractionPoint)
            {
                return false;
            }

            const auto [it, bInserted] = ownerByObject_.try_emplace(objectEntity, agentEntity);
            return bInserted || it->second == agentEntity;
        }

        [[nodiscard]] bool Release(const EntityHandle objectEntity, const EntityHandle agentEntity) noexcept
        {
            const auto it = ownerByObject_.find(objectEntity);
            if (it == ownerByObject_.end()
                || it->second != agentEntity
                || agentEntity == kNullEntity)
            {
                return false;
            }
            ownerByObject_.erase(it);
            return true;
        }

        [[nodiscard]] bool IsReserved(const EntityHandle objectEntity) const noexcept
        {
            return objectEntity != kNullEntity && ownerByObject_.contains(objectEntity);
        }

        [[nodiscard]] bool IsReservedBy(
            const EntityHandle objectEntity,
            const EntityHandle agentEntity) const noexcept
        {
            return agentEntity != kNullEntity && GetReservationOwner(objectEntity) == agentEntity;
        }

        [[nodiscard]] EntityHandle GetReservationOwner(const EntityHandle objectEntity) const noexcept
        {
            if (objectEntity == kNullEntity)
            {
                return kNullEntity;
            }
            const auto it = ownerByObject_.find(objectEntity);
            return it != ownerByObject_.end() ? it->second : kNullEntity;
        }

```
