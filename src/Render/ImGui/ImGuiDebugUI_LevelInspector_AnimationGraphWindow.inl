
namespace rendern::ui::level_ui_detail
{
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

    struct AnimationGraphContext
    {
        int nodeIndex = -1;
        rendern::LevelNode* node = nullptr;
        rendern::SkinnedDrawItem* skinnedItem = nullptr;
        const rendern::AnimationControllerAsset* controllerAsset = nullptr;
    };

    [[nodiscard]] static AnimationGraphContext GetAnimationGraphContext(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        rendern::Scene& scene,
        LevelEditorUIState& st)
    {
        AnimationGraphContext ctx{};
        if (!NodeAlive(level, st.selectedNode))
        {
            return ctx;
        }

        rendern::LevelNode& node = level.nodes[static_cast<std::size_t>(st.selectedNode)];
        if (node.skinnedMesh.empty())
        {
            return ctx;
        }

        const int drawIndex = levelInst.GetNodeSkinnedDrawIndex(st.selectedNode);
        rendern::SkinnedDrawItem* skinnedItem = levelInst.GetSkinnedDrawItem(scene, drawIndex);
        if (skinnedItem == nullptr)
        {
            return ctx;
        }

        ctx.nodeIndex = st.selectedNode;
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
        if (!NodeAlive(level, nodeIndex))
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
        LevelEditorUIState& st,
        const rendern::GameplayRuntime* gameplayRuntime)
    {
        if (gameplayRuntime == nullptr || !st.selection.HasObservedRuntimeEntity())
        {
            return {};
        }

        const rendern::EntityHandle observedRuntimeEntity = st.selection.GetObservedRuntimeEntity();
        const rendern::GameplayWorld& gameplayWorld = gameplayRuntime->GetWorld();
        const rendern::GameplayNodeLinkComponent* nodeLink = gameplayWorld.IsEntityValid(observedRuntimeEntity)
            ? gameplayWorld.TryGetNodeLink(observedRuntimeEntity)
            : nullptr;
        const rendern::GameplayAnimationLinkComponent* animationLink = gameplayWorld.IsEntityValid(observedRuntimeEntity)
            ? gameplayWorld.TryGetAnimationLink(observedRuntimeEntity)
            : nullptr;
        if (nodeLink == nullptr || animationLink == nullptr)
        {
            st.selection.ClearObservedRuntimeEntity();
            st.animationRuntimeObservedEntityUnavailable = true;
            return {};
        }

        AnimationGraphContext observedContext = GetAnimationGraphContextForNodeIndex(level, levelInst, scene, nodeLink->nodeIndex);
        if (observedContext.node == nullptr || observedContext.skinnedItem == nullptr)
        {
            st.selection.ClearObservedRuntimeEntity();
            st.animationRuntimeObservedEntityUnavailable = true;
            return {};
        }

        st.animationRuntimePinnedNodeIndex = nodeLink->nodeIndex;
        st.animationRuntimeObservedEntityUnavailable = false;
        return observedContext;
    }

    static void SetAnimationRuntimePinnedTarget(
        LevelEditorUIState& st,
        const rendern::GameplayRuntime* gameplayRuntime,
        const int nodeIndex) noexcept
    {
        st.animationRuntimePinnedNodeIndex = nodeIndex;
        st.animationRuntimeObservedEntityUnavailable = false;
        if (nodeIndex < 0)
        {
            st.selection.ClearObservedRuntimeEntity();
            return;
        }

        const rendern::EntityHandle observedRuntimeEntity = FindAnimationRuntimeEntityForNodeIndex(gameplayRuntime, nodeIndex);
        if (observedRuntimeEntity != rendern::kNullEntity)
        {
            st.selection.SetObservedRuntimeEntity(observedRuntimeEntity);
        }
        else
        {
            st.selection.ClearObservedRuntimeEntity();
            st.animationRuntimeObservedEntityUnavailable = true;
        }
    }

