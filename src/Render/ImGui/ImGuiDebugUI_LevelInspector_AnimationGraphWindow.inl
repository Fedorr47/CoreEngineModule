
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
        const rendern::LevelAsset& level,
        const rendern::LevelNode& node,
        const rendern::AnimationControllerAsset& controllerAsset)
    {
        const std::string controllerId = controllerAsset.id.empty() ? std::string("<controller>") : controllerAsset.id;

        const std::string selectedNodeKey = AnimationGraphMakeKey("asset", controllerId, std::string("node:") + node.name);
        if (!st.animationGraphAssetNodePositions.contains(selectedNodeKey))
        {
            st.animationGraphAssetNodePositions.emplace(selectedNodeKey, ImVec2(40.0f, 140.0f));
        }

        const std::string controllerKey = AnimationGraphMakeKey("asset", controllerId, std::string("controller:") + controllerId);
        if (!st.animationGraphAssetNodePositions.contains(controllerKey))
        {
            st.animationGraphAssetNodePositions.emplace(controllerKey, ImVec2(310.0f, 140.0f));
        }

        if (!controllerAsset.notifyAssetPath.empty())
        {
            const std::string notifyKey = AnimationGraphMakeKey("asset", controllerId, std::string("notify:") + controllerAsset.notifyAssetPath);
            if (!st.animationGraphAssetNodePositions.contains(notifyKey))
            {
                st.animationGraphAssetNodePositions.emplace(notifyKey, ImVec2(580.0f, 70.0f));
            }
        }

        if (!controllerAsset.eventBindingsAssetPath.empty())
        {
            const std::string bindingsKey = AnimationGraphMakeKey("asset", controllerId, std::string("bindings:") + controllerAsset.eventBindingsAssetPath);
            if (!st.animationGraphAssetNodePositions.contains(bindingsKey))
            {
                st.animationGraphAssetNodePositions.emplace(bindingsKey, ImVec2(580.0f, 220.0f));
            }
        }

        std::vector<std::string> clipIds;
        for (const rendern::AnimationStateDesc& state : controllerAsset.states)
        {
            if (!state.clipSourceAssetId.empty() && !AnimationGraphContainsString(clipIds, state.clipSourceAssetId))
            {
                clipIds.push_back(state.clipSourceAssetId);
            }
            for (const rendern::AnimationBlend2DPoint& point : state.blend2D)
            {
                if (!point.clipSourceAssetId.empty() && !AnimationGraphContainsString(clipIds, point.clipSourceAssetId))
                {
                    clipIds.push_back(point.clipSourceAssetId);
                }
            }
        }

        for (std::size_t i = 0; i < clipIds.size(); ++i)
        {
            const std::string clipKey = AnimationGraphMakeKey("asset", controllerId, std::string("clip:") + clipIds[i]);
            if (!st.animationGraphAssetNodePositions.contains(clipKey))
            {
                st.animationGraphAssetNodePositions.emplace(clipKey, ImVec2(860.0f, 24.0f + static_cast<float>(i) * 88.0f));
            }
        }
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

    static void DrawAnimationGraphStateInspector(
        const rendern::AnimationControllerAsset& controllerAsset,
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

        if (!selectedState->blend2D.empty())
        {
            ImGui::SeparatorText("Blend2D");
            ImGui::Text("X: %s   Y: %s", selectedState->blendParameterX.c_str(), selectedState->blendParameterY.c_str());
            for (const rendern::AnimationBlend2DPoint& point : selectedState->blend2D)
            {
                const char* clipId = point.clipSourceAssetId.empty() ? point.clipName.c_str() : point.clipSourceAssetId.c_str();
                ImGui::BulletText("(%.2f, %.2f) -> %s", point.x, point.y, clipId);
            }
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

        const float inspectorReserve = 330.0f;
        const ImVec2 canvasSize = ImVec2(std::max(200.0f, ImGui::GetContentRegionAvail().x - inspectorReserve), 420.0f);
        ImGui::BeginChild("##AnimationGraphFsmCanvas", canvasSize, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(canvasPos, AddImVec2(canvasPos, avail), true);
        drawList->AddRectFilled(canvasPos, AddImVec2(canvasPos, avail), IM_COL32(24, 24, 28, 255), 4.0f);

        const bool fsmCanvasHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        if (fsmCanvasHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
        {
            st.animationGraphFsmPan = AddImVec2(st.animationGraphFsmPan, ImGui::GetIO().MouseDelta);
        }

        const ImVec2 nodeSize = ImVec2(180.0f, 62.0f);

        auto StateRectMin = [&](const rendern::AnimationStateDesc& state) -> ImVec2
            {
                const std::string key = AnimationGraphMakeKey("fsm", controllerAsset.id, state.name);
                auto it = st.animationGraphFsmNodePositions.find(key);
                const ImVec2 localPos = (it != st.animationGraphFsmNodePositions.end()) ? it->second : ImVec2(24.0f, 24.0f);
                return AddImVec2(canvasPos, AddImVec2(st.animationGraphFsmPan, localPos));
            };

        for (const rendern::AnimationTransitionDesc& transition : controllerAsset.transitions)
        {
            const rendern::AnimationStateDesc* fromState = rendern::FindAnimationControllerState(controllerAsset, transition.fromState);
            const rendern::AnimationStateDesc* toState = rendern::FindAnimationControllerState(controllerAsset, transition.toState);
            if (fromState == nullptr || toState == nullptr)
            {
                continue;
            }

            const ImVec2 fromMin = StateRectMin(*fromState);
            const ImVec2 toMin = StateRectMin(*toState);
            const ImVec2 p1 = AddImVec2(fromMin, ImVec2(nodeSize.x, nodeSize.y * 0.5f));
            const ImVec2 p4 = AddImVec2(toMin, ImVec2(0.0f, nodeSize.y * 0.5f));
            const float dx = std::max(70.0f, std::fabs(p4.x - p1.x) * 0.4f);
            const ImVec2 p2 = AddImVec2(p1, ImVec2(dx, 0.0f));
            const ImVec2 p3 = AddImVec2(p4, ImVec2(-dx, 0.0f));
            const bool activeEdge =
                runtime.transitionActive &&
                runtime.transitionSourceStateName == transition.fromState &&
                runtime.currentStateName == transition.toState;
            const ImU32 edgeColor = activeEdge ? IM_COL32(120, 200, 255, 255) : IM_COL32(140, 140, 150, 180);
            drawList->AddBezierCubic(p1, p2, p3, p4, edgeColor, activeEdge ? 3.0f : 2.0f, 0);

            const ImVec2 arrowTip = p4;
            const ImVec2 arrowBase = AddImVec2(arrowTip, ImVec2(-10.0f, 0.0f));
            drawList->AddTriangleFilled(
                arrowTip,
                AddImVec2(arrowBase, ImVec2(0.0f, -5.0f)),
                AddImVec2(arrowBase, ImVec2(0.0f, 5.0f)),
                edgeColor);

            const ImVec2 labelMid = MulImVec2(AddImVec2(p1, p4), 0.5f);
            const ImVec2 labelPos = AddImVec2(labelMid, ImVec2(-40.0f, -10.0f));
            char labelBuf[64]{};
            std::snprintf(labelBuf, sizeof(labelBuf), "p%d / %.2f", transition.priority, transition.blendDurationSeconds);
            drawList->AddText(labelPos, IM_COL32(205, 205, 205, 255), labelBuf);
        }

        for (const rendern::AnimationStateDesc& state : controllerAsset.states)
        {
            const std::string key = AnimationGraphMakeKey("fsm", controllerAsset.id, state.name);
            ImVec2& localPos = st.animationGraphFsmNodePositions[key];
            const ImVec2 minPos = AddImVec2(canvasPos, AddImVec2(st.animationGraphFsmPan, localPos));
            const ImVec2 maxPos = AddImVec2(minPos, nodeSize);
            const bool isCurrent = runtime.currentStateName == state.name;
            const bool isSelected = st.animationGraphSelectedStateName == state.name;

            drawList->AddRectFilled(minPos, maxPos, AnimationGraphStateColor(state, isCurrent), 8.0f);
            drawList->AddRect(minPos, maxPos, isSelected ? IM_COL32(255, 230, 120, 255) : IM_COL32(25, 25, 25, 255), 8.0f, 0, isSelected ? 3.0f : 1.0f);
            drawList->AddText(AddImVec2(minPos, ImVec2(10.0f, 10.0f)), IM_COL32(255, 255, 255, 255), state.name.c_str());
            drawList->AddText(AddImVec2(minPos, ImVec2(10.0f, 32.0f)), IM_COL32(220, 220, 220, 255), AnimationGraphStateCategory(state));

            const std::string buttonId = "##anim_graph_state_" + state.name;
            ImGui::SetCursorScreenPos(minPos);
            ImGui::InvisibleButton(buttonId.c_str(), nodeSize);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                st.animationGraphSelectedStateName = state.name;
            }
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
            {
                localPos = AddImVec2(localPos, ImGui::GetIO().MouseDelta);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("State: %s", state.name.c_str());
                ImGui::Text("Category: %s", AnimationGraphStateCategory(state));
                ImGui::Text("Loop: %s", state.looping ? "true" : "false");
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
        ImGui::BeginChild("##AnimationGraphFsmInspector", ImVec2(0.0f, canvasSize.y), true);
        DrawAnimationGraphLegend();
        if (ImGui::Button("Reset FSM Layout"))
        {
            std::vector<std::string> keysToErase;
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
        DrawAnimationGraphStateInspector(controllerAsset, st);
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

        std::vector<AnimationAssetNodeDef> nodes;
        const std::string controllerId = controllerAsset.id.empty() ? std::string("<controller>") : controllerAsset.id;

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

        std::vector<std::string> clipIds;
        for (const rendern::AnimationStateDesc& state : controllerAsset.states)
        {
            if (!state.clipSourceAssetId.empty() && !AnimationGraphContainsString(clipIds, state.clipSourceAssetId))
            {
                clipIds.push_back(state.clipSourceAssetId);
            }
            for (const rendern::AnimationBlend2DPoint& point : state.blend2D)
            {
                if (!point.clipSourceAssetId.empty() && !AnimationGraphContainsString(clipIds, point.clipSourceAssetId))
                {
                    clipIds.push_back(point.clipSourceAssetId);
                }
            }
        }

        for (const std::string& clipId : clipIds)
        {
            const bool exists = level.animations.contains(clipId);
            nodes.push_back(AnimationAssetNodeDef{
                std::string("clip:") + clipId,
                clipId,
                exists ? "Animation clip asset" : "Missing clip asset",
                exists ? IM_COL32(80, 100, 140, 255) : IM_COL32(155, 70, 70, 255)
            });
        }

        const ImVec2 nodeSize = ImVec2(190.0f, 60.0f);
        ImGui::BeginChild("##AnimationGraphAssetCanvas", ImVec2(0.0f, 360.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(canvasPos, AddImVec2(canvasPos, avail), true);
        drawList->AddRectFilled(canvasPos, AddImVec2(canvasPos, avail), IM_COL32(24, 24, 28, 255), 4.0f);

        const bool assetCanvasHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        if (assetCanvasHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
        {
            st.animationGraphAssetPan = AddImVec2(st.animationGraphAssetPan, ImGui::GetIO().MouseDelta);
        }

        auto AssetNodeMin = [&](std::string_view localId) -> ImVec2
            {
                const std::string key = AnimationGraphMakeKey("asset", controllerId, localId);
                auto it = st.animationGraphAssetNodePositions.find(key);
                const ImVec2 localPos = (it != st.animationGraphAssetNodePositions.end()) ? it->second : ImVec2(20.0f, 20.0f);
                return AddImVec2(canvasPos, AddImVec2(st.animationGraphAssetPan, localPos));
            };

        auto DrawEdge = [&](std::string_view fromId, std::string_view toId, ImU32 color)
            {
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
                drawList->AddTriangleFilled(
                    p4,
                    AddImVec2(p4, ImVec2(-10.0f, -5.0f)),
                    AddImVec2(p4, ImVec2(-10.0f, 5.0f)),
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
        for (const std::string& clipId : clipIds)
        {
            DrawEdge(std::string("controller:") + controllerId, std::string("clip:") + clipId, IM_COL32(125, 165, 205, 180));
        }

        for (const AnimationAssetNodeDef& visualNode : nodes)
        {
            const std::string key = AnimationGraphMakeKey("asset", controllerId, visualNode.id);
            ImVec2& localPos = st.animationGraphAssetNodePositions[key];
            const ImVec2 minPos = AddImVec2(canvasPos, AddImVec2(st.animationGraphAssetPan, localPos));
            const ImVec2 maxPos = AddImVec2(minPos, nodeSize);

            drawList->AddRectFilled(minPos, maxPos, visualNode.color, 8.0f);
            drawList->AddRect(minPos, maxPos, IM_COL32(25, 25, 25, 255), 8.0f, 0, 1.0f);
            drawList->AddText(AddImVec2(minPos, ImVec2(10.0f, 10.0f)), IM_COL32(255, 255, 255, 255), visualNode.title.c_str());
            drawList->AddText(AddImVec2(minPos, ImVec2(10.0f, 32.0f)), IM_COL32(220, 220, 220, 255), visualNode.subtitle.c_str());

            const std::string buttonId = "##anim_asset_node_" + visualNode.id;
            ImGui::SetCursorScreenPos(minPos);
            ImGui::InvisibleButton(buttonId.c_str(), nodeSize);
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
            {
                localPos = AddImVec2(localPos, ImGui::GetIO().MouseDelta);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(visualNode.title.c_str());
                ImGui::TextDisabled("%s", visualNode.subtitle.c_str());
                ImGui::EndTooltip();
            }
        }

        drawList->PopClipRect();
        ImGui::EndChild();

        if (ImGui::Button("Reset Asset Layout"))
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
