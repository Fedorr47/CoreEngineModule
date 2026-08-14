module;

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

export module core:animation_workspace;

import :animation_controller;
import :animation_clip;
import :math_utils;

export namespace rendern
{
    enum class AnimationWorkspaceContentMode
    {
        LegacyDirect,
        Semantic
    };

    enum class AnimationWorkspaceMappingStatus
    {
        Ok,
        MissingMapping,
        MissingSource,
        MissingExplicitClip,
        SourceNotLoaded
    };

    struct AnimationWorkspaceResolution
    {
        AnimationWorkspaceContentMode contentMode{ AnimationWorkspaceContentMode::LegacyDirect };
        std::string motionId;
        std::string authoredSourceAssetId;
        std::string authoredClipName;
        int boundClipIndex{ -1 };
        std::string boundClipName;
        std::string boundSourceAssetId;
        bool reloadRequired{ false };
    };

    struct AnimationProfileEditorState
    {
        bool dirty{ false };
        bool reloadRequired{ false };
        std::string message;
    };

    struct AnimationRootTrajectoryDiagnostics
    {
        bool available{ false };
        mathUtils::Vec3 first{};
        mathUtils::Vec3 last{};
        mathUtils::Vec3 delta{};
        float horizontalDisplacement{ 0.0f };
        float verticalDisplacement{ 0.0f };
        std::vector<mathUtils::Vec3> sampledPoints;
    };

    [[nodiscard]] std::vector<std::string> CollectAnimationWorkspaceRequiredMotionIds(
        const AnimationControllerAsset& controller)
    {
        std::vector<std::string> result;
        for (const AnimationStateDesc& state : controller.states)
        {
            if (!state.motionId.empty() && std::find(result.begin(), result.end(), state.motionId.value) == result.end())
                result.push_back(state.motionId.value);
        }
        std::ranges::sort(result);
        return result;
    }

    [[nodiscard]] AnimationWorkspaceResolution BuildAnimationWorkspaceStateResolution(
        const AnimationControllerAsset& controller,
        const AnimationStateDesc& state,
        const AnimationProfileAsset* profile,
        const AnimationControllerRuntime* runtime,
        const std::vector<AnimationClip>& clips,
        const std::vector<std::string>& clipSourceAssetIds)
    {
        AnimationWorkspaceResolution result{};
        result.contentMode = state.motionId.empty() ? AnimationWorkspaceContentMode::LegacyDirect : AnimationWorkspaceContentMode::Semantic;
        result.motionId = state.motionId.value;
        if (result.contentMode == AnimationWorkspaceContentMode::LegacyDirect)
        {
            result.authoredSourceAssetId = state.clipSourceAssetId;
            result.authoredClipName = state.clipName;
        }
        else if (profile != nullptr)
        {
            if (const auto it = profile->motions.find(state.motionId.value); it != profile->motions.end())
            {
                result.authoredSourceAssetId = it->second.sourceAssetId;
                result.authoredClipName = it->second.clipName;
            }
        }

        const int stateIndex = FindAnimationControllerStateIndex(controller, state.name);
        if (runtime != nullptr && stateIndex >= 0 && static_cast<std::size_t>(stateIndex) < runtime->resolvedStateClipIndices.size())
            result.boundClipIndex = runtime->resolvedStateClipIndices[static_cast<std::size_t>(stateIndex)];
        if (result.boundClipIndex >= 0 && static_cast<std::size_t>(result.boundClipIndex) < clips.size())
        {
            result.boundClipName = clips[static_cast<std::size_t>(result.boundClipIndex)].name;
            if (static_cast<std::size_t>(result.boundClipIndex) < clipSourceAssetIds.size())
                result.boundSourceAssetId = clipSourceAssetIds[static_cast<std::size_t>(result.boundClipIndex)];
        }

        if (result.contentMode == AnimationWorkspaceContentMode::Semantic &&
    !result.authoredSourceAssetId.empty())
        {
            const bool sourceMatches = result.authoredSourceAssetId == result.boundSourceAssetId;
            bool clipMatches = false;

            if (result.authoredClipName.empty())
            {
                // Empty semantic clip means "use the first clip from the authored source".
                // Compare against the exact runtime index that Stage 2 binding would resolve.
                int expectedDefaultClipIndex = -1;

                const std::size_t clipCount = std::min(clips.size(), clipSourceAssetIds.size());
                for (std::size_t i = 0; i < clipCount; ++i)
                {
                    if (clipSourceAssetIds[i] == result.authoredSourceAssetId)
                    {
                        expectedDefaultClipIndex = static_cast<int>(i);
                        break;
                    }
                }

                clipMatches = expectedDefaultClipIndex >= 0 && result.boundClipIndex == expectedDefaultClipIndex;
            }
            else
            {
                clipMatches = result.authoredClipName == result.boundClipName;
            }
            
            result.reloadRequired =
                result.boundClipIndex < 0 ||
                !sourceMatches ||
                !clipMatches;
        }
        return result;
    }

