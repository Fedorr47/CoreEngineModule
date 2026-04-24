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
    struct MovingSphereCcdInput
    {
        mathUtils::Vec3 centerA{ 0.0f, 0.0f, 0.0f };
        mathUtils::Vec3 centerB{ 0.0f, 0.0f, 0.0f };
        mathUtils::Vec3 velocityA{ 0.0f, 0.0f, 0.0f };
        mathUtils::Vec3 velocityB{ 0.0f, 0.0f, 0.0f };
        float deltaTime{ 0.0f };
        float radiusA{ 0.5f };
        float radiusB{ 0.5f };
    };

    struct MovingSphereCcdResult
    {
        bool hit{ false };
        bool startsOverlapped{ false };
        bool tangent{ false };
        float time01{ -1.0f };
        float discriminant{ 0.0f };
        mathUtils::Vec3 centerAAtHit{ 0.0f, 0.0f, 0.0f };
        mathUtils::Vec3 centerBAtHit{ 0.0f, 0.0f, 0.0f };
        mathUtils::Vec3 normalFromBToA{ 1.0f, 0.0f, 0.0f };
    };

    [[nodiscard]] MovingSphereCcdResult SolveMovingSphereSphereCcd(const MovingSphereCcdInput& input) noexcept
    {
        MovingSphereCcdResult result{};

        const float radiusSum = std::max(0.0f, input.radiusA) + std::max(0.0f, input.radiusB);
        const mathUtils::Vec3 m = input.centerA - input.centerB;
        const mathUtils::Vec3 dRel = (input.velocityA - input.velocityB) * input.deltaTime;

        const float a = mathUtils::Dot(dRel, dRel);
        const float b = 2.0f * mathUtils::Dot(m, dRel);
        const float c = mathUtils::Dot(m, m) - radiusSum * radiusSum;

        constexpr float kEpsilon = 1e-6f;
        if (c <= kEpsilon)
        {
            result.hit = true;
            result.startsOverlapped = true;
            result.time01 = 0.0f;
            result.discriminant = 0.0f;
            result.centerAAtHit = input.centerA;
            result.centerBAtHit = input.centerB;

            const float mLen = mathUtils::Length(m);
            if (mLen > kEpsilon)
            {
                result.normalFromBToA = m / mLen;
            }
            else
            {
                const mathUtils::Vec3 fallback = input.velocityA - input.velocityB;
                const float fallbackLen = mathUtils::Length(fallback);
                result.normalFromBToA = (fallbackLen > kEpsilon)
                    ? (fallback / fallbackLen)
                    : mathUtils::Vec3(1.0f, 0.0f, 0.0f);
            }
            return result;
        }

        if (a <= kEpsilon)
        {
            result.hit = false;
            result.time01 = -1.0f;
            result.discriminant = -1.0f;
            return result;
        }

        float discriminant = b * b - 4.0f * a * c;
        result.discriminant = discriminant;
        if (discriminant < -kEpsilon)
        {
            result.hit = false;
            result.time01 = -1.0f;
            return result;
        }

        discriminant = std::max(0.0f, discriminant);
        result.discriminant = discriminant;
        result.tangent = std::abs(discriminant) <= kEpsilon;

        const float sqrtDisc = std::sqrt(discriminant);
        const float inv2A = 0.5f / a;
        const float t0 = (-b - sqrtDisc) * inv2A;
        const float t1 = (-b + sqrtDisc) * inv2A;

        float earliest = std::numeric_limits<float>::max();
        if (t0 >= -kEpsilon && t0 <= 1.0f + kEpsilon)
        {
            earliest = std::min(earliest, t0);
        }
        if (t1 >= -kEpsilon && t1 <= 1.0f + kEpsilon)
        {
            earliest = std::min(earliest, t1);
        }

        if (!std::isfinite(earliest) || earliest == std::numeric_limits<float>::max())
        {
            result.hit = false;
            result.time01 = -1.0f;
            return result;
        }

        result.hit = true;
        result.time01 = std::clamp(earliest, 0.0f, 1.0f);
        result.centerAAtHit = input.centerA + input.velocityA * (input.deltaTime * result.time01);
        result.centerBAtHit = input.centerB + input.velocityB * (input.deltaTime * result.time01);

        const mathUtils::Vec3 deltaAtHit = result.centerAAtHit - result.centerBAtHit;
        const float deltaLen = mathUtils::Length(deltaAtHit);
        result.normalFromBToA = (deltaLen > kEpsilon)
            ? (deltaAtHit / deltaLen)
            : mathUtils::Vec3(1.0f, 0.0f, 0.0f);

        return result;
    }

    struct Ray
    {
        mathUtils::Vec3 origin{ 0.0f, 0.0f, 0.0f };
        mathUtils::Vec3 dir{ 0.0f, 0.0f, 1.0f }; // normalized
    };
}
