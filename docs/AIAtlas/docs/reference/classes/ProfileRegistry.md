# ProfileRegistry

**class** · домен «NavMesh» · `src/Navigation/Navigation.cppm:158–177` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «NavMesh».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 161 | конструктор | public | `ProfileRegistry();` |
| 162 | конструктор | public | `~ProfileRegistry();` |
| 163 | конструктор | public | `ProfileRegistry(ProfileRegistry&&) noexcept;` |
| 164 | поле | public | `ProfileRegistry& operator=(ProfileRegistry&&) noexcept;` |
| 165 | конструктор | public | `ProfileRegistry(const ProfileRegistry&) = delete;` |
| 166 | поле | public | `ProfileRegistry& operator=(const ProfileRegistry&) = delete;` |
| 168 | метод | public | `[[nodiscard]] ProfileResolution Initialize(const Geometry&, const BuildSettings& = {});` |
| 169 | метод | public | `[[nodiscard]] ProfileResolution ResolveProfile(const AgentSettings&);` |
| 170 | метод | public | `[[nodiscard]] const World* TryGetWorld(ProfileHandle) const noexcept;` |
| 171 | метод | public | `[[nodiscard]] std::size_t GetProfileCount() const noexcept;` |
| 172 | метод | public | `void Reset() noexcept;` |
| 176 | поле | private | `std::unique_ptr<Impl> impl_;` |

## Объявление

Включено из исходника при сборке; `src/Navigation/Navigation.cppm:158–177`.

```cpp linenums="158"
    class ProfileRegistry final
    {
    public:
        ProfileRegistry();
        ~ProfileRegistry();
        ProfileRegistry(ProfileRegistry&&) noexcept;
        ProfileRegistry& operator=(ProfileRegistry&&) noexcept;
        ProfileRegistry(const ProfileRegistry&) = delete;
        ProfileRegistry& operator=(const ProfileRegistry&) = delete;

        [[nodiscard]] ProfileResolution Initialize(const Geometry&, const BuildSettings& = {});
        [[nodiscard]] ProfileResolution ResolveProfile(const AgentSettings&);
        [[nodiscard]] const World* TryGetWorld(ProfileHandle) const noexcept;
        [[nodiscard]] std::size_t GetProfileCount() const noexcept;
        void Reset() noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
```
