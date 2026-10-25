# AIAgentWorldStateHash

**struct** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:142–181` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ядро GOAP».

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:142–181`.

```cpp linenums="142"
    struct AIAgentWorldStateHash
    {
        [[nodiscard]] std::size_t operator()(
            const AIAgentWorldState& state) const noexcept
        {
            // FNV-1a over every logical fact keeps hashing independent of the
            // bitset representation and covers the complete state identity.
            std::size_t hash = sizeof(std::size_t) == 8u
                ? static_cast<std::size_t>(14695981039346656037ull)
                : static_cast<std::size_t>(2166136261u);
            constexpr std::size_t prime = sizeof(std::size_t) == 8u
                ? static_cast<std::size_t>(1099511628211ull)
                : static_cast<std::size_t>(16777619u);

            for (std::size_t index = 0u; index < AIAgentWorldState::FactCapacity; ++index)
            {
                hash ^= state.IsFactSet(
                    AIWorldFactId{ static_cast<AIWorldFactId::ValueType>(index) })
                    ? 1u
                    : 0u;
                hash *= prime;
            }

            for (std::size_t index = 0u;
                index < AIAgentWorldState::IntegerFactCapacity;
                ++index)
            {
                const std::uint32_t value = static_cast<std::uint32_t>(
                    state.GetIntegerFact(AIWorldIntegerFactId{
                        static_cast<AIWorldIntegerFactId::ValueType>(index) }));
                for (std::size_t byteIndex = 0u; byteIndex < sizeof(value); ++byteIndex)
                {
                    hash ^= (value >> (byteIndex * 8u)) & 0xffu;
                    hash *= prime;
                }
            }

            return hash;
        }
    };
```
