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