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

        if (uiState.animationGraphSelectedStateName.empty() || rendern::FindAnimationControllerState(*ctx.controllerAsset, uiState.animationGraphSelectedStateName) == nullptr)
        {
            uiState.animationGraphSelectedStateName = ctx.skinnedItem->controller.currentStateName.empty()
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
                DrawAnimationGraphFsmCanvas(*ctx.controllerAsset, ctx.skinnedItem->controller, uiState);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Assets"))
            {
                DrawAnimationGraphAssetCanvas(level, *ctx.node, *ctx.controllerAsset, uiState);
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
