namespace rendern::ui
{
    void DrawLevelEditorUI(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        AssetManager& assets,
        rendern::Scene& scene,
        rendern::CameraController& camCtl)
    {
        ImGui::Begin("Level Editor");

        ImGui::Text("Nodes: %d   DrawItems: %d", static_cast<int>(level.nodes.size()), static_cast<int>(scene.drawItems.size()));
        ImGui::Separator();

        // Persistent UI state
        static int selectedNode = -1;
        static int prevSelectedNode = -2;
        static bool addAsChildOfSelection = false;

        static char nameBuf[128]{};
        static char importPathBuf[512]{};

        // Selection is driven by the main viewport (mouse picking) or by this UI.
        if (scene.editorSelectedNode != selectedNode)
        {
            selectedNode = scene.editorSelectedNode;
        }

        // Save state
        static char savePathBuf[512]{};
        static char saveStatusBuf[512]{};
        static std::string cachedSourcePath;
        static bool saveStatusIsError = false;

        // Keep save path input synced with loaded sourcePath (unless user edits it).
        if (cachedSourcePath != level.sourcePath)
        {
            cachedSourcePath = level.sourcePath;
            const std::string fallback = cachedSourcePath.empty() ? std::string("levels/edited.level.json") : cachedSourcePath;
            std::snprintf(savePathBuf, sizeof(savePathBuf), "%s", fallback.c_str());
        }

        // ------------------------------------------------------------
        // File
        // ------------------------------------------------------------
        if (ImGui::CollapsingHeader("File", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::InputText("Level path", savePathBuf, sizeof(savePathBuf));

            auto doSaveToPath = [&](const std::string& path)
                {
                    try
                    {
                        // Persist camera/lights from the current scene into the level asset.
                        level.camera = scene.camera;
                        level.lights = scene.lights;

                        rendern::SaveLevelAssetToJson(path, level);
                        level.sourcePath = path;
                        cachedSourcePath = path;
                        std::snprintf(saveStatusBuf, sizeof(saveStatusBuf), "Saved: %s", path.c_str());
                        saveStatusIsError = false;
                    }
                    catch (const std::exception& e)
                    {
                        std::snprintf(saveStatusBuf, sizeof(saveStatusBuf), "Save failed: %s", e.what());
                        saveStatusIsError = true;
                    }
                };

            const bool canHotkey = !ImGui::GetIO().WantTextInput;
            const bool ctrlS = canHotkey && ImGui::IsKeyDown(ImGuiKey_ModCtrl) && ImGui::IsKeyPressed(ImGuiKey_S);

            const std::string pathStr = std::string(savePathBuf);
            bool clickedSave = ImGui::Button("Save (Ctrl+S)");
            ImGui::SameLine();
            bool clickedSaveAs = ImGui::Button("Save As");

            if (ctrlS || clickedSave)
            {
                const std::string usePath = !level.sourcePath.empty() ? level.sourcePath : pathStr;
                if (!usePath.empty())
                {
                    doSaveToPath(usePath);
                }
                else
                {
                    std::snprintf(saveStatusBuf, sizeof(saveStatusBuf), "Save failed: empty path");
                    saveStatusIsError = true;
                }
            }
            else if (clickedSaveAs)
            {
                if (!pathStr.empty())
                {
                    doSaveToPath(pathStr);
                }
                else
                {
                    std::snprintf(saveStatusBuf, sizeof(saveStatusBuf), "Save failed: empty path");
                    saveStatusIsError = true;
                }
            }

            if (saveStatusBuf[0] != '\0')
            {
                if (saveStatusIsError)
                    ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", saveStatusBuf);
                else
                    ImGui::Text("%s", saveStatusBuf);
            }
        }

        // Build children adjacency (alive only)
        const std::size_t ncount = level.nodes.size();
        std::vector<std::vector<int>> children;
        children.resize(ncount);

        auto nodeAlive = [&](int idx) -> bool
            {
                if (idx < 0)
                {
                    return false;
                }
                const std::size_t i = static_cast<std::size_t>(idx);
                if (i >= ncount)
                {
                    return false;
                }
                return level.nodes[i].alive;
            };

        for (std::size_t i = 0; i < ncount; ++i)
        {
            const auto& n = level.nodes[i];
            if (!n.alive) continue;
            if (n.parent < 0) continue;
            if (!nodeAlive(n.parent)) continue;
            children[static_cast<std::size_t>(n.parent)].push_back(static_cast<int>(i));
        }

        // Roots
        std::vector<int> roots;
        roots.reserve(ncount);
        for (std::size_t i = 0; i < ncount; ++i)
        {
            const auto& n = level.nodes[i];
            if (!n.alive) continue;
            if (n.parent < 0 || !nodeAlive(n.parent))
                roots.push_back(static_cast<int>(i));
        }

        // Mesh/material id lists (sorted)
        std::vector<std::string> meshIds;
        meshIds.reserve(level.meshes.size());
        for (const auto& [id, _] : level.meshes) meshIds.push_back(id);
        std::sort(meshIds.begin(), meshIds.end());

        std::vector<std::string> materialIds;
        materialIds.reserve(level.materials.size());
        for (const auto& [id, _] : level.materials) materialIds.push_back(id);
        std::sort(materialIds.begin(), materialIds.end());

        // Layout: hierarchy + inspector
        ImGui::BeginChild("##Hierarchy", ImVec2(280.0f, 0.0f), true);

        auto drawNode = [&](auto&& self, int idx) -> void
            {
                const auto& n = level.nodes[static_cast<std::size_t>(idx)];

                ImGuiTreeNodeFlags flags =
                    ImGuiTreeNodeFlags_OpenOnArrow |
                    ImGuiTreeNodeFlags_SpanFullWidth;

                if (children[static_cast<std::size_t>(idx)].empty())
                    flags |= ImGuiTreeNodeFlags_Leaf;

                if (idx == selectedNode)
                    flags |= ImGuiTreeNodeFlags_Selected;

                char label[256]{};
                const char* name = n.name.empty() ? "<unnamed>" : n.name.c_str();
                if (!n.mesh.empty())
                    std::snprintf(label, sizeof(label), "%d: %s  [mesh=%s]", idx, name, n.mesh.c_str());
                else
                    std::snprintf(label, sizeof(label), "%d: %s", idx, name);

                const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<std::intptr_t>(idx)), flags, "%s", label);

                if (ImGui::IsItemClicked())
                {
                    selectedNode = idx;
                }

                if (open)
                {
                    for (int ch : children[static_cast<std::size_t>(idx)])
                        self(self, ch);
                    ImGui::TreePop();
                }
            };

