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
        for (const navigation::DebugTriangle& triangle : geometry.triangles)
        {
            debugDrawList.AddTriangle(triangle.a, triangle.b, triangle.c,
                triangle.rgbaA, triangle.rgbaB, triangle.rgbaC);
        }
        for (const navigation::DebugLine& line : geometry.lines)
        {
            debugDrawList.AddLine(line.start, line.end, line.rgba);
        }
    }

    void AppendNavigationGeometry(
        const navigation::DebugGeometry& geometry,
        rendern::Scene& scene)
    {
        scene.externalDebugTriangles.reserve(scene.externalDebugTriangles.size() + geometry.triangles.size());
        for (const navigation::DebugTriangle& triangle : geometry.triangles)
        {
            scene.externalDebugTriangles.push_back({ triangle.a, triangle.b, triangle.c,
                triangle.rgbaA, triangle.rgbaB, triangle.rgbaC });
        }
        scene.externalDebugLines.reserve(scene.externalDebugLines.size() + geometry.lines.size());
        for (const navigation::DebugLine& line : geometry.lines)
        {
            scene.externalDebugLines.push_back({ line.start, line.end, line.rgba });
        }
    }
}