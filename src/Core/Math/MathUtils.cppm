module;

#include <array>
#include <cstdint>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numbers>
#include <string_view>

export module core:math_utils;

using namespace std::numbers;
using namespace std::string_view_literals;

export namespace mathUtils
{
	// Conventions are intentionally compatible with the previous GLM usage:
	//  - Mat4 is COLUMN-major and stores 4 columns (Vec4).
	//  - Indexing matches GLM: m[col][row].
	//  - Functions (Translate/Rotate/Scale/LookAtRH/PerspectiveRH_ZO/OrthoRH_ZO)
	//    follow the same formulas as GLM.

	inline constexpr float Pi = pi_v<float>;
	inline constexpr float TwoPi = Pi * 2;
	
	constexpr float kLengthEpsilon = 1e-6f;
	constexpr float kLengthEpsilonSq = kLengthEpsilon * kLengthEpsilon;
	constexpr float kMoveEpsilon = 1e-4f;
	constexpr float kMoveEpsilonSq = kMoveEpsilon * kMoveEpsilon;

	struct Vec2
	{
		float x{ 0 }, y{ 0 };

		constexpr Vec2() = default;
		constexpr Vec2(float X, float Y) :
			x(X),
			y(Y) {
		}

		constexpr float& operator[](std::size_t i) noexcept
		{
			return const_cast<float&>(std::as_const(*this)[i]);
		}
		constexpr const float& operator[](std::size_t i) const noexcept
		{
			return (i == 0) ? x : y;
		}

		friend constexpr bool operator==(const Vec2& a, const Vec2& b) noexcept
		{
			return a.x == b.x && a.y == b.y;
		}
	};

	struct Vec3
	{
		float x{ 0 }, y{ 0 }, z{ 0 };

		constexpr Vec3() = default;
		constexpr Vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}

		constexpr float& operator[](std::size_t i) noexcept
		{
			return const_cast<float&>(std::as_const(*this)[i]);
		}
		constexpr const float& operator[](std::size_t i) const noexcept
		{
			return (i == 0) ? x : (i == 1) ? y : z;
		}

		Vec3 operator-() const noexcept { return Vec3(-x, -y, -z); }

		friend constexpr bool operator==(const Vec3& a, const Vec3& b) noexcept
		{
			return a.x == b.x && a.y == b.y && a.z == b.z;
		}
		
