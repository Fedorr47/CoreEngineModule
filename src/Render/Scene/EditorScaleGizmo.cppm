module;

#include <algorithm>

export module core:editor_scale_gizmo;

import :scene;
import :level;
import :picking;
import :geometry;
import :math_utils;

import std;

namespace
{
	static mathUtils::Vec3 SafeNormalizeOr(const mathUtils::Vec3& v, const mathUtils::Vec3& fallback) noexcept
	{
		if (mathUtils::Length(v) < 1e-5f)
		{
			return fallback;
		}
		return mathUtils::Normalize(v);
	}

	static mathUtils::Vec3 AxisDirection(const rendern::ScaleGizmoState& gizmo, rendern::GizmoAxis axis) noexcept
	{
		switch (axis)
		{
		case rendern::GizmoAxis::X: return gizmo.axisXWorld;
		case rendern::GizmoAxis::Y: return gizmo.axisYWorld;
		case rendern::GizmoAxis::Z: return gizmo.axisZWorld;
		default: return mathUtils::Vec3(0.0f, 0.0f, 0.0f);
		}
	}

	static bool ProjectWorldToScreen(const rendern::Scene& scene,
		const mathUtils::Vec3& worldPos,
		float viewportW,
		float viewportH,
		mathUtils::Vec2& outScreen) noexcept
	{
		if (viewportW <= 1.0f || viewportH <= 1.0f)
		{
			return false;
		}

		const float aspect = viewportW / viewportH;
		const mathUtils::Mat4 proj = mathUtils::PerspectiveRH_ZO(mathUtils::DegToRad(scene.camera.fovYDeg), aspect, scene.camera.nearZ, scene.camera.farZ);
		const mathUtils::Mat4 view = mathUtils::LookAt(scene.camera.position, scene.camera.target, scene.camera.up);
		const mathUtils::Vec4 clip = (proj * view) * mathUtils::Vec4(worldPos, 1.0f);
		if (std::fabs(clip.w) < 1e-6f || clip.w <= 0.0f)
		{
			return false;
		}

		const float ndcX = clip.x / clip.w;
		const float ndcY = clip.y / clip.w;
		outScreen.x = (ndcX * 0.5f + 0.5f) * viewportW;
		outScreen.y = (1.0f - (ndcY * 0.5f + 0.5f)) * viewportH;
		return true;
	}

	static rendern::GizmoAxis HitTestScaleHandle(const rendern::Scene& scene,
		const rendern::ScaleGizmoState& gizmo,
		float mouseX,
		float mouseY,
		float viewportW,
		float viewportH) noexcept
	{
		const mathUtils::Vec2 mouse{ mouseX, mouseY };
		const float thresholdSq = 12.0f * 12.0f;
		float bestDistSq = std::numeric_limits<float>::infinity();
		rendern::GizmoAxis bestAxis = rendern::GizmoAxis::None;

		auto TestAxis = [&](rendern::GizmoAxis axis, const mathUtils::Vec3& dir) noexcept
			{
				mathUtils::Vec2 handleScreen{};
				if (!ProjectWorldToScreen(scene, gizmo.pivotWorld + dir * gizmo.axisLengthWorld, viewportW, viewportH, handleScreen))
				{
					return;
				}

				const mathUtils::Vec2 delta = mouse - handleScreen;
				const float distSq = mathUtils::Dot(delta, delta);
				if (distSq <= thresholdSq && distSq < bestDistSq)
				{
					bestDistSq = distSq;
					bestAxis = axis;
				}
			};

		TestAxis(rendern::GizmoAxis::X, gizmo.axisXWorld);
		TestAxis(rendern::GizmoAxis::Y, gizmo.axisYWorld);
		TestAxis(rendern::GizmoAxis::Z, gizmo.axisZWorld);
		return bestAxis;
	}

	static bool IntersectRayPlane(const geometry::Ray& ray,
		const mathUtils::Vec3& planePoint,
		const mathUtils::Vec3& planeNormal,
		mathUtils::Vec3& outPoint) noexcept
	{
		const float denom = mathUtils::Dot(planeNormal, ray.dir);
		if (std::fabs(denom) < 1e-6f)
		{
			return false;
		}
		const float t = mathUtils::Dot(planePoint - ray.origin, planeNormal) / denom;
		if (t < 0.0f)
		{
			return false;
		}

		outPoint = ray.origin + ray.dir * t;
		return true;
	}

