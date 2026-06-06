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
