module;

#include <array>
#include <cstdint>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numbers>
#include <string_view>

export module core:geometry;

import :math_utils;

export namespace geometry
{
    struct Ray
    {
        mathUtils::Vec3 origin{ 0.0f, 0.0f, 0.0f };
        mathUtils::Vec3 dir{ 0.0f, 0.0f, 1.0f }; // normalized
    };
}