	static mathUtils::Vec3 ComputeAxisDragPlaneNormal(const mathUtils::Vec3& axisWorld, const mathUtils::Vec3& viewDir) noexcept
	{
		mathUtils::Vec3 planeNormal = mathUtils::Cross(axisWorld, mathUtils::Cross(viewDir, axisWorld));
		if (mathUtils::Length(planeNormal) < 1e-5f)
		{
			planeNormal = mathUtils::Cross(axisWorld, mathUtils::Vec3(0.0f, 1.0f, 0.0f));
			if (mathUtils::Length(planeNormal) < 1e-5f)
			{
				planeNormal = mathUtils::Cross(axisWorld, mathUtils::Vec3(1.0f, 0.0f, 0.0f));
			}
		}
		return mathUtils::Normalize(planeNormal);
	}
}

export namespace rendern
{
	class ScaleGizmoController
	{
	public:
		void SyncVisual(const LevelAsset& asset, const LevelInstance& levelInst, Scene& scene) const noexcept
		{
			ScaleGizmoState& gizmo = scene.editorScaleGizmo;
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

			const mathUtils::Mat4& nodeWorld = levelInst.GetNodeWorldMatrix(selectedNode);
			gizmo.visible = true;
			gizmo.pivotWorld = levelInst.GetNodeWorldPosition(selectedNode);
			gizmo.axisXWorld = SafeNormalizeOr(mathUtils::TransformVector(nodeWorld, mathUtils::Vec3(1.0f, 0.0f, 0.0f)), mathUtils::Vec3(1.0f, 0.0f, 0.0f));
			gizmo.axisYWorld = SafeNormalizeOr(mathUtils::TransformVector(nodeWorld, mathUtils::Vec3(0.0f, 1.0f, 0.0f)), mathUtils::Vec3(0.0f, 1.0f, 0.0f));
			gizmo.axisZWorld = SafeNormalizeOr(mathUtils::TransformVector(nodeWorld, mathUtils::Vec3(0.0f, 0.0f, 1.0f)), mathUtils::Vec3(0.0f, 0.0f, 1.0f));

			const float distToCamera = mathUtils::Length(scene.camera.position - gizmo.pivotWorld);
			gizmo.axisLengthWorld = std::clamp(distToCamera * 0.18f, 0.35f, 4.0f);
		}

		void ClearHover(Scene& scene) const noexcept
		{
			scene.editorScaleGizmo.hoveredAxis = GizmoAxis::None;
		}

		void UpdateHover(Scene& scene, float mouseX, float mouseY, float viewportW, float viewportH) const noexcept
		{
			ScaleGizmoState& gizmo = scene.editorScaleGizmo;
			if (!gizmo.enabled || !gizmo.visible || gizmo.activeAxis != GizmoAxis::None)
			{
				gizmo.hoveredAxis = GizmoAxis::None;
				return;
			}

			gizmo.hoveredAxis = HitTestScaleHandle(scene, gizmo, mouseX, mouseY, viewportW, viewportH);
		}

		bool TryBeginDrag(const LevelAsset& asset,
			const LevelInstance& levelInst,
			Scene& scene,
			float mouseX,
			float mouseY,
			float viewportW,
			float viewportH) noexcept
		{
			ScaleGizmoState& gizmo = scene.editorScaleGizmo;
			if (!gizmo.enabled || !gizmo.visible || dragging_)
			{
				return false;
			}

			const int selectedNode = scene.editorSelectedNode;
			if (!levelInst.IsNodeAlive(asset, selectedNode))
			{
				return false;
			}

			const GizmoAxis axis = HitTestScaleHandle(scene, gizmo, mouseX, mouseY, viewportW, viewportH);
			if (axis == GizmoAxis::None)
			{
				return false;
			}

			const mathUtils::Vec3 axisWorld = AxisDirection(gizmo, axis);
			const mathUtils::Vec3 viewDir = mathUtils::Normalize(scene.camera.target - scene.camera.position);
			const mathUtils::Vec3 planeNormal = ComputeAxisDragPlaneNormal(axisWorld, viewDir);
			if (mathUtils::Length(planeNormal) < 1e-5f)
			{
				return false;
			}

			mathUtils::Vec3 startHit{};
			if (!IntersectRayPlane(BuildMouseRay(scene, mouseX, mouseY, viewportW, viewportH), gizmo.pivotWorld, planeNormal, startHit))
			{
				return false;
			}

			dragging_ = true;
			dragNodeIndex_ = selectedNode;
			dragAxis_ = axis;
			dragStartLocalScale_ = asset.nodes[static_cast<std::size_t>(selectedNode)].transform.scale;
			dragStartWorldHit_ = startHit;
			dragAxisWorld_ = axisWorld;
			dragPlaneNormal_ = planeNormal;
			dragPivotWorld_ = gizmo.pivotWorld;
			dragReferenceLengthWorld_ = std::max(gizmo.axisLengthWorld, 0.001f);

			gizmo.activeAxis = axis;
			gizmo.hoveredAxis = axis;
			return true;
		}

