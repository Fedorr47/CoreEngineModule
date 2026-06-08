    static void DrawAnimationGraphStateInspector(
        const rendern::AnimationControllerAsset& controllerAsset,
        const rendern::AnimationControllerRuntime& runtime,
        LevelEditorUIState& uiState)
    {
        const rendern::AnimationStateDesc* selectedState = nullptr;
        if (!uiState.animationGraphSelectedStateName.empty())
        {
            selectedState = rendern::FindAnimationControllerState(controllerAsset, uiState.animationGraphSelectedStateName);
        }
        if (selectedState == nullptr && !controllerAsset.states.empty())
        {
            selectedState = &controllerAsset.states.front();
            uiState.animationGraphSelectedStateName = selectedState->name;
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
            DrawAnimationGraphBlend2DPreview(*selectedState, runtime, uiState);
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
        LevelEditorUIState& uiState,
        const rendern::GameplayRuntime* gameplayRuntime)
    {
        if (!uiState.animationRuntimeWindowOpen)
        {
            return;
        }

        if (!ImGui::Begin("Animation Runtime", &uiState.animationRuntimeWindowOpen))
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

        bool pinTarget = uiState.animationRuntimePinnedNodeIndex >= 0;
        if (ImGui::Checkbox("Pin target", &pinTarget))
        {
            if (!pinTarget)
            {
                SetAnimationRuntimePinnedTarget(uiState, gameplayRuntime, -1);
            }
            else if (NodeAlive(level, uiState.selectedNode))
            {
                const rendern::LevelNode& selectedNode = level.nodes[static_cast<std::size_t>(uiState.selectedNode)];
                if (!selectedNode.skinnedMesh.empty())
                {
                    SetAnimationRuntimePinnedTarget(uiState, gameplayRuntime, uiState.selectedNode);
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Use current selection") && NodeAlive(level, uiState.selectedNode))
        {
            const rendern::LevelNode& selectedNode = level.nodes[static_cast<std::size_t>(uiState.selectedNode)];
            if (!selectedNode.skinnedMesh.empty())
            {
                SetAnimationRuntimePinnedTarget(uiState, gameplayRuntime, uiState.selectedNode);
            }
        }

        const char* previewText = "Auto target";
        if (uiState.animationRuntimePinnedNodeIndex >= 0 && NodeAlive(level, uiState.animationRuntimePinnedNodeIndex))
        {
            previewText = level.nodes[static_cast<std::size_t>(uiState.animationRuntimePinnedNodeIndex)].name.c_str();
        }
        if (uiState.animationRuntimeObservedEntityUnavailable)
        {
            ImGui::TextDisabled("Observed runtime entity is unavailable; showing the resolved editor target when possible.");
        }

        if (ImGui::BeginCombo("Target", previewText))
        {
            const bool autoSelected = uiState.animationRuntimePinnedNodeIndex < 0;
            if (ImGui::Selectable("Auto target", autoSelected))
            {
                SetAnimationRuntimePinnedTarget(uiState, gameplayRuntime, -1);
            }
            for (const int nodeIndex : candidates)
            {
                const bool selected = uiState.animationRuntimePinnedNodeIndex == nodeIndex;
                const rendern::LevelNode& node = level.nodes[static_cast<std::size_t>(nodeIndex)];
                if (ImGui::Selectable(node.name.c_str(), selected))
                {
                    SetAnimationRuntimePinnedTarget(uiState, gameplayRuntime, nodeIndex);
                }
            }
            ImGui::EndCombo();
        }

        AnimationGraphContext ctx = GetAnimationRuntimeContext(level, levelInst, scene, uiState, gameplayRuntime);
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
            uiState.selectedNode = animationRuntimeViewModel.nodeIndex;
            scene.EditorSetSelectionSingle(animationRuntimeViewModel.nodeIndex);
        }
        ImGui::SameLine();
        if (ImGui::Button("Open graph"))
        {
            uiState.selectedNode = animationRuntimeViewModel.nodeIndex;
            scene.EditorSetSelectionSingle(animationRuntimeViewModel.nodeIndex);
            uiState.animationGraphWindowOpen = true;
            uiState.animationGraphRequestFocus = true;
            uiState.animationGraphSelectedStateName = animationRuntimeViewModel.currentStateName;
        }

        DrawAnimationRuntimeTargetSection(animationRuntimeViewModel);
        DrawAnimationRuntimeLiveStateSection(animationRuntimeViewModel);

        ImGui::SeparatorText("Active Clips");
        DrawAnimationRuntimeWeightedClips(animationRuntimeViewModel.activeClips);

        if (animationRuntimeViewModel.blend2DDisplayData.available)
        {
            ImGui::SeparatorText(animationRuntimeViewModel.blend2DDisplayData.live ? "Live Blend2D" : "Blend2D Preview");
            DrawAnimationRuntimeBlend2DDisplayData(animationRuntimeViewModel.blend2DDisplayData, uiState);
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
