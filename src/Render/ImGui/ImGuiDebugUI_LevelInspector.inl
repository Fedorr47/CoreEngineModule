namespace rendern::ui::level_ui_detail
{
    static void DrawCreateImportSection(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        AssetManager& assets,
        rendern::Scene& scene,
        rendern::CameraController& camCtl,
        LevelEditorUIState& st)
    {
        ImGui::Text("Create / Import");
        ImGui::Checkbox("Add as child of selected", &st.addAsChildOfSelection);

        const int parentForNew = ParentForNewNode(level, st);

        if (ImGui::Button("Add Cube"))
        {
            EnsureDefaultMesh(level, "cube", "models/cube.obj");
            const int newIdx = levelInst.AddNode(level, scene, assets, "cube", "", parentForNew, ComputeSpawnTransform(scene, camCtl), "Cube");
            st.selectedNode = newIdx;
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Quad"))
        {
            EnsureDefaultMesh(level, "quad", "models/quad.obj");
            rendern::Transform t = ComputeSpawnTransform(scene, camCtl);
            t.scale = mathUtils::Vec3(3.0f, 1.0f, 3.0f);
            const int newIdx = levelInst.AddNode(level, scene, assets, "quad", "", parentForNew, t, "Quad");
            st.selectedNode = newIdx;
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Empty"))
        {
            const int newIdx = levelInst.AddNode(level, scene, assets, "", "", parentForNew, ComputeSpawnTransform(scene, camCtl), "Empty");
            st.selectedNode = newIdx;
        }

        ImGui::Spacing();
        ImGui::InputText("OBJ path", st.importPathBuf, sizeof(st.importPathBuf));

        if (ImGui::Button("Import mesh into library"))
        {
            const std::string pathStr = std::string(st.importPathBuf);
            if (!pathStr.empty())
            {
                std::string base = std::filesystem::path(pathStr).stem().string();
                if (base.empty())
                    base = "mesh";

                const std::string meshId = MakeUniqueMeshId(level, base);

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
            const std::string pathStr = std::string(st.importPathBuf);
            if (!pathStr.empty())
            {
                std::string base = std::filesystem::path(pathStr).stem().string();
                if (base.empty())
                    base = "mesh";

                const std::string meshId = MakeUniqueMeshId(level, base);

                if (!level.meshes.contains(meshId))
                {
                    rendern::LevelMeshDef def{};
                    def.path = pathStr;
                    def.debugName = meshId;
                    level.meshes.emplace(meshId, std::move(def));
                }

                const int newIdx = levelInst.AddNode(level, scene, assets, meshId, "", parentForNew, ComputeSpawnTransform(scene, camCtl), meshId);
                st.selectedNode = newIdx;
            }
        }
    }

    static void DrawSelectionInspector(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        AssetManager& assets,
        rendern::Scene& scene,
        const DerivedLists& derived,
        LevelEditorUIState& st)
    {
        ImGui::Separator();
        ImGui::Text("Selection");

        if (st.selectedNode >= 0 && !NodeAlive(level, st.selectedNode))
            st.selectedNode = -1;

        if (st.selectedNode >= 0 && NodeAlive(level, st.selectedNode))
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

            // Mesh combo
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

            // Material combo
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

            ImGui::Spacing();

            if (ImGui::Button("Duplicate"))
            {
                rendern::Transform t = node.transform;
                t.position.x += 1.0f;

                const int newIdx = levelInst.AddNode(level, scene, assets, node.mesh, node.material, node.parent, t, node.name);
                st.selectedNode = newIdx;
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
            }
        }
        else
        {
            ImGui::TextDisabled("No node selected.");
            st.prevSelectedNode = -2;
        }
    }

    static void DrawInspectorPanel(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        AssetManager& assets,
        rendern::Scene& scene,
        rendern::CameraController& camCtl,
        const DerivedLists& derived,
        LevelEditorUIState& st)
    {
        ImGui::BeginChild("##Inspector", ImVec2(0.0f, 0.0f), true);

        DrawCreateImportSection(level, levelInst, assets, scene, camCtl, st);
        DrawSelectionInspector(level, levelInst, assets, scene, derived, st);

        ImGui::EndChild();
    }
}
