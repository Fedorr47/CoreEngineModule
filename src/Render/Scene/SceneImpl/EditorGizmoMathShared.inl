	static mathUtils::Vec3 GizmoSafeNormalizeOr(const mathUtils::Vec3& v, const mathUtils::Vec3& fallback) noexcept
	{
		if (mathUtils::Length(v) < 1e-5f)
		{
			return fallback;
		}
		return mathUtils::Normalize(v);
	}

	template <typename TGizmoState>
	static mathUtils::Vec3 GizmoAxisDirection(const TGizmoState& gizmo, rendern::GizmoAxis axis) noexcept
	{
		switch (axis)
		{
		case rendern::GizmoAxis::X: return gizmo.axisXWorld;
		case rendern::GizmoAxis::Y: return gizmo.axisYWorld;
		case rendern::GizmoAxis::Z: return gizmo.axisZWorld;
		default: return mathUtils::Vec3(0.0f, 0.0f, 0.0f);
		}
	}

	template <typename TGizmoState>
	static void GizmoPlaneBasis(
		const TGizmoState& gizmo,
		rendern::GizmoAxis axis,
		mathUtils::Vec3& basisA,
		mathUtils::Vec3& basisB,
		mathUtils::Vec3& planeNormal) noexcept
	{
		switch (axis)
		{
		case rendern::GizmoAxis::XY:
			basisA = gizmo.axisXWorld;
			basisB = gizmo.axisYWorld;
			planeNormal = mathUtils::Normalize(mathUtils::Cross(basisA, basisB));
			break;
		case rendern::GizmoAxis::XZ:
			basisA = gizmo.axisXWorld;
			basisB = gizmo.axisZWorld;
			planeNormal = mathUtils::Normalize(mathUtils::Cross(basisA, basisB));
			break;
		case rendern::GizmoAxis::YZ:
			basisA = gizmo.axisYWorld;
			basisB = gizmo.axisZWorld;
			planeNormal = mathUtils::Normalize(mathUtils::Cross(basisA, basisB));
			break;
		default:
			basisA = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
			basisB = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
			planeNormal = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
			break;
		}
	}

	static bool GizmoProjectWorldToScreen(
		const rendern::Scene& scene,
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
		if (std::abs(clip.w) < 1e-6f || clip.w <= 0.0f)
		{
			return false;
		}

		const float ndcX = clip.x / clip.w;
		const float ndcY = clip.y / clip.w;
		outScreen.x = (ndcX * 0.5f + 0.5f) * viewportW;
		outScreen.y = (1.0f - (ndcY * 0.5f + 0.5f)) * viewportH;
		return true;
	}

	static bool GizmoIntersectRayPlane(
		const geometry::Ray& ray,
		const mathUtils::Vec3& planePoint,
		const mathUtils::Vec3& planeNormal,
		mathUtils::Vec3& outPoint) noexcept
	{
		const float denom = mathUtils::Dot(planeNormal, ray.dir);
		if (std::abs(denom) < 1e-6f)
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

	static bool GizmoPointInTriangle2D(
		const mathUtils::Vec2& p,
		const mathUtils::Vec2& a,
		const mathUtils::Vec2& b,
		const mathUtils::Vec2& c) noexcept
	{
		const float c0 = mathUtils::Cross2D(b - a, p - a);
		const float c1 = mathUtils::Cross2D(c - b, p - b);
		const float c2 = mathUtils::Cross2D(a - c, p - c);
		const bool hasNeg = (c0 < 0.0f) || (c1 < 0.0f) || (c2 < 0.0f);
		const bool hasPos = (c0 > 0.0f) || (c1 > 0.0f) || (c2 > 0.0f);
		return !(hasNeg && hasPos);
	}

	static bool GizmoPointInQuad2D(
		const mathUtils::Vec2& p,
		const mathUtils::Vec2& a,
		const mathUtils::Vec2& b,
		const mathUtils::Vec2& c,
		const mathUtils::Vec2& d) noexcept
	{
		return GizmoPointInTriangle2D(p, a, b, c) || GizmoPointInTriangle2D(p, a, c, d);
	}

	static bool GizmoIsAxisHandle(rendern::GizmoAxis axis) noexcept
	{
		return axis == rendern::GizmoAxis::X || axis == rendern::GizmoAxis::Y || axis == rendern::GizmoAxis::Z;
	}

	static bool GizmoIsPlaneHandle(rendern::GizmoAxis axis) noexcept
	{
		return axis == rendern::GizmoAxis::XY || axis == rendern::GizmoAxis::XZ || axis == rendern::GizmoAxis::YZ;
	}
