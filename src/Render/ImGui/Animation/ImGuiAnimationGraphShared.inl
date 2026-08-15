    [[nodiscard]] static ImVec2 AddImVec2(const ImVec2& a, const ImVec2& b) noexcept
    {
        return ImVec2(a.x + b.x, a.y + b.y);
    }

    [[nodiscard]] static ImVec2 MulImVec2(const ImVec2& a, float s) noexcept
    {
        return ImVec2(a.x * s, a.y * s);
    }


    [[nodiscard]] static ImVec2 SubImVec2(const ImVec2& a, const ImVec2& b) noexcept
    {
        return ImVec2(a.x - b.x, a.y - b.y);
    }

    [[nodiscard]] static ImVec2 DivImVec2(const ImVec2& a, float s) noexcept
    {
        return (std::fabs(s) > 1e-6f) ? ImVec2(a.x / s, a.y / s) : ImVec2(0.0f, 0.0f);
    }

    [[nodiscard]] static float AnimationGraphClampZoom(float zoom) noexcept
    {
        return std::clamp(zoom, 0.45f, 2.50f);
    }
    
    static int InputTextStringResizeCallback(ImGuiInputTextCallbackData* data)
    {
    	if (data->EventFlag != ImGuiInputTextFlags_CallbackResize)
    	{
    		return 0;
    	}
    	std::string& value = *static_cast<std::string*>(data->UserData);
    	value.resize(static_cast<std::size_t>(data->BufTextLen));
    	data->Buf = value.data();
    	return 0;
    }
    
    static bool InputTextString(
    	const char* label,
    	std::string& value,
    	ImGuiInputTextFlags flags = ImGuiInputTextFlags_None)
    {
    	flags |= ImGuiInputTextFlags_CallbackResize;
    	return ImGui::InputText(
    		label,
    		value.data(),
    		value.capacity() + 1,
    		flags,
    		InputTextStringResizeCallback,
    		&value);
    }

    static void AnimationGraphHandleCanvasZoom(
        float& zoom,
        ImVec2& pan,
        const ImVec2& canvasPos,
        const ImVec2& canvasSize,
        bool hovered)
    {
        if (!hovered)
        {
            return;
        }

        const float wheel = ImGui::GetIO().MouseWheel;
        if (std::fabs(wheel) <= 1e-6f)
        {
            return;
        }

        const ImVec2 mousePos = ImGui::GetIO().MousePos;
        if (mousePos.x < canvasPos.x || mousePos.y < canvasPos.y ||
            mousePos.x > canvasPos.x + canvasSize.x || mousePos.y > canvasPos.y + canvasSize.y)
        {
            return;
        }

        const float oldZoom = zoom;
        const float zoomFactor = std::pow(1.12f, wheel);
        const float newZoom = AnimationGraphClampZoom(oldZoom * zoomFactor);
        if (std::fabs(newZoom - oldZoom) <= 1e-6f)
        {
            return;
        }

        const ImVec2 pivot = SubImVec2(mousePos, canvasPos);
        const ImVec2 graphPoint = DivImVec2(SubImVec2(pivot, pan), oldZoom);
        zoom = newZoom;
        pan = SubImVec2(pivot, MulImVec2(graphPoint, newZoom));
    }

    [[nodiscard]] static ImVec2 AnimationGraphCanvasPointToScreen(
        const ImVec2& canvasPos,
        const ImVec2& pan,
        float zoom,
        const ImVec2& graphPoint) noexcept
    {
        return AddImVec2(canvasPos, AddImVec2(pan, MulImVec2(graphPoint, zoom)));
    }

    [[nodiscard]] static ImVec2 AnimationGraphScreenDeltaToGraphDelta(const ImVec2& screenDelta, float zoom) noexcept
    {
        return DivImVec2(screenDelta, zoom);
    }

    static void AnimationGraphDrawGrid(
        ImDrawList* drawList,
        const ImVec2& canvasPos,
        const ImVec2& canvasSize,
        const ImVec2& pan,
        float zoom)
    {
        const float minorStep = 48.0f * zoom;
        const float majorStep = 240.0f * zoom;
        if (minorStep < 10.0f)
        {
            return;
        }

        auto PositiveModulo = [](float value, float base) -> float
            {
                const float mod = std::fmod(value, base);
                return mod < 0.0f ? mod + base : mod;
            };

        const ImU32 minorColor = IM_COL32(42, 42, 48, 255);
        const ImU32 majorColor = IM_COL32(58, 58, 66, 255);

        for (float x = PositiveModulo(pan.x, minorStep); x < canvasSize.x; x += minorStep)
        {
            drawList->AddLine(
                AddImVec2(canvasPos, ImVec2(x, 0.0f)),
                AddImVec2(canvasPos, ImVec2(x, canvasSize.y)),
                minorColor,
                1.0f);
        }
        for (float y = PositiveModulo(pan.y, minorStep); y < canvasSize.y; y += minorStep)
        {
            drawList->AddLine(
                AddImVec2(canvasPos, ImVec2(0.0f, y)),
                AddImVec2(canvasPos, ImVec2(canvasSize.x, y)),
                minorColor,
                1.0f);
        }
        for (float x = PositiveModulo(pan.x, majorStep); x < canvasSize.x; x += majorStep)
        {
            drawList->AddLine(
                AddImVec2(canvasPos, ImVec2(x, 0.0f)),
                AddImVec2(canvasPos, ImVec2(x, canvasSize.y)),
                majorColor,
                1.3f);
        }
        for (float y = PositiveModulo(pan.y, majorStep); y < canvasSize.y; y += majorStep)
        {
            drawList->AddLine(
                AddImVec2(canvasPos, ImVec2(0.0f, y)),
                AddImVec2(canvasPos, ImVec2(canvasSize.x, y)),
                majorColor,
                1.3f);
        }
    }

    template <typename Range>
    static bool AnimationGraphFitView(
        const Range& graphPositions,
        const ImVec2& nodeSize,
        const ImVec2& canvasSize,
        ImVec2& outPan,
        float& outZoom)
    {
        bool any = false;
        ImVec2 boundsMin(0.0f, 0.0f);
        ImVec2 boundsMax(0.0f, 0.0f);
        for (const ImVec2& point : graphPositions)
        {
            const ImVec2 maxPoint = AddImVec2(point, nodeSize);
            if (!any)
            {
                boundsMin = point;
                boundsMax = maxPoint;
                any = true;
            }
            else
            {
                boundsMin.x = std::min(boundsMin.x, point.x);
                boundsMin.y = std::min(boundsMin.y, point.y);
                boundsMax.x = std::max(boundsMax.x, maxPoint.x);
                boundsMax.y = std::max(boundsMax.y, maxPoint.y);
            }
        }

        if (!any)
        {
            return false;
        }

        const float padding = 36.0f;
        const float boundsWidth = std::max(1.0f, boundsMax.x - boundsMin.x);
        const float boundsHeight = std::max(1.0f, boundsMax.y - boundsMin.y);
        const float zoomX = (canvasSize.x - padding * 2.0f) / boundsWidth;
        const float zoomY = (canvasSize.y - padding * 2.0f) / boundsHeight;
        outZoom = AnimationGraphClampZoom(std::min(zoomX, zoomY));
        outPan = ImVec2(
            padding + std::max(0.0f, (canvasSize.x - padding * 2.0f) - boundsWidth * outZoom) * 0.5f - boundsMin.x * outZoom,
            padding + std::max(0.0f, (canvasSize.y - padding * 2.0f) - boundsHeight * outZoom) * 0.5f - boundsMin.y * outZoom);
        return true;
    }

    [[nodiscard]] static std::string AnimationGraphMakeKey(std::string_view graphKind, std::string_view controllerId, std::string_view nodeId)
    {
        std::string key;
        key.reserve(graphKind.size() + controllerId.size() + nodeId.size() + 2);
        key.append(graphKind);
        key.push_back(':');
        key.append(controllerId);
        key.push_back(':');
        key.append(nodeId);
        return key;
    }

    [[nodiscard]] static bool AnimationGraphContainsString(const std::vector<std::string>& values, std::string_view value)
    {
        for (const std::string& existing : values)
        {
            if (existing == value)
            {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] static std::string AnimationGraphConditionValueText(const rendern::AnimationParameterValue& value)
    {
        char buf[64]{};
        switch (value.type)
        {
        case rendern::AnimationParameterType::Bool:
            return value.boolValue ? "true" : "false";
        case rendern::AnimationParameterType::Int:
            std::snprintf(buf, sizeof(buf), "%d", value.intValue);
            return std::string(buf);
        case rendern::AnimationParameterType::Float:
            std::snprintf(buf, sizeof(buf), "%.3f", value.floatValue);
            return std::string(buf);
        case rendern::AnimationParameterType::Trigger:
            return value.triggerValue ? "triggered" : "idle";
        }
        return {};
    }

    [[nodiscard]] static std::string AnimationGraphConditionText(const rendern::AnimationConditionDesc& cond)
    {
        std::string text = cond.parameter + " ";
        switch (cond.op)
        {
        case rendern::AnimationConditionOp::IfTrue:       text += "== true"; break;
        case rendern::AnimationConditionOp::IfFalse:      text += "== false"; break;
        case rendern::AnimationConditionOp::Greater:      text += "> "  + AnimationGraphConditionValueText(cond.value); break;
        case rendern::AnimationConditionOp::GreaterEqual: text += ">= " + AnimationGraphConditionValueText(cond.value); break;
        case rendern::AnimationConditionOp::Less:         text += "< "  + AnimationGraphConditionValueText(cond.value); break;
        case rendern::AnimationConditionOp::LessEqual:    text += "<= " + AnimationGraphConditionValueText(cond.value); break;
        case rendern::AnimationConditionOp::Equal:        text += "== " + AnimationGraphConditionValueText(cond.value); break;
        case rendern::AnimationConditionOp::NotEqual:     text += "!= " + AnimationGraphConditionValueText(cond.value); break;
        case rendern::AnimationConditionOp::Triggered:    text += "triggered"; break;
        }
        return text;
    }

    [[nodiscard]] static std::string AnimationGraphConditionsSummary(const std::vector<rendern::AnimationConditionDesc>& conditions)
    {
        if (conditions.empty())
        {
            return "always";
        }

        std::string text;
        for (std::size_t i = 0; i < conditions.size(); ++i)
        {
            if (i > 0)
            {
                text += " && ";
            }
            text += AnimationGraphConditionText(conditions[i]);
        }
        return text;
    }

    [[nodiscard]] static bool AnimationGraphStateIsLocomotion(const rendern::AnimationStateDesc& state) noexcept
    {
        return rendern::AnimationStateHasTag(state, "locomotion") ||
            rendern::AnimationStateHasTag(state, "move") ||
            !state.blend1D.empty() ||
            !state.blend2D.empty();
    }

    [[nodiscard]] static const char* AnimationGraphStateCategory(const rendern::AnimationStateDesc& state) noexcept
    {
        if (rendern::AnimationStateHasTag(state, "idle"))
        {
            return "Idle";
        }
        if (AnimationGraphStateIsLocomotion(state))
        {
            return "Locomotion";
        }
        if (rendern::AnimationStateHasTag(state, "turn"))
        {
            return "Turn";
        }
        if (rendern::AnimationStateHasTag(state, "action") || rendern::AnimationStateHasTag(state, "attack") || rendern::AnimationStateHasTag(state, "jump"))
        {
            return "Action";
        }
        return "Other";
    }

    [[nodiscard]] static ImU32 AnimationGraphStateColor(const rendern::AnimationStateDesc& state, bool isCurrent)
    {
        if (isCurrent)
        {
            return IM_COL32(100, 180, 255, 255);
        }
        if (rendern::AnimationStateHasTag(state, "idle"))
        {
            return IM_COL32(85, 105, 150, 255);
        }
        if (AnimationGraphStateIsLocomotion(state))
        {
            return IM_COL32(80, 130, 95, 255);
        }
        if (rendern::AnimationStateHasTag(state, "turn"))
        {
            return IM_COL32(135, 110, 70, 255);
        }
        if (rendern::AnimationStateHasTag(state, "action") || rendern::AnimationStateHasTag(state, "attack") || rendern::AnimationStateHasTag(state, "jump"))
        {
            return IM_COL32(140, 85, 85, 255);
        }
        return IM_COL32(90, 90, 90, 255);
    }

    [[nodiscard]] static bool AnimationNodeAlive(
        const rendern::LevelAsset& level,
        const int nodeIndex) noexcept
    {
        return nodeIndex >= 0 &&
            static_cast<std::size_t>(nodeIndex) < level.nodes.size() &&
            level.nodes[static_cast<std::size_t>(nodeIndex)].alive;
    }

    struct AnimationGraphContext
    {
        int nodeIndex = -1;
        rendern::LevelNode* node = nullptr;
        rendern::SkinnedDrawItem* skinnedItem = nullptr;
        const rendern::AnimationControllerAsset* controllerAsset = nullptr;
    };

[[nodiscard]] static AnimationControllerEditorState::StateContentMode InferAnimationStateEditorContentMode(
		const rendern::AnimationStateDesc& state)
	{
		return state.motionId.empty()
			? AnimationControllerEditorState::StateContentMode::LegacyContent
			: AnimationControllerEditorState::StateContentMode::SemanticMotion;
	}

	static void RebuildAnimationStateEditorContentModes(AnimationControllerEditorState& editor)
	{
		editor.stateContentModes.clear();
		for (const rendern::AnimationStateDesc& state : editor.workingController.states)
		{
			editor.stateContentModes.emplace(state.name, InferAnimationStateEditorContentMode(state));
		}
	}

	static void RebuildControllerEditorTopology(AnimationControllerEditorState& editor)
	{
		try
		{
			editor.effectiveTransitions = rendern::BuildEffectiveAnimationTransitions(editor.workingController);
			editor.topologyValid = true;
			editor.message = editor.dirty ? "Authored controller changed. Save and rebind to update runtime." : "Controller topology is valid.";
		}
		catch (const std::exception& error)
		{
			editor.effectiveTransitions.clear();
			editor.topologyValid = false;
			editor.message = error.what();
		}
	}

	static void MarkControllerEditorChanged(AnimationControllerEditorState& editor)
	{
		editor.dirty = true;
		editor.reloadRequired = true;
		RebuildControllerEditorTopology(editor);
	}

	static void MarkControllerEditorSaved(AnimationControllerEditorState& editor)
	{
		editor.persistedController = editor.workingController;
		editor.persistedDiffersFromBound = true;
		editor.dirty = false;
		editor.reloadRequired = true;
		editor.message = "Controller saved. Reload / Rebind to update runtime.";
	}

	static void MarkControllerEditorRebound(
		AnimationControllerEditorState& editor,
		const rendern::AnimationControllerAsset& controller)
	{
		editor.workingController = controller;
		editor.persistedController = controller;
		editor.dirty = false;
		editor.reloadRequired = false;
		editor.persistedDiffersFromBound = false;
		editor.stateRenameSourceName.clear();
        editor.stateNameDraft.clear();
		RebuildAnimationStateEditorContentModes(editor);
		RebuildControllerEditorTopology(editor);
		editor.message = "Controller reloaded and rebound.";
	}

	static void DiscardControllerWorkingChanges(AnimationControllerEditorState& editor)
	{
		if (!editor.dirty) return;
		editor.workingController = editor.persistedController;
		editor.dirty = false;
		editor.reloadRequired = editor.persistedDiffersFromBound;
		editor.stateRenameSourceName.clear();
        editor.stateNameDraft.clear();
		RebuildAnimationStateEditorContentModes(editor);
		RebuildControllerEditorTopology(editor);
		editor.message = "Unsaved controller changes discarded.";
	}

    [[nodiscard]] static AnimationGraphContext GetAnimationGraphContext(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        rendern::Scene& scene)
    {
        AnimationGraphContext ctx{};
        if (!AnimationNodeAlive(level, scene.editorSelectedNode))
        {
            return ctx;
        }

        rendern::LevelNode& node = level.nodes[static_cast<std::size_t>(scene.editorSelectedNode)];
        if (node.skinnedMesh.empty())
        {
            return ctx;
        }

        const int drawIndex = levelInst.GetNodeSkinnedDrawIndex(scene.editorSelectedNode);
        rendern::SkinnedDrawItem* skinnedItem = levelInst.GetSkinnedDrawItem(scene, drawIndex);
        if (skinnedItem == nullptr)
        {
            return ctx;
        }

        ctx.nodeIndex = scene.editorSelectedNode;
        ctx.node = &node;
        ctx.skinnedItem = skinnedItem;
        ctx.controllerAsset = skinnedItem->controller.stateMachineAsset;
        return ctx;
    }

    [[nodiscard]] static AnimationGraphContext GetAnimationGraphContextForNodeIndex(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        rendern::Scene& scene,
        int nodeIndex)
    {
        AnimationGraphContext ctx{};
        if (!AnimationNodeAlive(level, nodeIndex))
        {
            return ctx;
        }

        rendern::LevelNode& node = level.nodes[static_cast<std::size_t>(nodeIndex)];
        if (node.skinnedMesh.empty())
        {
            return ctx;
        }

        const int drawIndex = levelInst.GetNodeSkinnedDrawIndex(nodeIndex);
        rendern::SkinnedDrawItem* skinnedItem = levelInst.GetSkinnedDrawItem(scene, drawIndex);
        if (skinnedItem == nullptr)
        {
            return ctx;
        }

        ctx.nodeIndex = nodeIndex;
        ctx.node = &node;
        ctx.skinnedItem = skinnedItem;
        ctx.controllerAsset = skinnedItem->controller.stateMachineAsset;
        return ctx;
    }

    [[nodiscard]] static std::vector<int> BuildAnimationRuntimeNodeCandidates(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst)
    {
        std::vector<int> result;
        result.reserve(level.nodes.size());
        for (std::size_t i = 0; i < level.nodes.size(); ++i)
        {
            const rendern::LevelNode& node = level.nodes[i];
            if (!node.alive || node.skinnedMesh.empty())
            {
                continue;
            }
            const int nodeIndex = static_cast<int>(i);
            if (levelInst.GetNodeSkinnedDrawIndex(nodeIndex) >= 0)
            {
                result.push_back(nodeIndex);
            }
        }
        return result;
    }

    [[nodiscard]] static rendern::EntityHandle FindAnimationRuntimeEntityForNodeIndex(
        const rendern::GameplayRuntime* gameplayRuntime,
        const int nodeIndex) noexcept
    {
        if (gameplayRuntime == nullptr || nodeIndex < 0)
        {
            return rendern::kNullEntity;
        }

        const rendern::GameplayWorld& gameplayWorld = gameplayRuntime->GetWorld();
        for (const rendern::EntityHandle runtimeEntity : gameplayRuntime->GetNodeBoundEntities())
        {
            if (!gameplayWorld.IsEntityValid(runtimeEntity))
            {
                continue;
            }

            const rendern::GameplayNodeLinkComponent* nodeLink = gameplayWorld.TryGetNodeLink(runtimeEntity);
            const rendern::GameplayAnimationLinkComponent* animationLink = gameplayWorld.TryGetAnimationLink(runtimeEntity);
            if (nodeLink != nullptr && animationLink != nullptr && nodeLink->nodeIndex == nodeIndex)
            {
                return runtimeEntity;
            }
        }

        return rendern::kNullEntity;
    }

    [[nodiscard]] static AnimationGraphContext ResolveObservedAnimationRuntimeEntity(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        rendern::Scene& scene,
        AnimationUIState& uiState,
        rendern::EditorSelectionService& selection,
        const rendern::GameplayRuntime* gameplayRuntime)
    {
        if (gameplayRuntime == nullptr || !selection.HasObservedRuntimeEntity())
        {
            return {};
        }

        const rendern::EntityHandle observedRuntimeEntity = selection.GetObservedRuntimeEntity();
        const rendern::GameplayWorld& gameplayWorld = gameplayRuntime->GetWorld();
        const rendern::GameplayNodeLinkComponent* nodeLink = gameplayWorld.IsEntityValid(observedRuntimeEntity)
            ? gameplayWorld.TryGetNodeLink(observedRuntimeEntity)
            : nullptr;
        const rendern::GameplayAnimationLinkComponent* animationLink = gameplayWorld.IsEntityValid(observedRuntimeEntity)
            ? gameplayWorld.TryGetAnimationLink(observedRuntimeEntity)
            : nullptr;
        if (nodeLink == nullptr || animationLink == nullptr)
        {
            selection.ClearObservedRuntimeEntity();
            uiState.animationRuntimeObservedEntityUnavailable = true;
            return {};
        }

        AnimationGraphContext observedContext = GetAnimationGraphContextForNodeIndex(level, levelInst, scene, nodeLink->nodeIndex);
        if (observedContext.node == nullptr || observedContext.skinnedItem == nullptr)
        {
            selection.ClearObservedRuntimeEntity();
            uiState.animationRuntimeObservedEntityUnavailable = true;
            return {};
        }

        uiState.animationRuntimePinnedNodeIndex = nodeLink->nodeIndex;
        uiState.animationRuntimeObservedEntityUnavailable = false;
        return observedContext;
    }

    static void SetAnimationRuntimePinnedTarget(
        AnimationUIState& uiState,
        rendern::EditorSelectionService& selection,
        const rendern::GameplayRuntime* gameplayRuntime,
        const int nodeIndex) noexcept
    {
        uiState.animationRuntimePinnedNodeIndex = nodeIndex;
        uiState.animationRuntimeObservedEntityUnavailable = false;
        if (nodeIndex < 0)
        {
            selection.ClearObservedRuntimeEntity();
            return;
        }

        const rendern::EntityHandle observedRuntimeEntity = FindAnimationRuntimeEntityForNodeIndex(gameplayRuntime, nodeIndex);
        if (observedRuntimeEntity != rendern::kNullEntity)
        {
            selection.SetObservedRuntimeEntity(observedRuntimeEntity);
        }
        else
        {
            selection.ClearObservedRuntimeEntity();
            uiState.animationRuntimeObservedEntityUnavailable = true;
        }
    }

    [[nodiscard]] static AnimationGraphContext GetAnimationRuntimeContext(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        rendern::Scene& scene,
        AnimationUIState& uiState,
        rendern::EditorSelectionService& selection,
        const rendern::GameplayRuntime* gameplayRuntime)
    {
        AnimationGraphContext observedContext = ResolveObservedAnimationRuntimeEntity(
            level, levelInst, scene, uiState, selection, gameplayRuntime);
        if (observedContext.node != nullptr && observedContext.skinnedItem != nullptr)
        {
            return observedContext;
        }

        if (uiState.animationRuntimePinnedNodeIndex >= 0)
        {
            AnimationGraphContext pinned = GetAnimationGraphContextForNodeIndex(level, levelInst, scene, uiState.animationRuntimePinnedNodeIndex);
            if (pinned.node != nullptr && pinned.skinnedItem != nullptr)
            {
                const rendern::EntityHandle observedRuntimeEntity = FindAnimationRuntimeEntityForNodeIndex(gameplayRuntime, pinned.nodeIndex);
                if (observedRuntimeEntity != rendern::kNullEntity)
                {
                    selection.SetObservedRuntimeEntity(observedRuntimeEntity);
                    uiState.animationRuntimeObservedEntityUnavailable = false;
                }
                return pinned;
            }
            SetAnimationRuntimePinnedTarget(uiState, selection, gameplayRuntime, -1);
        }

        AnimationGraphContext selected = GetAnimationGraphContext(level, levelInst, scene);
        if (selected.node != nullptr && selected.skinnedItem != nullptr)
        {
            return selected;
        }

        const std::vector<int> candidates = BuildAnimationRuntimeNodeCandidates(level, levelInst);
        if (!candidates.empty())
        {
            return GetAnimationGraphContextForNodeIndex(level, levelInst, scene, candidates.front());
        }

        return {};
    }

    static void EnsureAnimationGraphFsmLayout(
        AnimationUIState& uiState,
        const rendern::AnimationControllerAsset& controllerAsset)
    {
        std::unordered_map<std::string, int> categoryRows;
        for (const rendern::AnimationStateDesc& state : controllerAsset.states)
        {
            const char* category = AnimationGraphStateCategory(state);
            int& row = categoryRows[std::string(category)];
            const std::string key = AnimationGraphMakeKey("fsm", controllerAsset.id, state.name);
            if (!uiState.animationGraphFsmNodePositions.contains(key))
            {
                float x = 0.0f;
                if (std::strcmp(category, "Idle") == 0)
                {
                    x = 40.0f;
                }
                else if (std::strcmp(category, "Locomotion") == 0)
                {
                    x = 290.0f;
                }
                else if (std::strcmp(category, "Turn") == 0)
                {
                    x = 540.0f;
                }
                else if (std::strcmp(category, "Action") == 0)
                {
                    x = 790.0f;
                }
                else
                {
                    x = 1040.0f;
                }
                uiState.animationGraphFsmNodePositions.emplace(key, ImVec2(x, 28.0f + static_cast<float>(row) * 110.0f));
            }
            ++row;
        }
    }

    static void EnsureAnimationGraphAssetLayout(
        AnimationUIState& uiState,
        [[maybe_unused]] const rendern::LevelAsset& level,
        const rendern::LevelNode& node,
        const rendern::AnimationControllerAsset& controllerAsset)
    {
        const std::string controllerId = controllerAsset.id.empty() ? std::string("<controller>") : controllerAsset.id;

        const std::string selectedNodeKey = AnimationGraphMakeKey("asset", controllerId, std::string("node:") + node.name);
        if (!uiState.animationGraphAssetNodePositions.contains(selectedNodeKey))
        {
            uiState.animationGraphAssetNodePositions.emplace(selectedNodeKey, ImVec2(40.0f, 110.0f));
        }

        const std::string controllerKey = AnimationGraphMakeKey("asset", controllerId, std::string("controller:") + controllerId);
        if (!uiState.animationGraphAssetNodePositions.contains(controllerKey))
        {
            uiState.animationGraphAssetNodePositions.emplace(controllerKey, ImVec2(340.0f, 110.0f));
        }

        if (!controllerAsset.notifyAssetPath.empty())
        {
            const std::string notifyKey = AnimationGraphMakeKey("asset", controllerId, std::string("notify:") + controllerAsset.notifyAssetPath);
            if (!uiState.animationGraphAssetNodePositions.contains(notifyKey))
            {
                uiState.animationGraphAssetNodePositions.emplace(notifyKey, ImVec2(650.0f, 35.0f));
            }
        }

        if (!controllerAsset.eventBindingsAssetPath.empty())
        {
            const std::string bindingsKey = AnimationGraphMakeKey("asset", controllerId, std::string("bindings:") + controllerAsset.eventBindingsAssetPath);
            if (!uiState.animationGraphAssetNodePositions.contains(bindingsKey))
            {
                uiState.animationGraphAssetNodePositions.emplace(bindingsKey, ImVec2(650.0f, 185.0f));
            }
        }

        const std::string clipsKey = AnimationGraphMakeKey("asset", controllerId, "clips");
        if (!uiState.animationGraphAssetNodePositions.contains(clipsKey))
        {
            uiState.animationGraphAssetNodePositions.emplace(clipsKey, ImVec2(965.0f, 110.0f));
        }
    }

    struct AnimationGraphClipReference
    {
        std::string clipId;
        bool exists = false;
        std::vector<std::string> stateNames;
    };

    [[nodiscard]] static std::vector<AnimationGraphClipReference> BuildAnimationGraphClipReferences(
        const rendern::LevelAsset& level,
        const rendern::AnimationControllerAsset& controllerAsset)
    {
        std::vector<AnimationGraphClipReference> refs;

        auto FindOrCreateReference = [&](std::string_view clipId) -> AnimationGraphClipReference&
            {
                for (AnimationGraphClipReference& ref : refs)
                {
                    if (ref.clipId == clipId)
                    {
                        return ref;
                    }
                }

                refs.push_back(AnimationGraphClipReference{});
                AnimationGraphClipReference& ref = refs.back();
                ref.clipId = clipId;
                ref.exists = level.animations.contains(std::string(clipId));
                return ref;
            };

        for (const rendern::AnimationStateDesc& state : controllerAsset.states)
        {
            auto RegisterUsage = [&](std::string_view clipId)
                {
                    if (clipId.empty())
                    {
                        return;
                    }

                    AnimationGraphClipReference& ref = FindOrCreateReference(clipId);
                    if (!AnimationGraphContainsString(ref.stateNames, state.name))
                    {
                        ref.stateNames.push_back(state.name);
                    }
                };

            RegisterUsage(state.clipSourceAssetId);
            for (const rendern::AnimationBlend2DPoint& point : state.blend2D)
            {
                RegisterUsage(point.clipSourceAssetId);
            }
        }

        return refs;
    }

    struct AnimationGraphDiagnostic
    {
        bool warning = true;
        std::string text;
    };

    [[nodiscard]] static std::vector<AnimationGraphDiagnostic> BuildAnimationGraphDiagnostics(
        const rendern::LevelAsset& level,
        const rendern::AnimationControllerAsset& controllerAsset)
    {
        std::vector<AnimationGraphDiagnostic> out;
        std::vector<std::string> stateNames;
        stateNames.reserve(controllerAsset.states.size());
        for (const rendern::AnimationStateDesc& state : controllerAsset.states)
        {
            stateNames.push_back(state.name);
        }

        std::vector<std::string> notifyIds;
        for (const rendern::AnimationStateDesc& state : controllerAsset.states)
        {
            if (!state.clipSourceAssetId.empty() && !level.animations.contains(state.clipSourceAssetId))
            {
                out.push_back(AnimationGraphDiagnostic{ false, "Missing clip asset for state '" + state.name + "': " + state.clipSourceAssetId });
            }
            for (const rendern::AnimationBlend2DPoint& point : state.blend2D)
            {
                if (!point.clipSourceAssetId.empty() && !level.animations.contains(point.clipSourceAssetId))
                {
                    out.push_back(AnimationGraphDiagnostic{ false, "Missing blend2D clip asset in state '" + state.name + "': " + point.clipSourceAssetId });
                }
            }
            for (const rendern::AnimationNotifyDesc& notify : state.notifies)
            {
                if (!AnimationGraphContainsString(notifyIds, notify.id))
                {
                    notifyIds.push_back(notify.id);
                }
            }
        }

        std::vector<std::string> bindingIds;
        for (const rendern::AnimationEventBindingDesc& binding : controllerAsset.eventBindings)
        {
            if (!AnimationGraphContainsString(bindingIds, binding.animationEventId))
            {
                bindingIds.push_back(binding.animationEventId);
            }
        }

        for (const rendern::AnimationTransitionDesc& transition : controllerAsset.transitions)
        {
            if (!AnimationGraphContainsString(stateNames, transition.fromState))
            {
                out.push_back(AnimationGraphDiagnostic{ false, "Transition source state missing: " + transition.fromState + " -> " + transition.toState });
            }
            if (!AnimationGraphContainsString(stateNames, transition.toState))
            {
                out.push_back(AnimationGraphDiagnostic{ false, "Transition target state missing: " + transition.fromState + " -> " + transition.toState });
            }
        }

        for (const std::string& notifyId : notifyIds)
        {
            if (!AnimationGraphContainsString(bindingIds, notifyId))
            {
                out.push_back(AnimationGraphDiagnostic{ true, "Notify event has no gameplay binding: " + notifyId });
            }
        }

        for (const std::string& bindingId : bindingIds)
        {
            if (!AnimationGraphContainsString(notifyIds, bindingId))
            {
                out.push_back(AnimationGraphDiagnostic{ true, "Gameplay binding has no source notify: " + bindingId });
            }
        }

        for (std::size_t i = 0; i < controllerAsset.transitions.size(); ++i)
        {
            for (std::size_t j = i + 1; j < controllerAsset.transitions.size(); ++j)
            {
                const rendern::AnimationTransitionDesc& a = controllerAsset.transitions[i];
                const rendern::AnimationTransitionDesc& b = controllerAsset.transitions[j];
                if (a.fromState == b.toState && a.toState == b.fromState)
                {
                    const std::string condA = AnimationGraphConditionsSummary(a.conditions);
                    const std::string condB = AnimationGraphConditionsSummary(b.conditions);
                    if (condA == condB)
                    {
                        out.push_back(AnimationGraphDiagnostic{
                            true,
                            "Reciprocal transitions share identical conditions: " + a.fromState + " <-> " + a.toState
                        });
                    }
                }
                if (a.fromState == b.fromState && a.priority == b.priority)
                {
                    out.push_back(AnimationGraphDiagnostic{
                        true,
                        "Transitions from '" + a.fromState + "' share the same priority " + std::to_string(a.priority)
                    });
                }
            }
        }

        if (!controllerAsset.defaultState.empty())
        {
            std::vector<std::string> reachable;
            reachable.push_back(controllerAsset.defaultState);
            bool changed = true;
            while (changed)
            {
                changed = false;
                for (const rendern::AnimationTransitionDesc& transition : controllerAsset.transitions)
                {
                    if (AnimationGraphContainsString(reachable, transition.fromState) &&
                        !AnimationGraphContainsString(reachable, transition.toState))
                    {
                        reachable.push_back(transition.toState);
                        changed = true;
                    }
                }
            }

            for (const rendern::AnimationStateDesc& state : controllerAsset.states)
            {
                if (!AnimationGraphContainsString(reachable, state.name))
                {
                    out.push_back(AnimationGraphDiagnostic{ true, "State not reachable from defaultState: " + state.name });
                }
            }
        }

        return out;
    }


    [[nodiscard]] static float AnimationRuntimeGetNormalizedTime(const rendern::AnimatorState& animator) noexcept
    {
        if (animator.clip == nullptr || animator.clip->ticksPerSecond <= 0.0f)
        {
            return 0.0f;
        }
        const float durationSeconds = animator.clip->durationTicks / animator.clip->ticksPerSecond;
        if (durationSeconds <= 1e-6f)
        {
            return 0.0f;
        }
        const float normalized = animator.timeSeconds / durationSeconds;
        if (animator.looping)
        {
            const float wrapped = normalized - std::floor(normalized);
            return std::clamp(wrapped, 0.0f, 1.0f);
        }
        return std::clamp(normalized, 0.0f, 1.0f);
    }

    [[nodiscard]] static const rendern::AnimationStateDesc* FindAnimationRuntimeStateDesc(
        const rendern::AnimationControllerRuntime& runtime) noexcept
    {
        if (runtime.stateMachineAsset == nullptr ||
            runtime.currentStateIndex < 0 ||
            static_cast<std::size_t>(runtime.currentStateIndex) >= runtime.stateMachineAsset->states.size())
        {
            return nullptr;
        }
        return &runtime.stateMachineAsset->states[static_cast<std::size_t>(runtime.currentStateIndex)];
    }

    [[nodiscard]] static const rendern::AnimationStateDesc* FindAnimationRuntimeBlend2DPreviewState(
        const rendern::AnimationControllerRuntime& runtime) noexcept
    {
        const rendern::AnimationStateDesc* currentState = FindAnimationRuntimeStateDesc(runtime);
        if (currentState != nullptr && !currentState->blend2D.empty())
        {
            return currentState;
        }

        if (runtime.stateMachineAsset == nullptr)
        {
            return nullptr;
        }

        for (const rendern::AnimationStateDesc& state : runtime.stateMachineAsset->states)
        {
            if (!state.blendParameterX.empty() &&
                !state.blendParameterY.empty() &&
                !state.blend2D.empty())
            {
                return &state;
            }
        }

        return nullptr;
    }
