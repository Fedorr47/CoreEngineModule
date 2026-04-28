#pragma once

import core;
using namespace mathUtils;

namespace MathTestHelper
{
    constexpr float kEpsVec = 1e-5f;
    constexpr float kEpsMat = 1e-4f;
    constexpr float kEpsTrig = 1e-5f;

    inline void ExpectVec3Near(const Vec3& a, const Vec3& b, float eps = kEpsVec)
    {
        EXPECT_NEAR(a.x, b.x, eps) << "Vec3.x";
        EXPECT_NEAR(a.y, b.y, eps) << "Vec3.y";
        EXPECT_NEAR(a.z, b.z, eps) << "Vec3.z";
    }

    inline void ExpectVec4Near(const Vec4& a, const Vec4& b, float eps = kEpsVec)
    {
        EXPECT_NEAR(a.x, b.x, eps) << "Vec4.x";
        EXPECT_NEAR(a.y, b.y, eps) << "Vec4.y";
        EXPECT_NEAR(a.z, b.z, eps) << "Vec4.z";
        EXPECT_NEAR(a.w, b.w, eps) << "Vec4.w";
    }

    inline void ExpectMat4Near(const Mat4& a, const Mat4& b, float eps = kEpsMat)
    {
        for (int col = 0; col < 4; ++col)
        {
            for (int row = 0; row < 4; ++row)
            {
                EXPECT_NEAR(a[col][row], b[col][row], eps)
                    << "Mat4 mismatch at [col=" << col << "][row=" << row << "]";
            }
        }
    }

    inline void ExpectIdentityNear(const Mat4& m, float eps = kEpsMat)
    {
        for (int col = 0; col < 4; ++col)
        {
            for (int row = 0; row < 4; ++row)
            {
                const float expected = (col == row) ? 1.0f : 0.0f;
                EXPECT_NEAR(m[col][row], expected, eps)
                    << "Not identity at [col=" << col << "][row=" << row << "]";
            }
        }
    }
}