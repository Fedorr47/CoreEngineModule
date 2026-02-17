			renderGraph::RenderGraph graph;

			// -------------------------------------------------------------------------
			// IMPORTANT (DX12): UpdateBuffer() is flushed at the beginning of SubmitCommandList().
			// Therefore, all UpdateBuffer() calls MUST happen before graph.Execute().
			// -------------------------------------------------------------------------

			// --- camera (used for fallback lights too) ---
			const mathUtils::Vec3 camPos = scene.camera.position;

			// Upload lights once per frame (t2 StructuredBuffer SRV)
			const std::uint32_t lightCount = UploadLights(scene, camPos);

			// ---------------- Directional CSM (atlas) ----------------
			// 3 cascades packed into a single D32 atlas:
			//   atlas = (tileSize * cascadeCount) x tileSize.
			// The shader selects the cascade and remaps UVs into the atlas.
			constexpr std::uint32_t kMaxDirCascades = 3;
			constexpr std::uint32_t dirTileSize = 2048; // user request
			const std::uint32_t dirCascadeCount = std::clamp(settings_.dirShadowCascadeCount, 1u, kMaxDirCascades);
			const rhi::Extent2D shadowExtent{ dirTileSize * dirCascadeCount, dirTileSize };
			const auto shadowRG = graph.CreateTexture(renderGraph::RGTextureDesc{
				.extent = shadowExtent,
				.format = rhi::Format::D32_FLOAT,
				.usage = renderGraph::ResourceUsage::DepthStencil,
				.debugName = "DirShadowAtlas"
				});

			// Choose first directional light (or a default).
			mathUtils::Vec3 lightDir = mathUtils::Normalize(mathUtils::Vec3(-0.4f, -1.0f, -0.3f)); // FROM light towards scene
			for (const auto& light : scene.lights)
			{
				if (light.type == LightType::Directional)
				{
					lightDir = mathUtils::Normalize(light.direction);
					break;
				}
			}

			// --------------------------------
			// Fit each cascade ortho projection to a camera frustum slice in light-space.
			// The bounds are snapped to shadow texels to reduce shimmering.
			const rhi::SwapChainDesc scDesc = swapChain.GetDesc();
			const float aspect = (scDesc.extent.height > 0)
				? (static_cast<float>(scDesc.extent.width) / static_cast<float>(scDesc.extent.height))
				: 1.0f;


			const mathUtils::Mat4 cameraProj = mathUtils::PerspectiveRH_ZO(mathUtils::DegToRad(scene.camera.fovYDeg), aspect, scene.camera.nearZ, scene.camera.farZ);
			const mathUtils::Mat4 cameraView = mathUtils::LookAt(scene.camera.position, scene.camera.target, scene.camera.up);
			const mathUtils::Mat4 cameraViewProj = cameraProj * cameraView;
			const mathUtils::Frustum cameraFrustum = mathUtils::ExtractFrustumRH_ZO(cameraViewProj);
			const bool doFrustumCulling = settings_.enableFrustumCulling;

			auto IsVisible = [&](const rendern::MeshResource* meshRes, const mathUtils::Mat4& model) -> bool
				{
					if (!doFrustumCulling || !meshRes)
					{
						return true;
					}
					const auto& b = meshRes->GetBounds();
					if (b.sphereRadius <= 0.0f)
					{
						return true;
					}

					const mathUtils::Vec4 wc4 = model * mathUtils::Vec4(b.sphereCenter, 1.0f);
					const mathUtils::Vec3 worldCenter{ wc4.x, wc4.y, wc4.z };

					const mathUtils::Vec3 c0{ model[0].x, model[0].y, model[0].z };
					const mathUtils::Vec3 c1{ model[1].x, model[1].y, model[1].z };
					const mathUtils::Vec3 c2{ model[2].x, model[2].y, model[2].z };
					const float s0 = mathUtils::Length(c0);
					const float s1 = mathUtils::Length(c1);
					const float s2 = mathUtils::Length(c2);
					const float maxScale = std::max(s0, std::max(s1, s2));
					const float worldRadius = b.sphereRadius * maxScale;

					return mathUtils::IntersectsSphere(cameraFrustum, worldCenter, worldRadius);
				};

			// Limit how far we render directional shadows to keep resolution usable.
			const float shadowFar = std::min(scene.camera.farZ, settings_.dirShadowDistance);
			const float shadowNear = std::max(scene.camera.nearZ, 0.05f);

			// Camera basis (orthonormal).
			const mathUtils::Vec3 camF = mathUtils::Normalize(scene.camera.target - scene.camera.position);
			mathUtils::Vec3 camR = mathUtils::Cross(camF, scene.camera.up);
			camR = mathUtils::Normalize(camR);
			const mathUtils::Vec3 camU = mathUtils::Cross(camR, camF);

			const float fovY = mathUtils::DegToRad(scene.camera.fovYDeg);
			const float tanHalf = std::tan(fovY * 0.5f);

			auto MakeFrustumCorner = [&](float dist, float sx, float sy) -> mathUtils::Vec3
				{
					// sx,sy are in {-1,+1} (left/right, bottom/top).
					const float halfH = dist * tanHalf;
					const float halfW = halfH * aspect;
					const mathUtils::Vec3 planeCenter = scene.camera.position + camF * dist;
					return planeCenter + camU * (sy * halfH) + camR * (sx * halfW);
				};

			// Cascade split distances (camera-space) using the "practical" split scheme.
			std::array<float, kMaxDirCascades + 1> dirSplits{};
			dirSplits[0] = shadowNear;
			dirSplits[dirCascadeCount] = shadowFar;
			for (std::uint32_t i = 1; i < dirCascadeCount; ++i)
			{
				const float p = static_cast<float>(i) / static_cast<float>(dirCascadeCount);
				const float logSplit = shadowNear * std::pow(shadowFar / shadowNear, p);
				const float uniSplit = shadowNear + (shadowFar - shadowNear) * p;
				dirSplits[i] = std::lerp(uniSplit, logSplit, settings_.dirShadowSplitLambda);
			}

			// Stable "up" for light view.
			const mathUtils::Vec3 worldUp(0.0f, 1.0f, 0.0f);
			const mathUtils::Vec3 lightUp = (std::abs(mathUtils::Dot(lightDir, worldUp)) > 0.99f)
				? mathUtils::Vec3(0.0f, 0.0f, 1.0f)
				: worldUp;

			// Build a view-proj per cascade.
			std::array<mathUtils::Mat4, kMaxDirCascades> dirCascadeVP{};
			for (std::uint32_t c = 0; c < dirCascadeCount; ++c)
			{
				const float cNear = dirSplits[c];
				const float cFar = dirSplits[c + 1];

				std::array<mathUtils::Vec3, 8> frustumCorners{};
				// Near plane
				frustumCorners[0] = MakeFrustumCorner(cNear, -1.0f, -1.0f);
				frustumCorners[1] = MakeFrustumCorner(cNear, 1.0f, -1.0f);
				frustumCorners[2] = MakeFrustumCorner(cNear, 1.0f, 1.0f);
				frustumCorners[3] = MakeFrustumCorner(cNear, -1.0f, 1.0f);
				// Far plane
				frustumCorners[4] = MakeFrustumCorner(cFar, -1.0f, -1.0f);
				frustumCorners[5] = MakeFrustumCorner(cFar, 1.0f, -1.0f);
				frustumCorners[6] = MakeFrustumCorner(cFar, 1.0f, 1.0f);
				frustumCorners[7] = MakeFrustumCorner(cFar, -1.0f, 1.0f);

				// Frustum center + radius (for stable light placement).
				mathUtils::Vec3 center{ 0.0f, 0.0f, 0.0f };
				for (const auto& corner : frustumCorners)
				{
					center = center + corner;
				}
				center = center * (1.0f / 8.0f);

				float radius = 0.0f;
				for (const auto& corner : frustumCorners)
				{
					radius = std::max(radius, mathUtils::Length(corner - center));
				}

				// Place the light far enough so all corners are in front of it.
				const float lightDist = radius + 100.0f;
				const mathUtils::Vec3 lightPos = center - lightDir * lightDist;
				const mathUtils::Mat4 lightView = mathUtils::LookAt(lightPos, center, lightUp);

				// Compute light-space AABB of the camera frustum slice.
				float minX = 1e30f, minY = 1e30f, minZ = 1e30f;
				float maxX = -1e30f, maxY = -1e30f, maxZ = -1e30f;
				for (const auto& corner : frustumCorners)
				{
					const mathUtils::Vec4 ls4 = lightView * mathUtils::Vec4(corner, 1.0f);
					minX = std::min(minX, ls4.x); maxX = std::max(maxX, ls4.x);
					minY = std::min(minY, ls4.y); maxY = std::max(maxY, ls4.y);
					minZ = std::min(minZ, ls4.z); maxZ = std::max(maxZ, ls4.z);
				}

				// Conservative padding in light-space to avoid hard clipping.
				const float extX = maxX - minX;
				const float extY = maxY - minY;
				const float extZ = maxZ - minZ;

				const float padXY = 0.05f * std::max(extX, extY) + 1.0f;
				const float padZ = 0.10f * extZ + 5.0f;
				minX -= padXY; maxX += padXY;
				minY -= padXY; maxY += padXY;
				minZ -= padZ;  maxZ += padZ;

				// Extra depth margin for casters outside the camera frustum (increases with cascade index).
				const float casterMargin = 20.0f + 30.0f * static_cast<float>(c);
				minZ -= casterMargin;

				// Snap the ortho window to shadow texels (reduces shimmering / popping).
				const float widthLS = maxX - minX;
				const float heightLS = maxY - minY;
				const float wuPerTexelX = widthLS / static_cast<float>(dirTileSize);
				const float wuPerTexelY = heightLS / static_cast<float>(dirTileSize);
				float cx = 0.5f * (minX + maxX);
				float cy = 0.5f * (minY + maxY);
				cx = std::floor(cx / wuPerTexelX) * wuPerTexelX;
				cy = std::floor(cy / wuPerTexelY) * wuPerTexelY;
				minX = cx - widthLS * 0.5f;  maxX = cx + widthLS * 0.5f;
				minY = cy - heightLS * 0.5f; maxY = cy + heightLS * 0.5f;

				// OrthoRH_ZO expects positive zNear/zFar distances where view-space z is negative in front of the camera.
				const float zNear = std::max(0.1f, -maxZ);
				const float zFar = std::max(zNear + 1.0f, -minZ);
				const mathUtils::Mat4 lightProj = mathUtils::OrthoRH_ZO(minX, maxX, minY, maxY, zNear, zFar);
				dirCascadeVP[c] = lightProj * lightView;
			}

			// For legacy constant-buffer field (kept for compatibility with older shaders).
			const mathUtils::Mat4 dirLightViewProj = dirCascadeVP[0];

			std::vector<SpotShadowRec> spotShadows;
			std::vector<PointShadowRec> pointShadows;
			spotShadows.reserve(kMaxSpotShadows);
			pointShadows.reserve(kMaxPointShadows);