    [[nodiscard]] static AnimationGraphContext GetAnimationRuntimeContext(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        rendern::Scene& scene,
        LevelEditorUIState& st,
        const rendern::GameplayRuntime* gameplayRuntime)
    {
        AnimationGraphContext observedContext = ResolveObservedAnimationRuntimeEntity(level, levelInst, scene, st, gameplayRuntime);
        if (observedContext.node != nullptr && observedContext.skinnedItem != nullptr)
        {
            return observedContext;
        }

        if (st.animationRuntimePinnedNodeIndex >= 0)
        {
            AnimationGraphContext pinned = GetAnimationGraphContextForNodeIndex(level, levelInst, scene, st.animationRuntimePinnedNodeIndex);
            if (pinned.node != nullptr && pinned.skinnedItem != nullptr)
            {
                const rendern::EntityHandle observedRuntimeEntity = FindAnimationRuntimeEntityForNodeIndex(gameplayRuntime, pinned.nodeIndex);
                if (observedRuntimeEntity != rendern::kNullEntity)
                {
                    st.selection.SetObservedRuntimeEntity(observedRuntimeEntity);
                    st.animationRuntimeObservedEntityUnavailable = false;
                }
                return pinned;
            }
            SetAnimationRuntimePinnedTarget(st, gameplayRuntime, -1);
        }

        AnimationGraphContext selected = GetAnimationGraphContext(level, levelInst, scene, st);
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
        LevelEditorUIState& st,
        const rendern::AnimationControllerAsset& controllerAsset)
    {
        std::unordered_map<std::string, int> categoryRows;
        for (const rendern::AnimationStateDesc& state : controllerAsset.states)
        {
            const char* category = AnimationGraphStateCategory(state);
            int& row = categoryRows[std::string(category)];
            const std::string key = AnimationGraphMakeKey("fsm", controllerAsset.id, state.name);
            if (!st.animationGraphFsmNodePositions.contains(key))
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
                st.animationGraphFsmNodePositions.emplace(key, ImVec2(x, 28.0f + static_cast<float>(row) * 110.0f));
            }
            ++row;
        }
    }

    static void EnsureAnimationGraphAssetLayout(
        LevelEditorUIState& st,
        [[maybe_unused]] const rendern::LevelAsset& level,
        const rendern::LevelNode& node,
        const rendern::AnimationControllerAsset& controllerAsset)
    {
        const std::string controllerId = controllerAsset.id.empty() ? std::string("<controller>") : controllerAsset.id;

        const std::string selectedNodeKey = AnimationGraphMakeKey("asset", controllerId, std::string("node:") + node.name);
        if (!st.animationGraphAssetNodePositions.contains(selectedNodeKey))
        {
            st.animationGraphAssetNodePositions.emplace(selectedNodeKey, ImVec2(40.0f, 110.0f));
        }

        const std::string controllerKey = AnimationGraphMakeKey("asset", controllerId, std::string("controller:") + controllerId);
        if (!st.animationGraphAssetNodePositions.contains(controllerKey))
        {
            st.animationGraphAssetNodePositions.emplace(controllerKey, ImVec2(340.0f, 110.0f));
        }

        if (!controllerAsset.notifyAssetPath.empty())
        {
            const std::string notifyKey = AnimationGraphMakeKey("asset", controllerId, std::string("notify:") + controllerAsset.notifyAssetPath);
            if (!st.animationGraphAssetNodePositions.contains(notifyKey))
            {
                st.animationGraphAssetNodePositions.emplace(notifyKey, ImVec2(650.0f, 35.0f));
            }
        }

        if (!controllerAsset.eventBindingsAssetPath.empty())
        {
            const std::string bindingsKey = AnimationGraphMakeKey("asset", controllerId, std::string("bindings:") + controllerAsset.eventBindingsAssetPath);
            if (!st.animationGraphAssetNodePositions.contains(bindingsKey))
            {
                st.animationGraphAssetNodePositions.emplace(bindingsKey, ImVec2(650.0f, 185.0f));
            }
        }

        const std::string clipsKey = AnimationGraphMakeKey("asset", controllerId, "clips");
        if (!st.animationGraphAssetNodePositions.contains(clipsKey))
        {
            st.animationGraphAssetNodePositions.emplace(clipsKey, ImVec2(965.0f, 110.0f));
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

    static void DrawAnimationGraphLegend()
    {
        ImGui::TextDisabled("Legend:");
        ImGui::SameLine(); ImGui::ColorButton("##idleLegend", ImColor(85, 105, 150, 255), ImGuiColorEditFlags_NoTooltip, ImVec2(12.0f, 12.0f));
        ImGui::SameLine(); ImGui::TextUnformatted("Idle");
        ImGui::SameLine(); ImGui::ColorButton("##locLegend", ImColor(80, 130, 95, 255), ImGuiColorEditFlags_NoTooltip, ImVec2(12.0f, 12.0f));
        ImGui::SameLine(); ImGui::TextUnformatted("Locomotion");
        ImGui::SameLine(); ImGui::ColorButton("##turnLegend", ImColor(135, 110, 70, 255), ImGuiColorEditFlags_NoTooltip, ImVec2(12.0f, 12.0f));
        ImGui::SameLine(); ImGui::TextUnformatted("Turn");
        ImGui::SameLine(); ImGui::ColorButton("##actionLegend", ImColor(140, 85, 85, 255), ImGuiColorEditFlags_NoTooltip, ImVec2(12.0f, 12.0f));
        ImGui::SameLine(); ImGui::TextUnformatted("Action");
        ImGui::SameLine(); ImGui::ColorButton("##curLegend", ImColor(100, 180, 255, 255), ImGuiColorEditFlags_NoTooltip, ImVec2(12.0f, 12.0f));
        ImGui::SameLine(); ImGui::TextUnformatted("Current");
    }


    static void DrawAnimationGraphBlend1DPreview(
        const rendern::AnimationStateDesc& state,
        const rendern::AnimationControllerRuntime& runtime)
    {
        if (state.blend1D.empty())
        {
            return;
        }

        ImGui::SeparatorText("Blend1D Preview");
        ImGui::Text("Parameter: %s", state.blendParameter.c_str());

        float minValue = state.blend1D.front().value;
        float maxValue = state.blend1D.front().value;
        for (const rendern::AnimationBlend1DPoint& point : state.blend1D)
        {
            minValue = std::min(minValue, point.value);
            maxValue = std::max(maxValue, point.value);
        }
        if (std::fabs(maxValue - minValue) <= 1e-6f)
        {
            minValue -= 1.0f;
            maxValue += 1.0f;
        }

        const bool showRuntimeMarker =
            runtime.currentStateName == state.name &&
            runtime.currentStateUsesBlend1D &&
            runtime.currentBlendParameterName == state.blendParameter;

        const float currentValue = runtime.currentBlendParameterValue;

        const float width = std::max(180.0f, ImGui::GetContentRegionAvail().x - 10.0f);
        const float height = 96.0f;
        ImGui::BeginChild("##AnimationGraphBlend1DPreview", ImVec2(0.0f, height), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(canvasPos, AddImVec2(canvasPos, canvasSize), true);
        drawList->AddRectFilled(canvasPos, AddImVec2(canvasPos, canvasSize), IM_COL32(22, 22, 26, 255), 6.0f);

        const float left = 28.0f;
        const float right = canvasSize.x - 18.0f;
        const float midY = canvasSize.y * 0.58f;
        drawList->AddLine(
            AddImVec2(canvasPos, ImVec2(left, midY)),
            AddImVec2(canvasPos, ImVec2(right, midY)),
            IM_COL32(150, 150, 158, 255),
            2.0f);

        auto ValueToX = [&](float value) -> float
        {
            const float t = std::clamp((value - minValue) / (maxValue - minValue), 0.0f, 1.0f);
            return left + (right - left) * t;
        };

        for (const rendern::AnimationBlend1DPoint& point : state.blend1D)
        {
            const float x = ValueToX(point.value);
            const ImVec2 markerPos = AddImVec2(canvasPos, ImVec2(x, midY));
            drawList->AddCircleFilled(markerPos, 5.0f, IM_COL32(110, 190, 255, 255));
            drawList->AddLine(
                AddImVec2(markerPos, ImVec2(0.0f, -14.0f)),
                AddImVec2(markerPos, ImVec2(0.0f, 14.0f)),
                IM_COL32(80, 120, 160, 180),
                1.0f);

            char valueBuf[32]{};
            std::snprintf(valueBuf, sizeof(valueBuf), "%.2f", point.value);
            drawList->AddText(AddImVec2(markerPos, ImVec2(-12.0f, 10.0f)), IM_COL32(210, 210, 215, 255), valueBuf);
            drawList->AddText(
                AddImVec2(markerPos, ImVec2(-18.0f, -24.0f)),
                IM_COL32(230, 230, 235, 255),
                point.clipName.c_str());
        }

        if (showRuntimeMarker)
        {
            const float x = ValueToX(currentValue);
            const ImVec2 markerPos = AddImVec2(canvasPos, ImVec2(x, midY));
            drawList->AddTriangleFilled(
                AddImVec2(markerPos, ImVec2(0.0f, -20.0f)),
                AddImVec2(markerPos, ImVec2(-7.0f, -8.0f)),
                AddImVec2(markerPos, ImVec2(7.0f, -8.0f)),
                IM_COL32(255, 220, 90, 255));

            char currentBuf[64]{};
            std::snprintf(currentBuf, sizeof(currentBuf), "runtime %.2f", currentValue);
            drawList->AddText(AddImVec2(markerPos, ImVec2(10.0f, -24.0f)), IM_COL32(255, 220, 90, 255), currentBuf);
        }

        drawList->PopClipRect();
        ImGui::EndChild();

        for (const rendern::AnimationBlend1DPoint& point : state.blend1D)
        {
            ImGui::BulletText("%.2f -> %s", point.value, point.clipName.c_str());
        }
    }

    static void DrawAnimationGraphBlend2DPreview(
        const rendern::AnimationStateDesc& state,
        const rendern::AnimationControllerRuntime& runtime,
        LevelEditorUIState& st)
    {
        if (state.blend2D.empty())
        {
            return;
        }

        ImGui::SeparatorText("Blend2D Preview");
        ImGui::Text("X: %s   Y: %s", state.blendParameterX.c_str(), state.blendParameterY.c_str());

        if (ImGui::Button("Fit Blend2D"))
        {
            st.animationGraphBlend2DZoom = 1.0f;
            st.animationGraphBlend2DPan = ImVec2(0.0f, 0.0f);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(170.0f);
        ImGui::SliderFloat("##AnimationGraphBlend2DZoom", &st.animationGraphBlend2DZoom, 0.45f, 2.50f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
        st.animationGraphBlend2DZoom = AnimationGraphClampZoom(st.animationGraphBlend2DZoom);

        const ImVec2 previewAvail = ImGui::GetContentRegionAvail();
        const float width = std::max(220.0f, previewAvail.x);
        const float height = std::clamp(width * 0.80f, 240.0f, 360.0f);

        ImGui::BeginChild("##AnimationGraphBlend2DPreview", ImVec2(0.0f, height), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(canvasPos, AddImVec2(canvasPos, canvasSize), true);
        drawList->AddRectFilled(canvasPos, AddImVec2(canvasPos, canvasSize), IM_COL32(22, 22, 26, 255), 6.0f);

        ImGui::InvisibleButton("##AnimationGraphBlend2DCanvasButton", canvasSize);
        const bool hovered = ImGui::IsItemHovered();
        if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
        {
            st.animationGraphBlend2DPan = AddImVec2(st.animationGraphBlend2DPan, ImGui::GetIO().MouseDelta);
        }
        AnimationGraphHandleCanvasZoom(st.animationGraphBlend2DZoom, st.animationGraphBlend2DPan, canvasPos, canvasSize, hovered);

        float minX = state.blend2D.front().x;
        float maxX = state.blend2D.front().x;
        float minY = state.blend2D.front().y;
        float maxY = state.blend2D.front().y;
        for (const rendern::AnimationBlend2DPoint& point : state.blend2D)
        {
            minX = std::min(minX, point.x);
            maxX = std::max(maxX, point.x);
            minY = std::min(minY, point.y);
            maxY = std::max(maxY, point.y);
        }

        if (std::fabs(maxX - minX) <= 1e-6f)
        {
            minX -= 1.0f;
            maxX += 1.0f;
        }
        if (std::fabs(maxY - minY) <= 1e-6f)
        {
            minY -= 1.0f;
            maxY += 1.0f;
        }

        minX = std::min(minX, 0.0f);
        maxX = std::max(maxX, 0.0f);
        minY = std::min(minY, 0.0f);
        maxY = std::max(maxY, 0.0f);

        const float paddingX = std::max(0.10f * (maxX - minX), 0.15f);
        const float paddingY = std::max(0.10f * (maxY - minY), 0.15f);
        minX -= paddingX;
        maxX += paddingX;
        minY -= paddingY;
        maxY += paddingY;

        const ImVec2 plotMin(44.0f, 28.0f);
        const ImVec2 plotMax(std::max(plotMin.x + 10.0f, canvasSize.x - 20.0f), std::max(plotMin.y + 10.0f, canvasSize.y - 24.0f));
        const ImVec2 plotSize = SubImVec2(plotMax, plotMin);

        auto PlotPointFromBlend = [&](float x, float y) -> ImVec2
        {
            const float tx = std::clamp((x - minX) / (maxX - minX), 0.0f, 1.0f);
            const float ty = std::clamp((y - minY) / (maxY - minY), 0.0f, 1.0f);
            return ImVec2(
                plotMin.x + tx * std::max(1.0f, plotSize.x),
                plotMin.y + (1.0f - ty) * std::max(1.0f, plotSize.y));
        };

        auto ToScreen = [&](const ImVec2& localPoint) -> ImVec2
        {
            return AnimationGraphCanvasPointToScreen(canvasPos, st.animationGraphBlend2DPan, st.animationGraphBlend2DZoom, localPoint);
        };

        const ImVec2 plotMinScreen = ToScreen(plotMin);
        const ImVec2 plotMaxScreen = ToScreen(plotMax);
        drawList->AddRectFilled(plotMinScreen, plotMaxScreen, IM_COL32(18, 18, 22, 245), 4.0f);
        drawList->AddRect(plotMinScreen, plotMaxScreen, IM_COL32(80, 86, 98, 255), 4.0f, 0, 1.2f);

        const float tickValues[] = { -1.0f, -0.5f, 0.0f, 0.5f, 1.0f };
        for (float tick : tickValues)
        {
            if (tick >= minX && tick <= maxX)
            {
                const ImVec2 p0 = ToScreen(PlotPointFromBlend(tick, minY));
                const ImVec2 p1 = ToScreen(PlotPointFromBlend(tick, maxY));
                const bool isZero = std::fabs(tick) <= 1e-6f;
                drawList->AddLine(p0, p1, isZero ? IM_COL32(132, 142, 164, 220) : IM_COL32(58, 62, 72, 165), isZero ? 1.8f : 1.0f);

                char tickBuf[16]{};
                std::snprintf(tickBuf, sizeof(tickBuf), "%.1f", tick);
                drawList->AddText(AddImVec2(p0, ImVec2(-8.0f, 4.0f)), IM_COL32(170, 175, 188, 200), tickBuf);
            }
            if (tick >= minY && tick <= maxY)
            {
                const ImVec2 p0 = ToScreen(PlotPointFromBlend(minX, tick));
                const ImVec2 p1 = ToScreen(PlotPointFromBlend(maxX, tick));
                const bool isZero = std::fabs(tick) <= 1e-6f;
                drawList->AddLine(p0, p1, isZero ? IM_COL32(132, 142, 164, 220) : IM_COL32(58, 62, 72, 165), isZero ? 1.8f : 1.0f);
            }
        }

        const ImVec2 xAxisLabelPos = AddImVec2(plotMaxScreen, ImVec2(-52.0f, 6.0f));
        const ImVec2 yAxisLabelPos = AddImVec2(plotMinScreen, ImVec2(8.0f, 6.0f));
        drawList->AddText(xAxisLabelPos, IM_COL32(210, 210, 215, 255), state.blendParameterX.c_str());
        drawList->AddText(yAxisLabelPos, IM_COL32(210, 210, 215, 255), state.blendParameterY.c_str());

        const ImVec2 originScreen = ToScreen(PlotPointFromBlend(0.0f, 0.0f));
        drawList->AddCircleFilled(originScreen, 2.5f, IM_COL32(190, 196, 208, 215));

        int pointIndex = 0;
        for (const rendern::AnimationBlend2DPoint& point : state.blend2D)
        {
            const ImVec2 screenPoint = ToScreen(PlotPointFromBlend(point.x, point.y));
            const float pointRadius = 5.5f * st.animationGraphBlend2DZoom;
            drawList->AddCircleFilled(screenPoint, pointRadius, IM_COL32(110, 190, 255, 255));
            drawList->AddCircle(screenPoint, pointRadius + 3.0f, IM_COL32(70, 104, 138, 220), 0, 1.2f);

            const char* clipId = point.clipSourceAssetId.empty() ? point.clipName.c_str() : point.clipSourceAssetId.c_str();
            std::string label = clipId;
            if (label.size() > 18)
            {
                label = label.substr(0, 15) + "...";
            }

            const ImVec2 labelOffset = (pointIndex % 2 == 0)
                ? ((point.x >= 0.0f) ? ImVec2(10.0f, -18.0f) : ImVec2(10.0f, 6.0f))
                : ((point.x >= 0.0f) ? ImVec2(-84.0f, -18.0f) : ImVec2(-84.0f, 6.0f));
            const ImVec2 labelPos = AddImVec2(screenPoint, labelOffset);
            const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
            drawList->AddRectFilled(
                AddImVec2(labelPos, ImVec2(-4.0f, -2.0f)),
                AddImVec2(labelPos, AddImVec2(textSize, ImVec2(4.0f, 2.0f))),
                IM_COL32(20, 20, 24, 210),
                4.0f);
            drawList->AddText(labelPos, IM_COL32(232, 234, 238, 245), label.c_str());

            const float hoverRadius = std::max(8.0f, pointRadius + 4.0f);
            if (hovered)
            {
                const ImVec2 mousePos = ImGui::GetIO().MousePos;
                const float dx = mousePos.x - screenPoint.x;
                const float dy = mousePos.y - screenPoint.y;
                if ((dx * dx + dy * dy) <= (hoverRadius * hoverRadius))
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", clipId);
                    ImGui::Separator();
                    ImGui::Text("X: %.2f", point.x);
                    ImGui::Text("Y: %.2f", point.y);
                    ImGui::EndTooltip();
                }
            }
            ++pointIndex;
        }

        const bool showRuntimeMarker =
            runtime.currentStateName == state.name &&
            runtime.currentStateUsesBlend2D &&
            runtime.currentBlendParameterName == state.blendParameterX &&
            runtime.currentBlendParameterNameY == state.blendParameterY;
        if (showRuntimeMarker)
        {
            const ImVec2 screenPoint = ToScreen(PlotPointFromBlend(runtime.currentBlendParameterValue, runtime.currentBlendParameterValueY));
            drawList->AddLine(AddImVec2(screenPoint, ImVec2(-10.0f, 0.0f)), AddImVec2(screenPoint, ImVec2(10.0f, 0.0f)), IM_COL32(255, 220, 90, 255), 2.0f);
            drawList->AddLine(AddImVec2(screenPoint, ImVec2(0.0f, -10.0f)), AddImVec2(screenPoint, ImVec2(0.0f, 10.0f)), IM_COL32(255, 220, 90, 255), 2.0f);
            drawList->AddCircle(screenPoint, 10.0f, IM_COL32(255, 220, 90, 140), 0, 2.0f);

            char valueBuf[96]{};
            std::snprintf(valueBuf, sizeof(valueBuf), "Current (%.2f, %.2f)", runtime.currentBlendParameterValue, runtime.currentBlendParameterValueY);
            const ImVec2 valuePos = AddImVec2(screenPoint, ImVec2(12.0f, -18.0f));
            const ImVec2 textSize = ImGui::CalcTextSize(valueBuf);
            drawList->AddRectFilled(
                AddImVec2(valuePos, ImVec2(-4.0f, -2.0f)),
                AddImVec2(valuePos, AddImVec2(textSize, ImVec2(4.0f, 2.0f))),
                IM_COL32(36, 30, 16, 220),
                4.0f);
            drawList->AddText(valuePos, IM_COL32(255, 220, 90, 255), valueBuf);
        }

        drawList->PopClipRect();
        ImGui::EndChild();

        ImGui::TextDisabled("MMB drag to pan, wheel to zoom. Hover a point for full clip id.");
        if (ImGui::CollapsingHeader("Blend Samples"))
        {
            for (const rendern::AnimationBlend2DPoint& point : state.blend2D)
            {
                const char* clipId = point.clipSourceAssetId.empty() ? point.clipName.c_str() : point.clipSourceAssetId.c_str();
                ImGui::BulletText("(%.2f, %.2f) -> %s", point.x, point.y, clipId);
            }
        }
    }

    static void DrawAnimationGraphStateInspector(
        const rendern::AnimationControllerAsset& controllerAsset,
        const rendern::AnimationControllerRuntime& runtime,
        LevelEditorUIState& st)
    {
        const rendern::AnimationStateDesc* selectedState = nullptr;
        if (!st.animationGraphSelectedStateName.empty())
        {
            selectedState = rendern::FindAnimationControllerState(controllerAsset, st.animationGraphSelectedStateName);
        }
        if (selectedState == nullptr && !controllerAsset.states.empty())
        {
            selectedState = &controllerAsset.states.front();
            st.animationGraphSelectedStateName = selectedState->name;
        }

        if (selectedState == nullptr)
        {
            ImGui::TextDisabled("No state selected.");
            return;
        }

        ImGui::SeparatorText("State Inspector");
        ImGui::Text("Name: %s", selectedState->name.c_str());
        ImGui::Text("Category: %s", AnimationGraphStateCategory(*selectedState));
        ImGui::Text("Loop: %s", selectedState->looping ? "true" : "false");
        ImGui::Text("Play rate: %.2f", selectedState->playRate);

        if (!selectedState->tags.empty())
        {
            ImGui::TextUnformatted("Tags:");
            for (const std::string& tag : selectedState->tags)
            {
                ImGui::BulletText("%s", tag.c_str());
            }
        }

        if (!selectedState->clipSourceAssetId.empty())
        {
            ImGui::Text("Clip source asset: %s", selectedState->clipSourceAssetId.c_str());
        }
        else if (!selectedState->clipName.empty())
        {
            ImGui::Text("Clip name: %s", selectedState->clipName.c_str());
        }

        if (!selectedState->blend1D.empty())
        {
            DrawAnimationGraphBlend1DPreview(*selectedState, runtime);
        }

        if (!selectedState->blend2D.empty())
        {
            DrawAnimationGraphBlend2DPreview(*selectedState, runtime, st);
        }

        if (!selectedState->notifies.empty())
        {
            ImGui::SeparatorText("Notify Timeline");
            const float width = std::max(120.0f, ImGui::GetContentRegionAvail().x - 20.0f);
            const float height = 56.0f;
            const ImVec2 p0 = ImGui::GetCursorScreenPos();
            const ImVec2 p1 = AddImVec2(p0, ImVec2(width, height));
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(p0, p1, IM_COL32(30, 30, 34, 255), 6.0f);
            drawList->AddLine(AddImVec2(p0, ImVec2(16.0f, height * 0.5f)), AddImVec2(p0, ImVec2(width - 16.0f, height * 0.5f)), IM_COL32(160, 160, 170, 255), 2.0f);
            for (std::size_t notifyIndex = 0; notifyIndex < selectedState->notifies.size(); ++notifyIndex)
            {
                const rendern::AnimationNotifyDesc& notify = selectedState->notifies[notifyIndex];
                const float t = std::clamp(notify.timeNormalized, 0.0f, 1.0f);
                const float x = 16.0f + t * (width - 32.0f);
                const ImVec2 marker = AddImVec2(p0, ImVec2(x, height * 0.5f));
                drawList->AddCircleFilled(marker, 5.0f, notify.fireOnEnter ? IM_COL32(255, 200, 90, 255) : IM_COL32(110, 190, 255, 255));
                drawList->AddText(AddImVec2(marker, ImVec2(-10.0f, -18.0f)), IM_COL32(220, 220, 220, 255), notify.id.c_str());
            }
            ImGui::Dummy(ImVec2(width, height));
        }

        ImGui::SeparatorText("Transitions");
        ImGui::TextUnformatted("Outgoing:");
        bool anyOutgoing = false;
        for (const rendern::AnimationTransitionDesc& transition : controllerAsset.transitions)
        {
            if (transition.fromState == selectedState->name)
            {
                anyOutgoing = true;
                ImGui::BulletText(
                    "-> %s | prio %d | blend %.2f | %s",
                    transition.toState.c_str(),
                    transition.priority,
                    transition.blendDurationSeconds,
                    AnimationGraphConditionsSummary(transition.conditions).c_str());
            }
        }
        if (!anyOutgoing)
        {
            ImGui::TextDisabled("None");
        }

        ImGui::TextUnformatted("Incoming:");
        bool anyIncoming = false;
        for (const rendern::AnimationTransitionDesc& transition : controllerAsset.transitions)
        {
            if (transition.toState == selectedState->name)
            {
                anyIncoming = true;
                ImGui::BulletText(
                    "%s -> | prio %d | blend %.2f",
                    transition.fromState.c_str(),
                    transition.priority,
                    transition.blendDurationSeconds);
            }
        }
        if (!anyIncoming)
        {
            ImGui::TextDisabled("None");
        }
    }

    static void DrawAnimationGraphFsmCanvas(
        const rendern::AnimationControllerAsset& controllerAsset,
        const rendern::AnimationControllerRuntime& runtime,
        LevelEditorUIState& st)
    {
        EnsureAnimationGraphFsmLayout(st, controllerAsset);

        if (ImGui::Button("Auto Layout"))
        {
            std::vector<std::string> keysToErase;
            keysToErase.reserve(controllerAsset.states.size());
            for (const rendern::AnimationStateDesc& state : controllerAsset.states)
            {
                keysToErase.push_back(AnimationGraphMakeKey("fsm", controllerAsset.id, state.name));
            }
            for (const std::string& key : keysToErase)
            {
                st.animationGraphFsmNodePositions.erase(key);
            }
            EnsureAnimationGraphFsmLayout(st, controllerAsset);
        }
        ImGui::SameLine();
        if (ImGui::Button("Fit"))
        {
            std::vector<ImVec2> positions;
            positions.reserve(controllerAsset.states.size());
            for (const rendern::AnimationStateDesc& state : controllerAsset.states)
            {
                const std::string key = AnimationGraphMakeKey("fsm", controllerAsset.id, state.name);
                positions.push_back(st.animationGraphFsmNodePositions[key]);
            }
            AnimationGraphFitView(positions, ImVec2(200.0f, 78.0f), ImVec2(std::max(200.0f, ImGui::GetContentRegionAvail().x - 360.0f), 440.0f), st.animationGraphFsmPan, st.animationGraphFsmZoom);
        }
        ImGui::SameLine();
        if (ImGui::Button("Center Current") && !runtime.currentStateName.empty())
        {
            const std::string key = AnimationGraphMakeKey("fsm", controllerAsset.id, runtime.currentStateName);
            auto it = st.animationGraphFsmNodePositions.find(key);
            if (it != st.animationGraphFsmNodePositions.end())
            {
                const ImVec2 canvasGuess(std::max(200.0f, ImGui::GetContentRegionAvail().x - 360.0f), 440.0f);
                const ImVec2 nodeCenter = AddImVec2(it->second, ImVec2(100.0f, 39.0f));
                st.animationGraphFsmPan = SubImVec2(MulImVec2(canvasGuess, 0.5f), MulImVec2(nodeCenter, st.animationGraphFsmZoom));
            }
        }
        ImGui::SameLine();
        ImGui::Checkbox("Focus selection", &st.animationGraphFsmFocusSelection);
        ImGui::SameLine();
        ImGui::Checkbox("Labels", &st.animationGraphShowTransitionLabels);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(170.0f);
        ImGui::SliderFloat("Zoom", &st.animationGraphFsmZoom, 0.45f, 2.50f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
        st.animationGraphFsmZoom = AnimationGraphClampZoom(st.animationGraphFsmZoom);

        const float inspectorReserve = 350.0f;
        const float canvasHeight = 440.0f;
        const ImVec2 canvasSize = ImVec2(std::max(220.0f, ImGui::GetContentRegionAvail().x - inspectorReserve), canvasHeight);
        ImGui::BeginChild("##AnimationGraphFsmCanvas", canvasSize, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        const ImVec2 canvasViewSize = ImGui::GetContentRegionAvail();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(canvasPos, AddImVec2(canvasPos, canvasViewSize), true);
        drawList->AddRectFilled(canvasPos, AddImVec2(canvasPos, canvasViewSize), IM_COL32(22, 22, 26, 255), 6.0f);
        AnimationGraphDrawGrid(drawList, canvasPos, canvasViewSize, st.animationGraphFsmPan, st.animationGraphFsmZoom);

        const bool fsmCanvasHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        AnimationGraphHandleCanvasZoom(st.animationGraphFsmZoom, st.animationGraphFsmPan, canvasPos, canvasViewSize, fsmCanvasHovered);
        if (fsmCanvasHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
        {
            st.animationGraphFsmPan = AddImVec2(st.animationGraphFsmPan, ImGui::GetIO().MouseDelta);
        }

        struct CategoryLane
        {
            const char* name;
            float x;
            ImU32 fillColor;
        };
        constexpr CategoryLane lanes[] = {
            { "Idle", 20.0f, IM_COL32(70, 90, 135, 50) },
            { "Locomotion", 270.0f, IM_COL32(70, 115, 85, 54) },
            { "Turn", 520.0f, IM_COL32(120, 95, 60, 52) },
            { "Action", 770.0f, IM_COL32(125, 75, 75, 52) },
            { "Other", 1020.0f, IM_COL32(90, 90, 95, 40) },
        };
        const float laneWidth = 220.0f;
        for (const CategoryLane& lane : lanes)
        {
            const ImVec2 laneMin = AnimationGraphCanvasPointToScreen(canvasPos, st.animationGraphFsmPan, st.animationGraphFsmZoom, ImVec2(lane.x, 0.0f));
            const ImVec2 laneMax = AnimationGraphCanvasPointToScreen(canvasPos, st.animationGraphFsmPan, st.animationGraphFsmZoom, ImVec2(lane.x + laneWidth, 2000.0f));
            drawList->AddRectFilled(
                ImVec2(laneMin.x, canvasPos.y),
                ImVec2(laneMax.x, canvasPos.y + canvasViewSize.y),
                lane.fillColor,
                0.0f);
            drawList->AddText(
                ImGui::GetFont(),
                std::max(11.0f, ImGui::GetFontSize() * 0.95f),
                AddImVec2(ImVec2(laneMin.x, canvasPos.y), ImVec2(10.0f, 8.0f)),
                IM_COL32(205, 205, 215, 220),
                lane.name);
        }

        const ImVec2 nodeBaseSize = ImVec2(200.0f, 78.0f);
        const float textScale = std::clamp(st.animationGraphFsmZoom, 0.85f, 1.35f);
        const float titleFontSize = std::max(12.0f, ImGui::GetFontSize() * (1.00f * textScale));
        const float bodyFontSize = std::max(11.0f, ImGui::GetFontSize() * (0.92f * textScale));

        auto StateRectMin = [&](const rendern::AnimationStateDesc& state) -> ImVec2
            {
                const std::string key = AnimationGraphMakeKey("fsm", controllerAsset.id, state.name);
                return AnimationGraphCanvasPointToScreen(canvasPos, st.animationGraphFsmPan, st.animationGraphFsmZoom, st.animationGraphFsmNodePositions[key]);
            };

        for (const rendern::AnimationTransitionDesc& transition : controllerAsset.transitions)
        {
            const rendern::AnimationStateDesc* fromState = rendern::FindAnimationControllerState(controllerAsset, transition.fromState);
            const rendern::AnimationStateDesc* toState = rendern::FindAnimationControllerState(controllerAsset, transition.toState);
            if (fromState == nullptr || toState == nullptr)
            {
                continue;
            }

            const bool touchesSelection =
                st.animationGraphSelectedStateName.empty() ||
                transition.fromState == st.animationGraphSelectedStateName ||
                transition.toState == st.animationGraphSelectedStateName;
            const bool emphasize = !st.animationGraphFsmFocusSelection || touchesSelection;
            const ImVec2 nodeSize = MulImVec2(nodeBaseSize, st.animationGraphFsmZoom);
            const ImVec2 fromMin = StateRectMin(*fromState);
            const ImVec2 toMin = StateRectMin(*toState);
            const ImVec2 p1 = AddImVec2(fromMin, ImVec2(nodeSize.x, nodeSize.y * 0.5f));
            const ImVec2 p4 = AddImVec2(toMin, ImVec2(0.0f, nodeSize.y * 0.5f));
            const float dx = std::max(70.0f, std::fabs(p4.x - p1.x) * 0.38f);
            const ImVec2 p2 = AddImVec2(p1, ImVec2(dx, 0.0f));
            const ImVec2 p3 = AddImVec2(p4, ImVec2(-dx, 0.0f));
            const bool activeEdge =
                runtime.transitionActive &&
                runtime.transitionSourceStateName == transition.fromState &&
                runtime.currentStateName == transition.toState;
            const ImU32 edgeColor = activeEdge
                ? IM_COL32(120, 205, 255, 255)
                : (emphasize ? IM_COL32(155, 155, 170, 190) : IM_COL32(95, 95, 105, 80));
            drawList->AddBezierCubic(p1, p2, p3, p4, edgeColor, activeEdge ? 3.2f : (emphasize ? 2.0f : 1.2f), 0);

            const ImVec2 arrowTip = p4;
            const float arrowWidth = std::max(6.0f, 8.0f * st.animationGraphFsmZoom);
            drawList->AddTriangleFilled(
                arrowTip,
                AddImVec2(arrowTip, ImVec2(-arrowWidth * 1.6f, -arrowWidth * 0.65f)),
                AddImVec2(arrowTip, ImVec2(-arrowWidth * 1.6f, arrowWidth * 0.65f)),
                edgeColor);

            const bool showLabel = activeEdge ||
                (emphasize && (st.animationGraphShowTransitionLabels || st.animationGraphFsmZoom >= 1.18f));
            if (showLabel)
            {
                const ImVec2 labelMid = MulImVec2(AddImVec2(p1, p4), 0.5f);
                const ImVec2 labelPos = AddImVec2(labelMid, ImVec2(-34.0f, -11.0f));
                char labelBuf[64]{};
                std::snprintf(labelBuf, sizeof(labelBuf), "p%d / %.2f", transition.priority, transition.blendDurationSeconds);
                drawList->AddRectFilled(
                    AddImVec2(labelPos, ImVec2(-4.0f, -2.0f)),
                    AddImVec2(labelPos, ImVec2(62.0f, 15.0f)),
                    IM_COL32(20, 20, 22, 180),
                    4.0f);
                drawList->AddText(ImGui::GetFont(), bodyFontSize, labelPos, IM_COL32(225, 225, 230, 230), labelBuf);
            }
        }

        for (const rendern::AnimationStateDesc& state : controllerAsset.states)
        {
            const std::string key = AnimationGraphMakeKey("fsm", controllerAsset.id, state.name);
            ImVec2& localPos = st.animationGraphFsmNodePositions[key];
            const ImVec2 nodeSize = MulImVec2(nodeBaseSize, st.animationGraphFsmZoom);
            const ImVec2 minPos = AnimationGraphCanvasPointToScreen(canvasPos, st.animationGraphFsmPan, st.animationGraphFsmZoom, localPos);
            const ImVec2 maxPos = AddImVec2(minPos, nodeSize);
            const bool isCurrent = runtime.currentStateName == state.name;
            const bool isSelected = st.animationGraphSelectedStateName == state.name;
            const bool selectedFocus = st.animationGraphSelectedStateName.empty() || isSelected;
            const ImU32 bodyColor = AnimationGraphStateColor(state, isCurrent);

            drawList->AddRectFilled(minPos, maxPos, bodyColor, 10.0f);
            drawList->AddRectFilled(
                minPos,
                AddImVec2(minPos, ImVec2(nodeSize.x, std::max(18.0f, 24.0f * st.animationGraphFsmZoom))),
                IM_COL32(255, 255, 255, isCurrent ? 22 : 12),
                10.0f);
            drawList->AddRect(
                minPos,
                maxPos,
                isSelected ? IM_COL32(255, 228, 120, 255) : IM_COL32(18, 18, 20, selectedFocus ? 255 : 180),
                10.0f,
                0,
                isSelected ? 3.0f : 1.2f);
            drawList->AddText(
                ImGui::GetFont(),
                titleFontSize,
                AddImVec2(minPos, MulImVec2(ImVec2(12.0f, 10.0f), st.animationGraphFsmZoom)),
                IM_COL32(255, 255, 255, 255),
                state.name.c_str());
            drawList->AddText(
                ImGui::GetFont(),
                bodyFontSize,
                AddImVec2(minPos, MulImVec2(ImVec2(12.0f, 38.0f), st.animationGraphFsmZoom)),
                IM_COL32(225, 225, 225, 240),
                AnimationGraphStateCategory(state));

            const std::string buttonId = "##anim_graph_state_" + state.name;
            ImGui::SetCursorScreenPos(minPos);
            ImGui::InvisibleButton(buttonId.c_str(), nodeSize);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                st.animationGraphSelectedStateName = state.name;
            }
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
            {
                localPos = AddImVec2(localPos, AnimationGraphScreenDeltaToGraphDelta(ImGui::GetIO().MouseDelta, st.animationGraphFsmZoom));
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("State: %s", state.name.c_str());
                ImGui::Text("Category: %s", AnimationGraphStateCategory(state));
                ImGui::Text("Loop: %s", state.looping ? "true" : "false");
                ImGui::Text("Play rate: %.2f", state.playRate);
                if (!state.blend2D.empty())
                {
                    ImGui::Text("Blend2D: %s / %s", state.blendParameterX.c_str(), state.blendParameterY.c_str());
                }
                else if (!state.clipSourceAssetId.empty())
                {
                    ImGui::Text("Clip asset: %s", state.clipSourceAssetId.c_str());
                }
                ImGui::EndTooltip();
            }
        }

        drawList->PopClipRect();
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("##AnimationGraphFsmInspector", ImVec2(0.0f, canvasHeight), true);
        DrawAnimationGraphLegend();
        ImGui::SeparatorText("Graph");
        ImGui::Text("States: %d", static_cast<int>(controllerAsset.states.size()));
        ImGui::Text("Transitions: %d", static_cast<int>(controllerAsset.transitions.size()));
        if (!runtime.currentStateName.empty())
        {
            ImGui::Text("Current: %s", runtime.currentStateName.c_str());
        }
        DrawAnimationGraphStateInspector(controllerAsset, runtime, st);
        ImGui::EndChild();
    }

    struct AnimationAssetNodeDef
    {
        std::string id;
        std::string title;
        std::string subtitle;
        ImU32 color{ IM_COL32(80, 80, 85, 255) };
    };

    static void DrawAnimationGraphAssetCanvas(
        const rendern::LevelAsset& level,
        const rendern::LevelNode& node,
        const rendern::AnimationControllerAsset& controllerAsset,
        LevelEditorUIState& st)
    {
        EnsureAnimationGraphAssetLayout(st, level, node, controllerAsset);

        const std::string controllerId = controllerAsset.id.empty() ? std::string("<controller>") : controllerAsset.id;
        const std::vector<AnimationGraphClipReference> clipRefs = BuildAnimationGraphClipReferences(level, controllerAsset);
        int missingClipCount = 0;
        for (const AnimationGraphClipReference& ref : clipRefs)
        {
            if (!ref.exists)
            {
                ++missingClipCount;
            }
        }

        std::vector<AnimationAssetNodeDef> nodes;
        nodes.push_back(AnimationAssetNodeDef{
            std::string("node:") + node.name,
            node.name.empty() ? std::string("<node>") : node.name,
            "Skinned node",
            IM_COL32(90, 95, 130, 255)
        });
        nodes.push_back(AnimationAssetNodeDef{
            std::string("controller:") + controllerId,
            controllerId,
            "Animation controller",
            IM_COL32(85, 125, 95, 255)
        });

        if (!controllerAsset.notifyAssetPath.empty())
        {
            nodes.push_back(AnimationAssetNodeDef{
                std::string("notify:") + controllerAsset.notifyAssetPath,
                "Notify Asset",
                controllerAsset.notifyAssetPath,
                IM_COL32(135, 105, 70, 255)
            });
        }
        if (!controllerAsset.eventBindingsAssetPath.empty())
        {
            nodes.push_back(AnimationAssetNodeDef{
                std::string("bindings:") + controllerAsset.eventBindingsAssetPath,
                "Bindings Asset",
                controllerAsset.eventBindingsAssetPath,
                IM_COL32(120, 85, 95, 255)
            });
        }

        char clipSummary[128]{};
        std::snprintf(
            clipSummary,
            sizeof(clipSummary),
            "%d clip assets%s",
            static_cast<int>(clipRefs.size()),
            missingClipCount > 0 ? " (missing present)" : "");
        nodes.push_back(AnimationAssetNodeDef{
            "clips",
            "Clip Assets",
            clipSummary,
            missingClipCount > 0 ? IM_COL32(145, 80, 80, 255) : IM_COL32(80, 100, 140, 255)
        });

        if (ImGui::Button("Auto Layout"))
        {
            std::vector<std::string> keysToErase;
            for (const AnimationAssetNodeDef& visualNode : nodes)
            {
                keysToErase.push_back(AnimationGraphMakeKey("asset", controllerId, visualNode.id));
            }
            for (const std::string& key : keysToErase)
            {
                st.animationGraphAssetNodePositions.erase(key);
            }
            EnsureAnimationGraphAssetLayout(st, level, node, controllerAsset);
        }
        ImGui::SameLine();
        if (ImGui::Button("Fit"))
        {
            std::vector<ImVec2> positions;
            positions.reserve(nodes.size());
            for (const AnimationAssetNodeDef& visualNode : nodes)
            {
                positions.push_back(st.animationGraphAssetNodePositions[AnimationGraphMakeKey("asset", controllerId, visualNode.id)]);
            }
            AnimationGraphFitView(positions, ImVec2(220.0f, 76.0f), ImVec2(std::max(200.0f, ImGui::GetContentRegionAvail().x), 280.0f), st.animationGraphAssetPan, st.animationGraphAssetZoom);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(170.0f);
        ImGui::SliderFloat("Zoom", &st.animationGraphAssetZoom, 0.45f, 2.50f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
        st.animationGraphAssetZoom = AnimationGraphClampZoom(st.animationGraphAssetZoom);

        const ImVec2 nodeBaseSize = ImVec2(220.0f, 76.0f);
        ImGui::BeginChild("##AnimationGraphAssetCanvas", ImVec2(0.0f, 280.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(canvasPos, AddImVec2(canvasPos, canvasSize), true);
        drawList->AddRectFilled(canvasPos, AddImVec2(canvasPos, canvasSize), IM_COL32(22, 22, 26, 255), 6.0f);
        AnimationGraphDrawGrid(drawList, canvasPos, canvasSize, st.animationGraphAssetPan, st.animationGraphAssetZoom);

        const bool assetCanvasHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        AnimationGraphHandleCanvasZoom(st.animationGraphAssetZoom, st.animationGraphAssetPan, canvasPos, canvasSize, assetCanvasHovered);
        if (assetCanvasHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
        {
            st.animationGraphAssetPan = AddImVec2(st.animationGraphAssetPan, ImGui::GetIO().MouseDelta);
        }

        auto AssetNodeMin = [&](std::string_view localId) -> ImVec2
            {
                const std::string key = AnimationGraphMakeKey("asset", controllerId, localId);
                return AnimationGraphCanvasPointToScreen(canvasPos, st.animationGraphAssetPan, st.animationGraphAssetZoom, st.animationGraphAssetNodePositions[key]);
            };

        auto DrawEdge = [&](std::string_view fromId, std::string_view toId, ImU32 color)
            {
                const ImVec2 nodeSize = MulImVec2(nodeBaseSize, st.animationGraphAssetZoom);
                const ImVec2 fromMin = AssetNodeMin(fromId);
                const ImVec2 toMin = AssetNodeMin(toId);
                const ImVec2 p1 = AddImVec2(fromMin, ImVec2(nodeSize.x, nodeSize.y * 0.5f));
                const ImVec2 p4 = AddImVec2(toMin, ImVec2(0.0f, nodeSize.y * 0.5f));
                const float dx = std::max(55.0f, std::fabs(p4.x - p1.x) * 0.35f);
                drawList->AddBezierCubic(
                    p1,
                    AddImVec2(p1, ImVec2(dx, 0.0f)),
                    AddImVec2(p4, ImVec2(-dx, 0.0f)),
                    p4,
                    color,
                    2.0f,
                    0);
                const float arrowWidth = std::max(6.0f, 8.0f * st.animationGraphAssetZoom);
                drawList->AddTriangleFilled(
                    p4,
                    AddImVec2(p4, ImVec2(-arrowWidth * 1.5f, -arrowWidth * 0.65f)),
                    AddImVec2(p4, ImVec2(-arrowWidth * 1.5f, arrowWidth * 0.65f)),
                    color);
            };

        DrawEdge(std::string("node:") + node.name, std::string("controller:") + controllerId, IM_COL32(140, 140, 150, 180));
        if (!controllerAsset.notifyAssetPath.empty())
        {
            DrawEdge(std::string("controller:") + controllerId, std::string("notify:") + controllerAsset.notifyAssetPath, IM_COL32(175, 150, 110, 180));
        }
        if (!controllerAsset.eventBindingsAssetPath.empty())
        {
            DrawEdge(std::string("controller:") + controllerId, std::string("bindings:") + controllerAsset.eventBindingsAssetPath, IM_COL32(170, 130, 145, 180));
        }
        DrawEdge(std::string("controller:") + controllerId, "clips", IM_COL32(125, 165, 205, 190));

        const float titleFontSize = std::max(12.0f, ImGui::GetFontSize() * std::clamp(st.animationGraphAssetZoom, 0.85f, 1.30f));
        const float bodyFontSize = std::max(11.0f, ImGui::GetFontSize() * std::clamp(st.animationGraphAssetZoom, 0.85f, 1.20f));

        for (const AnimationAssetNodeDef& visualNode : nodes)
        {
            const std::string key = AnimationGraphMakeKey("asset", controllerId, visualNode.id);
            ImVec2& localPos = st.animationGraphAssetNodePositions[key];
            const ImVec2 nodeSize = MulImVec2(nodeBaseSize, st.animationGraphAssetZoom);
            const ImVec2 minPos = AnimationGraphCanvasPointToScreen(canvasPos, st.animationGraphAssetPan, st.animationGraphAssetZoom, localPos);
            const ImVec2 maxPos = AddImVec2(minPos, nodeSize);

            drawList->AddRectFilled(minPos, maxPos, visualNode.color, 10.0f);
            drawList->AddRectFilled(
                minPos,
                AddImVec2(minPos, ImVec2(nodeSize.x, std::max(18.0f, 22.0f * st.animationGraphAssetZoom))),
                IM_COL32(255, 255, 255, 12),
                10.0f);
            drawList->AddRect(minPos, maxPos, IM_COL32(18, 18, 20, 255), 10.0f, 0, 1.2f);
            drawList->AddText(
                ImGui::GetFont(),
                titleFontSize,
                AddImVec2(minPos, MulImVec2(ImVec2(12.0f, 10.0f), st.animationGraphAssetZoom)),
                IM_COL32(255, 255, 255, 255),
                visualNode.title.c_str());
            drawList->AddText(
                ImGui::GetFont(),
                bodyFontSize,
                AddImVec2(minPos, MulImVec2(ImVec2(12.0f, 38.0f), st.animationGraphAssetZoom)),
                IM_COL32(225, 225, 225, 240),
                visualNode.subtitle.c_str());

            if (visualNode.id == "clips")
            {
                const char* detail = missingClipCount > 0 ? "Inspect table below for missing refs" : "Inspect table below for usages";
                drawList->AddText(
                    ImGui::GetFont(),
                    std::max(10.0f, bodyFontSize - 1.0f),
                    AddImVec2(minPos, MulImVec2(ImVec2(12.0f, 56.0f), st.animationGraphAssetZoom)),
                    IM_COL32(210, 210, 210, 220),
                    detail);
            }

            const std::string buttonId = "##anim_asset_node_" + visualNode.id;
            ImGui::SetCursorScreenPos(minPos);
            ImGui::InvisibleButton(buttonId.c_str(), nodeSize);
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
            {
                localPos = AddImVec2(localPos, AnimationGraphScreenDeltaToGraphDelta(ImGui::GetIO().MouseDelta, st.animationGraphAssetZoom));
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(visualNode.title.c_str());
                ImGui::TextDisabled("%s", visualNode.subtitle.c_str());
                if (visualNode.id == "clips")
                {
                    ImGui::Text("Referenced clips: %d", static_cast<int>(clipRefs.size()));
                    ImGui::Text("Missing clips: %d", missingClipCount);
                }
                ImGui::EndTooltip();
            }
        }

        drawList->PopClipRect();
        ImGui::EndChild();

        ImGui::SeparatorText("Clip References");
        if (clipRefs.empty())
        {
            ImGui::TextDisabled("No clip asset references in this controller.");
            return;
        }

        if (ImGui::BeginTable("##AnimationGraphClipRefs", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Clip Asset");
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("Used By States");
            ImGui::TableHeadersRow();

            for (const AnimationGraphClipReference& ref : clipRefs)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(ref.clipId.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ref.exists ? ImVec4(0.55f, 0.88f, 0.55f, 1.0f) : ImVec4(1.0f, 0.45f, 0.45f, 1.0f), ref.exists ? "OK" : "Missing");
                ImGui::TableSetColumnIndex(2);
                std::string usageText;
                for (std::size_t i = 0; i < ref.stateNames.size(); ++i)
                {
                    if (i > 0)
                    {
                        usageText += ", ";
                    }
                    usageText += ref.stateNames[i];
                }
                ImGui::TextWrapped("%s", usageText.c_str());
            }

            ImGui::EndTable();
        }
    }

    static void DrawAnimationGraphBindings(const rendern::AnimationControllerAsset& controllerAsset)
    {
        if (controllerAsset.eventBindings.empty())
        {
            ImGui::TextDisabled("No gameplay bindings.");
            return;
        }

        if (ImGui::BeginTable("##AnimationGraphBindingsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Animation Event");
            ImGui::TableSetupColumn("Gameplay Event");
            ImGui::TableHeadersRow();

            for (const rendern::AnimationEventBindingDesc& binding : controllerAsset.eventBindings)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(binding.animationEventId.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(binding.gameplayEventId.c_str());
            }

            ImGui::EndTable();
        }
    }

    static void DrawAnimationGraphDiagnosticsPanel(
        const rendern::LevelAsset& level,
        const rendern::AnimationControllerAsset& controllerAsset)
    {
        const std::vector<AnimationGraphDiagnostic> diagnostics = BuildAnimationGraphDiagnostics(level, controllerAsset);
        if (diagnostics.empty())
        {
            ImGui::TextColored(ImVec4(0.55f, 0.9f, 0.55f, 1.0f), "No obvious issues found.");
            return;
        }

        for (const AnimationGraphDiagnostic& diagnostic : diagnostics)
        {
            const ImVec4 color = diagnostic.warning
                ? ImVec4(0.95f, 0.78f, 0.34f, 1.0f)
                : ImVec4(1.0f, 0.45f, 0.45f, 1.0f);
            ImGui::TextColored(color, "%s", diagnostic.warning ? "Warning" : "Error");
            ImGui::SameLine();
            ImGui::TextWrapped("%s", diagnostic.text.c_str());
        }
    }

    static void DrawAnimationGraphRuntimeSummary(const rendern::AnimationControllerRuntime& runtime)
    {
        ImGui::Text("Current state: %s", runtime.currentStateName.c_str());
        ImGui::Text("Mode: %s", runtime.currentStateUsesBlend2D ? "Blend2D" : (runtime.currentStateUsesBlend1D ? "Blend1D" : "Clip"));

        if (runtime.currentStateUsesBlend2D)
        {
            ImGui::Text(
                "Blend inputs: %s=%.2f   %s=%.2f",
                runtime.currentBlendParameterName.c_str(),
                runtime.currentBlendParameterValue,
                runtime.currentBlendParameterNameY.c_str(),
                runtime.currentBlendParameterValueY);
        }
        else if (runtime.currentStateUsesBlend1D)
        {
            ImGui::Text(
                "Blend input: %s=%.2f",
                runtime.currentBlendParameterName.c_str(),
                runtime.currentBlendParameterValue);
        }

        if (runtime.transitionActive)
        {
            const float alpha = (runtime.transitionDurationSeconds > 1e-6f)
                ? std::clamp(runtime.transitionElapsedSeconds / runtime.transitionDurationSeconds, 0.0f, 1.0f)
                : 1.0f;
            ImGui::Text("Transition: %s -> %s (%.2f)",
                runtime.transitionSourceStateName.c_str(),
                runtime.currentStateName.c_str(),
                alpha);
        }

        const auto& recentNotifies = rendern::PeekAnimationControllerNotifyEvents(runtime);
        if (!recentNotifies.empty())
        {
            ImGui::SeparatorText("Recent Notifies");
            const std::size_t firstNotify = recentNotifies.size() > 6 ? recentNotifies.size() - 6 : 0;
            for (std::size_t i = firstNotify; i < recentNotifies.size(); ++i)
            {
                const rendern::AnimationNotifyEvent& notify = recentNotifies[i];
                ImGui::BulletText("%s (%s @ %.2f)", notify.id.c_str(), notify.stateName.c_str(), notify.normalizedTime);
            }
        }
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

#include "ImGuiDebugUI_LevelInspector_AnimationRuntimeViewModel.inl"

    static void DrawAnimationRuntimeWeightedClips(const std::vector<AnimationRuntimeClipWeightViewModel>& clips)
    {
        if (clips.empty())
        {
            ImGui::TextDisabled("No active clip blend metadata.");
            return;
        }

        if (ImGui::BeginTable("##AnimationRuntimeWeights", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Clip");
            ImGui::TableSetupColumn("Weight", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Role", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableHeadersRow();

            for (const AnimationRuntimeClipWeightViewModel& clip : clips)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(clip.clipName.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f", clip.weight);
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(clip.role.c_str());
            }
            ImGui::EndTable();
        }
    }
    
    [[nodiscard]] static const char* AnimationRuntimeBlendSampleLabel(
        const std::string& clipName,
        const std::string& clipSourceAssetId) noexcept
    {
        return clipSourceAssetId.empty() ? clipName.c_str() : clipSourceAssetId.c_str();
    }

    static void DrawAnimationRuntimeBlend1DDisplayData(const AnimationRuntimeBlend1DViewModel& blend1DDisplayData)
    {
        if (!blend1DDisplayData.available || blend1DDisplayData.samples.empty())
        {
            ImGui::TextDisabled("No Blend1D display data available.");
            return;
        }

        ImGui::Text("Parameter: %s", blend1DDisplayData.parameterName.c_str());
        if (blend1DDisplayData.live)
        {
            ImGui::Text("Input: %s = %.3f", blend1DDisplayData.parameterName.c_str(), blend1DDisplayData.inputValue);
        }

        float minValue = blend1DDisplayData.samples.front().position;
        float maxValue = blend1DDisplayData.samples.front().position;
        for (const AnimationRuntimeBlend1DSampleViewModel& sample : blend1DDisplayData.samples)
        {
            minValue = std::min(minValue, sample.position);
            maxValue = std::max(maxValue, sample.position);
        }
        if (std::abs(maxValue - minValue) <= 1e-6f)
        {
            minValue -= 1.0f;
            maxValue += 1.0f;
        }

        const float height = 96.0f;
        ImGui::BeginChild("##AnimationRuntimeBlend1DPreview", ImVec2(0.0f, height), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(canvasPos, AddImVec2(canvasPos, canvasSize), true);
        drawList->AddRectFilled(canvasPos, AddImVec2(canvasPos, canvasSize), IM_COL32(22, 22, 26, 255), 6.0f);

        const float left = 28.0f;
        const float right = canvasSize.x - 18.0f;
        const float midY = canvasSize.y * 0.58f;
        drawList->AddLine(AddImVec2(canvasPos, ImVec2(left, midY)), AddImVec2(canvasPos, ImVec2(right, midY)), IM_COL32(150, 150, 158, 255), 2.0f);

        auto ValueToX = [&](float value) -> float
        {
            const float blendRange = std::max(maxValue - minValue, 1e-6f);
            const float t = std::clamp((value - minValue) / blendRange, 0.0f, 1.0f);
            return left + (right - left) * t;
        };

        for (const AnimationRuntimeBlend1DSampleViewModel& sample : blend1DDisplayData.samples)
        {
            const float x = ValueToX(sample.position);
            const ImVec2 markerPos = AddImVec2(canvasPos, ImVec2(x, midY));
            drawList->AddCircleFilled(markerPos, sample.active ? 6.5f : 5.0f, sample.active ? IM_COL32(255, 220, 90, 255) : IM_COL32(110, 190, 255, 255));
            drawList->AddLine(AddImVec2(markerPos, ImVec2(0.0f, -14.0f)), AddImVec2(markerPos, ImVec2(0.0f, 14.0f)), IM_COL32(80, 120, 160, 180), 1.0f);

            char valueBuffer[32]{};
            std::snprintf(valueBuffer, sizeof(valueBuffer), "%.2f", sample.position);
            drawList->AddText(AddImVec2(markerPos, ImVec2(-12.0f, 10.0f)), IM_COL32(210, 210, 215, 255), valueBuffer);
            drawList->AddText(AddImVec2(markerPos, ImVec2(-18.0f, -24.0f)), IM_COL32(230, 230, 235, 255), sample.clipName.c_str());
        }

        if (blend1DDisplayData.live)
        {
            const float x = ValueToX(blend1DDisplayData.inputValue);
            const ImVec2 markerPos = AddImVec2(canvasPos, ImVec2(x, midY));
            drawList->AddTriangleFilled(AddImVec2(markerPos, ImVec2(0.0f, -20.0f)), AddImVec2(markerPos, ImVec2(-7.0f, -8.0f)), AddImVec2(markerPos, ImVec2(7.0f, -8.0f)), IM_COL32(255, 220, 90, 255));
            char currentBuffer[64]{};
            std::snprintf(currentBuffer, sizeof(currentBuffer), "runtime %.2f", blend1DDisplayData.inputValue);
            drawList->AddText(AddImVec2(markerPos, ImVec2(10.0f, -24.0f)), IM_COL32(255, 220, 90, 255), currentBuffer);
        }

        drawList->PopClipRect();
        ImGui::EndChild();

        for (const AnimationRuntimeBlend1DSampleViewModel& sample : blend1DDisplayData.samples)
        {
            if (blend1DDisplayData.live)
            {
                ImGui::BulletText("%.2f -> %s (weight %.2f)", sample.position, sample.clipName.c_str(), sample.weight);
            }
            else
            {
                ImGui::BulletText("%.2f -> %s", sample.position, sample.clipName.c_str());
            }
        }
    }

    static void DrawAnimationRuntimeBlend2DDisplayData(
        const AnimationRuntimeBlend2DViewModel& blend2DDisplayData,
        LevelEditorUIState& st)
    {
        if (!blend2DDisplayData.available || blend2DDisplayData.samples.empty())
        {
            ImGui::TextDisabled("No Blend2D display data available.");
            return;
        }

        ImGui::Text("X: %s   Y: %s", blend2DDisplayData.parameterNameX.c_str(), blend2DDisplayData.parameterNameY.c_str());
        if (blend2DDisplayData.live)
        {
            ImGui::Text("Inputs: %s = %.3f, %s = %.3f", blend2DDisplayData.parameterNameX.c_str(), blend2DDisplayData.inputValueX, blend2DDisplayData.parameterNameY.c_str(), blend2DDisplayData.inputValueY);
        }
        else
        {
            ImGui::TextDisabled("Current state is not Blend2D; showing '%s' blend space.", blend2DDisplayData.stateName.c_str());
        }

        if (ImGui::Button("Fit Blend2D"))
        {
            st.animationRuntimeBlend2DZoom = 1.0f;
            st.animationRuntimeBlend2DPan = ImVec2(0.0f, 0.0f);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(170.0f);
        ImGui::SliderFloat("##AnimationRuntimeBlend2DZoom", &st.animationRuntimeBlend2DZoom, 0.45f, 2.50f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
        st.animationRuntimeBlend2DZoom = AnimationGraphClampZoom(st.animationRuntimeBlend2DZoom);

        const ImVec2 previewAvail = ImGui::GetContentRegionAvail();
        const float width = std::max(220.0f, previewAvail.x);
        const float height = std::clamp(width * 0.80f, 240.0f, 360.0f);
        ImGui::BeginChild("##AnimationRuntimeBlend2DPreview", ImVec2(0.0f, height), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(canvasPos, AddImVec2(canvasPos, canvasSize), true);
        drawList->AddRectFilled(canvasPos, AddImVec2(canvasPos, canvasSize), IM_COL32(22, 22, 26, 255), 6.0f);

        ImGui::InvisibleButton("##AnimationRuntimeBlend2DCanvasButton", canvasSize);
        const bool hovered = ImGui::IsItemHovered();
        if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
        {
            st.animationRuntimeBlend2DPan = AddImVec2(st.animationRuntimeBlend2DPan, ImGui::GetIO().MouseDelta);
        }
        AnimationGraphHandleCanvasZoom(st.animationRuntimeBlend2DZoom, st.animationRuntimeBlend2DPan, canvasPos, canvasSize, hovered);

        float minX = blend2DDisplayData.samples.front().x;
        float maxX = blend2DDisplayData.samples.front().x;
        float minY = blend2DDisplayData.samples.front().y;
        float maxY = blend2DDisplayData.samples.front().y;
        for (const AnimationRuntimeBlend2DSampleViewModel& sample : blend2DDisplayData.samples)
        {
            minX = std::min(minX, sample.x);
            maxX = std::max(maxX, sample.x);
            minY = std::min(minY, sample.y);
            maxY = std::max(maxY, sample.y);
        }
        if (std::abs(maxX - minX) <= 1e-6f) { minX -= 1.0f; maxX += 1.0f; }
        if (std::abs(maxY - minY) <= 1e-6f) { minY -= 1.0f; maxY += 1.0f; }
        minX = std::min(minX, 0.0f); maxX = std::max(maxX, 0.0f);
        minY = std::min(minY, 0.0f); maxY = std::max(maxY, 0.0f);
        const float paddingX = std::max(0.10f * (maxX - minX), 0.15f);
        const float paddingY = std::max(0.10f * (maxY - minY), 0.15f);
        minX -= paddingX; maxX += paddingX; minY -= paddingY; maxY += paddingY;

        const ImVec2 plotMin(44.0f, 28.0f);
        const ImVec2 plotMax(std::max(plotMin.x + 10.0f, canvasSize.x - 20.0f), std::max(plotMin.y + 10.0f, canvasSize.y - 24.0f));
        const ImVec2 plotSize = SubImVec2(plotMax, plotMin);
        auto PlotPointFromBlend = [&](float x, float y) -> ImVec2
        {
            const float tx = std::clamp((x - minX) / (maxX - minX), 0.0f, 1.0f);
            const float ty = std::clamp((y - minY) / (maxY - minY), 0.0f, 1.0f);
            return ImVec2(plotMin.x + tx * std::max(1.0f, plotSize.x), plotMin.y + (1.0f - ty) * std::max(1.0f, plotSize.y));
        };
        auto ToScreen = [&](const ImVec2& localPoint) -> ImVec2
        {
            return AnimationGraphCanvasPointToScreen(canvasPos, st.animationRuntimeBlend2DPan, st.animationRuntimeBlend2DZoom, localPoint);
        };

        const ImVec2 plotMinScreen = ToScreen(plotMin);
        const ImVec2 plotMaxScreen = ToScreen(plotMax);
        drawList->AddRectFilled(plotMinScreen, plotMaxScreen, IM_COL32(18, 18, 22, 245), 4.0f);
        drawList->AddRect(plotMinScreen, plotMaxScreen, IM_COL32(80, 86, 98, 255), 4.0f, 0, 1.2f);
        drawList->AddText(AddImVec2(plotMaxScreen, ImVec2(-52.0f, 6.0f)), IM_COL32(210, 210, 215, 255), blend2DDisplayData.parameterNameX.c_str());
        drawList->AddText(AddImVec2(plotMinScreen, ImVec2(8.0f, 6.0f)), IM_COL32(210, 210, 215, 255), blend2DDisplayData.parameterNameY.c_str());
        drawList->AddCircleFilled(ToScreen(PlotPointFromBlend(0.0f, 0.0f)), 2.5f, IM_COL32(190, 196, 208, 215));

        int pointIndex = 0;
        for (const AnimationRuntimeBlend2DSampleViewModel& sample : blend2DDisplayData.samples)
        {
            const ImVec2 screenPoint = ToScreen(PlotPointFromBlend(sample.x, sample.y));
            const float pointRadius = (sample.active ? 7.0f : 5.5f) * st.animationRuntimeBlend2DZoom;
            drawList->AddCircleFilled(screenPoint, pointRadius, sample.active ? IM_COL32(255, 220, 90, 255) : IM_COL32(110, 190, 255, 255));
            drawList->AddCircle(screenPoint, pointRadius + 3.0f, IM_COL32(70, 104, 138, 220), 0, 1.2f);

            const char* clipId = AnimationRuntimeBlendSampleLabel(sample.clipName, sample.clipSourceAssetId);
            std::string label = clipId;
            if (label.size() > 18) { label = label.substr(0, 15) + "..."; }
            const ImVec2 labelOffset = (pointIndex % 2 == 0)
                ? ((sample.x >= 0.0f) ? ImVec2(10.0f, -18.0f) : ImVec2(10.0f, 6.0f))
                : ((sample.x >= 0.0f) ? ImVec2(-84.0f, -18.0f) : ImVec2(-84.0f, 6.0f));
            const ImVec2 labelPos = AddImVec2(screenPoint, labelOffset);
            const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
            drawList->AddRectFilled(AddImVec2(labelPos, ImVec2(-4.0f, -2.0f)), AddImVec2(labelPos, AddImVec2(textSize, ImVec2(4.0f, 2.0f))), IM_COL32(20, 20, 24, 210), 4.0f);
            drawList->AddText(labelPos, IM_COL32(232, 234, 238, 245), label.c_str());

            const float hoverRadius = std::max(8.0f, pointRadius + 4.0f);
            if (hovered)
            {
                const ImVec2 mousePos = ImGui::GetIO().MousePos;
                const float mouseDeltaX = mousePos.x - screenPoint.x;
                const float mouseDeltaY = mousePos.y - screenPoint.y;
                if ((mouseDeltaX * mouseDeltaX + mouseDeltaY * mouseDeltaY) <= (hoverRadius * hoverRadius))
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", clipId);
                    ImGui::Separator();
                    ImGui::Text("X: %.2f", sample.x);
                    ImGui::Text("Y: %.2f", sample.y);
                    ImGui::EndTooltip();
                }
            }
            ++pointIndex;
        }

        if (blend2DDisplayData.live)
        {
            const ImVec2 screenPoint = ToScreen(PlotPointFromBlend(blend2DDisplayData.inputValueX, blend2DDisplayData.inputValueY));
            drawList->AddLine(AddImVec2(screenPoint, ImVec2(-10.0f, 0.0f)), AddImVec2(screenPoint, ImVec2(10.0f, 0.0f)), IM_COL32(255, 220, 90, 255), 2.0f);
            drawList->AddLine(AddImVec2(screenPoint, ImVec2(0.0f, -10.0f)), AddImVec2(screenPoint, ImVec2(0.0f, 10.0f)), IM_COL32(255, 220, 90, 255), 2.0f);
            drawList->AddCircle(screenPoint, 10.0f, IM_COL32(255, 220, 90, 140), 0, 2.0f);
        }

        drawList->PopClipRect();
        ImGui::EndChild();
        ImGui::TextDisabled("MMB drag to pan, wheel to zoom. Hover a point for full clip id.");
        if (ImGui::CollapsingHeader("Blend Samples"))
        {
            for (const AnimationRuntimeBlend2DSampleViewModel& sample : blend2DDisplayData.samples)
            {
                const char* clipId = AnimationRuntimeBlendSampleLabel(sample.clipName, sample.clipSourceAssetId);
                ImGui::BulletText("(%.2f, %.2f) -> %s", sample.x, sample.y, clipId);
            }
        }
    }

    static void DrawAnimationRuntimeTargetSection(const AnimationRuntimeViewModel& AnimationRuntimeViewModel)
    {
        ImGui::SeparatorText("Target");
        ImGui::Text("Node: %s", AnimationRuntimeViewModel.nodeName.c_str());
        ImGui::TextDisabled("Skinned mesh: %s", AnimationRuntimeViewModel.skinnedMesh.c_str());
        if (!AnimationRuntimeViewModel.controllerLabel.empty())
        {
            ImGui::TextDisabled("Controller: %s", AnimationRuntimeViewModel.controllerLabel.c_str());
        }
    }

    static void DrawAnimationRuntimeLiveStateSection(const AnimationRuntimeViewModel& AnimationRuntimeViewModel)
    {
        ImGui::SeparatorText("Live State");
        ImGui::Text("Current state: %s", AnimationRuntimeViewModel.currentStateDisplayName.c_str());
        ImGui::Text("Requested state: %s", AnimationRuntimeViewModel.requestedStateDisplayName.c_str());
        ImGui::Text("Mode: %s", AnimationRuntimeViewModel.modeName.c_str());
        ImGui::Text("Normalized time: %.3f", AnimationRuntimeViewModel.normalizedTime);
        ImGui::Text("Playback speed: %.3f", AnimationRuntimeViewModel.playbackSpeed);
        ImGui::Text("Looping: %s", AnimationRuntimeViewModel.loopingText.c_str());
        if (AnimationRuntimeViewModel.transitionActive)
        {
            ImGui::Text("Transition: %s -> %s (alpha %.2f)",
                AnimationRuntimeViewModel.transitionSourceStateName.c_str(),
                AnimationRuntimeViewModel.currentStateName.c_str(),
                AnimationRuntimeViewModel.transitionAlpha);
        }
        else
        {
            ImGui::Text("The last transition: %s -> %s (alpha %.2f)",
                AnimationRuntimeViewModel.transitionSourceStateName.c_str(),
                AnimationRuntimeViewModel.currentStateName.c_str(),
                AnimationRuntimeViewModel.transitionAlpha);
        }
    }

    static void DrawAnimationRuntimeRecentNotifies(const std::vector<AnimationRuntimeNotifyViewModel>& recentNotifies)
    {
        if (recentNotifies.empty())
        {
            ImGui::TextDisabled("No recent notify events.");
            return;
        }

        for (const AnimationRuntimeNotifyViewModel& notify : recentNotifies)
        {
            ImGui::BulletText(
                "#%llu %s (%s @ %.2f)",
                static_cast<unsigned long long>(notify.sequence),
                notify.id.c_str(),
                notify.stateName.c_str(),
                notify.normalizedTime);
        }
    }

    static void DrawAnimationRuntimeTextList(
        const std::vector<std::string>& items,
        const char* emptyMessage)
    {
        if (items.empty())
        {
            ImGui::TextDisabled("%s", emptyMessage);
            return;
        }

        for (const std::string& item : items)
        {
            ImGui::BulletText("%s", item.c_str());
        }
    }

    static void DrawAnimationRuntimeParameters(const std::vector<AnimationRuntimeParameterViewModel>& parameters)
    {
        if (parameters.empty())
        {
            ImGui::TextDisabled("No controller parameters.");
            return;
        }

        if (ImGui::BeginTable("##AnimationRuntimeParams", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Parameter");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            for (const AnimationRuntimeParameterViewModel& parameter : parameters)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(parameter.name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(parameter.valueText.c_str());
            }
            ImGui::EndTable();
        }
    }

    static void DrawAnimationRuntimeWindow(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        rendern::Scene& scene,
        LevelEditorUIState& st,
        const rendern::GameplayRuntime* gameplayRuntime)
    {
        if (!st.animationRuntimeWindowOpen)
        {
            return;
        }

        if (!ImGui::Begin("Animation Runtime", &st.animationRuntimeWindowOpen))
        {
            ImGui::End();
            return;
        }

        const std::vector<int> candidates = BuildAnimationRuntimeNodeCandidates(level, levelInst);
        if (candidates.empty())
        {
            ImGui::TextDisabled("No skinned runtime targets found.");
            ImGui::End();
            return;
        }

        bool pinTarget = st.animationRuntimePinnedNodeIndex >= 0;
        if (ImGui::Checkbox("Pin target", &pinTarget))
        {
            if (!pinTarget)
            {
                SetAnimationRuntimePinnedTarget(st, gameplayRuntime, -1);
            }
            else if (NodeAlive(level, st.selectedNode))
            {
                const rendern::LevelNode& selectedNode = level.nodes[static_cast<std::size_t>(st.selectedNode)];
                if (!selectedNode.skinnedMesh.empty())
                {
                    SetAnimationRuntimePinnedTarget(st, gameplayRuntime, st.selectedNode);
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Use current selection") && NodeAlive(level, st.selectedNode))
        {
            const rendern::LevelNode& selectedNode = level.nodes[static_cast<std::size_t>(st.selectedNode)];
            if (!selectedNode.skinnedMesh.empty())
            {
                SetAnimationRuntimePinnedTarget(st, gameplayRuntime, st.selectedNode);
            }
        }

        const char* previewText = "Auto target";
        if (st.animationRuntimePinnedNodeIndex >= 0 && NodeAlive(level, st.animationRuntimePinnedNodeIndex))
        {
            previewText = level.nodes[static_cast<std::size_t>(st.animationRuntimePinnedNodeIndex)].name.c_str();
        }
        if (st.animationRuntimeObservedEntityUnavailable)
        {
            ImGui::TextDisabled("Observed runtime entity is unavailable; showing the resolved editor target when possible.");
        }

        if (ImGui::BeginCombo("Target", previewText))
        {
            const bool autoSelected = st.animationRuntimePinnedNodeIndex < 0;
            if (ImGui::Selectable("Auto target", autoSelected))
            {
                SetAnimationRuntimePinnedTarget(st, gameplayRuntime, -1);
            }
            for (const int nodeIndex : candidates)
            {
                const bool selected = st.animationRuntimePinnedNodeIndex == nodeIndex;
                const rendern::LevelNode& node = level.nodes[static_cast<std::size_t>(nodeIndex)];
                if (ImGui::Selectable(node.name.c_str(), selected))
                {
                    SetAnimationRuntimePinnedTarget(st, gameplayRuntime, nodeIndex);
                }
            }
            ImGui::EndCombo();
        }

        AnimationGraphContext ctx = GetAnimationRuntimeContext(level, levelInst, scene, st, gameplayRuntime);
        if (ctx.node == nullptr || ctx.skinnedItem == nullptr)
        {
            ImGui::TextDisabled("No runtime animation target available.");
            ImGui::End();
            return;
        }
        
        const rendern::AnimationControllerRuntime& runtime = ctx.skinnedItem->controller;
        const AnimationRuntimeViewModel animationRuntimeViewModel = BuildAnimationRuntimeViewModel(ctx, runtime, ctx.skinnedItem->animator);

        if (ImGui::Button("Select in editor"))
        {
            st.selectedNode = animationRuntimeViewModel.nodeIndex;
            scene.EditorSetSelectionSingle(animationRuntimeViewModel.nodeIndex);
        }
        ImGui::SameLine();
        if (ImGui::Button("Open graph"))
        {
            st.selectedNode = animationRuntimeViewModel.nodeIndex;
            scene.EditorSetSelectionSingle(animationRuntimeViewModel.nodeIndex);
            st.animationGraphWindowOpen = true;
            st.animationGraphRequestFocus = true;
            st.animationGraphSelectedStateName = animationRuntimeViewModel.currentStateName;
        }

        DrawAnimationRuntimeTargetSection(animationRuntimeViewModel);
        DrawAnimationRuntimeLiveStateSection(animationRuntimeViewModel);

        ImGui::SeparatorText("Active Clips");
        DrawAnimationRuntimeWeightedClips(animationRuntimeViewModel.activeClips);

        if (animationRuntimeViewModel.blend2DDisplayData.available)
        {
            ImGui::SeparatorText(animationRuntimeViewModel.blend2DDisplayData.live ? "Live Blend2D" : "Blend2D Preview");
            DrawAnimationRuntimeBlend2DDisplayData(animationRuntimeViewModel.blend2DDisplayData, st);
        }
        else if (animationRuntimeViewModel.hasCurrentStateDesc)
        {
            if (animationRuntimeViewModel.blend1DDisplayData.available && animationRuntimeViewModel.blend1DDisplayData.live)
            {
                ImGui::SeparatorText("Live Blend1D");
                DrawAnimationRuntimeBlend1DDisplayData(animationRuntimeViewModel.blend1DDisplayData);
            }
            else if (!animationRuntimeViewModel.activeClips.empty())
            {
                ImGui::Text("Active clip: %s", animationRuntimeViewModel.activeClips.front().clipName.c_str());
            }
        }

        if (ImGui::CollapsingHeader("Controller Parameters", ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawAnimationRuntimeParameters(animationRuntimeViewModel.parameters);
        }
        
        if (ImGui::CollapsingHeader("Recent Notifies", ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawAnimationRuntimeRecentNotifies(animationRuntimeViewModel.recentNotifies);
        }

        if (ImGui::CollapsingHeader("Transition Candidates"))
        {
            DrawAnimationRuntimeTextList(
                animationRuntimeViewModel.transitionCandidates,
                "No transition diagnostics.");
        }

        if (ImGui::CollapsingHeader("Gameplay Events"))
        {
            DrawAnimationRuntimeTextList(
                animationRuntimeViewModel.routedGameplayEvents,
                "No routed gameplay events.");
        }

        ImGui::End();
    }

    void DrawAnimationGraphWindow(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        rendern::Scene& scene,
        LevelEditorUIState& st)
    {
        if (!st.animationGraphWindowOpen)
        {
            return;
        }

        if (st.animationGraphRequestFocus)
        {
            ImGui::SetNextWindowFocus();
            st.animationGraphRequestFocus = false;
        }

        if (!ImGui::Begin("Animation Graph", &st.animationGraphWindowOpen))
        {
            ImGui::End();
            return;
        }

        AnimationGraphContext ctx = GetAnimationGraphContext(level, levelInst, scene, st);
        if (ctx.node == nullptr || ctx.skinnedItem == nullptr)
        {
            ImGui::TextDisabled("Select a skinned node to inspect its animation graph.");
            ImGui::End();
            return;
        }

        ImGui::Text("Node: %s", ctx.node->name.c_str());
        ImGui::TextDisabled("Skinned mesh: %s", ctx.node->skinnedMesh.c_str());

        if (ctx.controllerAsset == nullptr)
        {
            ImGui::Separator();
            ImGui::TextDisabled("This node is currently in legacy clip mode.");
            ImGui::TextDisabled("Assign an animation controller to open the FSM graph.");
            ImGui::End();
            return;
        }

        if (st.animationGraphSelectedStateName.empty() || rendern::FindAnimationControllerState(*ctx.controllerAsset, st.animationGraphSelectedStateName) == nullptr)
        {
            st.animationGraphSelectedStateName = ctx.skinnedItem->controller.currentStateName.empty()
                ? ctx.controllerAsset->defaultState
                : ctx.skinnedItem->controller.currentStateName;
        }

        ImGui::Text("Controller: %s", ctx.controllerAsset->id.c_str());
        if (!ctx.controllerAsset->notifyAssetPath.empty())
        {
            ImGui::TextDisabled("Notify asset: %s", ctx.controllerAsset->notifyAssetPath.c_str());
        }
        if (!ctx.controllerAsset->eventBindingsAssetPath.empty())
        {
            ImGui::TextDisabled("Bindings asset: %s", ctx.controllerAsset->eventBindingsAssetPath.c_str());
        }

        DrawAnimationGraphRuntimeSummary(ctx.skinnedItem->controller);
        ImGui::Separator();

        if (ImGui::BeginTabBar("##AnimationGraphTabs"))
        {
            if (ImGui::BeginTabItem("FSM"))
            {
                DrawAnimationGraphFsmCanvas(*ctx.controllerAsset, ctx.skinnedItem->controller, st);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Assets"))
            {
                DrawAnimationGraphAssetCanvas(level, *ctx.node, *ctx.controllerAsset, st);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Bindings"))
            {
                DrawAnimationGraphBindings(*ctx.controllerAsset);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Diagnostics"))
            {
                DrawAnimationGraphDiagnosticsPanel(level, *ctx.controllerAsset);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}
