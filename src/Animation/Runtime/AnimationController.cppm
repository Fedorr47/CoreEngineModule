module;

#include <algorithm>
#include <concepts>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <cstddef>
#include <cctype>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <utility>
#include <limits>

export module core:animation_controller;

import :animation_clip;
import :animator;
import :skeleton;
import :string_utils;

export namespace rendern
{
	enum class AnimationParameterType : std::uint8_t
	{
		Bool = 0,
		Int = 1,
		Float = 2,
		Trigger = 3
	};

	struct AnimationParameterValue
	{
		AnimationParameterType type{ AnimationParameterType::Bool };
		bool boolValue{ false };
		int intValue{ 0 };
		float floatValue{ 0.0f };
		bool triggerValue{ false };
	};

	struct AnimationParameterStore
	{
		std::unordered_map<std::string, AnimationParameterValue> values;
	};
	
	struct MotionId
	{
		std::string value;
		[[nodiscard]] bool empty() const noexcept { return value.empty(); }
		friend bool operator==(const MotionId&, const MotionId&) = default;
	};

	struct AnimationClipRef
	{
		std::string sourceAssetId;
		std::string clipName;
	};

	struct AnimationProfileAsset
	{
		std::string id;
		std::unordered_map<std::string, AnimationClipRef> motions;
	};
	
	enum class AnimationConditionOp : std::uint8_t
	{
		IfTrue = 0,
		IfFalse,
		Greater,
		GreaterEqual,
		Less,
		LessEqual,
		Equal,
		NotEqual,
		Triggered
	};

	struct AnimationParameterDesc
	{
		std::string name;
		AnimationParameterValue defaultValue{};
	};

	struct AnimationBlend1DPoint
	{
		std::string clipName;
		float value{ 0.0f };
	};

	struct AnimationBlend2DPoint
	{
		std::string clipName;
		std::string clipSourceAssetId;
		float x{ 0.0f };
		float y{ 0.0f };
	};

	struct AnimationNotifyDesc
	{
		std::string id;
		float timeNormalized{ 0.0f };
		bool fireOnEnter{ false };
	};

	struct AnimationStateDesc
	{
		std::string name;
		MotionId motionId;
		std::string clipName;
		std::string clipSourceAssetId;
		std::string blendParameter;
		std::vector<AnimationBlend1DPoint> blend1D;
		std::string blendParameterX;
		std::string blendParameterY;
		std::vector<AnimationBlend2DPoint> blend2D;
		std::vector<AnimationNotifyDesc> notifies;
		std::vector<std::string> tags;
		bool looping{ true };
		float playRate{ 1.0f };
	};

	struct AnimationConditionDesc
	{
		std::string parameter;
		AnimationConditionOp op{ AnimationConditionOp::IfTrue };
		AnimationParameterValue value{};
	};

	struct AnimationTransitionDesc
	{
		std::string fromState;
		std::string toState;
		bool hasExitTime{ false };
		float exitTimeNormalized{ 1.0f };
		float blendDurationSeconds{ 0.15f };
		int priority{ 0 };
		std::vector<AnimationConditionDesc> conditions;
	};

	struct AnimationEventBindingDesc
	{
		std::string animationEventId;
		std::string gameplayEventId;
	};

	struct AnimationControllerAsset
	{
		std::string id;
		std::string defaultState;
		std::string notifyAssetPath;
		std::string eventBindingsAssetPath;
		std::vector<AnimationParameterDesc> parameters;
		std::vector<AnimationStateDesc> states;
		std::vector<AnimationTransitionDesc> transitions;
		std::vector<AnimationEventBindingDesc> eventBindings;
	};
	
	inline void ValidateAnimationStateContentMode(
		const AnimationControllerAsset& controller,
		const AnimationStateDesc& state)
	{
		if (state.motionId.empty())
		{
			return;
		}
		const auto unsupportedBlend = [&](std::string_view blendKind)
		{
			throw std::runtime_error("Animation controller '" + controller.id + "': state '" + state.name +
				"' uses semantic motion '" + state.motionId.value + "' together with " + std::string(blendKind) +
				", but semantic blend states are not supported.");
		};
		if (!state.blend2D.empty()) unsupportedBlend("Blend2D");
		if (!state.blend1D.empty()) unsupportedBlend("Blend1D");
		if (!state.clipName.empty() || !state.clipSourceAssetId.empty())
		{
			throw std::runtime_error("Animation controller '" + controller.id + "': state '" + state.name +
				"' must not define both semantic motion '" + state.motionId.value + "' and a direct clip reference.");
		}
	}

