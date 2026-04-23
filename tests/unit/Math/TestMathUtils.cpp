#include <gtest/gtest.h>

#include "MathTestHelper.h"
#include <cmath>

using namespace MathTestHelper;

TEST(MathUtils, Vec2)
{
	Vec2 vector2{ 1.0f, 2.0f };
	EXPECT_FLOAT_EQ(vector2.x, 1.0f);
	EXPECT_FLOAT_EQ(vector2.y, 2.0f);
}

TEST(MathUtils, Vec3)
{
	Vec3 vector3{ 1.0f, 2.0f, 3.0f };
	EXPECT_FLOAT_EQ(vector3.x, 1.0f);
	EXPECT_FLOAT_EQ(vector3.y, 2.0f);
	EXPECT_FLOAT_EQ(vector3.z, 3.0f);
}

TEST(MathUtils, Vec4)
{
	Vec4 vector4{ 1.0f, 2.0f, 3.0f, 1.0f };
	EXPECT_FLOAT_EQ(vector4.x, 1.0f);
	EXPECT_FLOAT_EQ(vector4.y, 2.0f);
	EXPECT_FLOAT_EQ(vector4.z, 3.0f);
	EXPECT_FLOAT_EQ(vector4.w, 1.0f);
}

TEST(MathUtils, Vec2Operations)
{
	Vec2 vector2_1{ 1.0f, 2.0f };
	Vec2 vector2_2{ 3.0f, 4.0f };

	const float value = 2.0f;

	Vec2 added = vector2_1 + vector2_2;
	EXPECT_FLOAT_EQ(added.x, 4.0f);
	EXPECT_FLOAT_EQ(added.y, 6.0f);

	Vec2 subtracted = vector2_1 - vector2_2;
	EXPECT_FLOAT_EQ(subtracted.x, -2.0f);
	EXPECT_FLOAT_EQ(subtracted.y, -2.0f);

	Vec2 multiplied = vector2_1 * value;
	EXPECT_FLOAT_EQ(multiplied.x, 2.0f);
	EXPECT_FLOAT_EQ(multiplied.y, 4.0f);

	Vec2 multiplied2 = value * vector2_1;
	EXPECT_FLOAT_EQ(multiplied2.x, 2.0f);
	EXPECT_FLOAT_EQ(multiplied2.y, 4.0f);

	Vec2 divided = vector2_1 / value;
	EXPECT_FLOAT_EQ(divided.x, 0.5f);
	EXPECT_FLOAT_EQ(divided.y, 1.0f);

	float dotProduct = Dot(vector2_1, vector2_2);
	EXPECT_FLOAT_EQ(dotProduct, 11.0f);

	Vec3 crossProduct2 = Cross2(vector2_1, vector2_2);
	EXPECT_FLOAT_EQ(crossProduct2.z, -2.0f);
}

TEST(MathUtils, Vec3Operations)
{
	Vec3 vector3_1{ 1.0f, 2.0f, 3.0f };
	Vec3 vector3_2{ 4.0f, 5.0f, 6.0f };

	const float value = 2.0f;

	Vec3 added = vector3_1 + vector3_2;
	EXPECT_FLOAT_EQ(added.x, 5.0f);
	EXPECT_FLOAT_EQ(added.y, 7.0f);
	EXPECT_FLOAT_EQ(added.z, 9.0f);

	Vec3 subtracted = vector3_1 - vector3_2;
	EXPECT_FLOAT_EQ(subtracted.x, -3.0f);
	EXPECT_FLOAT_EQ(subtracted.y, -3.0f);
	EXPECT_FLOAT_EQ(subtracted.z, -3.0f);

	Vec3 multiplied = vector3_1 * value;
	EXPECT_FLOAT_EQ(multiplied.x, 2.0f);
	EXPECT_FLOAT_EQ(multiplied.y, 4.0f);
	EXPECT_FLOAT_EQ(multiplied.z, 6.0f);

	Vec3 multiplied2 = value * vector3_1;
	EXPECT_FLOAT_EQ(multiplied2.x, 2.0f);
	EXPECT_FLOAT_EQ(multiplied2.y, 4.0f);
	EXPECT_FLOAT_EQ(multiplied2.z, 6.0f);

	Vec3 divided = vector3_1 / value;
	EXPECT_FLOAT_EQ(divided.x, 0.5f);
	EXPECT_FLOAT_EQ(divided.y, 1.0f);
	EXPECT_FLOAT_EQ(divided.z, 1.5f);

	float dotProduct = Dot(vector3_1, vector3_2);
	EXPECT_FLOAT_EQ(dotProduct, 32.0f);

	Vec3 crossProduct = Cross(vector3_1, vector3_2);
	EXPECT_FLOAT_EQ(crossProduct.x, -3.0f);
	EXPECT_FLOAT_EQ(crossProduct.y, 6.0f);
	EXPECT_FLOAT_EQ(crossProduct.z, -3.0f);
}

