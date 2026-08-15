    static bool DrawControllerConditionEditor(
        std::vector<rendern::AnimationConditionDesc>& conditions,
        const rendern::AnimationControllerAsset& controller)
    {
        bool changed = false;
        for (std::size_t index = 0; index < conditions.size(); ++index)
        {
            rendern::AnimationConditionDesc& condition = conditions[index];
            ImGui::PushID(static_cast<int>(index));
            const char* preview = condition.parameter.empty() ? "<parameter>" : condition.parameter.c_str();
            if (ImGui::BeginCombo("Parameter", preview))
            {
                for (const rendern::AnimationParameterDesc& parameter : controller.parameters)
                {
                    if (ImGui::Selectable(parameter.name.c_str(), condition.parameter == parameter.name))
                    {
                        condition.parameter = parameter.name;
                        condition.value.type = parameter.defaultValue.type;
                        condition.op = parameter.defaultValue.type == rendern::AnimationParameterType::Trigger
                            ? rendern::AnimationConditionOp::Triggered : rendern::AnimationConditionOp::IfTrue;
                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }
            
            static const char* opNames[] = { "true", "false", ">", ">=", "<", "<=", "==", "!=", "triggered" };
            int op = static_cast<int>(condition.op);
            if (ImGui::Combo("Operator", &op, opNames, IM_ARRAYSIZE(opNames)))
            {
                condition.op = static_cast<rendern::AnimationConditionOp>(op);
                changed = true;
            }
            if (condition.value.type == rendern::AnimationParameterType::Bool)
                changed |= ImGui::Checkbox("Value", &condition.value.boolValue);
            else if (condition.value.type == rendern::AnimationParameterType::Int)
                changed |= ImGui::InputInt("Value", &condition.value.intValue);
            else if (condition.value.type == rendern::AnimationParameterType::Float)
                changed |= ImGui::InputFloat("Value", &condition.value.floatValue);
            if (ImGui::SmallButton("Remove condition"))
            {
                conditions.erase(conditions.begin() + static_cast<std::ptrdiff_t>(index));
                --index;
                changed = true;
            }
            ImGui::Separator();
            ImGui::PopID();
        }

        if (ImGui::Button("Add Condition"))
        {
            rendern::AnimationConditionDesc condition;
            if (!controller.parameters.empty())
            {
                condition.parameter = controller.parameters.front().name;
                condition.value.type = controller.parameters.front().defaultValue.type;
                condition.op = condition.value.type == rendern::AnimationParameterType::Trigger
                    ? rendern::AnimationConditionOp::Triggered : rendern::AnimationConditionOp::IfTrue;
            }
            conditions.push_back(std::move(condition));
            changed = true;
        }
        return changed;
    }

    static bool DrawSelectorStringList(const char* label, std::vector<std::string>& values)
    {
        bool changed = false;
        ImGui::TextUnformatted(label);
        for (std::size_t index = 0; index < values.size(); ++index)
        {
            ImGui::PushID(static_cast<int>(index));
            ImGui::SetNextItemWidth(190.0f);
            changed |= InputTextString("##value", values[index]);
            ImGui::SameLine();
            if (ImGui::SmallButton("-")) { values.erase(values.begin() + static_cast<std::ptrdiff_t>(index)); --index; changed = true; }
            ImGui::PopID();
        }
        if (ImGui::SmallButton((std::string("+") + label).c_str())) { values.emplace_back(); changed = true; }
        return changed;
    }

    static bool DrawAnimationSelectorEditor(const char* label, rendern::AnimationStateSelector& selector)
    {
        bool changed = false;
        if (ImGui::TreeNode(label))
        {
            changed |= DrawSelectorStringList("States", selector.states);
            changed |= DrawSelectorStringList("All Tags", selector.allTags);
            changed |= DrawSelectorStringList("Any Tags", selector.anyTags);
            changed |= DrawSelectorStringList("No Tags", selector.noneTags);
            ImGui::TreePop();
        }
        return changed;
    }

    static void DrawAnimationGraphStateInspector(
        rendern::AnimationControllerAsset& controllerAsset,
        const std::vector<rendern::EffectiveAnimationTransition>& effectiveTransitions,
        const rendern::AnimationControllerRuntime& runtime,
        AnimationUIState& uiState,
        AnimationControllerEditorState& editor)
    {
        if (editor.selectedAuthoredTransition >= 0 && static_cast<std::size_t>(editor.selectedAuthoredTransition) < controllerAsset.transitions.size())
        {
            rendern::AnimationTransitionDesc& transition = controllerAsset.transitions[static_cast<std::size_t>(editor.selectedAuthoredTransition)];
            ImGui::SeparatorText("Explicit Transition Inspector");
            bool changed = false;
            auto stateCombo = [&](const char* label, std::string& value, bool allowWildcard)
            {
                if (ImGui::BeginCombo(label, value.empty() ? "<state>" : value.c_str()))
                {
                    if (allowWildcard && ImGui::Selectable("*", value == "*")) { value = "*"; changed = true; }
                    for (const auto& state : controllerAsset.states)
                        if (ImGui::Selectable(state.name.c_str(), value == state.name)) { value = state.name; changed = true; }
                    ImGui::EndCombo();
                }
            };
            stateCombo("From", transition.fromState, true);
            stateCombo("To", transition.toState, false);
            changed |= ImGui::InputInt("Priority", &transition.priority);
            changed |= ImGui::InputFloat("Blend Duration", &transition.blendDurationSeconds);
            changed |= ImGui::Checkbox("Has Exit Time", &transition.hasExitTime);
            if (transition.hasExitTime) changed |= ImGui::SliderFloat("Exit Time", &transition.exitTimeNormalized, 0.0f, 1.0f);
            changed |= DrawControllerConditionEditor(transition.conditions, controllerAsset);
            if (ImGui::Button("Delete Explicit Transition"))
            {
                controllerAsset.transitions.erase(controllerAsset.transitions.begin() + editor.selectedAuthoredTransition);
                editor.selectedAuthoredTransition = -1;
                changed = true;
            }
            if (changed)
            {
                MarkControllerEditorChanged(editor);
            }
            return;
        }

        if (editor.selectedRule >= 0 && static_cast<std::size_t>(editor.selectedRule) < controllerAsset.transitionRules.size())
        {
            rendern::AnimationTransitionRuleDesc& rule = controllerAsset.transitionRules[static_cast<std::size_t>(editor.selectedRule)];
            ImGui::SeparatorText("Transition Rule Inspector");
            bool changed = false;
            changed |= InputTextString("Id", rule.id);
            changed |= DrawAnimationSelectorEditor("From Selector", rule.from);
            changed |= DrawAnimationSelectorEditor("To Selector", rule.to);
            changed |= ImGui::InputInt("Priority", &rule.priority);
            changed |= ImGui::InputFloat("Blend Duration", &rule.blendDurationSeconds);
            changed |= ImGui::Checkbox("Has Exit Time", &rule.hasExitTime);
            if (rule.hasExitTime) changed |= ImGui::SliderFloat("Exit Time", &rule.exitTimeNormalized, 0.0f, 1.0f);
            changed |= DrawControllerConditionEditor(rule.conditions, controllerAsset);
            if (ImGui::Button("Delete Rule"))
            {
                controllerAsset.transitionRules.erase(controllerAsset.transitionRules.begin() + editor.selectedRule);
                editor.selectedRule = -1;
                changed = true;
            }
            if (changed)
            {
                MarkControllerEditorChanged(editor);
            }
            return;
        }
        
        rendern::AnimationStateDesc* selectedState = nullptr;
        if (!uiState.animationGraphSelectedStateName.empty())
        {
            const auto selected = std::find_if(controllerAsset.states.begin(), controllerAsset.states.end(),
                [&](const auto& state) { return state.name == uiState.animationGraphSelectedStateName; });
            if (selected != controllerAsset.states.end()) selectedState = &*selected;
        }
        if (selectedState == nullptr && !controllerAsset.states.empty())
        {
            selectedState = &controllerAsset.states.front();
            uiState.animationGraphSelectedStateName = selectedState->name;
        }
        if (selectedState == nullptr)
        {
            ImGui::TextDisabled("No state selected."); return;
        }

        ImGui::SeparatorText("State Inspector");
        bool changed = false;
        if (editor.stateRenameSourceName != selectedState->name)
        {
            editor.stateRenameSourceName = selectedState->name;
            editor.stateNameDraft = selectedState->name;
        }
        if (InputTextString("Name", editor.stateNameDraft, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            try
            {
                const std::string oldName = selectedState->name;
                rendern::RenameAnimationControllerState(controllerAsset, oldName, editor.stateNameDraft);
                if (oldName != editor.stateNameDraft)
                {
                    if (const auto mode = editor.stateContentModes.find(oldName); mode != editor.stateContentModes.end())
                    {
                        const AnimationControllerEditorState::StateContentMode modeValue = mode->second;
                        editor.stateContentModes.erase(mode);
                        editor.stateContentModes.insert_or_assign(editor.stateNameDraft, modeValue);
                    }
                }
                uiState.animationGraphSelectedStateName = editor.stateNameDraft;
                editor.stateRenameSourceName = editor.stateNameDraft;
                const std::string oldKey = AnimationGraphMakeKey("fsm", controllerAsset.id, oldName);
                const std::string newKey = AnimationGraphMakeKey("fsm", controllerAsset.id, editor.stateNameDraft);
                if (const auto position = uiState.animationGraphFsmNodePositions.find(oldKey); position != uiState.animationGraphFsmNodePositions.end())
                { uiState.animationGraphFsmNodePositions[newKey] = position->second; uiState.animationGraphFsmNodePositions.erase(position); }
                changed = true;
            }
            catch (const std::exception& error)
            {
                editor.message = error.what();
            }
        }
            
		const bool blendState = !selectedState->blend1D.empty() || !selectedState->blend2D.empty();
		auto mode = editor.stateContentModes.try_emplace(
			selectedState->name, InferAnimationStateEditorContentMode(*selectedState)).first;
		int contentMode = mode->second == AnimationControllerEditorState::StateContentMode::SemanticMotion ? 0 : 1;
		const char* contentModes[] = { "Semantic MotionId", blendState ? "Legacy Blend (preserved)" : "Legacy Direct Clip" };
		if (ImGui::Combo("Content Mode", &contentMode, contentModes, IM_ARRAYSIZE(contentModes)))
		{
			if (contentMode == 0)
			{
				mode->second = AnimationControllerEditorState::StateContentMode::SemanticMotion;
				selectedState->motionId.value.clear();
				selectedState->clipName.clear(); selectedState->clipSourceAssetId.clear(); selectedState->blend1D.clear(); selectedState->blend2D.clear();
			}
			else
			{
				mode->second = AnimationControllerEditorState::StateContentMode::LegacyContent;
				selectedState->motionId.value.clear();
			}
			changed = true;
		}
		if (mode->second == AnimationControllerEditorState::StateContentMode::SemanticMotion)
		{
			changed |= InputTextString("MotionId", selectedState->motionId.value);
		}
		else if (!blendState)
		{
			changed |= InputTextString("Clip", selectedState->clipName);
            changed |= InputTextString("Clip Source Asset", selectedState->clipSourceAssetId);
		}
		else ImGui::TextDisabled("Blend data is preserved; use existing blend tooling for point editing.");
        changed |= ImGui::Checkbox("Loop", &selectedState->looping);
        changed |= ImGui::InputFloat("Play Rate", &selectedState->playRate);
        ImGui::TextUnformatted("Tags");
        for (std::size_t index = 0; index < selectedState->tags.size(); ++index)
        {
            ImGui::PushID(static_cast<int>(index));
            ImGui::SetNextItemWidth(180.0f); changed |= InputTextString("##tag", selectedState->tags[index]);
            ImGui::SameLine(); if (ImGui::SmallButton("Remove")) { selectedState->tags.erase(selectedState->tags.begin() + static_cast<std::ptrdiff_t>(index)); --index; changed = true; }
            ImGui::PopID();
        }
         ImGui::SetNextItemWidth(180.0f); InputTextString("##newTag", editor.newTag); ImGui::SameLine();
        if (ImGui::Button("Add Tag") && !editor.newTag.empty()) { selectedState->tags.push_back(std::move(editor.newTag)); editor.newTag.clear(); changed = true; }
		if (!selectedState->blend1D.empty()) DrawAnimationGraphBlend1DPreview(*selectedState, runtime);
		if (!selectedState->blend2D.empty()) DrawAnimationGraphBlend2DPreview(*selectedState, runtime, uiState);
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
			for (const rendern::AnimationNotifyDesc& notify : selectedState->notifies)
			{
				const float x = 16.0f + std::clamp(notify.timeNormalized, 0.0f, 1.0f) * (width - 32.0f);
				const ImVec2 marker = AddImVec2(p0, ImVec2(x, height * 0.5f));
				drawList->AddCircleFilled(marker, 5.0f, notify.fireOnEnter ? IM_COL32(255, 200, 90, 255) : IM_COL32(110, 190, 255, 255));
				drawList->AddText(AddImVec2(marker, ImVec2(-10.0f, -18.0f)), IM_COL32(220, 220, 220, 255), notify.id.c_str());
			}
			ImGui::Dummy(ImVec2(width, height));
		}
		const std::vector<std::string> references = rendern::FindAnimationControllerStateReferences(controllerAsset, selectedState->name);
        if (!references.empty()) ImGui::TextDisabled("Delete blocked: %s", references.front().c_str());
		bool deletedState = false;
		if (references.empty() && ImGui::Button("Delete State"))
		{
			const std::string deleted = selectedState->name;
			try { rendern::DeleteAnimationControllerState(controllerAsset, deleted); editor.stateContentModes.erase(deleted); uiState.animationGraphSelectedStateName.clear(); changed = true; deletedState = true; }
			catch (const std::exception& error) { editor.message = error.what(); }
		}
		if (changed) MarkControllerEditorChanged(editor);
		if (deletedState) return;

        ImGui::SeparatorText("Effective Topology");
        for (const rendern::EffectiveAnimationTransition& effective : effectiveTransitions)
        {
            const auto& transition = effective.transition;
            if (transition.fromState != selectedState->name && transition.toState != selectedState->name) continue;
            const char* origin = effective.origin == rendern::AnimationTransitionOrigin::Rule ? "Rule" :
                (effective.origin == rendern::AnimationTransitionOrigin::ExplicitWildcard ? "Wildcard explicit" : "Explicit");
            ImGui::PushID(&effective);
            if (ImGui::Selectable((transition.fromState + " -> " + transition.toState).c_str()))
            {
				if (effective.origin == rendern::AnimationTransitionOrigin::Rule)
				{
					editor.selectedRule = effective.authoredRuleIndex < controllerAsset.transitionRules.size()
						? static_cast<int>(effective.authoredRuleIndex) : -1;
                    editor.selectedAuthoredTransition = -1;
                }
                else { editor.selectedAuthoredTransition = static_cast<int>(effective.authoredTransitionIndex); editor.selectedRule = -1; }
            }
            ImGui::SameLine(); ImGui::TextDisabled("%s | p%d | %.2fs%s", origin, transition.priority, transition.blendDurationSeconds,
                effective.origin == rendern::AnimationTransitionOrigin::Rule ? (" | " + effective.sourceRuleId).c_str() : "");
            ImGui::PopID();
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
        AnimationUIState& uiState,
        rendern::EditorSelectionService& selection,
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
                SetAnimationRuntimePinnedTarget(uiState, selection, gameplayRuntime, -1);
            }
            else if (AnimationNodeAlive(level, scene.editorSelectedNode))
            {
                const rendern::LevelNode& selectedNode = level.nodes[static_cast<std::size_t>(scene.editorSelectedNode)];
                if (!selectedNode.skinnedMesh.empty())
                {
                    SetAnimationRuntimePinnedTarget(uiState, selection, gameplayRuntime, scene.editorSelectedNode);
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Use current selection") && AnimationNodeAlive(level, scene.editorSelectedNode))
        {
            const rendern::LevelNode& selectedNode = level.nodes[static_cast<std::size_t>(scene.editorSelectedNode)];
            if (!selectedNode.skinnedMesh.empty())
            {
                SetAnimationRuntimePinnedTarget(uiState, selection, gameplayRuntime, scene.editorSelectedNode);
            }
        }

        const char* previewText = "Auto target";
        if (uiState.animationRuntimePinnedNodeIndex >= 0 && AnimationNodeAlive(level, uiState.animationRuntimePinnedNodeIndex))
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
                SetAnimationRuntimePinnedTarget(uiState, selection, gameplayRuntime, -1);
            }
            for (const int nodeIndex : candidates)
            {
                const bool selected = uiState.animationRuntimePinnedNodeIndex == nodeIndex;
                const rendern::LevelNode& node = level.nodes[static_cast<std::size_t>(nodeIndex)];
                if (ImGui::Selectable(node.name.c_str(), selected))
                {
                    SetAnimationRuntimePinnedTarget(uiState, selection, gameplayRuntime, nodeIndex);
                }
            }
            ImGui::EndCombo();
        }

        AnimationGraphContext ctx = GetAnimationRuntimeContext(level, levelInst, scene, uiState, selection, gameplayRuntime);
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
            scene.EditorSetSelectionSingle(animationRuntimeViewModel.nodeIndex);
        }
        ImGui::SameLine();
        if (ImGui::Button("Open graph"))
        {
            scene.EditorSetSelectionSingle(animationRuntimeViewModel.nodeIndex);
            uiState.animationGraphWindowOpen = true;
            uiState.animationGraphRequestFocus = true;
            uiState.animationGraphSelectedStateName = animationRuntimeViewModel.currentStateName;
        }

        DrawAnimationRuntimeTargetSection(animationRuntimeViewModel);
        DrawAnimationRuntimeLiveStateSection(animationRuntimeViewModel);
        if (ctx.controllerAsset != nullptr)
        {
            if (const auto* currentState = rendern::FindAnimationControllerState(*ctx.controllerAsset, runtime.currentStateName))
            {
                DrawWorkspaceResolution(level, ctx, *currentState, uiState);
            }
        }

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
