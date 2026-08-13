module;

export module core:navigation_debug_draw;

import :debug_draw;
import :navigation;
import :scene;

// Outer-layer bridge: when RendererSettings::drawNavigationMesh is enabled, the
// application owner can extract its World and submit it through the existing list.
export namespace app::debugDraw
{
    void AppendNavigationGeometry(
        const navigation::DebugGeometry& geometry,
        rendern::debugDraw::DebugDrawList& debugDrawList)
    {
        for (const navigation::DebugLine& line : geometry.lines)
        {
            debugDrawList.AddLine(line.start, line.end, line.rgba);
        }
    }

    void SetNavigationGeometry(
        const navigation::DebugGeometry& geometry,
        rendern::Scene& scene)
    {
        scene.externalDebugLines.clear();
        scene.externalDebugLines.reserve(geometry.lines.size());
        for (const navigation::DebugLine& line : geometry.lines)
        {
            scene.externalDebugLines.push_back({ line.start, line.end, line.rgba });
        }
    }
}