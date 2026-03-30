module;

#include <cstdint>

export module core:debug_color_utils;

export namespace rendern::debugColor
{
    [[nodiscard]] constexpr std::uint32_t PackRGBA8(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) noexcept
    {
        return static_cast<std::uint32_t>(r)
            | (static_cast<std::uint32_t>(g) << 8)
            | (static_cast<std::uint32_t>(b) << 16)
            | (static_cast<std::uint32_t>(a) << 24);
    }
}