	[[nodiscard]] inline AnimationClipRef ResolveAnimationStateContentBinding(
		const AnimationControllerAsset& controller,
		const AnimationStateDesc& state,
		const AnimationProfileAsset* profile)
	{
		ValidateAnimationStateContentMode(controller, state);
		if (state.motionId.empty())
		{
			return { state.clipSourceAssetId, state.clipName };
		}
		if (profile == nullptr)
		{
			throw std::runtime_error("Animation controller '" + controller.id + "': state '" + state.name +
				"' requires motion '" + state.motionId.value + "', but no animation profile is bound.");
		}
		const auto binding = profile->motions.find(state.motionId.value);
		if (binding == profile->motions.end())
		{
			throw std::runtime_error("Animation controller '" + controller.id + "': state '" + state.name +
				"' requires motion '" + state.motionId.value + "', but animation profile '" + profile->id +
				"' does not define it.");
		}
		return binding->second;
	}

	enum class AnimationRootMotionMode : std::uint8_t
	{
		InPlace = 0,
		Allow = 1
	};

	struct AnimationNotifyEvent
	{
		std::uint64_t sequence{ 0 };
		std::string id;
		std::string stateName;
		std::string clipName;
		float normalizedTime{ 0.0f };
	};

	enum class AnimationControllerMode : std::uint8_t
	{
		LegacyClip = 0,
		StateMachine = 1
	};

	struct AnimationControllerRuntime
	{
		AnimationControllerMode mode{ AnimationControllerMode::LegacyClip };
		AnimationRootMotionMode rootMotionMode{ AnimationRootMotionMode::InPlace };
		std::string rootMotionBoneName;
		const Skeleton* skeleton{ nullptr };
		const std::vector<AnimationClip>* clips{ nullptr };
		const std::vector<std::string>* clipSourceAssetIds{ nullptr };

		std::string controllerAssetId;
		std::string currentStateName;
		std::string requestedStateName;
		const AnimationControllerAsset* stateMachineAsset{ nullptr };
		const AnimationProfileAsset* animationProfile{ nullptr };
		int currentStateIndex{ -1 };
		std::vector<int> resolvedStateClipIndices;
		std::vector<std::vector<int>> resolvedStateBlend1DClipIndices;
		std::vector<std::vector<int>> resolvedStateBlend2DClipIndices;

		bool currentStateUsesBlend1D{ false };
		bool currentStateUsesBlend2D{ false };
		std::string currentBlendParameterName;
		float currentBlendParameterValue{ 0.0f };
		std::string currentBlendParameterNameY;
		float currentBlendParameterValueY{ 0.0f };
		std::string currentBlendPrimaryClipName;
		std::string currentBlendSecondaryClipName;
		std::string currentBlendTertiaryClipName;
		AnimatorState blendSecondaryAnimator{};
		AnimatorState blendTertiaryAnimator{};
		int blendSecondaryClipIndex{ -1 };
		int blendTertiaryClipIndex{ -1 };
		float blendSecondaryAlpha{ 0.0f };
		float blendTertiaryAlpha{ 0.0f };

		bool transitionActive{ false };
		int transitionSourceStateIndex{ -1 };
		std::string transitionSourceStateName;
		float transitionElapsedSeconds{ 0.0f };
		float transitionDurationSeconds{ 0.0f };
		AnimatorState transitionSourceAnimator{};
		AnimatorState transitionSourceBlendSecondaryAnimator{};
		AnimatorState transitionSourceBlendTertiaryAnimator{};
		int transitionSourceSecondaryClipIndex{ -1 };
		int transitionSourceTertiaryClipIndex{ -1 };
		float transitionSourceSecondaryAlpha{ 0.0f };
		float transitionSourceTertiaryAlpha{ 0.0f };

		int legacyClipIndex{ -1 };
		bool autoplay{ true };
		bool looping{ true };
		float playRate{ 1.0f };
		bool paused{ false };
		bool forceBindPose{ false };
		mathUtils::Vec3 lastAppliedRootMotionDelta{ 0.0f, 0.0f, 0.0f };
		float previousStateNormalizedTime{ 0.0f };
		bool stateEnteredThisFrame{ true };
		std::uint64_t nextNotifySequence{ 0 };
		std::vector<AnimationNotifyEvent> pendingNotifyEvents;
		std::vector<AnimationNotifyEvent> notifyHistory;
		std::vector<std::string> recentRoutedGameplayEvents;
		std::vector<std::string> debugTransitionCandidates;
		std::string debugLastTransitionSelection;

		AnimationParameterStore parameters{};
	};

	[[nodiscard]] inline const AnimationClip* ResolveLegacyAnimationClip(const AnimationControllerRuntime& runtime) noexcept;

	#include "AnimationController_detail.inl"

	#include "AnimationController_api.inl"

}