TEST(MathUtils, Vec3CrossProductProducesOrthogonalVector)
{
	Vec3 vector3_1{ 1.0f, 0.0f, 0.0f };
	Vec3 vector3_2{ 0.0f, 1.0f, 0.0f };

	Vec3 crossProduct = Cross(vector3_1, vector3_2);
	EXPECT_FLOAT_EQ(crossProduct.x, 0.0f);
	EXPECT_FLOAT_EQ(crossProduct.y, 0.0f);
	EXPECT_FLOAT_EQ(crossProduct.z, 1.0f);

	float dotProduct = Dot(crossProduct, vector3_1);
	EXPECT_FLOAT_EQ(dotProduct, 0.0f);
	dotProduct = Dot(crossProduct, vector3_2);
	EXPECT_FLOAT_EQ(dotProduct, 0.0f);
}

TEST(MathUtils, Normalize)
{
	Vec3 vector3{ 3.0f, 4.0f, 0.0f };
	float length = Length(vector3);
	EXPECT_FLOAT_EQ(length, 5.0f);

	Vec3 normalized = Normalize(vector3);
	length = Length(normalized);
	EXPECT_NEAR(length, 1.0f, 1e-5f);

	Vec3 vector_zero{ 0.0f, 0.0f, 0.0f };
	Vec3 normalized_zero = Normalize(vector_zero);
	EXPECT_FLOAT_EQ(normalized_zero.x, 0.0f);
	EXPECT_FLOAT_EQ(normalized_zero.y, 0.0f);
	EXPECT_FLOAT_EQ(normalized_zero.z, 0.0f);
}

TEST(MathUtils, DegRad)
{
	float degrees = 180.0f;
	float radians = DegToRad(degrees);
	EXPECT_NEAR(radians, Pi, kEpsTrig);

	float converted_back = RadToDeg(radians);
	EXPECT_NEAR(converted_back, degrees, kEpsMat);

	degrees = 90.0f;
	radians = DegToRad(degrees);
	EXPECT_NEAR(radians, Pi / 2.0f, kEpsTrig);
	converted_back = RadToDeg(radians);
	EXPECT_FLOAT_EQ(converted_back, degrees);

	degrees = 45.0f;
	radians = DegToRad(degrees);
	EXPECT_NEAR(radians, Pi / 4.0f, kEpsTrig);
	converted_back = RadToDeg(radians);
	EXPECT_FLOAT_EQ(converted_back, degrees);
}

TEST(MathUtils, Mat4Identity)
{
	Mat4 identity;
	ExpectIdentityNear(identity);
}

TEST(MathUtils, Mat4Transpose)
{
	Mat4 matrix{};

	float counter = 1.0f;
	for (int col = 0; col < 4; ++col)
	{
		for (int row = 0; row < 4; ++row)
		{
			matrix[col][row] = counter++;
		}
	}

	Mat4 transposed = Transpose(matrix);
	for (int col = 0; col < 4; ++col)
	{
		for (int row = 0; row < 4; ++row)
		{
			EXPECT_FLOAT_EQ(transposed[col][row], matrix[row][col]);
		}
	}
}

