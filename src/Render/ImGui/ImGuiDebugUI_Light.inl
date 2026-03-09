namespace rendern::ui
{
    constexpr const char* kLightTypeNames[] = { "Directional", "Point", "Spot" };

    static int ToIndex(rendern::LightType t)
    {
        switch (t)
        {
        case rendern::LightType::Directional: return 0;
        case rendern::LightType::Point:       return 1;
        case rendern::LightType::Spot:        return 2;
        default:                             return 0;
        }
    }

    static rendern::LightType FromIndex(int i)
    {
        switch (i)
        {
        case 0: return rendern::LightType::Directional;
        case 1: return rendern::LightType::Point;
        case 2: return rendern::LightType::Spot;
        default: return rendern::LightType::Directional;
        }
    }

    static void EnsureNormalized(mathUtils::Vec3& v)
    {
        const float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
        if (len2 > 1e-12f)
        {
            v = mathUtils::Normalize(v);
        }
        else
        {
            v = { 0.0f, -1.0f, 0.0f };
        }
    }

    static bool Color3(const char* label, mathUtils::Vec3& v)
    {
        float a[3] = { v.x, v.y, v.z };
        const bool changed = ImGui::ColorEdit3(label, a);
        if (changed)
        {
            v.x = a[0];
            v.y = a[1];
            v.z = a[2];
        }
        return changed;
    }

    static void ApplyDefaultsForType(rendern::Light& l)
    {
        if (l.type == rendern::LightType::Directional)
        {
            l.position = { 0.0f, 0.0f, 0.0f };
            l.direction = { -0.4f, -1.0f, -0.3f };
            EnsureNormalized(l.direction);
            l.color = { 1.0f, 1.0f, 1.0f };
            l.intensity = 0.5f;
        }
        else if (l.type == rendern::LightType::Point)
        {
            l.position = { 0.0f, 5.0f, 0.0f };
            l.direction = { 0.0f, -1.0f, 0.0f };
            l.color = { 1.0f, 1.0f, 1.0f };
            l.intensity = 1.0f;
            l.range = 30.0f;
            l.attConstant = 1.0f;
            l.attLinear = 0.09f;
            l.attQuadratic = 0.032f;
        }
        else // Spot
        {
            l.position = { 2.0f, 4.0f, 2.0f };
            l.direction = { -1.0f, -2.0f, -1.0f };
            EnsureNormalized(l.direction);
            l.color = { 1.0f, 1.0f, 1.0f };
            l.intensity = 5.0f;
            l.range = 50.0f;
            l.innerHalfAngleDeg = 20.0f;
            l.outerHalfAngleDeg = 35.0f;
            l.attConstant = 1.0f;
            l.attLinear = 0.09f;
            l.attQuadratic = 0.032f;
        }
    }

    static std::vector<float>& GetPrevLightIntensityCache()
    {
        static std::vector<float> prevIntensity;
        return prevIntensity;
    }

    static void EnsurePrevLightIntensityCacheSize(std::size_t lightCount)
    {
        auto& prevIntensity = GetPrevLightIntensityCache();
        if (prevIntensity.size() != lightCount)
        {
            prevIntensity.resize(lightCount, 1.0f);
        }
    }

    static void ApplyLightEnabledToggle(rendern::Light& l, std::size_t idx, bool enabled)
    {
        auto& prevIntensity = GetPrevLightIntensityCache();
        if (idx >= prevIntensity.size())
        {
            prevIntensity.resize(idx + 1u, 1.0f);
        }

        if (!enabled && l.intensity > 0.0f)
        {
            prevIntensity[idx] = std::max(prevIntensity[idx], l.intensity);
            l.intensity = 0.0f;
        }
        else if (enabled && l.intensity <= 0.00001f)
        {
            l.intensity = (prevIntensity[idx] > 0.0f) ? prevIntensity[idx] : 1.0f;
        }
    }

    static void DeleteLightAtIndex(rendern::Scene& scene, std::size_t idx)
    {
        auto& prevIntensity = GetPrevLightIntensityCache();
        EnsurePrevLightIntensityCacheSize(scene.lights.size());

        const int removedLightIndex = static_cast<int>(idx);
        for (int& selectedLight : scene.editorSelectedLights)
        {
            if (selectedLight == removedLightIndex)
            {
                selectedLight = -1;
            }
            else if (selectedLight > removedLightIndex)
            {
                --selectedLight;
            }
        }
        if (scene.editorSelectedLight == removedLightIndex)
        {
            scene.editorSelectedLight = -1;
        }
        else if (scene.editorSelectedLight > removedLightIndex)
        {
            --scene.editorSelectedLight;
        }

        scene.lights.erase(scene.lights.begin() + static_cast<std::ptrdiff_t>(idx));
        if (idx < prevIntensity.size())
        {
            prevIntensity.erase(prevIntensity.begin() + static_cast<std::ptrdiff_t>(idx));
        }
        scene.EditorSanitizeLightSelection(scene.lights.size());
    }

    static void DrawOneLightEditor(rendern::Light& l, std::size_t idx)
    {
        ImGui::PushID(static_cast<int>(idx));

        int typeIdx = ToIndex(l.type);
        if (ImGui::Combo("Type", &typeIdx, kLightTypeNames, 3))
        {
            l.type = FromIndex(typeIdx);
            ApplyDefaultsForType(l);
        }

        Color3("Color", l.color);
        ImGui::DragFloat("Intensity", &l.intensity, 0.01f, 0.0f, 200.0f, "%.3f");

        ImGui::Separator();

        switch (l.type)
        {
        case rendern::LightType::Directional:
            DragVec3("Direction", l.direction, 0.02f, -1.0f, 1.0f);
            if (ImGui::Button("Normalize direction"))
                EnsureNormalized(l.direction);
            break;

        case rendern::LightType::Point:
            DragVec3("Position", l.position, 0.05f);
            ImGui::DragFloat("Range", &l.range, 0.1f, 0.1f, 500.0f, "%.2f");
            ImGui::DragFloat("Att const", &l.attConstant, 0.01f, 0.0f, 10.0f, "%.3f");
            ImGui::DragFloat("Att linear", &l.attLinear, 0.001f, 0.0f, 10.0f, "%.4f");
            ImGui::DragFloat("Att quad", &l.attQuadratic, 0.001f, 0.0f, 10.0f, "%.5f");
            break;

        case rendern::LightType::Spot:
            DragVec3("Position", l.position, 0.05f);
            DragVec3("Direction", l.direction, 0.02f, -1.0f, 1.0f);
            if (ImGui::Button("Normalize direction"))
                EnsureNormalized(l.direction);
            ImGui::DragFloat("Range", &l.range, 0.1f, 0.1f, 500.0f, "%.2f");
            ImGui::DragFloat("Inner half angle", &l.innerHalfAngleDeg, 0.1f, 0.0f, 89.0f, "%.2f deg");
            ImGui::DragFloat("Outer half angle", &l.outerHalfAngleDeg, 0.1f, 0.0f, 89.0f, "%.2f deg");
            if (l.innerHalfAngleDeg > l.outerHalfAngleDeg)
                l.innerHalfAngleDeg = l.outerHalfAngleDeg;
            ImGui::DragFloat("Att const", &l.attConstant, 0.01f, 0.0f, 10.0f, "%.3f");
            ImGui::DragFloat("Att linear", &l.attLinear, 0.001f, 0.0f, 10.0f, "%.4f");
            ImGui::DragFloat("Att quad", &l.attQuadratic, 0.001f, 0.0f, 10.0f, "%.5f");
            break;
        }

        ImGui::PopID();
    }

    static void DrawLightInspectorDetails(rendern::Scene& scene)
    {
        scene.EditorSanitizeLightSelection(scene.lights.size());
        if (scene.editorSelectedLight < 0)
        {
            return;
        }

        const std::size_t lightIndex = static_cast<std::size_t>(scene.editorSelectedLight);
        if (lightIndex >= scene.lights.size())
        {
            return;
        }

        const int selectedCount = static_cast<int>(scene.editorSelectedLights.size());
        if (selectedCount > 1)
        {
            ImGui::Text("Multi-selection: %d lights", selectedCount);
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear light selection"))
            {
                scene.EditorClearLightSelection();
                return;
            }
            ImGui::Text("Primary light: #%d", scene.editorSelectedLight);
        }
        else
        {
            ImGui::Text("Light #%d", scene.editorSelectedLight);
        }

        rendern::Light& l = scene.lights[lightIndex];

        EnsurePrevLightIntensityCacheSize(scene.lights.size());
        bool enabled = (l.intensity > 0.00001f);
        if (ImGui::Checkbox("Enabled", &enabled))
        {
            ApplyLightEnabledToggle(l, lightIndex, enabled);
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete Light"))
        {
            DeleteLightAtIndex(scene, lightIndex);
            return;
        }

        DrawOneLightEditor(l, lightIndex);
    }

    // ------------------------------------------------------------
    // Light header row with actions on the right (clickable)
    // ------------------------------------------------------------
    static bool LightHeaderWithActions(
        const char* headerText,
        bool defaultOpen,
        bool selected,
        bool& enabled,
        bool& doDelete,
        bool& headerClicked)
    {
        doDelete = false;
        headerClicked = false;

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_AllowOverlap |
            ImGuiTreeNodeFlags_FramePadding;

        if (defaultOpen)
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        if (selected)
            flags |= ImGuiTreeNodeFlags_Selected;

        // Tree node label is hidden; we render text via format string to keep ID stable.
        const bool open = ImGui::TreeNodeEx("##light_node", flags, "%s", headerText);
        headerClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

        // The last item is the header frame; place our controls on top-right of it.
        const ImVec2 rmin = ImGui::GetItemRectMin();
        const ImVec2 rmax = ImGui::GetItemRectMax();

        // Reserve width for controls (tweakable).
        const float deleteW = 62.0f; // typical "Delete" button width
        const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
        const float checkW = ImGui::GetFrameHeight(); // checkbox square
        const float totalW = checkW + spacing + deleteW;

        // Align to the center vertically.
        const float y = rmin.y + (rmax.y - rmin.y - ImGui::GetFrameHeight()) * 0.5f;

        // Start X a bit before the right edge.
        const float x = rmax.x - totalW - spacing;

        // Draw controls on the same line, but overlayed.
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));

        // Checkbox (no label text to stay compact)
        ImGui::Checkbox("##Enabled", &enabled);
        ImGui::SameLine();
        doDelete = ImGui::Button("Delete");

        ImGui::PopStyleVar();

        return open;
    }

    static void DrawLightsWindow(rendern::Scene& scene)
    {
        ImGui::Begin("Lights");

        scene.EditorSanitizeLightSelection(scene.lights.size());

        ImGui::Text("Count: %d", static_cast<int>(scene.lights.size()));

        if (ImGui::Button("Add Directional"))
        {
            rendern::Light l{};
            l.type = rendern::LightType::Directional;
            ApplyDefaultsForType(l);
            scene.AddLight(l);
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Point"))
        {
            rendern::Light l{};
            l.type = rendern::LightType::Point;
            ApplyDefaultsForType(l);
            scene.AddLight(l);
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Spot"))
        {
            rendern::Light l{};
            l.type = rendern::LightType::Spot;
            ApplyDefaultsForType(l);
            scene.AddLight(l);
        }

        ImGui::Spacing();

        EnsurePrevLightIntensityCacheSize(scene.lights.size());

        for (std::size_t i = 0; i < scene.lights.size();)
        {
            auto& l = scene.lights[i];

            ImGui::PushID(static_cast<int>(i));

            const char* typeName = kLightTypeNames[ToIndex(l.type)];
            char header[64]{};
            std::snprintf(header, sizeof(header), "[%s] #%zu", typeName, i);

            bool enabled = (l.intensity > 0.00001f);
            bool doDelete = false;
            bool headerClicked = false;
            const bool selected = scene.EditorIsLightSelected(static_cast<int>(i));

            const bool open = LightHeaderWithActions(header, true, selected, enabled, doDelete, headerClicked);

            if (headerClicked)
            {
                const bool ctrlDown = ImGui::GetIO().KeyCtrl;
                if (ctrlDown)
                {
                    scene.EditorToggleSelectionLight(static_cast<int>(i));
                }
                else
                {
                    scene.EditorSetLightSelectionSingle(static_cast<int>(i));
                }
            }

            ApplyLightEnabledToggle(l, i, enabled);

            if (open)
            {
                ImGui::TextDisabled("Edit light properties in Level Editor -> Inspector.");
                ImGui::TreePop();
            }

            ImGui::PopID();

            if (doDelete)
            {
                DeleteLightAtIndex(scene, i);
            }

            ++i;
        }

        ImGui::End();
    }
}
