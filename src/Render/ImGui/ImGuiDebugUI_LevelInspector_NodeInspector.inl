namespace rendern::ui::level_ui_detail
{
    static void DrawNodeSelectionInspector(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        AssetManager& assets,
        rendern::Scene& scene,
        const DerivedLists& derived,
        LevelEditorUIState& st)
    {
        rendern::LevelNode& node = level.nodes[static_cast<std::size_t>(st.selectedNode)];

        if (st.prevSelectedNode != st.selectedNode)
        {
            std::snprintf(st.nameBuf, sizeof(st.nameBuf), "%s", node.name.c_str());
            st.prevSelectedNode = st.selectedNode;
        }

        ImGui::Text("Node #%d", st.selectedNode);

        if (ImGui::InputText("Name", st.nameBuf, sizeof(st.nameBuf)))
            node.name = std::string(st.nameBuf);

        bool vis = node.visible;
        if (ImGui::Checkbox("Visible", &vis))
            levelInst.SetNodeVisible(level, scene, assets, st.selectedNode, vis);

        {
            std::vector<std::string> items;
            items.reserve(derived.meshIds.size() + 2);
            items.push_back("(none)");
            for (const auto& id : derived.meshIds) items.push_back(id);

            if (!node.mesh.empty() && !level.meshes.contains(node.mesh))
                items.push_back(std::string("<missing> ") + node.mesh);

            int current = 0;
            if (!node.mesh.empty())
            {
                for (std::size_t i = 1; i < items.size(); ++i)
                {
                    if (items[i] == node.mesh)
                    {
                        current = static_cast<int>(i);
                        break;
                    }
                }
            }

            std::vector<const char*> citems;
            citems.reserve(items.size());
            for (auto& s : items) citems.push_back(s.c_str());

            if (ImGui::Combo("Mesh", &current, citems.data(), static_cast<int>(citems.size())))
            {
                if (current == 0)
                    levelInst.SetNodeMesh(level, scene, assets, st.selectedNode, "");
                else
                    levelInst.SetNodeMesh(level, scene, assets, st.selectedNode, items[static_cast<std::size_t>(current)]);
            }
        }

        {
            std::vector<std::string> items;
            items.reserve(derived.modelIds.size() + 2);
            items.push_back("(none)");
            for (const auto& id : derived.modelIds) items.push_back(id);

            if (!node.model.empty() && !level.models.contains(node.model))
                items.push_back(std::string("<missing> ") + node.model);

            int current = 0;
            if (!node.model.empty())
            {
                for (std::size_t i = 1; i < items.size(); ++i)
                {
                    if (items[i] == node.model)
                    {
                        current = static_cast<int>(i);
                        break;
                    }
                }
            }

            std::vector<const char*> citems;
            citems.reserve(items.size());
            for (auto& s : items) citems.push_back(s.c_str());

            if (ImGui::Combo("Model", &current, citems.data(), static_cast<int>(citems.size())))
            {
                if (current == 0)
                    levelInst.SetNodeModel(level, scene, assets, st.selectedNode, "");
                else
                    levelInst.SetNodeModel(level, scene, assets, st.selectedNode, items[static_cast<std::size_t>(current)]);
            }
        }

        {
            std::vector<std::string> items;
            items.reserve(derived.skinnedMeshIds.size() + 2);
            items.push_back("(none)");
            for (const auto& id : derived.skinnedMeshIds) items.push_back(id);

            if (!node.skinnedMesh.empty() && !level.skinnedMeshes.contains(node.skinnedMesh))
                items.push_back(std::string("<missing> ") + node.skinnedMesh);

            int current = 0;
            if (!node.skinnedMesh.empty())
            {
                for (std::size_t i = 1; i < items.size(); ++i)
                {
                    if (items[i] == node.skinnedMesh)
                    {
                        current = static_cast<int>(i);
                        break;
                    }
                }
            }

            std::vector<const char*> citems;
            citems.reserve(items.size());
            for (auto& s : items) citems.push_back(s.c_str());

            if (ImGui::Combo("Skinned Mesh", &current, citems.data(), static_cast<int>(citems.size())))
            {
                if (current == 0)
                    levelInst.SetNodeSkinnedMesh(level, scene, assets, st.selectedNode, "");
                else
                    levelInst.SetNodeSkinnedMesh(level, scene, assets, st.selectedNode, items[static_cast<std::size_t>(current)]);
            }
        }

        {
            const bool isModelNode = !node.model.empty();
            const bool isSkinnedNode = !node.skinnedMesh.empty();
            ImGui::TextDisabled("Node kind: %s", isSkinnedNode ? "Skinned" : (isModelNode ? "Model" : (!node.mesh.empty() ? "Mesh" : "Empty")));
            if (isModelNode)
            {
                auto itModel = level.models.find(node.model);
                if (itModel != level.models.end())
                {
                    ImGui::TextDisabled("Model path: %s", itModel->second.path.c_str());
                    try
                    {
                        const rendern::ImportedModelScene meta = rendern::LoadAssimpScene(itModel->second.path, itModel->second.flipUVs);
                        ImGui::TextDisabled("Submeshes: %d", static_cast<int>(meta.submeshes.size()));
                        ImGui::SeparatorText("Material Overrides");
                        for (const rendern::ImportedSubmeshInfo& sub : meta.submeshes)
                        {
                            std::vector<std::string> overrideItems;
                            overrideItems.reserve(derived.materialIds.size() + 2);
                            overrideItems.push_back("(default)");
                            for (const auto& id : derived.materialIds) overrideItems.push_back(id);

                            int overrideCurrent = 0;
                            if (auto itOv = node.materialOverrides.find(sub.submeshIndex); itOv != node.materialOverrides.end())
                            {
                                for (std::size_t oi = 1; oi < overrideItems.size(); ++oi)
                                {
                                    if (overrideItems[oi] == itOv->second)
                                    {
                                        overrideCurrent = static_cast<int>(oi);
                                        break;
                                    }
                                }
                            }

                            std::vector<const char*> overrideCItems;
                            overrideCItems.reserve(overrideItems.size());
                            for (auto& s : overrideItems) overrideCItems.push_back(s.c_str());

                            const std::string label = "Submesh " + std::to_string(sub.submeshIndex) + "##mat_override" + std::to_string(sub.submeshIndex);
                            if (ImGui::Combo(label.c_str(), &overrideCurrent, overrideCItems.data(), static_cast<int>(overrideCItems.size())))
                            {
                                if (overrideCurrent == 0)
                                    levelInst.SetNodeMaterialOverride(level, scene, assets, st.selectedNode, sub.submeshIndex, "");
                                else
                                    levelInst.SetNodeMaterialOverride(level, scene, assets, st.selectedNode, sub.submeshIndex, overrideItems[static_cast<std::size_t>(overrideCurrent)]);
                            }
                            ImGui::SameLine();
                            ImGui::TextDisabled("%s", sub.name.c_str());
                        }
                    }
                    catch (...)
                    {
                        ImGui::TextDisabled("Failed to read model metadata.");
                    }
                }
            }
        }

        {
            const bool isSkinnedNode = !node.skinnedMesh.empty();
            if (isSkinnedNode)
            {
                auto RefreshSkinnedRuntime = [&]() -> rendern::SkinnedDrawItem*
                    {
                        const int drawIndex = levelInst.GetNodeSkinnedDrawIndex(st.selectedNode);
                        return levelInst.GetSkinnedDrawItem(scene, drawIndex);
                    };

                rendern::SkinnedDrawItem* skinnedItem = RefreshSkinnedRuntime();

                auto itSkinned = level.skinnedMeshes.find(node.skinnedMesh);
                if (itSkinned != level.skinnedMeshes.end())
                {
                    ImGui::TextDisabled("Skinned path: %s", itSkinned->second.path.c_str());
                }

                ImGui::SeparatorText("Animation");

                if (!skinnedItem || !skinnedItem->asset)
                {
                    ImGui::TextDisabled("Runtime skinned draw is not instantiated.");
                }
                else
                {
                    const auto& skeleton = skinnedItem->asset->mesh.skeleton;
                    const std::size_t boneCount = skeleton.bones.size();
                    const std::size_t clipCount = skinnedItem->asset->clips.size();
                    const std::string activeClipName =
                        (skinnedItem->animator.clip != nullptr)
                        ? skinnedItem->animator.clip->name
                        : std::string("<none>");

                    ImGui::Text("Bones: %d", static_cast<int>(boneCount));
                    ImGui::Text("Clips: %d", static_cast<int>(clipCount));
                    ImGui::Text("Active clip: %s", activeClipName.c_str());
                    ImGui::Text("Palette size: %d", static_cast<int>(skinnedItem->animator.skinMatrices.size()));

                    std::vector<std::string> animationAssetItems;
                    animationAssetItems.reserve(level.animations.size() + 1);
                    animationAssetItems.push_back("(embedded clips)");
                    for (const auto& [animationId, _] : level.animations)
                    {
                        animationAssetItems.push_back(animationId);
                    }

                    int animationAssetCurrent = 0;
                    if (!node.animation.empty())
                    {
                        for (std::size_t assetIndex = 1; assetIndex < animationAssetItems.size(); ++assetIndex)
                        {
                            if (animationAssetItems[assetIndex] == node.animation)
                            {
                                animationAssetCurrent = static_cast<int>(assetIndex);
                                break;
                            }
                        }
                    }

                    std::vector<const char*> animationAssetCItems;
                    animationAssetCItems.reserve(animationAssetItems.size());
                    for (auto& s : animationAssetItems) animationAssetCItems.push_back(s.c_str());

                    if (ImGui::Combo("Animation asset", &animationAssetCurrent, animationAssetCItems.data(), static_cast<int>(animationAssetCItems.size())))
                    {
                        const std::string selectedAnimationAsset =
                            (animationAssetCurrent <= 0)
                            ? std::string{}
                        : animationAssetItems[static_cast<std::size_t>(animationAssetCurrent)];
                        levelInst.SetNodeAnimationAsset(level, scene, assets, st.selectedNode, selectedAnimationAsset);
                        skinnedItem = RefreshSkinnedRuntime();
                    }

                    if (skinnedItem && skinnedItem->asset)
                    {
                        std::vector<std::string> controllerItems;
                        controllerItems.reserve(level.animationControllers.size() + 1);
                        controllerItems.push_back("(legacy clip mode)");
                        for (const auto& [controllerId, _] : level.animationControllers)
                        {
                            controllerItems.push_back(controllerId);
                        }

                        int controllerCurrent = 0;
                        if (!node.animationController.empty())
                        {
                            for (std::size_t controllerIndex = 1; controllerIndex < controllerItems.size(); ++controllerIndex)
                            {
                                if (controllerItems[controllerIndex] == node.animationController)
                                {
                                    controllerCurrent = static_cast<int>(controllerIndex);
                                    break;
                                }
                            }
                        }

                        std::vector<const char*> controllerCItems;
                        controllerCItems.reserve(controllerItems.size());
                        for (auto& s : controllerItems) controllerCItems.push_back(s.c_str());

                        if (ImGui::Combo("Animation controller", &controllerCurrent, controllerCItems.data(), static_cast<int>(controllerCItems.size())))
                        {
                            const std::string selectedController =
                                (controllerCurrent <= 0)
                                ? std::string{}
                            : controllerItems[static_cast<std::size_t>(controllerCurrent)];
                            levelInst.SetNodeAnimationController(level, scene, assets, st.selectedNode, selectedController);
                            skinnedItem = RefreshSkinnedRuntime();
                        }

                        if (skinnedItem && skinnedItem->asset)
                        {
                            if (!node.animation.empty())
                            {
                                for (const auto& sourceInfo : skinnedItem->asset->externalAnimationSources)
                                {
                                    if (sourceInfo.assetId == node.animation)
                                    {
                                        ImGui::TextDisabled(
                                            "External import: clips=%d matched=%d/%d ignored=%d",
                                            static_cast<int>(sourceInfo.clipCount),
                                            static_cast<int>(sourceInfo.matchedChannelCount),
                                            static_cast<int>(sourceInfo.sourceChannelCount),
                                            static_cast<int>(sourceInfo.ignoredChannelCount));
                                        if (!sourceInfo.diagnosticMessage.empty())
                                        {
                                            ImGui::TextDisabled("%s", sourceInfo.diagnosticMessage.c_str());
                                        }
                                        break;
                                    }
                                }
                            }

                            const bool usingController =
                                skinnedItem->controller.mode == rendern::AnimationControllerMode::StateMachine &&
                                skinnedItem->controller.stateMachineAsset != nullptr;

                            if (usingController)
                            {
                                ImGui::Text("Controller state: %s", skinnedItem->controller.currentStateName.c_str());
                                if (skinnedItem->controller.currentStateUsesBlend1D)
                                {
                                    ImGui::TextDisabled(
                                        "Blend1D: %s = %.3f",
                                        skinnedItem->controller.currentBlendParameterName.c_str(),
                                        skinnedItem->controller.currentBlendParameterValue);
                                    if (!skinnedItem->controller.currentBlendSecondaryClipName.empty())
                                    {
                                        ImGui::TextDisabled(
                                            "State blend: %s -> %s (%.2f)",
                                            skinnedItem->controller.currentBlendPrimaryClipName.c_str(),
                                            skinnedItem->controller.currentBlendSecondaryClipName.c_str(),
                                            skinnedItem->controller.blendSecondaryAlpha);
                                    }
                                    else if (!skinnedItem->controller.currentBlendPrimaryClipName.empty())
                                    {
                                        ImGui::TextDisabled(
                                            "State blend clip: %s",
                                            skinnedItem->controller.currentBlendPrimaryClipName.c_str());
                                    }
                                }
                                if (skinnedItem->controller.transitionActive)
                                {
                                    const float blendAlpha =
                                        (skinnedItem->controller.transitionDurationSeconds > 1e-6f)
                                        ? std::clamp(
                                            skinnedItem->controller.transitionElapsedSeconds / skinnedItem->controller.transitionDurationSeconds,
                                            0.0f,
                                            1.0f)
                                        : 1.0f;
                                    ImGui::TextDisabled(
                                        "Transition: %s -> %s (%.2f)",
                                        skinnedItem->controller.transitionSourceStateName.c_str(),
                                        skinnedItem->controller.currentStateName.c_str(),
                                        blendAlpha);
                                }

                                const auto& recentNotifies = PeekAnimationControllerNotifyEvents(skinnedItem->controller);
                                if (!recentNotifies.empty())
                                {
                                    ImGui::SeparatorText("Recent Notifies");
                                    const std::size_t firstNotify =
                                        (recentNotifies.size() > 6)
                                        ? recentNotifies.size() - 6
                                        : 0;
                                    for (std::size_t notifyIndex = firstNotify; notifyIndex < recentNotifies.size(); ++notifyIndex)
                                    {
                                        const auto& notify = recentNotifies[notifyIndex];
                                        ImGui::BulletText(
                                            "#%llu %s (%s @ %.2f)",
                                            static_cast<unsigned long long>(notify.sequence),
                                            notify.id.c_str(),
                                            notify.stateName.c_str(),
                                            notify.normalizedTime);
                                    }
                                }

                                if (!skinnedItem->controller.debugLastTransitionSelection.empty())
                                {
                                    ImGui::TextDisabled(
                                        "Selected transition: %s",
                                        skinnedItem->controller.debugLastTransitionSelection.c_str());
                                }
                                if (!skinnedItem->controller.debugTransitionCandidates.empty())
                                {
                                    ImGui::SeparatorText("Transition Candidates");
                                    const std::size_t firstCandidate =
                                        (skinnedItem->controller.debugTransitionCandidates.size() > 8)
                                        ? skinnedItem->controller.debugTransitionCandidates.size() - 8
                                        : 0;
                                    for (std::size_t candidateIndex = firstCandidate;
                                        candidateIndex < skinnedItem->controller.debugTransitionCandidates.size();
                                        ++candidateIndex)
                                    {
                                        ImGui::BulletText(
                                            "%s",
                                            skinnedItem->controller.debugTransitionCandidates[candidateIndex].c_str());
                                    }
                                }
                                if (!skinnedItem->controller.recentRoutedGameplayEvents.empty())
                                {
                                    ImGui::SeparatorText("Gameplay Events");
                                    const std::size_t firstEvent =
                                        (skinnedItem->controller.recentRoutedGameplayEvents.size() > 8)
                                        ? skinnedItem->controller.recentRoutedGameplayEvents.size() - 8
                                        : 0;
                                    for (std::size_t eventIndex = firstEvent;
                                        eventIndex < skinnedItem->controller.recentRoutedGameplayEvents.size();
                                        ++eventIndex)
                                    {
                                        ImGui::BulletText(
                                            "%s",
                                            skinnedItem->controller.recentRoutedGameplayEvents[eventIndex].c_str());
                                    }
                                }

                                const rendern::AnimationControllerAsset& controllerAsset = *skinnedItem->controller.stateMachineAsset;
                                if (!controllerAsset.states.empty())
                                {
                                    std::vector<const char*> stateItems;
                                    stateItems.reserve(controllerAsset.states.size());
                                    int stateCurrent = 0;
                                    for (std::size_t stateIndex = 0; stateIndex < controllerAsset.states.size(); ++stateIndex)
                                    {
                                        stateItems.push_back(controllerAsset.states[stateIndex].name.c_str());
                                        if (controllerAsset.states[stateIndex].name == skinnedItem->controller.currentStateName)
                                        {
                                            stateCurrent = static_cast<int>(stateIndex);
                                        }
                                    }

                                    if (ImGui::Combo("State override", &stateCurrent, stateItems.data(), static_cast<int>(stateItems.size())))
                                    {
                                        RequestAnimationControllerState(
                                            skinnedItem->controller,
                                            controllerAsset.states[static_cast<std::size_t>(stateCurrent)].name);
                                        UpdateAnimationControllerRuntime(skinnedItem->controller, skinnedItem->animator, 0.0f);
                                    }
                                }

                                if (!controllerAsset.parameters.empty())
                                {
                                    ImGui::SeparatorText("Controller Parameters");
                                    for (const rendern::AnimationParameterDesc& paramDesc : controllerAsset.parameters)
                                    {
                                        rendern::AnimationParameterValue* runtimeParam =
                                            FindAnimationParameter(skinnedItem->controller.parameters, paramDesc.name);
                                        if (runtimeParam == nullptr)
                                        {
                                            skinnedItem->controller.parameters.values[paramDesc.name] = paramDesc.defaultValue;
                                            runtimeParam = FindAnimationParameter(skinnedItem->controller.parameters, paramDesc.name);
                                        }
                                        if (runtimeParam == nullptr)
                                        {
                                            continue;
                                        }

                                        switch (paramDesc.defaultValue.type)
                                        {
                                        case rendern::AnimationParameterType::Bool:
                                        {
                                            bool value = runtimeParam->boolValue;
                                            const std::string label = paramDesc.name + "##anim_bool";
                                            if (ImGui::Checkbox(label.c_str(), &value))
                                            {
                                                SetAnimationParameter(skinnedItem->controller.parameters, paramDesc.name, value);
                                                UpdateAnimationControllerRuntime(skinnedItem->controller, skinnedItem->animator, 0.0f);
                                            }
                                            break;
                                        }
                                        case rendern::AnimationParameterType::Int:
                                        {
                                            int value = runtimeParam->intValue;
                                            const std::string label = paramDesc.name + "##anim_int";
                                            if (ImGui::InputInt(label.c_str(), &value))
                                            {
                                                SetAnimationParameter(skinnedItem->controller.parameters, paramDesc.name, value);
                                                UpdateAnimationControllerRuntime(skinnedItem->controller, skinnedItem->animator, 0.0f);
                                            }
                                            break;
                                        }
                                        case rendern::AnimationParameterType::Float:
                                        {
                                            float value = runtimeParam->floatValue;
                                            const std::string label = paramDesc.name + "##anim_float";
                                            if (ImGui::DragFloat(label.c_str(), &value, 0.01f))
                                            {
                                                SetAnimationParameter(skinnedItem->controller.parameters, paramDesc.name, value);
                                                UpdateAnimationControllerRuntime(skinnedItem->controller, skinnedItem->animator, 0.0f);
                                            }
                                            break;
                                        }
                                        case rendern::AnimationParameterType::Trigger:
                                        {
                                            const std::string label = "Fire " + paramDesc.name + "##anim_trigger";
                                            if (ImGui::Button(label.c_str()))
                                            {
                                                FireAnimationTrigger(skinnedItem->controller.parameters, paramDesc.name);
                                                UpdateAnimationControllerRuntime(skinnedItem->controller, skinnedItem->animator, 0.0f);
                                            }
                                            break;
                                        }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                std::vector<int> visibleClipIndices;
                                std::vector<std::string> clipItems;
                                visibleClipIndices.reserve(skinnedItem->asset->clips.size());
                                clipItems.reserve(skinnedItem->asset->clips.size() + 1);
                                clipItems.push_back("(bind pose)");
                                for (std::size_t clipIndex = 0; clipIndex < skinnedItem->asset->clips.size(); ++clipIndex)
                                {
                                    const bool sourceMatches =
                                        node.animation.empty()
                                        ? (clipIndex < skinnedItem->asset->clipSourceAssetIds.size() && skinnedItem->asset->clipSourceAssetIds[clipIndex].empty())
                                        : (clipIndex < skinnedItem->asset->clipSourceAssetIds.size() && skinnedItem->asset->clipSourceAssetIds[clipIndex] == node.animation);
                                    if (!sourceMatches)
                                    {
                                        continue;
                                    }
                                    visibleClipIndices.push_back(static_cast<int>(clipIndex));
                                    const auto& clip = skinnedItem->asset->clips[clipIndex];
                                    clipItems.push_back(clip.name.empty() ? std::string("<unnamed clip>") : clip.name);
                                }

                                int clipCurrent = 0;
                                if (skinnedItem->activeClipIndex >= 0)
                                {
                                    for (std::size_t visibleIndex = 0; visibleIndex < visibleClipIndices.size(); ++visibleIndex)
                                    {
                                        if (visibleClipIndices[visibleIndex] == skinnedItem->activeClipIndex)
                                        {
                                            clipCurrent = static_cast<int>(visibleIndex) + 1;
                                            break;
                                        }
                                    }
                                }

                                std::vector<const char*> clipCItems;
                                clipCItems.reserve(clipItems.size());
                                for (auto& s : clipItems) clipCItems.push_back(s.c_str());

                                if (ImGui::Combo("Clip", &clipCurrent, clipCItems.data(), static_cast<int>(clipCItems.size())))
                                {
                                    if (clipCurrent <= 0)
                                    {
                                        node.animationClip.clear();
                                        skinnedItem->activeClipIndex = -1;
                                        skinnedItem->debugForceBindPose = true;
                                        skinnedItem->autoplay = false;
                                        skinnedItem->animator.paused = true;
                                        SyncAnimationControllerLegacyClip(
                                            skinnedItem->controller,
                                            skinnedItem->asset->mesh.skeleton,
                                            skinnedItem->asset->clips,
                                            skinnedItem->activeClipIndex,
                                            false,
                                            node.animationLoop,
                                            node.animationPlayRate,
                                            true,
                                            true);
                                        UpdateAnimationControllerRuntime(skinnedItem->controller, skinnedItem->animator, 0.0f);
                                    }
                                    else
                                    {
                                        const int newClipIndex = visibleClipIndices[static_cast<std::size_t>(clipCurrent - 1)];
                                        node.animationClip = skinnedItem->asset->clips[static_cast<std::size_t>(newClipIndex)].name;
                                        skinnedItem->activeClipIndex = newClipIndex;
                                        skinnedItem->debugForceBindPose = false;
                                        skinnedItem->autoplay = node.animationAutoplay;
                                        skinnedItem->animator.paused = !node.animationAutoplay;
                                        SyncAnimationControllerLegacyClip(
                                            skinnedItem->controller,
                                            skinnedItem->asset->mesh.skeleton,
                                            skinnedItem->asset->clips,
                                            newClipIndex,
                                            node.animationAutoplay,
                                            node.animationLoop,
                                            node.animationPlayRate,
                                            !node.animationAutoplay,
                                            false);
                                        UpdateAnimationControllerRuntime(skinnedItem->controller, skinnedItem->animator, 0.0f);
                                    }
                                }
                            }

                            bool inPlaceRootMotion = node.animationInPlace;
                            if (ImGui::Checkbox("In-place root motion", &inPlaceRootMotion))
                            {
                                node.animationInPlace = inPlaceRootMotion;
                                skinnedItem->controller.rootMotionMode = inPlaceRootMotion
                                    ? rendern::AnimationRootMotionMode::InPlace
                                    : rendern::AnimationRootMotionMode::Allow;
                                UpdateAnimationControllerRuntime(skinnedItem->controller, skinnedItem->animator, 0.0f);
                            }
                            ImGui::TextDisabled(
                                "Suppressed root delta: (%.3f, %.3f, %.3f)",
                                skinnedItem->controller.lastAppliedRootMotionDelta.x,
                                skinnedItem->controller.lastAppliedRootMotionDelta.y,
                                skinnedItem->controller.lastAppliedRootMotionDelta.z);

                            bool autoplay = node.animationAutoplay;
                            if (ImGui::Checkbox("Autoplay", &autoplay))
                            {
                                node.animationAutoplay = autoplay;
                                skinnedItem->autoplay = autoplay;
                                skinnedItem->controller.autoplay = autoplay;
                                if (autoplay)
                                {
                                    skinnedItem->debugForceBindPose = false;
                                    skinnedItem->controller.forceBindPose = false;
                                    skinnedItem->animator.paused = false;
                                    skinnedItem->controller.paused = false;
                                }
                                else
                                {
                                    skinnedItem->animator.paused = true;
                                    skinnedItem->controller.paused = true;
                                }
                                UpdateAnimationControllerRuntime(skinnedItem->controller, skinnedItem->animator, 0.0f);
                            }

                            if (!usingController)
                            {
                                bool loop = node.animationLoop;
                                if (ImGui::Checkbox("Loop", &loop))
                                {
                                    node.animationLoop = loop;
                                    skinnedItem->animator.looping = loop;
                                    skinnedItem->controller.looping = loop;
                                    if (skinnedItem->animator.clip)
                                    {
                                        skinnedItem->animator.timeSeconds = NormalizeAnimationTimeSeconds(*skinnedItem->animator.clip, skinnedItem->animator.timeSeconds, loop);
                                    }
                                    UpdateAnimationControllerRuntime(skinnedItem->controller, skinnedItem->animator, 0.0f);
                                }

                                float playRate = node.animationPlayRate;
                                if (ImGui::SliderFloat("Play rate", &playRate, 0.0f, 4.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                                {
                                    node.animationPlayRate = playRate;
                                    skinnedItem->animator.playRate = playRate;
                                    skinnedItem->controller.playRate = playRate;
                                }
                            }

                            bool bindPose = skinnedItem->debugForceBindPose;
                            if (ImGui::Checkbox("Bind pose preview", &bindPose))
                            {
                                skinnedItem->debugForceBindPose = bindPose;
                                skinnedItem->controller.forceBindPose = bindPose;
                                if (bindPose)
                                {
                                    skinnedItem->autoplay = false;
                                    skinnedItem->controller.autoplay = false;
                                    skinnedItem->animator.paused = true;
                                    skinnedItem->controller.paused = true;
                                    ResetAnimatorToBindPose(skinnedItem->animator, skinnedItem->asset->mesh.skeleton);
                                }
                                UpdateAnimationControllerRuntime(skinnedItem->controller, skinnedItem->animator, 0.0f);
                            }

                            if (ImGui::Button(skinnedItem->autoplay && !skinnedItem->animator.paused ? "Pause" : "Play"))
                            {
                                const bool willPlay = skinnedItem->animator.paused || !skinnedItem->autoplay;
                                node.animationAutoplay = willPlay;
                                skinnedItem->autoplay = willPlay;
                                skinnedItem->controller.autoplay = willPlay;
                                skinnedItem->animator.paused = !willPlay;
                                skinnedItem->controller.paused = !willPlay;
                                if (willPlay)
                                {
                                    skinnedItem->debugForceBindPose = false;
                                    skinnedItem->controller.forceBindPose = false;
                                }
                                UpdateAnimationControllerRuntime(skinnedItem->controller, skinnedItem->animator, 0.0f);
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Restart clip"))
                            {
                                skinnedItem->animator.timeSeconds = 0.0f;
                                skinnedItem->debugForceBindPose = false;
                                skinnedItem->controller.forceBindPose = false;
                                UpdateAnimationControllerRuntime(skinnedItem->controller, skinnedItem->animator, 0.0f);
                            }

                            if (!usingController && skinnedItem->animator.clip != nullptr)
                            {
                                const float clipDurationSeconds = (skinnedItem->animator.clip->ticksPerSecond > 0.0f)
                                    ? (skinnedItem->animator.clip->durationTicks / skinnedItem->animator.clip->ticksPerSecond)
                                    : 0.0f;
                                float scrubTime = NormalizeAnimationTimeSeconds(*skinnedItem->animator.clip, skinnedItem->animator.timeSeconds, skinnedItem->animator.looping);
                                if (ImGui::SliderFloat("Time", &scrubTime, 0.0f, std::max(0.0f, clipDurationSeconds), "%.3f", ImGuiSliderFlags_AlwaysClamp))
                                {
                                    skinnedItem->animator.timeSeconds = scrubTime;
                                    skinnedItem->animator.paused = true;
                                    skinnedItem->controller.paused = true;
                                    skinnedItem->autoplay = false;
                                    skinnedItem->controller.autoplay = false;
                                    node.animationAutoplay = false;
                                    skinnedItem->debugForceBindPose = false;
                                    skinnedItem->controller.forceBindPose = false;
                                    UpdateAnimationControllerRuntime(skinnedItem->controller, skinnedItem->animator, 0.0f);
                                }
                                ImGui::TextDisabled("Duration: %.3f s", clipDurationSeconds);
                            }
                            else if (skinnedItem->animator.clip == nullptr)
                            {
                                ImGui::TextDisabled("No active clip. Skeleton stays in bind pose.");
                            }

                            ImGui::SeparatorText("Skinned Debug");
                            ImGui::Checkbox("Draw selected skeleton", &scene.editorDrawSelectedSkinnedSkeleton);
                            ImGui::Checkbox("Draw selected max bounds", &scene.editorDrawSelectedSkinnedBounds);
                        }
                        else
                        {
                            ImGui::TextDisabled("Runtime skinned draw is not instantiated.");
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled("Runtime skinned draw is not instantiated.");
                    }
                }
            }
        }

        {
            std::vector<std::string> items;
            items.reserve(derived.materialIds.size() + 2);
            items.push_back("(none)");
            for (const auto& id : derived.materialIds) items.push_back(id);

            if (!node.material.empty() && !level.materials.contains(node.material))
                items.push_back(std::string("<missing> ") + node.material);

            int current = 0;
            if (!node.material.empty())
            {
                for (std::size_t i = 1; i < items.size(); ++i)
                {
                    if (items[i] == node.material)
                    {
                        current = static_cast<int>(i);
                        break;
                    }
                }
            }

            std::vector<const char*> citems;
            citems.reserve(items.size());
            for (auto& s : items) citems.push_back(s.c_str());

            if (ImGui::Combo("Material", &current, citems.data(), static_cast<int>(citems.size())))
            {
                if (current == 0)
                    levelInst.SetNodeMaterial(level, scene, st.selectedNode, "");
                else
                    levelInst.SetNodeMaterial(level, scene, st.selectedNode, items[static_cast<std::size_t>(current)]);
            }
        }

        bool changed = false;
        changed |= DragVec3("Position", node.transform.position, 0.05f);
        changed |= DragVec3("Rotation (deg)", node.transform.rotationDegrees, 0.2f);

        mathUtils::Vec3 scale = node.transform.scale;
        if (DragVec3("Scale", scale, 0.02f))
        {
            scale.x = std::max(scale.x, 0.001f);
            scale.y = std::max(scale.y, 0.001f);
            scale.z = std::max(scale.z, 0.001f);
            node.transform.scale = scale;
            changed = true;
        }

        if (changed)
            levelInst.MarkTransformsDirty();

        ImGui::SeparatorText("Gizmo");
        int gizmoMode = static_cast<int>(scene.editorGizmoMode);
        constexpr const char* kGizmoModes[] = { "None", "Translate", "Rotate", "Scale" };
        if (ImGui::Combo("Mode", &gizmoMode, kGizmoModes, IM_ARRAYSIZE(kGizmoModes)))
            scene.editorGizmoMode = static_cast<rendern::GizmoMode>(gizmoMode);
        ImGui::TextUnformatted("Hotkeys: Q = None, W = Translate, E = Rotate, R = Scale, X = Toggle translate space");

        ImGui::SeparatorText("Translate Gizmo");
        bool gizmoEnabled = scene.editorTranslateGizmo.enabled;
        if (ImGui::Checkbox("Enable translate gizmo", &gizmoEnabled))
            scene.editorTranslateGizmo.enabled = gizmoEnabled;

        int translateSpace = static_cast<int>(scene.editorTranslateSpace);
        constexpr const char* kTranslateSpaceModes[] = { "World", "Local" };
        if (ImGui::Combo("Translate Space", &translateSpace, kTranslateSpaceModes, IM_ARRAYSIZE(kTranslateSpaceModes)))
            scene.editorTranslateSpace = static_cast<rendern::GizmoSpace>(translateSpace);

        ImGui::TextUnformatted("LMB drag axis X/Y/Z or plane handle XY/XZ/YZ in the main viewport. Hold Shift to snap by 0.5.");
        ImGui::Text("Translate space: %s", scene.editorTranslateSpace == rendern::GizmoSpace::World ? "World" : "Local");
        ImGui::Text("Visible: %s", scene.editorTranslateGizmo.visible ? "Yes" : "No");
        ImGui::Text("Hovered axis: %d", static_cast<int>(scene.editorTranslateGizmo.hoveredAxis));
        ImGui::Text("Active axis: %d", static_cast<int>(scene.editorTranslateGizmo.activeAxis));

        ImGui::SeparatorText("Rotate Gizmo");
        bool rotateGizmoEnabled = scene.editorRotateGizmo.enabled;
        if (ImGui::Checkbox("Enable rotate gizmo", &rotateGizmoEnabled))
            scene.editorRotateGizmo.enabled = rotateGizmoEnabled;

        ImGui::TextUnformatted("LMB drag local X/Y/Z rotation rings in the main viewport. Hold Shift to snap by 15 degrees.");
        ImGui::Text("Visible: %s", scene.editorRotateGizmo.visible ? "Yes" : "No");
        ImGui::Text("Hovered axis: %d", static_cast<int>(scene.editorRotateGizmo.hoveredAxis));
        ImGui::Text("Active axis: %d", static_cast<int>(scene.editorRotateGizmo.activeAxis));

        ImGui::SeparatorText("Scale Gizmo");
        bool scaleGizmoEnabled = scene.editorScaleGizmo.enabled;
        if (ImGui::Checkbox("Enable scale gizmo", &scaleGizmoEnabled))
            scene.editorScaleGizmo.enabled = scaleGizmoEnabled;

        ImGui::TextUnformatted("LMB drag local X/Y/Z scale handles, XY/XZ/YZ plane handles, or the center sphere for uniform scale in the main viewport. Hold Shift to snap by 0.1. Q/W/E/R switches modes.");
        ImGui::Text("Visible: %s", scene.editorScaleGizmo.visible ? "Yes" : "No");
        ImGui::Text("Hovered axis: %d", static_cast<int>(scene.editorScaleGizmo.hoveredAxis));
        ImGui::Text("Active axis: %d", static_cast<int>(scene.editorScaleGizmo.activeAxis));

        ImGui::Spacing();

        if (ImGui::Button("Duplicate"))
        {
            rendern::Transform t = node.transform;
            t.position.x += 1.0f;

            const int newIdx = levelInst.AddNode(level, scene, assets, node.mesh, node.material, node.parent, t, node.name);
            if (!node.model.empty())
            {
                levelInst.SetNodeModel(level, scene, assets, newIdx, node.model);
                for (const auto& [submeshIndex, materialId] : node.materialOverrides)
                {
                    levelInst.SetNodeMaterialOverride(level, scene, assets, newIdx, submeshIndex, materialId);
                }
            }
            if (!node.skinnedMesh.empty())
            {
                levelInst.SetNodeSkinnedMesh(level, scene, assets, newIdx, node.skinnedMesh);
                rendern::LevelNode& dup = level.nodes[static_cast<std::size_t>(newIdx)];
                dup.animation = node.animation;
                dup.animationClip = node.animationClip;
                dup.animationController = node.animationController;
                dup.animationAutoplay = node.animationAutoplay;
                dup.animationLoop = node.animationLoop;
                dup.animationPlayRate = node.animationPlayRate;
            }
            st.selectedNode = newIdx;
            st.selectedParticleEmitter = -1;
        }
        ImGui::SameLine();
        bool doDelete = ImGui::Button("Delete (recursive)");
        if (ImGui::IsKeyPressed(ImGuiKey_Delete))
            doDelete = true;

        if (doDelete)
        {
            const int parent = node.parent;
            levelInst.DeleteSubtree(level, scene, st.selectedNode);

            if (NodeAlive(level, parent))
                st.selectedNode = parent;
            else
                st.selectedNode = -1;
            st.selectedParticleEmitter = -1;
        }
    }

}