TEST(MathUtils, Mat4Mul)
{
	Mat4 matrixA{};
	Mat4 matrixB{};

	float counter = 1.0f;
	for (int col = 0; col < 4; ++col)
	{
		for (int row = 0; row < 4; ++row)
		{
			matrixA[col][row] = counter++;
			matrixB[col][row] = counter++;
		}
	}

	Mat4 multiplied = matrixA * matrixB;
	for (int col = 0; col < 4; ++col)
	{
		for (int row = 0; row < 4; ++row)
		{
			float expected_value = 0.0f;
			for (int k = 0; k < 4; ++k)
			{
				expected_value += matrixA[k][row] * matrixB[col][k];
			}
			EXPECT_FLOAT_EQ(multiplied[col][row], expected_value);
		}
	}
}

TEST(MathUtils, Mat4Vec4Mul)
{
	Mat4 matrix{};
	Vec4 vector{ 1.0f, 2.0f, 3.0f, 1.0f };
	float counter = 1.0f;
	for (int col = 0; col < 4; ++col)
	{
		for (int row = 0; row < 4; ++row)
		{
			matrix[col][row] = counter++;
		}
	}
	Vec4 result = matrix * vector;
	for (int row = 0; row < 4; ++row)
	{
		float expected_value = 0.0f;
		for (int k = 0; k < 4; ++k)
		{
			expected_value += matrix[k][row] * vector[k];
		}
		EXPECT_FLOAT_EQ(result[row], expected_value);
	}
}

TEST(MathUtils, Mat4Inverse)
{
	Mat4 matrix(1.0f);
	matrix.columns[0] = Vec4(1.0f, 0.0f, 0.0f, 0.0f);
	matrix.columns[1] = Vec4(1.0f, 1.0f, 0.0f, 0.0f);
	matrix.columns[2] = Vec4(0.0f, 1.0f, 1.0f, 0.0f);
	matrix.columns[3] = Vec4(0.0f, 0.0f, 1.0f, 1.0f);

	Mat4 invTest(1.0f);
	invTest.columns[0] = Vec4(1.0f, 0.0f, 0.0f, 0.0f);
	invTest.columns[1] = Vec4(-1.0f, 1.0f, 0.0f, 0.0f);
	invTest.columns[2] = Vec4(1.0f, -1.0f, 1.0f, 0.0f);
	invTest.columns[3] = Vec4(-1.0f, 1.0f, -1.0f, 1.0f);

	Mat4 inv = Inverse(matrix);

	ExpectMat4Near(inv, invTest);
	ExpectIdentityNear(matrix * inv);
	ExpectIdentityNear(inv * matrix);
}

TEST(MathUtils, LookAtRHFacingNegativeZMatchesIdentity)
{
	const Mat4 view = LookAtRH(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(0.0f, 1.0f, 0.0f));
	ExpectIdentityNear(view);
}

TEST(MathUtils, TransformPointAppliesTranslationButTransformVectorDoesNot)
{
	Mat4 transform(1.0f);
	transform = Translate(transform, Vec3(3.0f, -2.0f, 5.0f));

	ExpectVec3Near(TransformPoint(transform, Vec3(1.0f, 2.0f, 3.0f)), Vec3(4.0f, 0.0f, 8.0f));
	ExpectVec3Near(TransformVector(transform, Vec3(1.0f, 2.0f, 3.0f)), Vec3(1.0f, 2.0f, 3.0f));
}

TEST(MathUtils, PerspectiveRHZOProducesExpectedMatrixTerms)
{
	const float fovY = DegToRad(90.0f);
	const float aspect = 2.0f;
	const float nearZ = 0.1f;
	const float farZ = 100.0f;

	const Mat4 proj = PerspectiveRH_ZO(fovY, aspect, nearZ, farZ);
	EXPECT_NEAR(proj[0][0], 0.5f, 1e-5f);
	EXPECT_NEAR(proj[1][1], 1.0f, 1e-5f);
	EXPECT_NEAR(proj[2][3], -1.0f, 1e-5f);
	EXPECT_NEAR(proj[3][2], -(farZ * nearZ) / (farZ - nearZ), 1e-5f);
}


