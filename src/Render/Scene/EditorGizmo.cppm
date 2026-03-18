module;


#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <cstdlib>

export module core:editor_gizmo;

import :scene;
import :level;
import :picking;
import :math_utils;
import :geometry;

#include "SceneImpl/EditorGizmoMathShared.inl"

namespace
{

	static float DistancePointToSegmentSq(const mathUtils::Vec2& p, const mathUtils::Vec2& a, const mathUtils::Vec2& b) noexcept
	{
		const mathUtils::Vec2 ab = b - a;
		const mathUtils::Vec2 ap = p - a;
		const float abLenSq = mathUtils::Dot(ab, ab);
		if (abLenSq <= 1e-6f)
		{
			return mathUtils::Dot(ap, ap);
		}

		const float t = std::clamp(mathUtils::Dot(ap, ab) / abLenSq, 0.0f, 1.0f);
		const mathUtils::Vec2 closest = a + ab * t;
		const mathUtils::Vec2 delta = p - closest;
		return mathUtils::Dot(delta, delta);
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

	static bool BuildPlaneHandleQuad(const rendern::Scene& scene,
		const rendern::TranslateGizmoState& gizmo,
		rendern::GizmoAxis axis,
		float viewportW,
		float viewportH,
		mathUtils::Vec2& p0,
		mathUtils::Vec2& p1,
		mathUtils::Vec2& p2,
		mathUtils::Vec2& p3) noexcept
	{
		mathUtils::Vec3 basisA{};
		mathUtils::Vec3 basisB{};
		mathUtils::Vec3 planeNormal{};
		GizmoPlaneBasis(gizmo, axis, basisA, basisB, planeNormal);
		if (mathUtils::Length(planeNormal) < 1e-5f)
		{
			return false;
		}

		const float inner = gizmo.axisLengthWorld * 0.28f;
		const float outer = gizmo.axisLengthWorld * 0.46f;
		const mathUtils::Vec3 w0 = gizmo.pivotWorld + basisA * inner + basisB * inner;
		const mathUtils::Vec3 w1 = gizmo.pivotWorld + basisA * outer + basisB * inner;
		const mathUtils::Vec3 w2 = gizmo.pivotWorld + basisA * outer + basisB * outer;
		const mathUtils::Vec3 w3 = gizmo.pivotWorld + basisA * inner + basisB * outer;

		return GizmoProjectWorldToScreen(scene, w0, viewportW, viewportH, p0)
			&& GizmoProjectWorldToScreen(scene, w1, viewportW, viewportH, p1)
			&& GizmoProjectWorldToScreen(scene, w2, viewportW, viewportH, p2)
			&& GizmoProjectWorldToScreen(scene, w3, viewportW, viewportH, p3);
	}

	static rendern::GizmoAxis HitTestPlaneHandle(const rendern::Scene& scene,
		const rendern::TranslateGizmoState& gizmo,
		float mouseX,
		float mouseY,
		float viewportW,
		float viewportH) noexcept
	{
		const mathUtils::Vec2 mouse{ mouseX, mouseY };
		const rendern::GizmoAxis planeOrder[] =
		{
			rendern::GizmoAxis::XY,
			rendern::GizmoAxis::XZ,
			rendern::GizmoAxis::YZ,
		};

		for (rendern::GizmoAxis axis : planeOrder)
		{
			mathUtils::Vec2 p0{}, p1{}, p2{}, p3{};
			if (!BuildPlaneHandleQuad(scene, gizmo, axis, viewportW, viewportH, p0, p1, p2, p3))
			{
				continue;
			}

			if (GizmoPointInQuad2D(mouse, p0, p1, p2, p3))
			{
				return axis;
			}
		}

		return rendern::GizmoAxis::None;
	}

	static void TestProjectedAxis(
		const rendern::Scene& scene,
		const mathUtils::Vec3& pivotWorld,
		float axisLengthWorld,
		const mathUtils::Vec2& mouse,
		const mathUtils::Vec2& pivotScreen,
		float viewportW,
		float viewportH,
		float thresholdSq,
		rendern::GizmoAxis axis,
		const mathUtils::Vec3& dir,
		float& bestDistSq,
		rendern::GizmoAxis& bestAxis) noexcept
	{
		mathUtils::Vec2 endScreen{};
		if (!GizmoProjectWorldToScreen(scene, pivotWorld + dir * axisLengthWorld, viewportW, viewportH, endScreen))
		{
			return;
		}

		const float distSq = DistancePointToSegmentSq(mouse, pivotScreen, endScreen);
		if (distSq <= thresholdSq && distSq < bestDistSq)
		{
			bestDistSq = distSq;
			bestAxis = axis;
		}
	}

	static rendern::GizmoAxis HitTestAxis(const rendern::Scene& scene,
		const rendern::TranslateGizmoState& gizmo,
		float mouseX,
		float mouseY,
		float viewportW,
		float viewportH) noexcept
	{
		mathUtils::Vec2 pivotScreen{};
		if (!GizmoProjectWorldToScreen(scene, gizmo.pivotWorld, viewportW, viewportH, pivotScreen))
		{
			return rendern::GizmoAxis::None;
		}

		const mathUtils::Vec2 mouse{ mouseX, mouseY };
		const float thresholdSq = 10.0f * 10.0f;
		float bestDistSq = std::numeric_limits<float>::infinity();
		rendern::GizmoAxis bestAxis = rendern::GizmoAxis::None;

		TestProjectedAxis(scene, gizmo.pivotWorld, gizmo.axisLengthWorld, mouse, pivotScreen, viewportW, viewportH, thresholdSq,
			rendern::GizmoAxis::X, gizmo.axisXWorld, bestDistSq, bestAxis);
		TestProjectedAxis(scene, gizmo.pivotWorld, gizmo.axisLengthWorld, mouse, pivotScreen, viewportW, viewportH, thresholdSq,
			rendern::GizmoAxis::Y, gizmo.axisYWorld, bestDistSq, bestAxis);
		TestProjectedAxis(scene, gizmo.pivotWorld, gizmo.axisLengthWorld, mouse, pivotScreen, viewportW, viewportH, thresholdSq,
			rendern::GizmoAxis::Z, gizmo.axisZWorld, bestDistSq, bestAxis);

		return bestAxis;
	}

	static rendern::GizmoAxis HitTestHandle(const rendern::Scene& scene,
		const rendern::TranslateGizmoState& gizmo,
		float mouseX,
		float mouseY,
		float viewportW,
		float viewportH) noexcept
	{
		const rendern::GizmoAxis planeAxis = HitTestPlaneHandle(scene, gizmo, mouseX, mouseY, viewportW, viewportH);
		if (planeAxis != rendern::GizmoAxis::None)
		{
			return planeAxis;
		}
		return HitTestAxis(scene, gizmo, mouseX, mouseY, viewportW, viewportH);
	}

#include "SceneImpl/EditorGizmoShared.inl"
}

export namespace rendern
{
	bool ProjectWorldToScreenForGizmo(const Scene& scene,
		const mathUtils::Vec3& worldPos,
		float viewportW,
		float viewportH,
		mathUtils::Vec2& outScreen) noexcept
	{
		return GizmoProjectWorldToScreen(scene, worldPos, viewportW, viewportH, outScreen);
	}

