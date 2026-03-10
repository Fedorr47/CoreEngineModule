module;

#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

export module core:animation_clip;

import :math_utils;

export namespace rendern
{
	struct TranslationKey
	{
		float timeTicks{ 0.0f };
		mathUtils::Vec3 value{ 0.0f, 0.0f, 0.0f };
	};

	struct RotationKey
	{
		float timeTicks{ 0.0f };
		// Normalized quaternion stored as (x, y, z, w).
		mathUtils::Vec4 value{ 0.0f, 0.0f, 0.0f, 1.0f };
	};

	struct ScaleKey
	{
		float timeTicks{ 0.0f };
		mathUtils::Vec3 value{ 1.0f, 1.0f, 1.0f };
	};

	struct BoneAnimationChannel
	{
		std::string boneName;
		int boneIndex{ -1 };
		std::vector<TranslationKey> translationKeys;
		std::vector<RotationKey> rotationKeys;
		std::vector<ScaleKey> scaleKeys;
	};

	struct AnimationClip
	{
		std::string name;
		float durationTicks{ 0.0f };
		float ticksPerSecond{ 25.0f };
		bool looping{ true };
		std::vector<BoneAnimationChannel> channels;
	};

	[[nodiscard]] inline bool IsValidAnimationClip(const AnimationClip& clip) noexcept
	{
		return !clip.name.empty() && clip.durationTicks >= 0.0f && clip.ticksPerSecond > 0.0f;
	}

	[[nodiscard]] inline float GetAnimationDurationSeconds(const AnimationClip& clip) noexcept
	{
		if (clip.ticksPerSecond <= 0.0f)
		{
			return 0.0f;
		}
		return std::max(0.0f, clip.durationTicks / clip.ticksPerSecond);
	}

	[[nodiscard]] inline float NormalizeAnimationTimeSeconds(const AnimationClip& clip, float timeSeconds, bool looping) noexcept
	{
		const float durationSeconds = GetAnimationDurationSeconds(clip);
		if (durationSeconds <= 1e-8f)
		{
			return 0.0f;
		}
		if (looping)
		{
			float wrapped = std::fmod(timeSeconds, durationSeconds);
			if (wrapped < 0.0f)
			{
				wrapped += durationSeconds;
			}
			return wrapped;
		}
		return std::clamp(timeSeconds, 0.0f, durationSeconds);
	}

	[[nodiscard]] inline mathUtils::Vec4 NormalizeQuat(mathUtils::Vec4 q) noexcept
	{
		const float lenSq = mathUtils::Dot(q, q);
		if (lenSq <= 1e-12f)
		{
			return mathUtils::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}
		const float invLen = 1.0f / std::sqrt(lenSq);
		return q * invLen;
	}

	[[nodiscard]] inline mathUtils::Vec4 NlerpQuat(mathUtils::Vec4 a, mathUtils::Vec4 b, float t) noexcept
	{
		if (mathUtils::Dot(a, b) < 0.0f)
		{
			b = b * -1.0f;
		}
		return NormalizeQuat(mathUtils::Lerp(a, b, t));
	}