		bool UpdateDrag(LevelAsset& asset,
			const LevelInstance& levelInst,
			Scene& scene,
			float mouseX,
			float mouseY,
			float viewportW,
			float viewportH,
			bool snapEnabled) noexcept
		{
			if (!dragging_ || dragNodeIndex_ < 0 || !levelInst.IsNodeAlive(asset, dragNodeIndex_))
			{
				return false;
			}

			mathUtils::Vec3 currentHit{};
			if (!IntersectRayPlane(BuildMouseRay(scene, mouseX, mouseY, viewportW, viewportH), dragPivotWorld_, dragPlaneNormal_, currentHit))
			{
				return false;
			}

			const float axisDelta = mathUtils::Dot(currentHit - dragStartWorldHit_, dragAxisWorld_);
			float factor = 1.0f + (axisDelta / dragReferenceLengthWorld_);
			factor = std::max(factor, 0.001f);

			auto scale = dragStartLocalScale_;
			switch (dragAxis_)
			{
			case GizmoAxis::X: scale.x = std::max(dragStartLocalScale_.x * factor, 0.001f); break;
			case GizmoAxis::Y: scale.y = std::max(dragStartLocalScale_.y * factor, 0.001f); break;
			case GizmoAxis::Z: scale.z = std::max(dragStartLocalScale_.z * factor, 0.001f); break;
			default: return false;
			}

			if (snapEnabled)
			{
				constexpr float kScaleSnapStep = 0.1f;
				scale.x = std::max(std::round(scale.x / kScaleSnapStep) * kScaleSnapStep, 0.001f);
				scale.y = std::max(std::round(scale.y / kScaleSnapStep) * kScaleSnapStep, 0.001f);
				scale.z = std::max(std::round(scale.z / kScaleSnapStep) * kScaleSnapStep, 0.001f);
			}

			asset.nodes[static_cast<std::size_t>(dragNodeIndex_)].transform.scale = scale;
			scene.editorScaleGizmo.hoveredAxis = dragAxis_;
			return true;
		}

		void EndDrag(Scene& scene) noexcept
		{
			dragging_ = false;
			dragNodeIndex_ = -1;
			dragAxis_ = GizmoAxis::None;
			dragStartLocalScale_ = mathUtils::Vec3(1.0f, 1.0f, 1.0f);
			dragStartWorldHit_ = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
			dragAxisWorld_ = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
			dragPlaneNormal_ = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
			dragPivotWorld_ = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
			dragReferenceLengthWorld_ = 1.0f;
			scene.editorScaleGizmo.activeAxis = GizmoAxis::None;
			scene.editorScaleGizmo.hoveredAxis = GizmoAxis::None;
		}

		bool IsDragging() const noexcept
		{
			return dragging_;
		}

		private:
			bool dragging_{ false };
			int dragNodeIndex_{ -1 };
			GizmoAxis dragAxis_{ GizmoAxis::None };
			mathUtils::Vec3 dragStartLocalScale_{ 1.0f, 1.0f, 1.0f };
			mathUtils::Vec3 dragStartWorldHit_{ 0.0f, 0.0f, 0.0f };
			mathUtils::Vec3 dragAxisWorld_{ 0.0f, 0.0f, 0.0f };
			mathUtils::Vec3 dragPlaneNormal_{ 0.0f, 0.0f, 0.0f };
			mathUtils::Vec3 dragPivotWorld_{ 0.0f, 0.0f, 0.0f };
			float dragReferenceLengthWorld_{ 1.0f };
	};
}