	GizmoAxis HitTestTranslateGizmoAxis(const Scene& scene,
		const TranslateGizmoState& gizmo,
		float mouseX,
		float mouseY,
		float viewportW,
		float viewportH) noexcept
	{
		return HitTestAxis(scene, gizmo, mouseX, mouseY, viewportW, viewportH);
	}

	mathUtils::Vec3 TranslateGizmoGizmoAxisDirection(const TranslateGizmoState& gizmo, GizmoAxis axis) noexcept
	{
		return GizmoAxisDirection(gizmo, axis);
	}

	mathUtils::Vec3 ComputeTranslateGizmoAxisDragPlaneNormal(const mathUtils::Vec3& axisWorld, const mathUtils::Vec3& viewDir) noexcept
	{
		return ComputeAxisDragPlaneNormal(axisWorld, viewDir);
	}

	bool IntersectRayPlaneForGizmo(const geometry::Ray& ray,
		const mathUtils::Vec3& planePoint,
		const mathUtils::Vec3& planeNormal,
		mathUtils::Vec3& outPoint) noexcept
	{
		return GizmoIntersectRayPlane(ray, planePoint, planeNormal, outPoint);
	}

	class TranslateGizmoController
	{
	public:
		void SyncVisual(const LevelAsset& asset, const LevelInstance& levelInst, Scene& scene) const noexcept
		{
			TranslateGizmoState& gizmo = scene.editorTranslateGizmo;
			scene.EditorSanitizeLightSelection(scene.lights.size());
			if (!gizmo.enabled || scene.editorGizmoMode != GizmoMode::Translate)
			{
				gizmo.visible = false;
				gizmo.hoveredAxis = GizmoAxis::None;
				gizmo.activeAxis = GizmoAxis::None;
				return;
			}

			if (!scene.editorSelectedLights.empty())
			{
				mathUtils::Vec3 sum{ 0.0f, 0.0f, 0.0f };
				int count = 0;
				for (const int lightIndex : scene.editorSelectedLights)
				{
					if (lightIndex < 0 || static_cast<std::size_t>(lightIndex) >= scene.lights.size())
					{
						continue;
					}
					const Light& light = scene.lights[static_cast<std::size_t>(lightIndex)];
					if (light.type != LightType::Point && light.type != LightType::Spot)
					{
						continue;
					}
					sum = sum + light.position;
					++count;
				}

				if (count == 0)
				{
					gizmo.visible = false;
					gizmo.hoveredAxis = GizmoAxis::None;
					gizmo.activeAxis = GizmoAxis::None;
					return;
				}

				gizmo.visible = true;
				gizmo.pivotWorld = sum * (1.0f / static_cast<float>(count));
				gizmo.axisXWorld = mathUtils::Vec3(1.0f, 0.0f, 0.0f);
				gizmo.axisYWorld = mathUtils::Vec3(0.0f, 1.0f, 0.0f);
				gizmo.axisZWorld = mathUtils::Vec3(0.0f, 0.0f, 1.0f);
				const float distToCamera = mathUtils::Length(scene.camera.position - gizmo.pivotWorld);
				gizmo.axisLengthWorld = std::clamp(distToCamera * 0.18f, 0.35f, 4.0f);
				return;
			}

			// Multi-selection pivot: centroid of all selected nodes.
			mathUtils::Vec3 sum{ 0.0f, 0.0f, 0.0f };
			int count = 0;
			for (const int nodeIndex : scene.editorSelectedNodes)
			{
				if (levelInst.IsNodeAlive(asset, nodeIndex))
				{
					sum = sum + levelInst.GetNodeWorldPosition(nodeIndex);
					++count;
				}
			}
			const int selectedNode = scene.editorSelectedNode;
			if (!levelInst.IsNodeAlive(asset, selectedNode))
			{
				gizmo.visible = false;
				gizmo.hoveredAxis = GizmoAxis::None;
				gizmo.activeAxis = GizmoAxis::None;
				return;
			}
			if (count == 0)
			{
				sum = levelInst.GetNodeWorldPosition(selectedNode);
				count = 1;
			}

			gizmo.visible = true;
			gizmo.pivotWorld = sum * (1.0f / static_cast<float>(count));

			if (scene.editorTranslateSpace == GizmoSpace::Local)
			{
				const mathUtils::Mat4 world = levelInst.GetNodeWorldMatrix(selectedNode);
				gizmo.axisXWorld = GizmoSafeNormalizeOr(mathUtils::TransformVector(world, mathUtils::Vec3(1.0f, 0.0f, 0.0f)), mathUtils::Vec3(1.0f, 0.0f, 0.0f));
				gizmo.axisYWorld = GizmoSafeNormalizeOr(mathUtils::TransformVector(world, mathUtils::Vec3(0.0f, 1.0f, 0.0f)), mathUtils::Vec3(0.0f, 1.0f, 0.0f));
				gizmo.axisZWorld = GizmoSafeNormalizeOr(mathUtils::TransformVector(world, mathUtils::Vec3(0.0f, 0.0f, 1.0f)), mathUtils::Vec3(0.0f, 0.0f, 1.0f));
			}
			else
			{
				gizmo.axisXWorld = mathUtils::Vec3(1.0f, 0.0f, 0.0f);
				gizmo.axisYWorld = mathUtils::Vec3(0.0f, 1.0f, 0.0f);
				gizmo.axisZWorld = mathUtils::Vec3(0.0f, 0.0f, 1.0f);
			}

			const float distToCamera = mathUtils::Length(scene.camera.position - gizmo.pivotWorld);
			gizmo.axisLengthWorld = std::clamp(distToCamera * 0.18f, 0.35f, 4.0f);
		}

