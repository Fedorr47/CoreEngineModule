namespace rendern::ui
{
    static void DrawReflectionsWindow(rendern::RendererSettings& rs, rendern::Scene& scene)
    {
        ImGui::Begin("Reflections");

        ImGui::Checkbox("Enable reflection capture", &rs.enableReflectionCapture);

        ImGui::BeginDisabled(!rs.enableReflectionCapture);
        ImGui::Checkbox("Update every frame", &rs.reflectionCaptureUpdateEveryFrame);
        ImGui::Checkbox("Follow selected object", &rs.reflectionCaptureFollowSelectedObject);

        // Capture owner is separate from the current editor selection.
        // If set, it defines the capture position for the reflection cubemap.
        {
            int ownerNode = scene.editorReflectionCaptureOwnerNode;
            if (ImGui::InputInt("Capture owner node", &ownerNode))
            {
                if (ownerNode < -1)
                {
                    ownerNode = -1;
                }
                scene.editorReflectionCaptureOwnerNode = ownerNode;
            }
            ImGui::SameLine();
            if (ImGui::Button("Set owner = selected"))
            {
                scene.editorReflectionCaptureOwnerNode = scene.editorSelectedNode;
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear owner"))
            {
                scene.editorReflectionCaptureOwnerNode = -1;
            }
            ImGui::TextDisabled("Resolved draw item: %d", scene.editorReflectionCaptureOwnerDrawItem);
        }

        int res = static_cast<int>(rs.reflectionCaptureResolution);
        if (ImGui::InputInt("Capture resolution", &res))
        {
            res = std::clamp(res, 32, 2048);
            rs.reflectionCaptureResolution = static_cast<std::uint32_t>(res);
        }

        ImGui::DragFloat("Capture near Z", &rs.reflectionCaptureNearZ, 0.01f, 0.001f, 10.0f, "%.3f");
        ImGui::DragFloat("Capture far Z", &rs.reflectionCaptureFarZ, 1.0f, 1.0f, 5000.0f, "%.1f");
        ImGui::SliderFloat("Capture FOV pad (deg)", &rs.reflectionCaptureFovPadDeg, 0.0f, 10.0f, "%.2f");

        if (rs.reflectionCaptureFarZ < rs.reflectionCaptureNearZ)
            rs.reflectionCaptureFarZ = rs.reflectionCaptureNearZ;

        ImGui::EndDisabled();
        ImGui::End();
    }
}
