module;

#include <algorithm>
#include <concepts>
#include <cmath>
#include <cstdint>
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

	struct AnimationNotifyDesc
	{
		std::string id;
		float timeNormalized{ 0.0f };
		bool fireOnEnter{ false };
	};

	struct AnimationStateDesc
	{
		std::string name;
		std::string clipName;
		std::string clipSourceAssetId;
		std::string blendParameter;
		std::vector<AnimationBlend1DPoint> blend1D;
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
		int currentStateIndex{ -1 };
		std::vector<int> resolvedStateClipIndices;
		std::vector<std::vector<int>> resolvedStateBlendClipIndices;

		bool currentStateUsesBlend1D{ false };
		std::string currentBlendParameterName;
		float currentBlendParameterValue{ 0.0f };
		std::string currentBlendPrimaryClipName;
		std::string currentBlendSecondaryClipName;
		AnimatorState blendSecondaryAnimator{};
		int blendSecondaryClipIndex{ -1 };
		float blendSecondaryAlpha{ 0.0f };

		bool transitionActive{ false };
		int transitionSourceStateIndex{ -1 };
		std::string transitionSourceStateName;
		float transitionElapsedSeconds{ 0.0f };
		float transitionDurationSeconds{ 0.0f };
		AnimatorState transitionSourceAnimator{};
		AnimatorState transitionSourceBlendSecondaryAnimator{};
		int transitionSourceSecondaryClipIndex{ -1 };
		float transitionSourceSecondaryAlpha{ 0.0f };

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