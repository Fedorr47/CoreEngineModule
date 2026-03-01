module;

#include <cstdint>

export module core:debug_draw;

import std;

import :math_utils;

export namespace rendern::debugDraw
{
	struct DebugVertex
	{
		mathUtils::Vec3 pos{};
		std::uint32_t rgba{ 0xffffffffu }; // 0xAABBGGRR in memory (little-endian -> RR GG BB AA)
	};

	// Packs 0..255 components into a uint32 that matches R8G8B8A8_UNORM.
	// Memory layout on little-endian is: RR GG BB AA.
	constexpr std::uint32_t PackRGBA8(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) noexcept
	{
		return static_cast<std::uint32_t>(r)
			| (static_cast<std::uint32_t>(g) << 8)
			| (static_cast<std::uint32_t>(b) << 16)
			| (static_cast<std::uint32_t>(a) << 24);
	}

	struct DebugDrawList
	{
		std::vector<DebugVertex> lineVertices;
		std::vector<DebugVertex> overlayLineVertices;

		void Clear()
		{
			lineVertices.clear();
			overlayLineVertices.clear();
		}

		void ReserveLines(std::size_t lineCount)
		{
			lineVertices.reserve(lineCount * 2);
		}

		void ReserveOverlayLines(std::size_t lineCount)
		{
			overlayLineVertices.reserve(lineCount * 2);
		}

		std::size_t VertexCount() const noexcept
		{
			return lineVertices.size() + overlayLineVertices.size();
		}

		std::size_t DepthVertexCount() const noexcept
		{
			return lineVertices.size();
		}

		std::size_t OverlayVertexCount() const noexcept
		{
			return overlayLineVertices.size();
		}

		void AddLine(const mathUtils::Vec3& a, const mathUtils::Vec3& b, std::uint32_t rgba, bool overlay = false)
		{
			auto& dst = overlay ? overlayLineVertices : lineVertices;
			dst.push_back(DebugVertex{ a, rgba });
			dst.push_back(DebugVertex{ b, rgba });
		}

		void AddAxesCross(const mathUtils::Vec3& origin, float halfSize, std::uint32_t rgba, bool overlay = false)
		{
			AddLine(origin - mathUtils::Vec3(halfSize, 0.0f, 0.0f), origin + mathUtils::Vec3(halfSize, 0.0f, 0.0f), rgba, overlay);
			AddLine(origin - mathUtils::Vec3(0.0f, halfSize, 0.0f), origin + mathUtils::Vec3(0.0f, halfSize, 0.0f), rgba, overlay);
			AddLine(origin - mathUtils::Vec3(0.0f, 0.0f, halfSize), origin + mathUtils::Vec3(0.0f, 0.0f, halfSize), rgba, overlay);
		}

		void AddArrow(const mathUtils::Vec3& start,
			const mathUtils::Vec3& end,
			std::uint32_t rgba,
			float headFrac = 0.25f,
			float headWidthFrac = 0.15f,
			bool overlay = false)
		{
			AddLine(start, end, rgba, overlay);

			const mathUtils::Vec3 dir = end - start;
			const float len = mathUtils::Length(dir);
			if (len <= 1e-5f)
			{
				return;
			}

			const mathUtils::Vec3 fwd = dir / len;
			mathUtils::Vec3 up = mathUtils::Vec3(0.0f, 1.0f, 0.0f);
			if (std::abs(mathUtils::Dot(fwd, up)) > 0.95f)
			{
				up = mathUtils::Vec3(1.0f, 0.0f, 0.0f);
			}
			const mathUtils::Vec3 right = mathUtils::Normalize(mathUtils::Cross(fwd, up));
			const mathUtils::Vec3 up2 = mathUtils::Normalize(mathUtils::Cross(right, fwd));

			const float headLen = len * std::clamp(headFrac, 0.05f, 0.45f);
			const float headW = len * std::clamp(headWidthFrac, 0.02f, 0.35f);
			const mathUtils::Vec3 base = end - fwd * headLen;

			AddLine(end, base + right * headW, rgba, overlay);
			AddLine(end, base - right * headW, rgba, overlay);
			AddLine(end, base + up2 * headW, rgba, overlay);
			AddLine(end, base - up2 * headW, rgba, overlay);
		}

