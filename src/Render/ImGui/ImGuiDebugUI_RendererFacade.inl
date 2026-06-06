namespace rendern::ui
{
    void DrawRendererDebugUI(
        rendern::RendererSettings& rs,
        rendern::Scene& scene,
        rendern::CameraController& camCtl)
    {
        DrawRendererCoreWindow(rs, scene, camCtl);
        DrawPerformanceWindow(rs);
        DrawReflectionsWindow(rs, scene);
        DrawLightsWindow(scene);
    }
}