		void ClearHover(Scene& scene) const noexcept
		{
			scene.editorTranslateGizmo.hoveredAxis = GizmoAxis::None;
		}

		void UpdateHover(Scene& scene, float mouseX, float mouseY, float viewportW, float viewportH) const noexcept
		{
			TranslateGizmoState& gizmo = scene.editorTranslateGizmo;
			if (!gizmo.enabled || !gizmo.visible || gizmo.activeAxis != GizmoAxis::None)
			{
				gizmo.hoveredAxis = GizmoAxis::None;
				return;
			}

			gizmo.hoveredAxis = HitTestHandle(scene, gizmo, mouseX, mouseY, viewportW, viewportH);
		}

		bool TryBeginDrag(const LevelAsset& asset,
			const LevelInstance& levelInst,
			Scene& scene,
			float mouseX,
			float mouseY,
			float viewportW,
			float viewportH) noexcept
		{
			TranslateGizmoState& gizmo = scene.editorTranslateGizmo;
			scene.EditorSanitizeLightSelection(scene.lights.size());
			if (!gizmo.enabled || !gizmo.visible || dragging_)
			{
				return false;
			}

			const GizmoAxis axis = HitTestHandle(scene, gizmo, mouseX, mouseY, viewportW, viewportH);
			if (axis == GizmoAxis::None)
			{
				return false;
			}

			mathUtils::Vec3 planeNormal{};
			mathUtils::Vec3 axisWorld{};
			if (GizmoIsAxisHandle(axis))
			{
				axisWorld = GizmoAxisDirection(gizmo, axis);
				const mathUtils::Vec3 viewDir = mathUtils::Normalize(scene.camera.target - scene.camera.position);
				planeNormal = ComputeAxisDragPlaneNormal(axisWorld, viewDir);
			}
			else if (GizmoIsPlaneHandle(axis))
			{
				mathUtils::Vec3 basisA{};
				mathUtils::Vec3 basisB{};
				GizmoPlaneBasis(gizmo, axis, basisA, basisB, planeNormal);
			}

			if (mathUtils::Length(planeNormal) < 1e-5f)
			{
				return false;
			}

			mathUtils::Vec3 startHit{};
			if (!GizmoIntersectRayPlane(BuildMouseRay(scene, mouseX, mouseY, viewportW, viewportH), gizmo.pivotWorld, planeNormal, startHit))
			{
				return false;
			}

			dragging_ = true;
			dragSelectionIsLight_ = false;
			dragNodeIndices_.clear();
			dragStartLocalPositions_.clear();
			dragInvParentWorld_.clear();
			dragLightIndices_.clear();
			dragStartLightPositions_.clear();

			if (!scene.editorSelectedLights.empty())
			{
				dragSelectionIsLight_ = true;
				dragLightIndices_.reserve(scene.editorSelectedLights.size());
				dragStartLightPositions_.reserve(scene.editorSelectedLights.size());
				for (const int lightIndex : scene.editorSelectedLights)
				{
					if (lightIndex < 0 || static_cast<std::size_t>(lightIndex) >= scene.lights.size())
					{
						continue;
					}
					const Light& light = scene.lights[static_cast<std::size_t>(lightIndex)];
					if (light.type != LightType::Point && light.type != LightType::Spot)
					{
						continue;
					}
					dragLightIndices_.push_back(lightIndex);
					dragStartLightPositions_.push_back(light.position);
				}

				if (dragLightIndices_.empty())
				{
					dragging_ = false;
					dragSelectionIsLight_ = false;
					return false;
				}
			}
			else
			{
				const int primaryNode = scene.editorSelectedNode;
				if (!levelInst.IsNodeAlive(asset, primaryNode))
				{
					dragging_ = false;
					return false;
				}
				if (scene.editorSelectedNodes.empty())
				{
					scene.editorSelectedNodes.push_back(primaryNode);
				}

				dragNodeIndices_.reserve(scene.editorSelectedNodes.size());
				dragStartLocalPositions_.reserve(scene.editorSelectedNodes.size());
				dragInvParentWorld_.reserve(scene.editorSelectedNodes.size());
				for (const int nodeIndex : scene.editorSelectedNodes)
				{
					if (!levelInst.IsNodeAlive(asset, nodeIndex))
					{
						continue;
					}
					dragNodeIndices_.push_back(nodeIndex);
					dragStartLocalPositions_.push_back(asset.nodes[static_cast<std::size_t>(nodeIndex)].transform.position);
					dragInvParentWorld_.push_back(mathUtils::Inverse(levelInst.GetParentWorldMatrix(asset, nodeIndex)));
				}

				if (dragNodeIndices_.empty())
				{
					dragging_ = false;
					return false;
				}
			}

			dragAxis_ = axis;
			dragStartWorldHit_ = startHit;
			dragAxisWorld_ = axisWorld;
			dragPlaneNormal_ = planeNormal;
			dragPivotWorld_ = gizmo.pivotWorld;

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
			if (!dragging_)
			{
				return false;
			}

			mathUtils::Vec3 currentHit{};
			if (!GizmoIntersectRayPlane(BuildMouseRay(scene, mouseX, mouseY, viewportW, viewportH), dragPivotWorld_, dragPlaneNormal_, currentHit))
			{
				return false;
			}

			mathUtils::Vec3 worldDelta{};
			if (GizmoIsAxisHandle(dragAxis_))
			{
				const float axisDelta = mathUtils::Dot(currentHit - dragStartWorldHit_, dragAxisWorld_);
				worldDelta = dragAxisWorld_ * axisDelta;
			}
			else if (GizmoIsPlaneHandle(dragAxis_))
			{
				worldDelta = currentHit - dragStartWorldHit_;
			}
			else
			{
				return false;
			}

			if (dragSelectionIsLight_)
			{
				const std::size_t n = dragLightIndices_.size();
				for (std::size_t i = 0; i < n; ++i)
				{
					const int lightIndex = dragLightIndices_[i];
					if (lightIndex < 0 || static_cast<std::size_t>(lightIndex) >= scene.lights.size())
					{
						continue;
					}

					mathUtils::Vec3 newPosition = dragStartLightPositions_[i] + worldDelta;
					if (snapEnabled)
					{
						constexpr float kTranslateSnapStep = 0.5f;
						newPosition.x = std::round(newPosition.x / kTranslateSnapStep) * kTranslateSnapStep;
						newPosition.y = std::round(newPosition.y / kTranslateSnapStep) * kTranslateSnapStep;
						newPosition.z = std::round(newPosition.z / kTranslateSnapStep) * kTranslateSnapStep;
					}

					scene.lights[static_cast<std::size_t>(lightIndex)].position = newPosition;
				}
				scene.editorTranslateGizmo.hoveredAxis = dragAxis_;
				return true;
			}

			const std::size_t n = dragNodeIndices_.size();
			for (std::size_t i = 0; i < n; ++i)
			{
				const int nodeIndex = dragNodeIndices_[i];
				if (!levelInst.IsNodeAlive(asset, nodeIndex))
				{
					continue;
				}

				const mathUtils::Vec3 localDelta = mathUtils::TransformVector(dragInvParentWorld_[i], worldDelta);
				mathUtils::Vec3 newLocalPosition = dragStartLocalPositions_[i] + localDelta;
				if (snapEnabled)
				{
					constexpr float kTranslateSnapStep = 0.5f;
					newLocalPosition.x = std::round(newLocalPosition.x / kTranslateSnapStep) * kTranslateSnapStep;
					newLocalPosition.y = std::round(newLocalPosition.y / kTranslateSnapStep) * kTranslateSnapStep;
					newLocalPosition.z = std::round(newLocalPosition.z / kTranslateSnapStep) * kTranslateSnapStep;
				}

				asset.nodes[static_cast<std::size_t>(nodeIndex)].transform.position = newLocalPosition;
			}
			scene.editorTranslateGizmo.hoveredAxis = dragAxis_;
			return true;
		}