		void AddWireCone(const mathUtils::Vec3& apex,
			const mathUtils::Vec3& direction,
			float length,
			float outerHalfAngleRad,
			std::uint32_t rgba,
			std::uint32_t segments = 24,
			bool overlay = false)
		{
			if (length <= 1e-5f || segments < 3)
			{
				return;
			}

			const mathUtils::Vec3 dir = mathUtils::Normalize(direction);

			mathUtils::Vec3 up = mathUtils::Vec3(0.0f, 1.0f, 0.0f);
			if (std::abs(mathUtils::Dot(dir, up)) > 0.95f)
			{
				up = mathUtils::Vec3(1.0f, 0.0f, 0.0f);
			}
			const mathUtils::Vec3 right = mathUtils::Normalize(mathUtils::Cross(dir, up));
			const mathUtils::Vec3 up2 = mathUtils::Normalize(mathUtils::Cross(right, dir));

			const float radius = std::tan(outerHalfAngleRad) * length;
			const mathUtils::Vec3 baseCenter = apex + dir * length;

			std::vector<mathUtils::Vec3> circle;
			circle.reserve(segments);
			for (std::uint32_t i = 0; i < segments; ++i)
			{
				const float t = (static_cast<float>(i) / static_cast<float>(segments)) * (mathUtils::Pi * 2.0f);
				circle.push_back(baseCenter + right * (std::cos(t) * radius) + up2 * (std::sin(t) * radius));
			}

			for (std::uint32_t i = 0; i < segments; ++i)
			{
				const std::uint32_t j = (i + 1) % segments;
				AddLine(circle[i], circle[j], rgba, overlay);
				AddLine(apex, circle[i], rgba, overlay);
			}
		}

		void AddWireCircle(const mathUtils::Vec3& center,
			const mathUtils::Vec3& axisA,
			const mathUtils::Vec3& axisB,
			float radius,
			std::uint32_t rgba,
			std::uint32_t segments = 48,
			bool overlay = false)
		{
			if (radius <= 1e-5f || segments < 6)
			{
				return;
			}

			const mathUtils::Vec3 a = mathUtils::Normalize(axisA);
			const mathUtils::Vec3 b = mathUtils::Normalize(axisB);
			mathUtils::Vec3 prev{};
			for (std::uint32_t i = 0; i <= segments; ++i)
			{
				const float t = (static_cast<float>(i) / static_cast<float>(segments)) * (mathUtils::Pi * 2.0f);
				const mathUtils::Vec3 p = center + a * (std::cos(t) * radius) + b * (std::sin(t) * radius);
				if (i != 0)
				{
					AddLine(prev, p, rgba, overlay);
				}
				prev = p;
			}
		}

		void AddCircle3D(
			const mathUtils::Vec3& center,
			const mathUtils::Vec3& axisA,
			const mathUtils::Vec3& axisB,
			float radius,
			std::uint32_t rgba,
			std::uint32_t segments,
			bool overlay = false)
		{
			mathUtils::Vec3 prev{};
			for (std::uint32_t i = 0; i <= segments; ++i)
			{
				const float t = (static_cast<float>(i) / static_cast<float>(segments)) * (mathUtils::Pi * 2.0f);
				const mathUtils::Vec3 p = center + axisA * (std::cos(t) * radius) + axisB * (std::sin(t) * radius);
				if (i != 0)
				{
					AddLine(prev, p, rgba, overlay);
				}
				prev = p;
			}
		}

		void AddWireSphere(const mathUtils::Vec3& center,
			float radius,
			std::uint32_t rgba,
			std::uint32_t segments = 24,
			bool overlay = false)
		{
			if (radius <= 1e-5f || segments < 6)
			{
				return;
			}

			AddCircle3D(center, mathUtils::Vec3(1.0f, 0.0f, 0.0f), mathUtils::Vec3(0.0f, 1.0f, 0.0f), radius, rgba, segments, overlay);
			AddCircle3D(center, mathUtils::Vec3(1.0f, 0.0f, 0.0f), mathUtils::Vec3(0.0f, 0.0f, 1.0f), radius, rgba, segments, overlay);
			AddCircle3D(center, mathUtils::Vec3(0.0f, 1.0f, 0.0f), mathUtils::Vec3(0.0f, 0.0f, 1.0f), radius, rgba, segments, overlay);
		}
	};
}