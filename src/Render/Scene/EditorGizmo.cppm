module;

#include <algorithm>
#include <cmath>

export module core:editor_gizmo;

import :scene;
import :level;
import :math_utils;

export namespace rendern
{
	class TranslateGizmoController
	{
	public:
		void SyncVisual(const LevelAsset& asset, const LevelInstance& levelInst, Scene& scene) const noexcept
		{
			TranslateGizmoState& gizmo = scene.editorTranslateGizmo;
			if (!gizmo.enabled)
			{
				gizmo.visible = false;
				gizmo.hoveredAxis = GizmoAxis::None;
				gizmo.activeAxis = GizmoAxis::None;
				return;
			}

			const int selectedNode = scene.editorSelectedNode;
			if (!levelInst.IsNodeAlive(asset, selectedNode))
			{
				gizmo.visible = false;
				gizmo.hoveredAxis = GizmoAxis::None;
				gizmo.activeAxis = GizmoAxis::None;
				return;
			}

			gizmo.visible = true;
			gizmo.pivotWorld = levelInst.GetNodeWorldPosition(selectedNode);

			const float distToCamera = mathUtils::Length(scene.camera.position - gizmo.pivotWorld);
			gizmo.axisLengthWorld = std::clamp(distToCamera * 0.18f, 0.35f, 4.0f);
		}
	};
}