TEST(MathUtils, GeometryRayDefaultsToForwardZ)
{
	// Verifies the public Ray default contract used by picking paths:
	// origin at world zero and forward direction on +Z.
	const geometry::Ray ray{};
	ExpectVec3Near(ray.origin, Vec3(0.0f, 0.0f, 0.0f));
	ExpectVec3Near(ray.dir, Vec3(0.0f, 0.0f, 1.0f));
}

TEST(MathUtils, FrustumSphereIntersectionTreatsTangentAsHit)
{
	// Checks sphere/frustum boundary behavior:
	// tangent contact is a hit, exact boundary point is a hit, and just outside is a miss.
	Frustum frustum{};
	frustum.planes[static_cast<int>(FrustumPlane::Left)] = Plane{ Vec3(1.0f, 0.0f, 0.0f), 0.0f };

	EXPECT_TRUE(IntersectsSphere(frustum, Vec3(-1.0f, 0.0f, 0.0f), 1.0f));
	EXPECT_TRUE(IntersectsSphere(frustum, Vec3(0.0f, 0.0f, 0.0f), 0.0f));
	EXPECT_FALSE(IntersectsSphere(frustum, Vec3(-1.001f, 0.0f, 0.0f), 1.0f));
}

TEST(MathUtils, NormalizePlaneDegenerateInputProducesZeroPlane)
{
	// Degenerate plane normal should return the zero plane and produce zero signed distance
	// for any point, which documents current graceful-fallback behavior.
	const Plane plane = NormalizePlane(Vec4(0.0f, 0.0f, 0.0f, 42.0f));
	ExpectVec3Near(plane.norm, Vec3(0.0f, 0.0f, 0.0f));
	EXPECT_FLOAT_EQ(plane.dist, 0.0f);
	EXPECT_FLOAT_EQ(Distance(plane, Vec3(123.0f, -77.0f, 5.0f)), 0.0f);
}

TEST(MathUtils, TransformVectorWithNonUniformAndZeroScaleHandlesDegenerates)
{
	// Verifies transform-vector semantics under scaling edge cases:
	// non-uniform scaling stretches each axis independently, and zero-scale collapses that axis.
	const Mat4 nonUniform = Scale(Mat4(1.0f), Vec3(2.0f, 3.0f, 4.0f));
	ExpectVec3Near(TransformVector(nonUniform, Vec3(1.0f, -2.0f, 0.5f)), Vec3(2.0f, -6.0f, 2.0f));

	const Mat4 zeroScaleY = Scale(Mat4(1.0f), Vec3(2.0f, 0.0f, 4.0f));
	ExpectVec3Near(TransformVector(zeroScaleY, Vec3(1.0f, -2.0f, 0.5f)), Vec3(2.0f, 0.0f, 2.0f));
}

TEST(MathUtils, RotateWithZeroAxisReturnsDegenerateLinearPart)
{
	// Documents current degenerate rotation behavior:
	// rotating around a zero axis collapses the linear part for this 90-degree case.
	const Mat4 rotated = Rotate(Mat4(1.0f), DegToRad(90.0f), Vec3(0.0f, 0.0f, 0.0f));
	ExpectVec3Near(TransformVector(rotated, Vec3(1.0f, 2.0f, 3.0f)), Vec3(0.0f, 0.0f, 0.0f));
	ExpectVec3Near(TransformPoint(rotated, Vec3(1.0f, 2.0f, 3.0f)), Vec3(0.0f, 0.0f, 0.0f));
}

TEST(MathUtils, LookAtRHWithParallelUpAndForwardProducesFiniteValues)
{
	// Ensures LookAtRH remains numerically stable for parallel up/forward inputs:
	// no NaN/Inf values and deterministic transformed-vector outputs.
	const Mat4 view = LookAtRH(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));

	for (int col = 0; col < 4; ++col)
	{
		for (int row = 0; row < 4; ++row)
		{
			EXPECT_TRUE(std::isfinite(view[col][row]));
		}
	}

	ExpectVec3Near(TransformVector(view, Vec3(1.0f, 0.0f, 0.0f)), Vec3(0.0f, 0.0f, 0.0f));
	EXPECT_NEAR(TransformVector(view, Vec3(0.0f, 1.0f, 0.0f)).z, -1.0f, kEpsVec);
}
