module;

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

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
		mathUtils::Vec4 value{ 0.0f, 0.0f, 0.0f, 1.0f };
	};

	struct ScaleKey
	{
		float timeTicks{ 0.0f };
		mathUtils::Vec3 value{ 1.0f, 1.0f, 1.0f };
	};

	struct BoneAnimationChannel
	{
		int boneIndex{ -1 };
		std::string boneName{};
		std::vector<TranslationKey> translationKeys;
		std::vector<RotationKey> rotationKeys;
		std::vector<ScaleKey> scaleKeys;
	};

	struct AnimationClip
	{
		std::string name{};
		float durationTicks{ 0.0f };
		float ticksPerSecond{ 25.0f };
		bool looping{ true };
		std::vector<BoneAnimationChannel> channels;
	};

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

	inline void DecomposeTRS(
		const mathUtils::Mat4& m,
		mathUtils::Vec3& outTranslation,
		mathUtils::Vec4& outRotation,
		mathUtils::Vec3& outScale) noexcept
	{
		outTranslation = m[3].xyz();

		const mathUtils::Vec3 basisX = m[0].xyz();
		const mathUtils::Vec3 basisY = m[1].xyz();
		const mathUtils::Vec3 basisZ = m[2].xyz();

		outScale.x = mathUtils::Length(basisX);
		outScale.y = mathUtils::Length(basisY);
		outScale.z = mathUtils::Length(basisZ);

		mathUtils::Vec3 rotX(1.0f, 0.0f, 0.0f);
		mathUtils::Vec3 rotY(0.0f, 1.0f, 0.0f);
		mathUtils::Vec3 rotZ(0.0f, 0.0f, 1.0f);

		if (outScale.x > 1e-8f)
		{
			rotX = basisX / outScale.x;
		}
		if (outScale.y > 1e-8f)
		{
			rotY = basisY / outScale.y;
		}
		if (outScale.z > 1e-8f)
		{
			rotZ = basisZ / outScale.z;
		}

		outRotation = Mat3ToQuat(rotX, rotY, rotZ);
	}

	[[nodiscard]] inline bool IsValidAnimationClip(const AnimationClip& clip) noexcept
	{
		return clip.durationTicks >= 0.0f
			&& clip.ticksPerSecond > 0.0f
			&& !clip.channels.empty();
	}

	[[nodiscard]] inline float NormalizeAnimationTimeSeconds(
		const AnimationClip& clip,
		float timeSeconds,
		bool looping) noexcept
	{
		if (clip.ticksPerSecond <= 0.0f || clip.durationTicks <= 0.0f)
		{
			return 0.0f;
		}

		const float durationSeconds = clip.durationTicks / clip.ticksPerSecond;
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

	template <typename KeyT, typename ValueT, typename AccessFn>
	[[nodiscard]] inline ValueT SampleKeys(
		const std::vector<KeyT>& keys,
		float timeTicks,
		const ValueT& fallback,
		AccessFn&& access)
	{
		if (keys.empty())
		{
			return fallback;
		}
		if (keys.size() == 1 || timeTicks <= keys.front().timeTicks)
		{
			return access(keys.front());
		}
		if (timeTicks >= keys.back().timeTicks)
		{
			return access(keys.back());
		}

		for (std::size_t i = 0; i + 1 < keys.size(); ++i)
		{
			const KeyT& a = keys[i];
			const KeyT& b = keys[i + 1];
			if (timeTicks < a.timeTicks || timeTicks > b.timeTicks)
			{
				continue;
			}

			const float dt = b.timeTicks - a.timeTicks;
			const float t = (dt > 1e-8f) ? ((timeTicks - a.timeTicks) / dt) : 0.0f;

			if constexpr (std::is_same_v<ValueT, mathUtils::Vec4>)
			{
				return NlerpQuat(access(a), access(b), t);
			}
			else
			{
				return mathUtils::Lerp(access(a), access(b), t);
			}
		}

		return access(keys.back());
	}

	[[nodiscard]] inline mathUtils::Vec3 SampleTranslationKeys(
		const std::vector<TranslationKey>& keys,
		float timeTicks,
		const mathUtils::Vec3& fallback)
	{
		return SampleKeys<TranslationKey, mathUtils::Vec3>(
			keys,
			timeTicks,
			fallback,
			[](const TranslationKey& key) noexcept { return key.value; });
	}

	[[nodiscard]] inline mathUtils::Vec4 SampleRotationKeys(
		const std::vector<RotationKey>& keys,
		float timeTicks,
		const mathUtils::Vec4& fallback)
	{
		return SampleKeys<RotationKey, mathUtils::Vec4>(
			keys,
			timeTicks,
			fallback,
			[](const RotationKey& key) noexcept { return key.value; });
	}

	[[nodiscard]] inline mathUtils::Vec3 SampleScaleKeys(
		const std::vector<ScaleKey>& keys,
		float timeTicks,
		const mathUtils::Vec3& fallback)
	{
		return SampleKeys<ScaleKey, mathUtils::Vec3>(
			keys,
			timeTicks,
			fallback,
			[](const ScaleKey& key) noexcept { return key.value; });
	}
}