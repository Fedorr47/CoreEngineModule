# AIAgentWorldState

**class** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:42–140` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Вектор знания агента: 128 bool (bitset) и 128 int32; единица учёта состояний в поиске.

## Архитектурный разбор

**Владелец и время жизни.** Значение. Факты агента принадлежат GameplayGOAPDecision (по одной копии на instance); копии создаёт поиск и проверка остатка плана.

**Основные данные: readers / writers.** Пишут: наблюдения (Observe/ObserveActionEvents), эффекты в копиях поиска. Читают: выбор цели, поиск, PurchaseRuntime, отладка.

**Runtime-методы, входы и выходы.** IsFactSet/SetFact/ClearFact, GetIntegerFact/SetIntegerFact, Clear, operator== (полное равенство). Хеш — AIAgentWorldStateHash (FNV-1a по всем ячейкам).

**Кто вызывает и в какой фазе.** Наблюдения в update решения; FindAIPlan; AIDecisionRuntime::IsRemainingPlanValid_.

**Ошибки, terminal states, отмена.** Невалидный индекс: assert в debug, безопасная деградация (false/0, запись игнорируется).

**Ссылки.** [теория: 03-state-model](../../chapters/03-state-model.md) · [схема d03-state-layout](../diagrams.md#d03-state-layout) · [WS-CLASS](../source-references.md#ws-class) · [WS-HASH](../source-references.md#ws-hash) · [WS-SETFACT](../source-references.md#ws-setfact) · тест `TestAIAgentWorldState.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 45 | поле | public | `static constexpr std::size_t FactCapacity = 128u;` |
| 46 | поле | public | `static constexpr std::size_t IntegerFactCapacity = 128u;` |
| 48 | метод | public | `[[nodiscard]] bool IsFactSet(` |
| 62 | метод | public | `void SetFact(` |
| 77 | метод | public | `void ClearFact(const AIWorldFactId factId) noexcept` |
| 82 | метод | public | `[[nodiscard]] std::int32_t GetIntegerFact(` |
| 96 | метод | public | `void SetIntegerFact(` |
| 111 | метод | public | `void Clear() noexcept` |
| 126 | метод | private | `[[nodiscard]] static constexpr bool IsFactValid_(` |
| 132 | метод | private | `[[nodiscard]] static constexpr bool IsIntegerFactValid_(` |
| 138 | поле | private | `std::bitset<FactCapacity> facts_{};` |
| 139 | поле | private | `std::array<std::int32_t, IntegerFactCapacity> integerFacts_{};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:42–102`.

```cpp linenums="42"
    class AIAgentWorldState
    {
    public:
        static constexpr std::size_t FactCapacity = 128u;
        static constexpr std::size_t IntegerFactCapacity = 128u;

        [[nodiscard]] bool IsFactSet(
            const AIWorldFactId factId) const noexcept
        {
            const bool bIsValidFact = IsFactValid_(factId);
            assert(bIsValidFact);

            if (!bIsValidFact)
            {
                return false;
            }

            return facts_.test(factId.index);
        }

        void SetFact(
            const AIWorldFactId factId,
            const bool bValue = true) noexcept
        {
            const bool bIsValidFact = IsFactValid_(factId);
            assert(bIsValidFact);

            if (!bIsValidFact)
            {
                return;
            }

            facts_.set(factId.index, bValue);
        }

        void ClearFact(const AIWorldFactId factId) noexcept
        {
            SetFact(factId, false);
        }

        [[nodiscard]] std::int32_t GetIntegerFact(
            const AIWorldIntegerFactId factId) const noexcept
        {
            const bool bIsValidFact = IsIntegerFactValid_(factId);
            assert(bIsValidFact);

            if (!bIsValidFact)
            {
                return 0;
            }

            return integerFacts_[factId.index];
        }

        void SetIntegerFact(
            const AIWorldIntegerFactId factId,
            const std::int32_t value) noexcept
        {
            const bool bIsValidFact = IsIntegerFactValid_(factId);
            assert(bIsValidFact);

```
