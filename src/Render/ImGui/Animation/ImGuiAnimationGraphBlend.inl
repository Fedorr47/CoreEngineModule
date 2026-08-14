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
        AnimationUIState& uiState)
    {
        if (state.blend2D.empty())
        {
            return;
        }

        ImGui::SeparatorText("Blend2D Preview");
        ImGui::Text("X: %s   Y: %s", state.blendParameterX.c_str(), state.blendParameterY.c_str());

        if (ImGui::Button("Fit Blend2D"))
        {
            uiState.animationGraphBlend2DZoom = 1.0f;
            uiState.animationGraphBlend2DPan = ImVec2(0.0f, 0.0f);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(170.0f);
        ImGui::SliderFloat("##AnimationGraphBlend2DZoom", &uiState.animationGraphBlend2DZoom, 0.45f, 2.50f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
        uiState.animationGraphBlend2DZoom = AnimationGraphClampZoom(uiState.animationGraphBlend2DZoom);

        const ImVec2 previewAvail = ImGui::GetContentRegionAvail();
        const float width = std::max(220.0f, previewAvail.x);
        const float height = std::clamp(width * 0.80f, 240.0f, 360.0f);

        ImGui::BeginChild(
            "##AnimationGraphBlend2DPreview",
            ImVec2(0.0f, height),
            true,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        const ImVec2 availableCanvasSize = ImGui::GetContentRegionAvail();
        constexpr float MinimumBlend2DCanvasWidth = 64.0f;
        constexpr float MinimumBlend2DCanvasHeight = 64.0f;

        const ImVec2 canvasSize{
            std::max(MinimumBlend2DCanvasWidth, availableCanvasSize.x),
            std::max(MinimumBlend2DCanvasHeight, availableCanvasSize.y)
        };

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(canvasPos, AddImVec2(canvasPos, canvasSize), true);
        drawList->AddRectFilled(canvasPos, AddImVec2(canvasPos, canvasSize), IM_COL32(22, 22, 26, 255), 6.0f);

        ImGui::InvisibleButton("##AnimationGraphBlend2DCanvasButton", canvasSize);

        const bool hovered = ImGui::IsItemHovered();
        if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
        {
            uiState.animationGraphBlend2DPan = AddImVec2(uiState.animationGraphBlend2DPan, ImGui::GetIO().MouseDelta);
        }

        AnimationGraphHandleCanvasZoom(
            uiState.animationGraphBlend2DZoom,
            uiState.animationGraphBlend2DPan,
            canvasPos,
            canvasSize,
            hovered);
        
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
            return AnimationGraphCanvasPointToScreen(canvasPos, uiState.animationGraphBlend2DPan, uiState.animationGraphBlend2DZoom, localPoint);
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
            const float pointRadius = 5.5f * uiState.animationGraphBlend2DZoom;
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
        AnimationUIState& uiState)
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
            uiState.animationRuntimeBlend2DZoom = 1.0f;
            uiState.animationRuntimeBlend2DPan = ImVec2(0.0f, 0.0f);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(170.0f);
        ImGui::SliderFloat("##AnimationRuntimeBlend2DZoom", &uiState.animationRuntimeBlend2DZoom, 0.45f, 2.50f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
        uiState.animationRuntimeBlend2DZoom = AnimationGraphClampZoom(uiState.animationRuntimeBlend2DZoom);

        const ImVec2 previewAvail = ImGui::GetContentRegionAvail();
        const float width = std::max(220.0f, previewAvail.x);
        const float height = std::clamp(width * 0.80f, 240.0f, 360.0f);
        
        ImGui::BeginChild(
            "##AnimationRuntimeBlend2DPreview",
            ImVec2(0.0f, height),
            true,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        const ImVec2 availableCanvasSize = ImGui::GetContentRegionAvail();
        const ImVec2 canvasSize{
            std::max(1.0f, availableCanvasSize.x),
            std::max(1.0f, availableCanvasSize.y)
        };

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(canvasPos, AddImVec2(canvasPos, canvasSize), true);
        drawList->AddRectFilled(canvasPos, AddImVec2(canvasPos, canvasSize), IM_COL32(22, 22, 26, 255), 6.0f);

        ImGui::InvisibleButton("##AnimationRuntimeBlend2DCanvasButton", canvasSize);
        const bool hovered = ImGui::IsItemHovered();
        if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
        {
            uiState.animationRuntimeBlend2DPan = AddImVec2(uiState.animationRuntimeBlend2DPan, ImGui::GetIO().MouseDelta);
        }
        AnimationGraphHandleCanvasZoom(uiState.animationRuntimeBlend2DZoom, uiState.animationRuntimeBlend2DPan, canvasPos, canvasSize, hovered);

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
            return AnimationGraphCanvasPointToScreen(canvasPos, uiState.animationRuntimeBlend2DPan, uiState.animationRuntimeBlend2DZoom, localPoint);
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
            const float pointRadius = (sample.active ? 7.0f : 5.5f) * uiState.animationRuntimeBlend2DZoom;
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