		void EndDrag(Scene& scene) noexcept
		{
			dragging_ = false;
			dragSelectionIsLight_ = false;
			dragNodeIndices_.clear();
			dragStartLocalPositions_.clear();
			dragInvParentWorld_.clear();
			dragLightIndices_.clear();
			dragStartLightPositions_.clear();
			dragAxis_ = GizmoAxis::None;
			dragStartWorldHit_ = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
			dragAxisWorld_ = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
			dragPlaneNormal_ = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
			dragPivotWorld_ = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
			scene.editorTranslateGizmo.activeAxis = GizmoAxis::None;
			scene.editorTranslateGizmo.hoveredAxis = GizmoAxis::None;
		}

		bool IsDragging() const noexcept
		{
			return dragging_;
		}

	private:
		bool dragging_{ false };
		bool dragSelectionIsLight_{ false };
		std::vector<int> dragNodeIndices_;
		std::vector<int> dragLightIndices_;
		GizmoAxis dragAxis_{ GizmoAxis::None };
		std::vector<mathUtils::Vec3> dragStartLocalPositions_;
		std::vector<mathUtils::Vec3> dragStartLightPositions_;
		std::vector<mathUtils::Mat4> dragInvParentWorld_;
		mathUtils::Vec3 dragStartWorldHit_{ 0.0f, 0.0f, 0.0f };
		mathUtils::Vec3 dragAxisWorld_{ 0.0f, 0.0f, 0.0f };
		mathUtils::Vec3 dragPlaneNormal_{ 0.0f, 0.0f, 0.0f };
		mathUtils::Vec3 dragPivotWorld_{ 0.0f, 0.0f, 0.0f };
	};
}