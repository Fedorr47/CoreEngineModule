namespace rendern::ui::level_ui_detail
{
    static void SaveLevelToPath(
        rendern::LevelAsset& level,
        rendern::Scene& scene,
        LevelEditorUIState& uiState,
        const std::string& path)
    {
        try
        {
            level.camera = scene.camera;
            level.lights = scene.lights;

            rendern::SaveLevelAssetToJson(path, level);
            level.sourcePath = path;
            uiState.cachedSourcePath = path;
            std::snprintf(uiState.saveStatusBuf, sizeof(uiState.saveStatusBuf), "Saved: %s", path.c_str());
            uiState.saveStatusIsError = false;
        }
        catch (const std::exception& e)
        {
            std::snprintf(uiState.saveStatusBuf, sizeof(uiState.saveStatusBuf), "Save failed: %s", e.what());
            uiState.saveStatusIsError = true;
        }
    }

    static void DrawFilePanel(
        rendern::LevelAsset& level,
        rendern::Scene& scene,
        LevelEditorUIState& uiState)
    {
        if (!ImGui::CollapsingHeader("File", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        ImGui::InputText("Level path", uiState.savePathBuf, sizeof(uiState.savePathBuf));

        const bool canHotkey = !ImGui::GetIO().WantTextInput;
        const bool ctrlS = canHotkey && ImGui::IsKeyDown(ImGuiKey_ModCtrl) && ImGui::IsKeyPressed(ImGuiKey_S);

        const std::string pathStr = std::string(uiState.savePathBuf);
        bool clickedSave = ImGui::Button("Save (Ctrl+S)");
        ImGui::SameLine();
        bool clickedSaveAs = ImGui::Button("Save As");

        if (ctrlS || clickedSave)
        {
            const std::string usePath = !level.sourcePath.empty() ? level.sourcePath : pathStr;
            if (!usePath.empty())
            {
                SaveLevelToPath(level, scene, uiState, usePath);
            }
            else
            {
                std::snprintf(uiState.saveStatusBuf, sizeof(uiState.saveStatusBuf), "Save failed: empty path");
                uiState.saveStatusIsError = true;
            }
        }
        else if (clickedSaveAs)
        {
            if (!pathStr.empty())
            {
                SaveLevelToPath(level, scene, uiState, pathStr);
            }
            else
            {
                std::snprintf(uiState.saveStatusBuf, sizeof(uiState.saveStatusBuf), "Save failed: empty path");
                uiState.saveStatusIsError = true;
            }
        }

        if (uiState.saveStatusBuf[0] != '\0')
        {
            if (uiState.saveStatusIsError)
                ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", uiState.saveStatusBuf);
            else
                ImGui::Text("%s", uiState.saveStatusBuf);
        }
    }
}