		static Vec3 ZeroVector()
		{
			return Vec3(0.0f, 0.0f, 0.0f);
		}
	};

	struct alignas(16) Vec4
	{
		float x{ 0 }, y{ 0 }, z{ 0 }, w{ 0 };

		constexpr Vec4() = default;
		constexpr Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
		constexpr Vec4(const Vec3& v, float w_) : x(v.x), y(v.y), z(v.z), w(w_) {}

		constexpr float& operator[](std::size_t i) noexcept
		{
			return const_cast<float&>(std::as_const(*this)[i]);
		}
		constexpr const float& operator[](std::size_t i) const noexcept
		{
			switch (i)
			{
			case 0: return x;
			case 1: return y;
			case 2: return z;
			default: return w;
			}
		}

		constexpr Vec3 xyz() const noexcept { return { x, y, z }; }

		friend constexpr bool operator==(const Vec4& a, const Vec4& b) noexcept
		{
			return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
		}
	};

	struct Mat4
	{
		// 4 columns
		Vec4 columns[4]{};

		constexpr Mat4() : Mat4(1.0f) {}
		explicit constexpr Mat4(float diag)
			: columns{ Vec4(diag, 0, 0, 0), Vec4(0, diag, 0, 0), Vec4(0, 0, diag, 0), Vec4(0, 0, 0, diag) }
		{
		}

		constexpr Vec4& operator[](std::size_t col) noexcept { return columns[col]; }
		constexpr const Vec4& operator[](std::size_t col) const noexcept { return columns[col]; }

		constexpr float& operator()(std::size_t row, std::size_t col) noexcept
		{
			return const_cast<float&>(std::as_const(*this)(row, col));
		}
		constexpr const float& operator()(std::size_t row, std::size_t col) const noexcept
		{
			return columns[col][row];
		}

		friend constexpr bool operator==(const Mat4& a, const Mat4& b) noexcept
		{
			return a.columns[0] == b.columns[0]
				&& a.columns[1] == b.columns[1]
				&& a.columns[2] == b.columns[2]
				&& a.columns[3] == b.columns[3];
		}
	};

	// --- basic ops ---
	constexpr Vec2 operator+(const Vec2& a, const Vec2& b) noexcept { return Vec2(a.x + b.x, a.y + b.y); }
	constexpr Vec2 operator-(const Vec2& a, const Vec2& b) noexcept { return Vec2(a.x - b.x, a.y - b.y); }
	constexpr Vec2 operator*(const Vec2& v, float s) noexcept { return Vec2(v.x * s, v.y * s); }
	constexpr Vec2 operator*(float s, const Vec2& v) noexcept { return v * s; }
	constexpr Vec2 operator/(const Vec2& v, float s) noexcept { return Vec2(v.x / s, v.y / s); }

	constexpr Vec3 operator+(const Vec3& a, const Vec3& b) noexcept { return Vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
	constexpr Vec3 operator-(const Vec3& a, const Vec3& b) noexcept { return Vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
	constexpr Vec3 operator*(const Vec3& v, float s) noexcept { return Vec3(v.x * s, v.y * s, v.z * s); }
	constexpr Vec3 operator*(float s, const Vec3& v) noexcept { return v * s; }
	constexpr Vec3 operator/(const Vec3& v, float s) noexcept { return Vec3(v.x / s, v.y / s, v.z / s); }

	constexpr Vec3& operator*=(Vec3& v, float s) noexcept {
		v = v * s;
		return v;
	}

	constexpr Vec4 operator+(const Vec4& a, const Vec4& b) noexcept { return Vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
	constexpr Vec4 operator-(const Vec4& a, const Vec4& b) noexcept { return Vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
	constexpr Vec4 operator*(const Vec4& v, float s) noexcept { return Vec4(v.x * s, v.y * s, v.z * s, v.w * s); }
	constexpr Vec4 operator*(float s, const Vec4& v) noexcept { return v * s; }

	[[nodiscard]] inline constexpr float Lerp(float a, float b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	[[nodiscard]] inline constexpr Vec2 Lerp(const Vec2& a, const Vec2& b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	[[nodiscard]] inline constexpr Vec3 Lerp(const Vec3& a, const Vec3& b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	[[nodiscard]] inline constexpr Vec4 Lerp(const Vec4& a, const Vec4& b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	inline float Dot(const Vec2& a, const Vec2& b) noexcept { return a.x * b.x + a.y * b.y; }
	inline float Dot(const Vec3& a, const Vec3& b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z; }
	inline float Dot(const Vec4& a, const Vec4& b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
	inline Vec3 Cross2(const Vec2& a, const Vec2& b) noexcept { return Vec3(0, 0, a.x * b.y - a.y * b.x); }
	inline const float Cross2D(const Vec2& a, const Vec2& b) noexcept { return a.x* b.y - a.y * b.x; }
	inline Vec3 Cross(const Vec3& a, const Vec3& b) noexcept
	{
		return Vec3(
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x);
	}

	inline float Length(const Vec3& v) noexcept { return std::sqrt(Dot(v, v)); }
	
	[[nodiscard]] inline float LengthSquared(const Vec3& value) noexcept
	{
		return Dot(value, value);
	}

	[[nodiscard]] inline float LengthSquared(const Vec4& value) noexcept
	{
		return Dot(value, value);
	}

	inline Vec3 MakeUnitVector(const Vec3& v) noexcept
	{
		const float len = Length(v);
		if (len <= 0.0f)
		{
			return Vec3(0, 0, 0);
		}
		return v / len;
	}

	inline Vec3 Normalize(const Vec3& v) noexcept { return MakeUnitVector(v); }

	[[nodiscard]] inline constexpr float DegToRad(float degrees) noexcept
	{
		return degrees * (Pi / 180.0f);
	}

	[[nodiscard]] inline constexpr float RadToDeg(float radians) noexcept
	{
		return radians * (180.0f / Pi);
	}

	inline Mat4 Transpose(const Mat4& m) noexcept
	{
		Mat4 result(0.0f);
		for (int col = 0; col < 4; ++col)
		{
			for (int row = 0; row < 4; ++row)
			{
				result[row][col] = m[col][row];
			}
		}
		return result;
	}

	inline const float* ValuePtr(const Mat4& m) noexcept
	{
		return &m.columns[0].x;
	}

	// Matrix * vector (column-vector convention): v' = M * v
	inline Vec4 Mul(const Mat4& m, const Vec4& v) noexcept
	{
		return m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w;
	}

	inline Vec4 operator*(const Mat4& m, const Vec4& v) noexcept { return Mul(m, v); }

	inline Mat4 Mul(const Mat4& a, const Mat4& b) noexcept
	{
		Mat4 multipliedMat(0.0f);
		// Each column of result is a * (column of b)
		for (int col = 0; col < 4; ++col)
		{
			multipliedMat[col] = Mul(a, b[col]);
		}
		return multipliedMat;
	}

	// ------------------------------------------------------------
	// Frustum culling helpers (RH, clip-space Z in [0..1])
	// ------------------------------------------------------------
	enum class FrustumPlane : std::uint32_t
	{
		Left = 0,
		Right,
		Bottom,
		Top,
		Near,
		Far
	};

	struct Plane
	{
		Vec3 norm{ 0.0f, 0.0f, 0.0f }; // normalized normal
		float dist{ 0.0f };           // plane: dot(n, x) + d = 0, inside if >= 0
	};

	inline float Distance(const Plane& p, const Vec3& x) noexcept
	{
		return Dot(p.norm, x) + p.dist;
	}

	struct Frustum
	{
		Plane planes[6]{};
	};

	inline Vec4 Row(const Mat4& m, int row) noexcept
	{
		return Vec4(m[0][row], m[1][row], m[2][row], m[3][row]);
	}

	inline Plane NormalizePlane(const Vec4& p) noexcept
	{
		Plane out{};
		const float len = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
		if (len > 0.0f)
		{
			out.norm = Vec3(p.x / len, p.y / len, p.z / len);
			out.dist = p.w / len;
		}
		return out;
	}

	inline Frustum ExtractFrustumRH_ZO(const Mat4& viewProj) noexcept
	{
		// Clip tests in D3D-style clip space (column-vector convention):
		//   -w <= x <= w
		//   -w <= y <= w
		//    0 <= z <= w
		const Vec4 r0 = Row(viewProj, 0);
		const Vec4 r1 = Row(viewProj, 1);
		const Vec4 r2 = Row(viewProj, 2);
		const Vec4 r3 = Row(viewProj, 3);

		Frustum frustum{};
		frustum.planes[static_cast<std::uint32_t>(FrustumPlane::Left)] = NormalizePlane(r3 + r0);
		frustum.planes[static_cast<std::uint32_t>(FrustumPlane::Right)] = NormalizePlane(r3 - r0);
		frustum.planes[static_cast<std::uint32_t>(FrustumPlane::Bottom)] = NormalizePlane(r3 + r1);
		frustum.planes[static_cast<std::uint32_t>(FrustumPlane::Top)] = NormalizePlane(r3 - r1);
		frustum.planes[static_cast<std::uint32_t>(FrustumPlane::Near)] = NormalizePlane(r2);
		frustum.planes[static_cast<std::uint32_t>(FrustumPlane::Far)] = NormalizePlane(r3 - r2);
		return frustum;
	}

	inline bool IntersectsSphere(const Frustum& frustrum, const Vec3& center, float radius) noexcept
	{
		for (const Plane& plane : frustrum.planes)
		{
			if (Distance(plane, center) < -radius)
			{
				return false;
			}
		}
		return true;
	}

	inline Mat4 operator*(const Mat4& a, const Mat4& b) noexcept { return Mul(a, b); }

	Mat4 Inverse(const Mat4& m) noexcept
	{
		const Vec3 a = m[0].xyz();
		const Vec3 b = m[1].xyz();
		const Vec3 c = m[2].xyz();
		const Vec3 d = m[3].xyz();

		const float x = m(3, 0);
		const float y = m(3, 1);
		const float z = m(3, 2);
		const float w = m(3, 3);

		Vec3 s = Cross(a, b);
		Vec3 t = Cross(c, d);
		Vec3 u = a * y - b * x;
		Vec3 v = c * w - d * z;

		const float det = Dot(s, v) + Dot(t, u);

		if (std::fabs(det) < 1e-8f)
		{
			return Mat4(1.0f);
		}

		float invDet = 1.0f / det;
		s *= invDet;
		t *= invDet;
		u *= invDet;
		v *= invDet;

		Vec3 r0 = Cross(b, v) + t * y;
		Vec3 r1 = Cross(v, a) - t * x;
		Vec3 r2 = Cross(d, u) + s * w;
		Vec3 r3 = Cross(u, c) - s * z;

		Mat4 inverse(0.0f);
		inverse[0] = Vec4(r0, -Dot(b, t));
		inverse[1] = Vec4(r1, Dot(a, t));
		inverse[2] = Vec4(r2, -Dot(d, s));
		inverse[3] = Vec4(r3, Dot(c, s));

		return Transpose(inverse);
	}

	// --- GLM-compatible transforms (column-major, post-multiply by transform) ---
	inline Mat4 Translate(const Mat4& m, const Vec3& v) noexcept
	{
		Mat4 Result = m;
		Result[3] = m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3];
		return Result;
	}

	inline Mat4 Scale(const Mat4& m, const Vec3& v) noexcept
	{
		Mat4 Result(0.0f);
		Result[0] = m[0] * v.x;
		Result[1] = m[1] * v.y;
		Result[2] = m[2] * v.z;
		Result[3] = m[3];
		return Result;
	}

	inline Mat4 Rotate(const Mat4& m, float angleRad, const Vec3& axisIn) noexcept
	{
		const float cosAngle = std::cos(angleRad);
		const float sinAngle = std::sin(angleRad);
		const Vec3 axis = MakeUnitVector(axisIn);
		const Vec3 temp = (1.0f - cosAngle) * axis;

		Mat4 RotateM(1.0f);
		RotateM[0][0] = cosAngle + temp.x * axis.x;
		RotateM[0][1] = temp.x * axis.y + sinAngle * axis.z;
		RotateM[0][2] = temp.x * axis.z - sinAngle * axis.y;

		RotateM[1][0] = temp.y * axis.x - sinAngle * axis.z;
		RotateM[1][1] = cosAngle + temp.y * axis.y;
		RotateM[1][2] = temp.y * axis.z + sinAngle * axis.x;

		RotateM[2][0] = temp.z * axis.x + sinAngle * axis.y;
		RotateM[2][1] = temp.z * axis.y - sinAngle * axis.x;
		RotateM[2][2] = cosAngle + temp.z * axis.z;

		Mat4 Result(0.0f);
		Result[0] = m[0] * RotateM[0][0] + m[1] * RotateM[0][1] + m[2] * RotateM[0][2];
		Result[1] = m[0] * RotateM[1][0] + m[1] * RotateM[1][1] + m[2] * RotateM[1][2];
		Result[2] = m[0] * RotateM[2][0] + m[1] * RotateM[2][1] + m[2] * RotateM[2][2];
		Result[3] = m[3];
		return Result;
	}

	inline Mat4 LookAtRH(const Vec3& eye, const Vec3& center, const Vec3& up) noexcept
	{
		const Vec3 f = MakeUnitVector(center - eye);
		const Vec3 s = MakeUnitVector(Cross(f, up));
		const Vec3 u = Cross(s, f);

		Mat4 Result(1.0f);
		Result[0][0] = s.x;
		Result[1][0] = s.y;
		Result[2][0] = s.z;
		Result[0][1] = u.x;
		Result[1][1] = u.y;
		Result[2][1] = u.z;
		Result[0][2] = -f.x;
		Result[1][2] = -f.y;
		Result[2][2] = -f.z;
		Result[3][0] = -Dot(s, eye);
		Result[3][1] = -Dot(u, eye);
		Result[3][2] = Dot(f, eye);
		return Result;
	}

	inline Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) noexcept
	{
		// Keep the previous behaviour (GLM default is RH unless configured otherwise).
		return LookAtRH(eye, center, up);
	}

	inline Mat4 OrthoRH_ZO(float left, float right, float bottom, float top, float zNear, float zFar) noexcept
	{
		Mat4 Result(1.0f);
		Result[0][0] = 2.0f / (right - left);
		Result[1][1] = 2.0f / (top - bottom);
		Result[2][2] = 1.0f / (zNear - zFar);
		Result[3][0] = -(right + left) / (right - left);
		Result[3][1] = -(top + bottom) / (top - bottom);
		Result[3][2] = -zNear / (zFar - zNear);
		return Result;
	}

	inline Mat4 PerspectiveRH_ZO(float fovy, float aspect, float zNear, float zFar) noexcept
	{
		const float tanHalfFovy = std::tan(fovy * 0.5f);
		Mat4 Result(0.0f);
		Result[0][0] = 1.0f / (aspect * tanHalfFovy);
		Result[1][1] = 1.0f / tanHalfFovy;
		Result[2][2] = zFar / (zNear - zFar);
		Result[2][3] = -1.0f;
		Result[3][2] = -(zFar * zNear) / (zFar - zNear);
		return Result;
	}
	// Oblique near-plane clipping for planar reflections / portals.
	// Modifies a standard perspective projection so that the given clip plane becomes the new near plane.
	//
	// Conventions:
	//  - RH, Z in [0..1] (D3D-style)
	//  - Column-major matrices, column-vector multiplication: clip = P * V * worldPos
	//  - clipPlaneView is in VIEW space: ax + by + cz + d = 0
	//
	// Based on Eric Lengyel's "Oblique View Frustum Clipping" (as popularized by Unity's CalculateObliqueMatrix).
	inline Mat4 PerspectiveRH_ZO_Oblique(const Mat4& proj, const Vec4& clipPlaneView) noexcept
	{
		// If plane is degenerate, return original projection.
		const Vec3 n{ clipPlaneView.x, clipPlaneView.y, clipPlaneView.z };
		const float nLen = Length(n);
		if (nLen < 1e-6f)
		{
			return proj;
		}

		// Build the 'q' point in clip space and transform it back by inverse(proj).
		auto Sign = [](float v) noexcept -> float { return (v >= 0.0f) ? 1.0f : -1.0f; };
		const Vec4 qClip{ Sign(clipPlaneView.x), Sign(clipPlaneView.y), 1.0f, 1.0f };

		const Mat4 invProj = Inverse(proj);
		const Vec4 q = invProj * qClip;

		const float denom = Dot(clipPlaneView, q);
		if (std::fabs(denom) < 1e-6f)
		{
			return proj;
		}

		const Vec4 c = clipPlaneView * (2.0f / denom);

		// Replace the third row of the projection matrix:
		//   row2 = c - row3
		Mat4 out = proj;
		out[0][2] = c.x - out[0][3];
		out[1][2] = c.y - out[1][3];
		out[2][2] = c.z - out[2][3];
		out[3][2] = c.w - out[3][3];
		return out;
	}

	const Vec3 TransformPoint(const Mat4& m, const Vec3& v) noexcept
	{
		const Vec4 r = m * Vec4(v, 1.0f);
		return { r.x, r.y, r.z };
	};

	const Vec3 TransformVector(const Mat4& m, const Vec3& v) noexcept
	{
		const Vec4 r = m * Vec4(v, 0.0f);
		return { r.x, r.y, r.z };
	};

	Vec3 MinVec3(const Vec3& a, const Vec3& b) noexcept
	{
		return { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) };
	}

	Vec3 MaxVec3(const Vec3& a, const Vec3& b) noexcept
	{
		return { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) };
	}

	auto MakeReflectionMatrix = [](const mathUtils::Vec3& nIn, float d) noexcept -> mathUtils::Mat4
		{
			mathUtils::Vec3 n = nIn;
			if (mathUtils::Length(n) < 1e-6f)
			{
				return mathUtils::Mat4(1.0f);
			}
			n = mathUtils::Normalize(n);
			const float nx = n.x;
			const float ny = n.y;
			const float nz = n.z;

			mathUtils::Mat4 R(1.0f);
			R(0, 0) = 1.0f - 2.0f * nx * nx;
			R(0, 1) = -2.0f * nx * ny;
			R(0, 2) = -2.0f * nx * nz;
			R(0, 3) = -2.0f * d * nx;

			R(1, 0) = -2.0f * ny * nx;
			R(1, 1) = 1.0f - 2.0f * ny * ny;
			R(1, 2) = -2.0f * ny * nz;
			R(1, 3) = -2.0f * d * ny;

			R(2, 0) = -2.0f * nz * nx;
			R(2, 1) = -2.0f * nz * ny;
			R(2, 2) = 1.0f - 2.0f * nz * nz;
			R(2, 3) = -2.0f * d * nz;

			R(3, 0) = 0.0f;
			R(3, 1) = 0.0f;
			R(3, 2) = 0.0f;
			R(3, 3) = 1.0f;
			return R;
		};

	std::pair<Vec3, float> CanonicalizePlane(Vec3 n, const Vec3& point) noexcept
	{
		n = Normalize(n);
		float d = -Dot(n, point);
		return { n, d }; // n·x + d = 0
	}

	Vec3 ReflectPoint(const Vec3& p, const Vec3& n, float d) noexcept
	{
		// p' = p - 2 * (n·p + d) * n
		const float dist = Dot(n, p) + d;
		return p - n * (2.0f * dist);
	}

	Vec3 ReflectVector(const Vec3& v, const Vec3& n) noexcept
	{
		// v' = v - 2 * (n·v) * n
		return v - n * (2.0f * Dot(n, v));
	}

	bool NearlyEqualVec3_(const mathUtils::Vec3& a, const mathUtils::Vec3& b, const float eps = 1e-4f) noexcept
	{
		return std::fabs(a.x - b.x) <= eps &&
			std::fabs(a.y - b.y) <= eps &&
			std::fabs(a.z - b.z) <= eps;
	}
	
	[[nodiscard]] inline bool IsFinite(const Vec2& value) noexcept
	{
		return std::isfinite(value.x)
			&& std::isfinite(value.y);
	}

	[[nodiscard]] inline bool IsFinite(const Vec3& value) noexcept
	{
		return std::isfinite(value.x)
			&& std::isfinite(value.y)
			&& std::isfinite(value.z);
	}

	[[nodiscard]] inline bool IsFinite(const Vec4& value) noexcept
	{
		return std::isfinite(value.x)
			&& std::isfinite(value.y)
			&& std::isfinite(value.z)
			&& std::isfinite(value.w);
	}
	
	[[nodiscard]] inline mathUtils::Vec4 NormalizeQuat(const mathUtils::Vec4& q) noexcept
	{
		const float len2 = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
		if (len2 <= 1e-20f)
		{
			return mathUtils::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}

		const float invLen = 1.0f / std::sqrt(len2);
		return mathUtils::Vec4(q.x * invLen, q.y * invLen, q.z * invLen, q.w * invLen);
	}

	[[nodiscard]] inline float DotQuat(const mathUtils::Vec4& a, const mathUtils::Vec4& b) noexcept
	{
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}

	[[nodiscard]] inline mathUtils::Vec4 NlerpQuat(
		const mathUtils::Vec4& a,
		const mathUtils::Vec4& b,
		float t) noexcept
	{
		mathUtils::Vec4 bb = b;
		if (DotQuat(a, b) < 0.0f)
		{
			bb.x = -bb.x;
			bb.y = -bb.y;
			bb.z = -bb.z;
			bb.w = -bb.w;
		}

		return NormalizeQuat(mathUtils::Vec4(
			a.x + (bb.x - a.x) * t,
			a.y + (bb.y - a.y) * t,
			a.z + (bb.z - a.z) * t,
			a.w + (bb.w - a.w) * t));
	}

	[[nodiscard]] inline mathUtils::Mat4 QuatToMat4(const mathUtils::Vec4& qIn) noexcept
	{
		const mathUtils::Vec4 q = NormalizeQuat(qIn);

		const float xx = q.x * q.x;
		const float yy = q.y * q.y;
		const float zz = q.z * q.z;
		const float xy = q.x * q.y;
		const float xz = q.x * q.z;
		const float yz = q.y * q.z;
		const float wx = q.w * q.x;
		const float wy = q.w * q.y;
		const float wz = q.w * q.z;

		mathUtils::Mat4 m(1.0f);
		m[0] = mathUtils::Vec4(1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy), 0.0f);
		m[1] = mathUtils::Vec4(2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx), 0.0f);
		m[2] = mathUtils::Vec4(2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy), 0.0f);
		m[3] = mathUtils::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
		return m;
	}

	[[nodiscard]] inline mathUtils::Mat4 ComposeTRS(
		const mathUtils::Vec3& translation,
		const mathUtils::Vec4& rotation,
		const mathUtils::Vec3& scale) noexcept
	{
		mathUtils::Mat4 m = QuatToMat4(rotation);
		m[0] = m[0] * scale.x;
		m[1] = m[1] * scale.y;
		m[2] = m[2] * scale.z;
		m[3] = mathUtils::Vec4(translation, 1.0f);
		return m;
	}

	[[nodiscard]] inline mathUtils::Vec4 Mat3ToQuat(
		const mathUtils::Vec3& c0,
		const mathUtils::Vec3& c1,
		const mathUtils::Vec3& c2) noexcept
	{
		// Matrix elements in row/col form reconstructed from column-major basis.
		const float m00 = c0.x; const float m01 = c1.x; const float m02 = c2.x;
		const float m10 = c0.y; const float m11 = c1.y; const float m12 = c2.y;
		const float m20 = c0.z; const float m21 = c1.z; const float m22 = c2.z;

		const float trace = m00 + m11 + m22;
		mathUtils::Vec4 q{ 0.0f, 0.0f, 0.0f, 1.0f };

		if (trace > 0.0f)
		{
			const float s = std::sqrt(trace + 1.0f) * 2.0f;
			q.w = 0.25f * s;
			q.x = (m21 - m12) / s;
			q.y = (m02 - m20) / s;
			q.z = (m10 - m01) / s;
		}
		else if (m00 > m11 && m00 > m22)
		{
			const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
			q.w = (m21 - m12) / s;
			q.x = 0.25f * s;
			q.y = (m01 + m10) / s;
			q.z = (m02 + m20) / s;
		}
		else if (m11 > m22)
		{
			const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
			q.w = (m02 - m20) / s;
			q.x = (m01 + m10) / s;
			q.y = 0.25f * s;
			q.z = (m12 + m21) / s;
		}
		else
		{
			const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
			q.w = (m10 - m01) / s;
			q.x = (m02 + m20) / s;
			q.y = (m12 + m21) / s;
			q.z = 0.25f * s;
		}

		return NormalizeQuat(q);
	}
	
	[[nodiscard]] inline mathUtils::Vec4 MultiplyRootMotionQuats(
			const mathUtils::Vec4& lhs,
			const mathUtils::Vec4& rhs) noexcept
	{
		return NormalizeQuat(mathUtils::Vec4(
			lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x,
			lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w,
			lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z));
	}
	
	bool TryConvertToInt(const float value, int& result)
	{
		if (!std::isfinite(value) ||
			static_cast<double>(value) < static_cast<double>(std::numeric_limits<int>::lowest()) ||
			static_cast<double>(value) > static_cast<double>(std::numeric_limits<int>::max()))
		{
			return false;
		}

		result = static_cast<int>(value);
		return true;
	}
}