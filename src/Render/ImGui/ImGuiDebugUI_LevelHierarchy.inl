namespace rendern::ui::level_ui_detail
{
    static void DrawHierarchyPanel(
        rendern::LevelAsset& level,
        const DerivedLists& derived,
        LevelEditorUIState& st)
    {
        ImGui::BeginChild("##Hierarchy", ImVec2(280.0f, 0.0f), true);

        auto drawNode = [&](auto&& self, int idx) -> void
        {
            const auto& n = level.nodes[static_cast<std::size_t>(idx)];

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanFullWidth;

            if (derived.children[static_cast<std::size_t>(idx)].empty())
                flags |= ImGuiTreeNodeFlags_Leaf;

            if (idx == st.selectedNode)
                flags |= ImGuiTreeNodeFlags_Selected;

            char label[256]{};
            const char* name = n.name.empty() ? "<unnamed>" : n.name.c_str();
            if (!n.mesh.empty())
                std::snprintf(label, sizeof(label), "%d: %s  [mesh=%s]", idx, name, n.mesh.c_str());
            else
                std::snprintf(label, sizeof(label), "%d: %s", idx, name);

            const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<std::intptr_t>(idx)), flags, "%s", label);

            if (ImGui::IsItemClicked())
                st.selectedNode = idx;

            if (open)
            {
                for (int ch : derived.children[static_cast<std::size_t>(idx)])
                    self(self, ch);
                ImGui::TreePop();
            }
        };

        for (int r : derived.roots)
            drawNode(drawNode, r);

        ImGui::EndChild();
    }
}
