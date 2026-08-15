    void DrawAnimationGraphWindow(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        rendern::Scene& scene,
        AnimationUIState& uiState)
    {
        if (!uiState.animationGraphWindowOpen)
        {
            return;
        }

        if (uiState.animationGraphRequestFocus)
        {
            ImGui::SetNextWindowFocus();
            uiState.animationGraphRequestFocus = false;
        }

        if (!ImGui::Begin("Animation Graph", &uiState.animationGraphWindowOpen))
        {
            ImGui::End();
            return;
        }

        AnimationGraphContext ctx = GetAnimationGraphContext(level, levelInst, scene);
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
        
        ImGui::Text("Controller: %s", ctx.controllerAsset->id.c_str());
        AnimationControllerEditorState& controllerEditor = uiState.controllerEditorStates[ctx.controllerAsset->id];
		if (!controllerEditor.initialized)
		{
			controllerEditor.initialized = true;
			controllerEditor.workingController = *ctx.controllerAsset;
			controllerEditor.persistedController = *ctx.controllerAsset;
			RebuildAnimationStateEditorContentModes(controllerEditor);
			RebuildControllerEditorTopology(controllerEditor);
		}
		rendern::AnimationControllerAsset& workingController = controllerEditor.workingController;
		if (uiState.animationGraphSelectedStateName.empty() ||
			rendern::FindAnimationControllerState(workingController, uiState.animationGraphSelectedStateName) == nullptr)
		{
			const std::string& runtimeState = ctx.skinnedItem->controller.currentStateName;
			if (!runtimeState.empty() && rendern::FindAnimationControllerState(workingController, runtimeState) != nullptr)
			{
				uiState.animationGraphSelectedStateName = runtimeState;
			}
			else if (rendern::FindAnimationControllerState(workingController, workingController.defaultState) != nullptr)
			{
				uiState.animationGraphSelectedStateName = workingController.defaultState;
			}
			else
			{
				uiState.animationGraphSelectedStateName = workingController.states.empty()
					? std::string{}
					: workingController.states.front().name;
			}
		}
		if (controllerEditor.reloadRequired)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.74f, 0.25f, 1.0f), "Authored controller differs from bound runtime; runtime highlighting is disabled until rebind.");
		}
		if (!controllerEditor.message.empty())
		{
			ImGui::TextColored(controllerEditor.topologyValid ? ImVec4(0.55f, 0.9f, 0.55f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", controllerEditor.message.c_str());
		}
		const auto path = level.animationControllerAssetPaths.find(ctx.controllerAsset->id);
		const bool writable = path != level.animationControllerAssetPaths.end() && !path->second.empty();
		ImGui::BeginDisabled(!controllerEditor.dirty || !controllerEditor.topologyValid || !writable);
		if (ImGui::Button("Save Controller"))
		{
			try
			{
				rendern::SaveAnimationControllerAssetToJson(path->second, workingController);
				MarkControllerEditorSaved(controllerEditor);
			}
			catch (const std::exception& error) { controllerEditor.message = error.what(); }
		}
		ImGui::EndDisabled();
		if (!writable) { ImGui::SameLine(); ImGui::TextDisabled("Save unavailable: controller is inline or has no external path."); }
		ImGui::SameLine();
		ImGui::BeginDisabled(!writable || controllerEditor.dirty);
		if (ImGui::Button("Reload / Rebind"))
		{
			try
			{
				rendern::AnimationControllerAsset loaded = rendern::LoadAnimationControllerAssetFromJson(path->second, ctx.controllerAsset->id);
				rendern::AnimationControllerAsset& stored = level.animationControllers.at(ctx.controllerAsset->id);
				stored = std::move(loaded);
				const rendern::AnimationProfileAsset* profile = nullptr;
				if (!ctx.node->animationProfile.empty()) if (const auto found = level.animationProfiles.find(ctx.node->animationProfile); found != level.animationProfiles.end()) profile = &found->second;
				ctx.skinnedItem->controller.stateMachineAsset = nullptr;
				rendern::BindAnimationControllerStateMachine(ctx.skinnedItem->controller, ctx.skinnedItem->asset->mesh.skeleton,
					ctx.skinnedItem->asset->clips, ctx.skinnedItem->asset->clipSourceAssetIds, stored, profile,
					ctx.skinnedItem->autoplay, ctx.skinnedItem->controller.paused, ctx.skinnedItem->debugForceBindPose);
				MarkControllerEditorRebound(controllerEditor, stored);
				ctx.controllerAsset = &stored;
			}
			catch (const std::exception& error) { controllerEditor.message = error.what(); }
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!controllerEditor.dirty);
		if (ImGui::Button("Discard Working Changes"))
		{
			DiscardControllerWorkingChanges(controllerEditor);
		}
		ImGui::EndDisabled();
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
            	DrawAnimationControllerParameters(workingController, controllerEditor);
            	DrawAnimationGraphFsmCanvas(workingController, controllerEditor.effectiveTransitions, ctx.skinnedItem->controller, uiState, controllerEditor);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Assets"))
            {
            	DrawAnimationGraphAssetCanvas(level, *ctx.node, workingController, uiState);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Bindings"))
            {
                DrawAnimationGraphBindings(*ctx.controllerAsset);
                ImGui::EndTabItem();
            }
        	if (ImGui::BeginTabItem("Diagnostics"))
        	{
        		DrawAnimationGraphDiagnosticsPanel(level, workingController);
        		if (!controllerEditor.topologyValid) ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", controllerEditor.message.c_str());
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

    	if (const auto* selected = rendern::FindAnimationControllerState(
    		workingController, uiState.animationGraphSelectedStateName))
        {
    		AnimationGraphContext authoredContext = ctx;
    		authoredContext.controllerAsset = &workingController;
    		DrawWorkspaceResolution(level, authoredContext, *selected, uiState);
        }
        
        ImGui::End();
    }