    [[nodiscard]] AnimationWorkspaceMappingStatus EvaluateAnimationWorkspaceMappingStatus(
        const AnimationProfileAsset* profile,
        std::string_view motionId,
        const std::vector<std::string>& registeredSourceIds,
        const std::vector<AnimationClip>& loadedClips,
        const std::vector<std::string>& loadedClipSourceIds)
    {
        if (profile == nullptr) return AnimationWorkspaceMappingStatus::MissingMapping;
        const auto bindingIt = profile->motions.find(std::string(motionId));
        if (bindingIt == profile->motions.end()) return AnimationWorkspaceMappingStatus::MissingMapping;
        const AnimationClipRef& binding = bindingIt->second;
        if (binding.sourceAssetId.empty() || std::find(registeredSourceIds.begin(), registeredSourceIds.end(), binding.sourceAssetId) == registeredSourceIds.end())
            return AnimationWorkspaceMappingStatus::MissingSource;
        bool sourceLoaded = false;
        bool clipLoaded = binding.clipName.empty();
        for (std::size_t i = 0; i < loadedClips.size() && i < loadedClipSourceIds.size(); ++i)
        {
            if (loadedClipSourceIds[i] != binding.sourceAssetId) continue;
            sourceLoaded = true;
            if (loadedClips[i].name == binding.clipName) clipLoaded = true;
        }
        if (!sourceLoaded) return AnimationWorkspaceMappingStatus::SourceNotLoaded;
        if (!clipLoaded) return AnimationWorkspaceMappingStatus::MissingExplicitClip;
        return AnimationWorkspaceMappingStatus::Ok;
    }

    [[nodiscard]] AnimationRootTrajectoryDiagnostics BuildAnimationRootTrajectoryDiagnostics(
        const AnimationClip& clip, std::string_view rootBoneName, std::size_t maximumSampleCount)
    {
        AnimationRootTrajectoryDiagnostics result{};
        if (rootBoneName.empty() || maximumSampleCount == 0) return result;
        const auto channelIt = std::find_if(clip.channels.begin(), clip.channels.end(),
            [&](const BoneAnimationChannel& channel) { return channel.boneName == rootBoneName; });
        if (channelIt == clip.channels.end() || channelIt->translationKeys.empty()) return result;
        const auto& keys = channelIt->translationKeys;
        result.available = true;
        result.first = keys.front().value;
        result.last = keys.back().value;
        result.delta = result.last - result.first;
        result.horizontalDisplacement = std::sqrt(result.delta.x * result.delta.x + result.delta.z * result.delta.z);
        result.verticalDisplacement = result.delta.y;
        const std::size_t sampleCount = std::min(maximumSampleCount, keys.size());
        result.sampledPoints.reserve(sampleCount);
        const float firstTimeTicks = keys.front().timeTicks;
        const float lastTimeTicks = keys.back().timeTicks;
        for (std::size_t i = 0; i < sampleCount; ++i)
        {
            const float alpha = sampleCount <= 1
                ? 0.0f
                : static_cast<float>(i) / static_cast<float>(sampleCount - 1);
            const float timeTicks = firstTimeTicks + (lastTimeTicks - firstTimeTicks) * alpha;
            result.sampledPoints.push_back(SampleTranslationKeys(keys, timeTicks, result.first));
        }
        return result;
    }
}