        for (int r : roots)
        {
            drawNode(drawNode, r);
        }

        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("##Inspector", ImVec2(0.0f, 0.0f), true);

        // Add / import controls
        ImGui::Text("Create / Import");
        ImGui::Checkbox("Add as child of selected", &addAsChildOfSelection);

        auto ensureDefaultMesh = [&](std::string_view id, std::string_view relPath)
            {
                if (!level.meshes.contains(std::string(id)))
                {
                    rendern::LevelMeshDef def{};
                    def.path = std::string(relPath);
                    def.debugName = std::string(id);
                    level.meshes.emplace(std::string(id), std::move(def));
                }
            };

        auto computeSpawnTransform = [&]() -> rendern::Transform
            {
                rendern::Transform t{};
                t.position = scene.camera.position + camCtl.Forward() * 5.0f;
                t.rotationDegrees = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
                t.scale = mathUtils::Vec3(1.0f, 1.0f, 1.0f);
                return t;
            };

        const int parentForNew =
            (addAsChildOfSelection && nodeAlive(selectedNode)) ? selectedNode : -1;

        if (ImGui::Button("Add Cube"))
        {
            ensureDefaultMesh("cube", "models/cube.obj");
            const int newIdx = levelInst.AddNode(level, scene, assets, "cube", "", parentForNew, computeSpawnTransform(), "Cube");
            selectedNode = newIdx;
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Quad"))
        {
            ensureDefaultMesh("quad", "models/quad.obj");
            rendern::Transform t = computeSpawnTransform();
            t.scale = mathUtils::Vec3(3.0f, 1.0f, 3.0f);
            const int newIdx = levelInst.AddNode(level, scene, assets, "quad", "", parentForNew, t, "Quad");
            selectedNode = newIdx;
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Empty"))
        {
            const int newIdx = levelInst.AddNode(level, scene, assets, "", "", parentForNew, computeSpawnTransform(), "Empty");
            selectedNode = newIdx;
        }

        ImGui::Spacing();
        ImGui::InputText("OBJ path", importPathBuf, sizeof(importPathBuf));

        auto sanitizeId = [](std::string s) -> std::string
            {
                if (s.empty())
                    s = "mesh";

                for (char& c : s)
                {
                    const unsigned char uc = static_cast<unsigned char>(c);
                    if (!(std::isalnum(uc) || c == '_' || c == '-'))
                        c = '_';
                }
                return s;
            };

        auto makeUniqueMeshId = [&](std::string base) -> std::string
            {
                std::string id = sanitizeId(std::move(base));
                if (id.empty())
                    id = "mesh";

                if (!level.meshes.contains(id))
                    return id;

                for (int suffix = 2; suffix < 10000; ++suffix)
                {
                    std::string tryId = id + "_" + std::to_string(suffix);
                    if (!level.meshes.contains(tryId))
                        return tryId;
                }
                return id + "_x";
            };

        if (ImGui::Button("Import mesh into library"))
        {
            const std::string pathStr = std::string(importPathBuf);
            if (!pathStr.empty())
            {
                std::string base = std::filesystem::path(pathStr).stem().string();
                if (base.empty())
                    base = "mesh";

                const std::string meshId = makeUniqueMeshId(base);

                rendern::LevelMeshDef def{};
                def.path = pathStr;
                def.debugName = meshId;
                level.meshes.emplace(meshId, std::move(def));

                // Kick async load (optional)
                try
                {
                    rendern::MeshProperties p{};
                    p.filePath = pathStr;
                    p.debugName = meshId;
                    assets.LoadMeshAsync(meshId, std::move(p));
                }
                catch (...)
                {
                    // Leave it - the actual load error will be visible in logs.
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Create object from path"))
        {
            const std::string pathStr = std::string(importPathBuf);
            if (!pathStr.empty())
            {
                std::string base = std::filesystem::path(pathStr).stem().string();
                if (base.empty())
                    base = "mesh";

                const std::string meshId = makeUniqueMeshId(base);

                if (!level.meshes.contains(meshId))
                {
                    rendern::LevelMeshDef def{};
                    def.path = pathStr;
                    def.debugName = meshId;
                    level.meshes.emplace(meshId, std::move(def));
                }

                const int newIdx = levelInst.AddNode(level, scene, assets, meshId, "", parentForNew, computeSpawnTransform(), meshId);
                selectedNode = newIdx;
            }
        }

        ImGui::Separator();
        ImGui::Text("Selection");

        // Selection validity
        if (selectedNode >= 0 && (!nodeAlive(selectedNode)))
        {
            selectedNode = -1;
        }

        if (selectedNode >= 0 && nodeAlive(selectedNode))
        {
            rendern::LevelNode& node = level.nodes[static_cast<std::size_t>(selectedNode)];

            if (prevSelectedNode != selectedNode)
            {
                // Refresh name buffer on selection change.
                std::snprintf(nameBuf, sizeof(nameBuf), "%s", node.name.c_str());
                prevSelectedNode = selectedNode;
            }

            ImGui::Text("Node #%d", selectedNode);

            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
            {
                node.name = std::string(nameBuf);
            }

            bool vis = node.visible;
            if (ImGui::Checkbox("Visible", &vis))
            {
                levelInst.SetNodeVisible(level, scene, assets, selectedNode, vis);
            }

            // Mesh combo
            {
                std::vector<std::string> items;
                items.reserve(meshIds.size() + 2);
                items.push_back("(none)");
                for (const auto& id : meshIds) items.push_back(id);

                // Ensure missing mesh id is still selectable (shows as last item)
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
                        levelInst.SetNodeMesh(level, scene, assets, selectedNode, "");
                    else
                        levelInst.SetNodeMesh(level, scene, assets, selectedNode, items[static_cast<std::size_t>(current)]);
                }
            }

            // Material combo
            {
                std::vector<std::string> items;
                items.reserve(materialIds.size() + 2);
                items.push_back("(none)");
                for (const auto& id : materialIds) items.push_back(id);

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
                        levelInst.SetNodeMaterial(level, scene, selectedNode, "");
                    else
                        levelInst.SetNodeMaterial(level, scene, selectedNode, items[static_cast<std::size_t>(current)]);
                }
            }

            // Transform
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
            {
                levelInst.MarkTransformsDirty();
            }

            ImGui::Spacing();

            // Actions
            if (ImGui::Button("Duplicate"))
            {
                rendern::Transform t = node.transform;
                t.position.x += 1.0f;

                const int newIdx = levelInst.AddNode(level, scene, assets, node.mesh, node.material, node.parent, t, node.name);
                selectedNode = newIdx;
            }
            ImGui::SameLine();
            bool doDelete = ImGui::Button("Delete (recursive)");
            if (ImGui::IsKeyPressed(ImGuiKey_Delete))
            {
                doDelete = true;
            }

            if (doDelete)
            {
                const int parent = node.parent;
                levelInst.DeleteSubtree(level, scene, selectedNode);

                if (nodeAlive(parent))
                {
                    selectedNode = parent;
                }
                else
                {
                    selectedNode = -1;
                }
            }
        }
        else
        {
            ImGui::TextDisabled("No node selected.");
            prevSelectedNode = -2;
        }

        ImGui::EndChild();

        // Expose selection to the rest of the app (main viewport already writes here too).
        scene.editorSelectedNode = selectedNode;

        // Push transforms to Scene if needed
        levelInst.SyncTransformsIfDirty(level, scene);

        ImGui::End();
    }
}