	[[nodiscard]] inline mathUtils::Mat4 QuatToMat4(mathUtils::Vec4 q) noexcept
	{
		q = NormalizeQuat(q);
		const float x = q.x;
		const float y = q.y;
		const float z = q.z;
		const float w = q.w;
		const float xx = x * x;
		const float yy = y * y;
		const float zz = z * z;
		const float xy = x * y;
		const float xz = x * z;
		const float yz = y * z;
		const float wx = w * x;
		const float wy = w * y;
		const float wz = w * z;

		mathUtils::Mat4 m(1.0f);
		m[0] = mathUtils::Vec4(1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy), 0.0f);
		m[1] = mathUtils::Vec4(2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx), 0.0f);
		m[2] = mathUtils::Vec4(2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy), 0.0f);
		m[3] = mathUtils::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
		return m;
	}

	[[nodiscard]] inline mathUtils::Vec4 RotationMatrixToQuat(const mathUtils::Mat4& m) noexcept
	{
		const float m00 = m(0, 0);
		const float m11 = m(1, 1);
		const float m22 = m(2, 2);
		const float trace = m00 + m11 + m22;

		mathUtils::Vec4 q{};
		if (trace > 0.0f)
		{
			const float s = std::sqrt(trace + 1.0f) * 2.0f;
			q.w = 0.25f * s;
			q.x = (m(2, 1) - m(1, 2)) / s;
			q.y = (m(0, 2) - m(2, 0)) / s;
			q.z = (m(1, 0) - m(0, 1)) / s;
		}
		else if (m00 > m11 && m00 > m22)
		{
			const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
			q.w = (m(2, 1) - m(1, 2)) / s;
			q.x = 0.25f * s;
			q.y = (m(0, 1) + m(1, 0)) / s;
			q.z = (m(0, 2) + m(2, 0)) / s;
		}
		else if (m11 > m22)
		{
			const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
			q.w = (m(0, 2) - m(2, 0)) / s;
			q.x = (m(0, 1) + m(1, 0)) / s;
			q.y = 0.25f * s;
			q.z = (m(1, 2) + m(2, 1)) / s;
		}
		else
		{
			const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
			q.w = (m(1, 0) - m(0, 1)) / s;
			q.x = (m(0, 2) + m(2, 0)) / s;
			q.y = (m(1, 2) + m(2, 1)) / s;
			q.z = 0.25f * s;
		}
		return NormalizeQuat(q);
	}

	inline void DecomposeTRS(const mathUtils::Mat4& m, mathUtils::Vec3& outT, mathUtils::Vec4& outR, mathUtils::Vec3& outS) noexcept
	{
		outT = m[3].xyz();

		mathUtils::Vec3 col0 = m[0].xyz();
		mathUtils::Vec3 col1 = m[1].xyz();
		mathUtils::Vec3 col2 = m[2].xyz();
		outS = mathUtils::Vec3(
			mathUtils::Length(col0),
			mathUtils::Length(col1),
			mathUtils::Length(col2));

		if (outS.x <= 1e-8f) outS.x = 1.0f;
		if (outS.y <= 1e-8f) outS.y = 1.0f;
		if (outS.z <= 1e-8f) outS.z = 1.0f;

		mathUtils::Mat4 rot(1.0f);
		rot[0] = mathUtils::Vec4(col0 / outS.x, 0.0f);
		rot[1] = mathUtils::Vec4(col1 / outS.y, 0.0f);
		rot[2] = mathUtils::Vec4(col2 / outS.z, 0.0f);
		rot[3] = mathUtils::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
		outR = RotationMatrixToQuat(rot);
	}

	[[nodiscard]] inline mathUtils::Mat4 ComposeTRS(const mathUtils::Vec3& translation, const mathUtils::Vec4& rotation, const mathUtils::Vec3& scale) noexcept
	{
		mathUtils::Mat4 m = QuatToMat4(rotation);
		m[0] = mathUtils::Vec4(m[0].xyz() * scale.x, 0.0f);
		m[1] = mathUtils::Vec4(m[1].xyz() * scale.y, 0.0f);
		m[2] = mathUtils::Vec4(m[2].xyz() * scale.z, 0.0f);
		m[3] = mathUtils::Vec4(translation, 1.0f);
		return m;
	}

	[[nodiscard]] inline mathUtils::Vec3 SampleTranslationKeys(
		const std::vector<TranslationKey>& keys,
		float timeTicks,
		const mathUtils::Vec3& defaultValue) noexcept
	{
		if (keys.empty())
		{
			return defaultValue;
		}
		if (keys.size() == 1 || timeTicks <= keys.front().timeTicks)
		{
			return keys.front().value;
		}
		if (timeTicks >= keys.back().timeTicks)
		{
			return keys.back().value;
		}
		for (std::size_t i = 0; i + 1 < keys.size(); ++i)
		{
			const TranslationKey& a = keys[i];
			const TranslationKey& b = keys[i + 1];
			if (timeTicks >= a.timeTicks && timeTicks <= b.timeTicks)
			{
				const float dt = b.timeTicks - a.timeTicks;
				const float t = (dt > 1e-8f) ? ((timeTicks - a.timeTicks) / dt) : 0.0f;
				return mathUtils::Lerp(a.value, b.value, std::clamp(t, 0.0f, 1.0f));
			}
		}
		return keys.back().value;
	}

	[[nodiscard]] inline mathUtils::Vec3 SampleScaleKeys(
		const std::vector<ScaleKey>& keys,
		float timeTicks,
		const mathUtils::Vec3& defaultValue) noexcept
	{
		if (keys.empty())
		{
			return defaultValue;
		}
		if (keys.size() == 1 || timeTicks <= keys.front().timeTicks)
		{
			return keys.front().value;
		}
		if (timeTicks >= keys.back().timeTicks)
		{
			return keys.back().value;
		}
		for (std::size_t i = 0; i + 1 < keys.size(); ++i)
		{
			const ScaleKey& a = keys[i];
			const ScaleKey& b = keys[i + 1];
			if (timeTicks >= a.timeTicks && timeTicks <= b.timeTicks)
			{
				const float dt = b.timeTicks - a.timeTicks;
				const float t = (dt > 1e-8f) ? ((timeTicks - a.timeTicks) / dt) : 0.0f;
				return mathUtils::Lerp(a.value, b.value, std::clamp(t, 0.0f, 1.0f));
			}
		}
		return keys.back().value;
	}

	[[nodiscard]] inline mathUtils::Vec4 SampleRotationKeys(
		const std::vector<RotationKey>& keys,
		float timeTicks,
		const mathUtils::Vec4& defaultValue) noexcept
	{
		if (keys.empty())
		{
			return defaultValue;
		}
		if (keys.size() == 1 || timeTicks <= keys.front().timeTicks)
		{
			return NormalizeQuat(keys.front().value);
		}
		if (timeTicks >= keys.back().timeTicks)
		{
			return NormalizeQuat(keys.back().value);
		}
		for (std::size_t i = 0; i + 1 < keys.size(); ++i)
		{
			const RotationKey& a = keys[i];
			const RotationKey& b = keys[i + 1];
			if (timeTicks >= a.timeTicks && timeTicks <= b.timeTicks)
			{
				const float dt = b.timeTicks - a.timeTicks;
				const float t = (dt > 1e-8f) ? ((timeTicks - a.timeTicks) / dt) : 0.0f;
				return NlerpQuat(a.value, b.value, std::clamp(t, 0.0f, 1.0f));
			}
		}
		return NormalizeQuat(keys.back().value);
	}
}