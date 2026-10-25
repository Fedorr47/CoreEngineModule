# World

**class** · домен «NavMesh» · `src/Navigation/Navigation.cppm:113–137` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> NavMesh одного профиля агента: bake Recast и запросы Detour.

## Архитектурный разбор

**Владелец и время жизни.** ProfileRegistry (по одному на профиль), принадлежит App.

**Runtime-методы, входы и выходы.** Build, ProjectPoint, FindPath (частичный коридор → NoPath), BuildDebugGeometry.

**Ссылки.** [теория: 11-spatial-routes](../../chapters/11-spatial-routes.md) · [схема d11-maze-sequence](../diagrams.md#d11-maze-sequence) · [NV-FINDPATH](../source-references.md#nv-findpath) · [NV-PARTIAL](../source-references.md#nv-partial) · [NV-CONFIG](../source-references.md#nv-config)

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 116 | конструктор | public | `World();` |
| 117 | конструктор | public | `~World();` |
| 119 | конструктор | public | `World(World&&) noexcept;` |
| 120 | поле | public | `World& operator=(World&&) noexcept;` |
| 122 | конструктор | public | `World(const World&) = delete;` |
| 123 | поле | public | `World& operator=(const World&) = delete;` |
| 125 | метод | public | `[[nodiscard]] BuildStatus Build(const Geometry&, const BuildSettings& = {});` |
| 126 | метод | public | `void Reset() noexcept;` |
| 127 | метод | public | `[[nodiscard]] bool IsInitialized() const noexcept;` |
| 128 | метод | public | `[[nodiscard]] ProjectionResult ProjectPoint(` |
| 131 | метод | public | `[[nodiscard]] PathResult FindPath(const PathRequest&) const;` |
| 132 | метод | public | `[[nodiscard]] DebugGeometry BuildDebugGeometry() const;` |
| 136 | поле | private | `std::unique_ptr<Impl> impl_;` |

## Объявление

Включено из исходника при сборке; `src/Navigation/Navigation.cppm:113–137`.

```cpp linenums="113"
    class World final
    {
    public:
        World();
        ~World();

        World(World&&) noexcept;
        World& operator=(World&&) noexcept;

        World(const World&) = delete;
        World& operator=(const World&) = delete;

        [[nodiscard]] BuildStatus Build(const Geometry&, const BuildSettings& = {});
        void Reset() noexcept;
        [[nodiscard]] bool IsInitialized() const noexcept;
        [[nodiscard]] ProjectionResult ProjectPoint(
            const mathUtils::Vec3&,
            const mathUtils::Vec3& searchExtents) const;
        [[nodiscard]] PathResult FindPath(const PathRequest&) const;
        [[nodiscard]] DebugGeometry BuildDebugGeometry() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
```
