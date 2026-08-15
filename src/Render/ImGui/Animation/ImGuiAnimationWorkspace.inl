    [[nodiscard]] static const rendern::AnimationClip* FindWorkspaceClip(
        const AnimationGraphContext& ctx, std::string_view sourceId, std::string_view clipName, int* outIndex = nullptr)
    {
        if (outIndex) *outIndex = -1;
        if (ctx.skinnedItem == nullptr || ctx.skinnedItem->asset == nullptr) return nullptr;
        const auto& bundle = *ctx.skinnedItem->asset;
        for (std::size_t i = 0; i < bundle.clips.size(); ++i)
        {
            if (i >= bundle.clipSourceAssetIds.size() || bundle.clipSourceAssetIds[i] != sourceId) continue;
            if (!clipName.empty() && bundle.clips[i].name != clipName) continue;
            if (outIndex) *outIndex = static_cast<int>(i);
            return &bundle.clips[i];
        }
        return nullptr;
    }

    [[nodiscard]] static bool WorkspaceSourceLoaded(const AnimationGraphContext& ctx, std::string_view sourceId)
    {
        if (ctx.skinnedItem == nullptr || ctx.skinnedItem->asset == nullptr) return false;
        return std::ranges::any_of(ctx.skinnedItem->asset->externalAnimationSources,
            [&](const auto& source) { return source.assetId == sourceId; });
    }
    
    [[nodiscard]] static std::vector<std::string> CollectWorkspaceSourceIds(const rendern::LevelAsset& level)
    {
    	std::vector<std::string> result;
    	result.reserve(level.animations.size());
    	for (const auto& [id, unused] : level.animations)
    	{
    		result.push_back(id);
    	}
    	std::ranges::sort(result);
    	return result;
    }
    
    static bool DrawWorkspaceSourceCombo(
    	const char* label,
    	const rendern::LevelAsset& level,
    	std::string& sourceId)
    {
    	bool changed = false;
    	const char* preview = sourceId.empty() ? "<Missing source>" : sourceId.c_str();
    	if (ImGui::BeginCombo(label, preview))
    	{
    		for (const std::string& id : CollectWorkspaceSourceIds(level))
    		{
    			if (ImGui::Selectable(id.c_str(), sourceId == id) && sourceId != id)
    			{
    				sourceId = id;
    				changed = true;
    			}
    		}
    		ImGui::EndCombo();
    	}
    	return changed;
    }
    
    static bool DrawWorkspaceClipCombo(
    	const char* label,
    	const AnimationGraphContext& ctx,
    	std::string_view sourceId,
    	std::string& clipName)
    {
    	bool changed = false;
    	const char* preview = clipName.empty() ? "<Default / first clip>" : clipName.c_str();
    	if (ImGui::BeginCombo(label, preview))
    	{
    		if (ImGui::Selectable("<Default / first clip>", clipName.empty()) && !clipName.empty())
    		{
    			clipName.clear();
    			changed = true;
    		}
    		if (ctx.skinnedItem != nullptr && ctx.skinnedItem->asset != nullptr)
    		{
    			const auto& bundle = *ctx.skinnedItem->asset;
    			for (std::size_t index = 0; index < bundle.clips.size() && index < bundle.clipSourceAssetIds.size(); ++index)
    			{
    				if (bundle.clipSourceAssetIds[index] != sourceId) continue;
    				const std::string& candidate = bundle.clips[index].name;
    				if (ImGui::Selectable(candidate.c_str(), clipName == candidate) && clipName != candidate)
    				{
    					clipName = candidate;
    					changed = true;
    				}
    			}
    		}
    		ImGui::EndCombo();
    	}
    	return changed;
    }

    static void SelectWorkspaceClip(AnimationUIState& state, std::string_view source, std::string_view clip)
    {
        state.selectedSourceAssetId = source;
        state.selectedClipName = clip;
        state.clipInspectorTimeSeconds = 0.0f;
    }

    static void DrawWorkspaceResolution(const rendern::LevelAsset& level, const AnimationGraphContext& ctx,
        const rendern::AnimationStateDesc& desc, AnimationUIState& state)
    {
        ImGui::SeparatorText("Content Resolution");
        const rendern::AnimationProfileAsset* profile = nullptr;
        if (ctx.node != nullptr)
        {
            if (const auto profileIt = level.animationProfiles.find(ctx.node->animationProfile); profileIt != level.animationProfiles.end())
                profile = &profileIt->second;
        }
        static const std::vector<rendern::AnimationClip> noClips;
        static const std::vector<std::string> noSources;
        const auto& clips = ctx.skinnedItem && ctx.skinnedItem->asset ? ctx.skinnedItem->asset->clips : noClips;
        const auto& sourceIds = ctx.skinnedItem && ctx.skinnedItem->asset ? ctx.skinnedItem->asset->clipSourceAssetIds : noSources;
        const auto resolution = rendern::BuildAnimationWorkspaceStateResolution(
            *ctx.controllerAsset, desc, profile, ctx.skinnedItem ? &ctx.skinnedItem->controller : nullptr, clips, sourceIds);
        if (resolution.contentMode == rendern::AnimationWorkspaceContentMode::LegacyDirect)
        {
            ImGui::Text("Content mode: Legacy Direct");
            ImGui::Text("Source: %s", desc.clipSourceAssetId.empty() ? "<embedded>" : desc.clipSourceAssetId.c_str());
            ImGui::Text("Clip: %s", desc.clipName.empty() ? "<Default / first clip>" : desc.clipName.c_str());
            ImGui::Text("Bound Runtime Clip: %s", resolution.boundClipName.empty() ? "<unavailable>" : resolution.boundClipName.c_str());
            ImGui::Text("Bound Runtime Index: %d", resolution.boundClipIndex);
            return;
        }
        ImGui::Text("Content mode: Semantic"); ImGui::Text("Motion: %s", desc.motionId.value.c_str());
        ImGui::Text("Profile: %s", ctx.node == nullptr || ctx.node->animationProfile.empty() ? "<none>" : ctx.node->animationProfile.c_str());
        if (ctx.node == nullptr) return;
        if (profile == nullptr) { ImGui::TextColored(ImVec4(1,.4f,.4f,1), "Missing profile"); return; }
        if (profile->motions.find(desc.motionId.value) == profile->motions.end()) { ImGui::TextColored(ImVec4(1,.4f,.4f,1), "Missing MotionId mapping"); return; }
        ImGui::SeparatorText("Authored Binding");
        ImGui::Text("Authored Source: %s", resolution.authoredSourceAssetId.c_str());
        ImGui::Text("Authored Clip: %s", resolution.authoredClipName.empty() ? "<Default / first clip>" : resolution.authoredClipName.c_str());
        ImGui::SeparatorText("Bound Runtime Resolution");
        ImGui::Text("Bound Runtime Clip: %s", resolution.boundClipName.empty() ? "<unavailable>" : resolution.boundClipName.c_str());
        ImGui::Text("Bound Runtime Index: %d", resolution.boundClipIndex);
        if (resolution.reloadRequired) ImGui::TextColored(ImVec4(1,.75f,.3f,1), "Reload required");
        if (ImGui::SmallButton("Inspect authored clip")) SelectWorkspaceClip(state, resolution.authoredSourceAssetId, resolution.authoredClipName);
    }

    static void DrawAnimationSourcesWindow(rendern::LevelAsset& level, rendern::LevelInstance& levelInst,
        rendern::Scene& scene, AnimationUIState& state)
    {
        if (!state.animationSourcesWindowOpen) return;
        if (!ImGui::Begin("Animation Sources", &state.animationSourcesWindowOpen)) { ImGui::End(); return; }
        const AnimationGraphContext ctx = GetAnimationGraphContext(level, levelInst, scene);
        ImGui::SeparatorText("Add Animation Source");
        InputTextString("Source Id", state.newAnimationSourceId);
        InputTextString("Path", state.newAnimationSourcePath);
        ImGui::BeginDisabled(state.newAnimationSourceId.empty() || state.newAnimationSourcePath.empty());
        if (ImGui::Button("Add Source"))
        {
        	if (level.animations.contains(state.newAnimationSourceId))
        	{
        		state.animationSourcesMessage = "Source id already exists.";
        	}
        	else if (std::ranges::any_of(level.animations, [&](const auto& entry)
        		{ return entry.second.path == state.newAnimationSourcePath; }))
        	{
        		state.animationSourcesMessage = "Animation source path is already registered.";
        	}
        	else
        	{
        		const std::string addedId = state.newAnimationSourceId;
        		level.animations.emplace(addedId, rendern::LevelAnimationDef{ .path = state.newAnimationSourcePath });
        		SelectWorkspaceClip(state, addedId, {});
        		state.newAnimationSourceId.clear();
        		state.newAnimationSourcePath.clear();
        		state.animationSourcesMessage = "Source registered. Use Level Save to persist it; reload the level/character to load its clips.";
        	}
        }
        ImGui::EndDisabled();
        if (!state.animationSourcesMessage.empty()) ImGui::TextWrapped("%s", state.animationSourcesMessage.c_str());
        ImGui::SeparatorText("Authored Sources");
        const std::vector<std::string> ids = CollectWorkspaceSourceIds(level);
        for (const std::string& id : ids)
        {
            const auto& source = level.animations.at(id);
            ImGui::PushID(id.c_str());
            const bool selected = state.selectedSourceAssetId == id;
            if (ImGui::Selectable(id.c_str(), selected)) SelectWorkspaceClip(state, id, {});
            ImGui::SameLine();
            ImGui::TextDisabled("%s | %s", source.path.c_str(), WorkspaceSourceLoaded(ctx, id) ? "Loaded for selected character" : "Registered");
            if (selected)
            {
                if (!source.debugName.empty()) ImGui::TextDisabled("Debug name: %s", source.debugName.c_str());
                bool any = false;
                if (ctx.skinnedItem != nullptr && ctx.skinnedItem->asset != nullptr)
                {
                    const auto& bundle = *ctx.skinnedItem->asset;
                    for (std::size_t i = 0; i < bundle.clips.size(); ++i)
                    {
                        if (i >= bundle.clipSourceAssetIds.size() || bundle.clipSourceAssetIds[i] != id) continue;
                        any = true;
                        const auto& clip = bundle.clips[i];
                        if (ImGui::Selectable(("  " + clip.name).c_str(), state.selectedClipName == clip.name))
                            SelectWorkspaceClip(state, id, clip.name);
                        ImGui::SameLine();
                        ImGui::TextDisabled("%.3f ticks, %.3f ticks/s, %zu channels",
                            clip.durationTicks, clip.ticksPerSecond, clip.channels.size());
                    }
                }
                if (!any) ImGui::TextDisabled("  Clip metadata is not loaded for the selected character.");
            }
            ImGui::PopID();
        }
        ImGui::End();
    }

    static void DrawAnimationProfileWindow(rendern::LevelAsset& level, rendern::LevelInstance& levelInst,
        rendern::Scene& scene, AnimationUIState& state)
    {
        if (!state.animationProfileWindowOpen) return;
        if (!ImGui::Begin("Animation Profile", &state.animationProfileWindowOpen)) { ImGui::End(); return; }
        const AnimationGraphContext ctx = GetAnimationGraphContext(level, levelInst, scene);
        if (ctx.node == nullptr) { ImGui::TextDisabled("Select a skinned node."); ImGui::End(); return; }
        ImGui::Text("Controller: %s", ctx.node->animationController.empty() ? "<none>" : ctx.node->animationController.c_str());
        const std::string& profileId = ctx.node->animationProfile;
        if (profileId.empty())
        {
            ImGui::TextUnformatted("Profile: <none>");
            ImGui::TextDisabled("A profile is optional for legacy-only controllers.");
            ImGui::End();
            return;
        }
        auto profileIt = level.animationProfiles.find(profileId);
        ImGui::Text("Profile: %s", profileId.c_str());
        if (profileIt == level.animationProfiles.end()) { ImGui::TextDisabled("Missing profile (valid for legacy-only controllers)."); ImGui::End(); return; }
        auto& profile = profileIt->second;
        auto& profileState = state.profileEditorStates[profile.id];
        if (profileState.dirty) { ImGui::SameLine(); ImGui::TextUnformatted("*"); }
        const std::vector<std::string> required = ctx.controllerAsset != nullptr
            ? rendern::CollectAnimationWorkspaceRequiredMotionIds(*ctx.controllerAsset)
            : std::vector<std::string>{};
        profileState.reloadRequired = false;
        if (ctx.controllerAsset != nullptr && ctx.skinnedItem != nullptr && ctx.skinnedItem->asset != nullptr)
            for (const auto& desc : ctx.controllerAsset->states)
                if (!desc.motionId.empty())
                    profileState.reloadRequired |= rendern::BuildAnimationWorkspaceStateResolution(
                        *ctx.controllerAsset, desc, &profile, &ctx.skinnedItem->controller,
                        ctx.skinnedItem->asset->clips, ctx.skinnedItem->asset->clipSourceAssetIds).reloadRequired;
        std::vector<std::string> rows = required;
        std::vector<std::string> additional;
        for (const auto& [motion, unused] : profile.motions)
        {
        	if (std::find(rows.begin(), rows.end(), motion) == rows.end()) additional.push_back(motion);
        }
        std::ranges::sort(additional);
        rows.insert(rows.end(), additional.begin(), additional.end());
        ImGui::SeparatorText("Required Mappings");
        bool additionalHeaderShown = false;
        for (const std::string& motion : rows)
        {
            ImGui::PushID(motion.c_str());
            const bool isRequired = std::find(required.begin(), required.end(), motion) != required.end();
            if (!isRequired && !additionalHeaderShown)
            {
            	ImGui::SeparatorText("Additional Mappings");
            	additionalHeaderShown = true;
            }
            auto bindingIt = profile.motions.find(motion);
            if (bindingIt == profile.motions.end())
            {
                ImGui::Text("%s | Missing MotionId mapping", motion.c_str()); ImGui::SameLine();
                if (ImGui::SmallButton("Add mapping")) { profile.motions.emplace(motion, rendern::AnimationClipRef{}); profileState.dirty = true; }
                ImGui::PopID(); continue;
            }
            auto& binding = bindingIt->second;
            if (ImGui::Selectable(motion.c_str(), state.selectedMotionId == motion))
            { state.selectedMotionId = motion; SelectWorkspaceClip(state, binding.sourceAssetId, binding.clipName); }
            ImGui::SameLine(); ImGui::TextDisabled("%s", isRequired ? "Required" : "Unused by selected controller");
            if (DrawWorkspaceSourceCombo("Source", level, binding.sourceAssetId))
            {
               binding.clipName.clear();
               profileState.dirty = true;
               SelectWorkspaceClip(state, binding.sourceAssetId, {});
            }
            if (DrawWorkspaceClipCombo("Clip", ctx, binding.sourceAssetId, binding.clipName))
            {
               profileState.dirty = true;
               SelectWorkspaceClip(state, binding.sourceAssetId, binding.clipName);
            }
            const bool registered = level.animations.contains(binding.sourceAssetId);
            const auto* clip = FindWorkspaceClip(ctx, binding.sourceAssetId, binding.clipName);
            if (binding.sourceAssetId.empty()) ImGui::TextColored(ImVec4(1,.5f,.3f,1), "Missing source");
            else if (!registered) ImGui::TextColored(ImVec4(1,.4f,.4f,1), "Missing source");
            else if (!WorkspaceSourceLoaded(ctx, binding.sourceAssetId)) ImGui::TextDisabled("Source not loaded / metadata unavailable");
            else if (!binding.clipName.empty() && clip == nullptr) ImGui::TextColored(ImVec4(1,.4f,.4f,1), "Missing explicit clip");
            else ImGui::TextColored(ImVec4(.5f,.9f,.5f,1), "OK");
            if (!isRequired && ImGui::SmallButton("Remove Mapping"))
            {
            	profile.motions.erase(bindingIt);
            	profileState.dirty = true;
            	if (state.selectedMotionId == motion) state.selectedMotionId.clear();
            	ImGui::PopID();
            	continue;
            }
            ImGui::Separator(); ImGui::PopID();
        }
        if (!additionalHeaderShown) ImGui::SeparatorText("Additional Mappings");
        ImGui::SeparatorText("Add Motion Mapping");
        ImGui::PushID("add-motion-mapping");
        InputTextString("MotionId", profileState.newMotionId);
        if (DrawWorkspaceSourceCombo("Source", level, profileState.newSourceAssetId))
        {
        	profileState.newClipName.clear();
        }
        DrawWorkspaceClipCombo("Clip", ctx, profileState.newSourceAssetId, profileState.newClipName);
        const bool mappingExists = profile.motions.contains(profileState.newMotionId);
        const bool sourceExists = level.animations.contains(profileState.newSourceAssetId);
        const bool explicitClipExists = profileState.newClipName.empty() ||
        	FindWorkspaceClip(ctx, profileState.newSourceAssetId, profileState.newClipName) != nullptr;
        if (mappingExists) ImGui::TextColored(ImVec4(1,.65f,.3f,1), "Mapping already exists; edit its existing row.");
        else if (!profileState.newSourceAssetId.empty() && !sourceExists) ImGui::TextColored(ImVec4(1,.4f,.4f,1), "Selected source is not registered.");
        else if (!explicitClipExists) ImGui::TextColored(ImVec4(1,.4f,.4f,1), "Selected clip is unavailable for this source.");
        const bool canAddMapping = !profileState.newMotionId.empty() && !profileState.newSourceAssetId.empty() &&
        	!mappingExists && sourceExists && explicitClipExists;
        ImGui::BeginDisabled(!canAddMapping);
        if (ImGui::Button("Add Mapping"))
        {
        	const std::string addedMotion = profileState.newMotionId;
        	profile.motions.emplace(addedMotion, rendern::AnimationClipRef{
        		.sourceAssetId = profileState.newSourceAssetId,
        		.clipName = profileState.newClipName
        	});
        	profileState.dirty = true;
        	state.selectedMotionId = addedMotion;
        	SelectWorkspaceClip(state, profileState.newSourceAssetId, profileState.newClipName);
        	profileState.newMotionId.clear();
        	profileState.newSourceAssetId.clear();
        	profileState.newClipName.clear();
        	profileState.message = "Motion mapping added.";
        }
        ImGui::EndDisabled();
        ImGui::PopID();
        const auto pathIt = level.animationProfileAssetPaths.find(profile.id);
        if (pathIt != level.animationProfileAssetPaths.end())
        {
            ImGui::TextDisabled("External profile: %s", pathIt->second.c_str());
            if (ImGui::Button("Save Profile")) try { rendern::SaveAnimationProfileAssetToJson(pathIt->second, profile); profileState.dirty = false; profileState.message = "Profile saved."; }
            catch (const std::exception& ex) { profileState.message = ex.what(); }
        }
        else ImGui::TextDisabled("Inline profile: use the existing Level Save command to persist changes.");
        if (!profileState.message.empty()) ImGui::TextWrapped("%s", profileState.message.c_str());
        if (profileState.reloadRequired) ImGui::TextColored(ImVec4(1,.75f,.3f,1), "Reload required");
        ImGui::End();
    }

    static void DrawAnimationClipInspectorWindow(rendern::LevelAsset& level, rendern::LevelInstance& levelInst,
        rendern::Scene& scene, AnimationUIState& state)
    {
        if (!state.animationClipInspectorWindowOpen) return;
        if (!ImGui::Begin("Animation Clip Inspector", &state.animationClipInspectorWindowOpen)) { ImGui::End(); return; }
        const AnimationGraphContext ctx = GetAnimationGraphContext(level, levelInst, scene);
        int clipIndex = -1; const auto* clip = FindWorkspaceClip(ctx, state.selectedSourceAssetId, state.selectedClipName, &clipIndex);
        ImGui::Text("Source: %s", state.selectedSourceAssetId.empty() ? "<none>" : state.selectedSourceAssetId.c_str());
        if (auto it = level.animations.find(state.selectedSourceAssetId); it != level.animations.end()) ImGui::TextDisabled("Path: %s", it->second.path.c_str());
        if (clip == nullptr) { ImGui::TextDisabled("Clip metadata unavailable (source may not be loaded for this character)."); ImGui::End(); return; }
        const float duration = clip->ticksPerSecond > 0.0f ? clip->durationTicks / clip->ticksPerSecond : 0.0f;
        ImGui::Text("Clip: %s", clip->name.c_str()); ImGui::Text("Resolved index: %d", clipIndex);
        ImGui::Text("Duration: %.3f s | %.3f ticks | %.3f ticks per second", duration, clip->durationTicks, clip->ticksPerSecond);
        ImGui::Text("Channels: %zu", clip->channels.size());
        if (ImGui::Button(state.clipInspectorPlaying ? "Pause" : "Play")) state.clipInspectorPlaying = !state.clipInspectorPlaying;
        ImGui::SameLine(); if (ImGui::Button("Restart")) state.clipInspectorTimeSeconds = 0.0f;
        ImGui::Checkbox("Loop", &state.clipInspectorLoop); ImGui::SliderFloat("Play rate", &state.clipInspectorPlayRate, 0.1f, 3.0f);
        if (state.clipInspectorPlaying && duration > 0.0f) { state.clipInspectorTimeSeconds += ImGui::GetIO().DeltaTime * state.clipInspectorPlayRate; if (state.clipInspectorTimeSeconds > duration) state.clipInspectorTimeSeconds = state.clipInspectorLoop ? std::fmod(state.clipInspectorTimeSeconds, duration) : duration; }
        ImGui::SliderFloat("Timeline", &state.clipInspectorTimeSeconds, 0.0f, std::max(duration, .001f));
        ImGui::Text("Normalized time: %.3f", duration > 0.0f ? state.clipInspectorTimeSeconds / duration : 0.0f);
        ImGui::TextDisabled("Timeline is tooling-local; isolated 3D pose preview is unavailable.");
        const std::string rootBone = ctx.node ? ctx.node->animationRootMotionBone : std::string{};
        const auto channelIt = rootBone.empty() ? clip->channels.end() : std::find_if(clip->channels.begin(), clip->channels.end(), [&](const auto& channel) { return channel.boneName == rootBone; });
        const std::size_t translationKeyCount = channelIt == clip->channels.end() ? 0 : channelIt->translationKeys.size();
        auto& cache = state.rootTrajectoryCache;
        const std::uintptr_t assetIdentity = reinterpret_cast<std::uintptr_t>(ctx.skinnedItem->asset.get());
        const bool cacheMatches = cache.nodeIndex == ctx.nodeIndex && cache.clipIndex == clipIndex && cache.assetIdentity == assetIdentity &&
            cache.sourceAssetId == state.selectedSourceAssetId && cache.clipName == clip->name &&
            cache.rootBoneName == rootBone && cache.durationTicks == clip->durationTicks &&
            cache.translationKeyCount == translationKeyCount;
        if (!cacheMatches)
        {
            cache.nodeIndex = ctx.nodeIndex;
            cache.clipIndex = clipIndex;
            cache.assetIdentity = assetIdentity;
            cache.sourceAssetId = state.selectedSourceAssetId;
            cache.clipName = clip->name;
            cache.rootBoneName = rootBone;
            cache.durationTicks = clip->durationTicks;
            cache.translationKeyCount = translationKeyCount;
            cache.diagnostics = rendern::BuildAnimationRootTrajectoryDiagnostics(*clip, rootBone, 64);
        }
        const auto& trajectory = cache.diagnostics;
        if (!trajectory.available) ImGui::TextDisabled("Root trajectory: unavailable (no explicit root-motion track).");
        else
        {
            ImGui::Text("Root bone: %s", rootBone.c_str()); ImGui::Text("First: %.3f %.3f %.3f", trajectory.first.x, trajectory.first.y, trajectory.first.z);
            ImGui::Text("Last: %.3f %.3f %.3f", trajectory.last.x, trajectory.last.y, trajectory.last.z); ImGui::Text("Delta: %.3f %.3f %.3f", trajectory.delta.x, trajectory.delta.y, trajectory.delta.z);
            ImGui::Text("Horizontal: %.3f | Vertical: %.3f", trajectory.horizontalDisplacement, trajectory.verticalDisplacement);
            const ImVec2 origin = ImGui::GetCursorScreenPos(); const ImVec2 size(std::max(180.0f, ImGui::GetContentRegionAvail().x), 130.0f); ImGui::GetWindowDrawList()->AddRect(origin, AddImVec2(origin, size), IM_COL32(100,100,110,255));
            float maxAbs = 0.001f; for (const auto& point : trajectory.sampledPoints) maxAbs = std::max(maxAbs, std::max(std::abs(point.x-trajectory.first.x), std::abs(point.z-trajectory.first.z)));
            ImVec2 previous{}; bool havePrevious = false; for (const auto& point : trajectory.sampledPoints) { ImVec2 p(origin.x + size.x*.5f + (point.x-trajectory.first.x)/maxAbs*size.x*.45f, origin.y + size.y*.5f - (point.z-trajectory.first.z)/maxAbs*size.y*.45f); if (havePrevious) ImGui::GetWindowDrawList()->AddLine(previous,p,IM_COL32(90,210,255,255),2); previous=p; havePrevious=true; } ImGui::Dummy(size);
        }
        ImGui::End();
    }