module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// D3D-style clip-space helpers (Z in [0..1]).
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_access.hpp>

#include <array>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

export module core:renderer_dx12;

import :rhi;
import :scene;
import :renderer_settings;
import :render_core;
import :render_graph;
import :file_system;
import :mesh;

export namespace rendern
{

	// multiple Spot/Point shadow casters (DX12). Keep small caps for now.
	constexpr std::uint32_t kMaxSpotShadows = 4;
	constexpr std::uint32_t kMaxPointShadows = 4;

	struct alignas(16) GPULight
	{
		std::array<float, 4> p0{}; // pos.xyz, type
		std::array<float, 4> p1{}; // dir.xyz (FROM light), intensity
		std::array<float, 4> p2{}; // color.rgb, range
		std::array<float, 4> p3{}; // cosInner, cosOuter, attLin, attQuad
	};

	struct alignas(16) InstanceData
	{
		// Column-major 4x4 model matrix (glm::mat4 columns).
		glm::vec4 c0{};
		glm::vec4 c1{};
		glm::vec4 c2{};
		glm::vec4 c3{};
	};

	struct BatchKey
	{
		const rendern::MeshRHI* mesh{};
		// Material key (must be immutable during RenderFrame)
		rhi::TextureDescIndex albedoDescIndex{};
		glm::vec4 baseColor{};
		float shininess{};
		float specStrength{};
		float shadowBias{};
		MaterialHandle material{};
	};

	struct BatchKeyHash
	{
		static std::size_t HashU32(std::uint32_t v) noexcept { return std::hash<std::uint32_t>{}(v); }
		static std::size_t HashPtr(const void* p) noexcept { return std::hash<const void*>{}(p); }

		static std::uint32_t FBits(float v) noexcept
		{
			std::uint32_t b{};
			std::memcpy(&b, &v, sizeof(b));
			return b;
		}

		static void HashCombine(std::size_t& h, std::size_t v) noexcept
		{
			h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
		}

		std::size_t operator()(const BatchKey& k) const noexcept
		{
			std::size_t h = HashPtr(k.mesh);
			HashCombine(h, HashU32(static_cast<std::uint32_t>(k.albedoDescIndex)));
			HashCombine(h, HashU32(FBits(k.baseColor.x)));
			HashCombine(h, HashU32(FBits(k.baseColor.y)));
			HashCombine(h, HashU32(FBits(k.baseColor.z)));
			HashCombine(h, HashU32(FBits(k.baseColor.w)));
			HashCombine(h, HashU32(FBits(k.shininess)));
			HashCombine(h, HashU32(FBits(k.specStrength)));
			HashCombine(h, HashU32(FBits(k.shadowBias)));
			return h;
		}
	};

	struct BatchKeyEq
	{
		bool operator()(const BatchKey& a, const BatchKey& b) const noexcept
		{
			return a.mesh == b.mesh &&
				a.albedoDescIndex == b.albedoDescIndex &&
				a.baseColor == b.baseColor &&
				a.shininess == b.shininess &&
				a.specStrength == b.specStrength &&
				a.shadowBias == b.shadowBias;
		}
	};

	struct BatchTemp
	{
		MaterialParams material{};
		MaterialHandle materialHandle{};
		std::vector<InstanceData> inst;
	};

	struct Batch
	{
		const rendern::MeshRHI* mesh{};
		MaterialParams material{};
		MaterialHandle materialHandle{};
		std::uint32_t instanceOffset = 0; // in instances[]
		std::uint32_t instanceCount = 0;
	};

	struct alignas(16) PerBatchConstants
	{
		std::array<float, 16> uViewProj{};
		std::array<float, 16> uLightViewProj{};
		std::array<float, 4>  uCameraAmbient{}; // xyz + ambient
		std::array<float, 4>  uBaseColor{};
		std::array<float, 4>  uMaterialFlags{}; // shininess, specStrength, shadowBias, flags
		std::array<float, 4>  uCounts{};         // lightCount, ...
	};
	static_assert(sizeof(PerBatchConstants) == 192);

	// shadow metadata for Spot/Point arrays (bound as StructuredBuffer at t11).
	// We pack indices/bias as floats to keep the struct simple across compilers.
	struct alignas(16) ShadowDataSB
	{
		// Spot view-projection matrices as ROWS (4 matrices * 4 rows = 16 float4).
		std::array<glm::vec4, kMaxSpotShadows * 4> spotVPRows{};
		// spotInfo[i] = { lightIndexBits, bias, 0, 0 }
		std::array<glm::vec4, kMaxSpotShadows>     spotInfo{};

		// pointPosRange[i] = { pos.x, pos.y, pos.z, range }
		std::array<glm::vec4, kMaxPointShadows>    pointPosRange{};
		// pointInfo[i] = { lightIndexBits, bias, 0, 0 }
		std::array<glm::vec4, kMaxPointShadows>    pointInfo{};
	};
	static_assert((sizeof(ShadowDataSB) % 16) == 0);

	struct alignas(16) ShadowConstants
	{
		std::array<float, 16> uMVP{}; // lightProj * lightView * model
	};

	class DX12Renderer
	{
	public:
		DX12Renderer(rhi::IRHIDevice& device, RendererSettings settings = {})
			: device_(device)
			, settings_(std::move(settings))
			, shaderLibrary_(device)
			, psoCache_(device)
		{
			CreateResources();
		}

		void RenderFrame(rhi::IRHISwapChain& swapChain, const Scene& scene)
		{
			renderGraph::RenderGraph graph;

			// ---------------- Shadow map (directional) ----------------
			const rhi::Extent2D shadowExtent{ 2048, 2048 };
			const auto shadowRG = graph.CreateTexture(renderGraph::RGTextureDesc{
				.extent = shadowExtent,
				.format = rhi::Format::D32_FLOAT,
				.usage = renderGraph::ResourceUsage::DepthStencil,
				.debugName = "ShadowMap"
			});

			// Choose first directional light (or a default).
			glm::vec3 lightDir = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f)); // FROM light towards scene
			for (const auto& l : scene.lights)
			{
				if (l.type == LightType::Directional)
				{
					lightDir = glm::normalize(l.direction);
					break;
				}
			}

			const glm::vec3 center = scene.camera.target; // stage-1 heuristic
			const float lightDist = 10.0f;
			const glm::vec3 lightPos = center - lightDir * lightDist;
			const glm::mat4 lightView = glm::lookAt(lightPos, center, glm::vec3(0, 1, 0));

			// Stage-1: fixed ortho volume around the origin/target.
			const float orthoHalf = 8.0f;
			const glm::mat4 lightProj = glm::orthoRH_ZO(-orthoHalf, orthoHalf, -orthoHalf, orthoHalf, 0.1f, 40.0f);

			// Shadow pass (depth-only)
			{
				rhi::ClearDesc clear{};
				clear.clearColor = false;
				clear.clearDepth = true;
				clear.depth = 1.0f;

				renderGraph::PassAttachments att{};
				att.useSwapChainBackbuffer = false;
				att.color = std::nullopt;
				att.depth = shadowRG;
				att.clearDesc = clear;

				graph.AddPass("ShadowPass", std::move(att),
					[this, &scene, lightView, lightProj](renderGraph::PassContext& ctx)
					{
						ctx.commandList.SetViewport(0, 0,
							static_cast<int>(ctx.passExtent.width),
							static_cast<int>(ctx.passExtent.height));

						ctx.commandList.SetState(shadowState_);
						ctx.commandList.BindPipeline(psoShadow_);

						// --- Instance data (ROWS!) -------------------------------------------------
						struct alignas(16) InstanceData
						{
							glm::vec4 r0{};
							glm::vec4 r1{};
							glm::vec4 r2{};
							glm::vec4 r3{};
						};

						struct ShadowBatch
						{
							const rendern::MeshRHI* mesh{};
							std::uint32_t instanceOffset = 0; // in instances[]
							std::uint32_t instanceCount = 0;
						};

						std::unordered_map<const rendern::MeshRHI*, std::vector<InstanceData>> tmp;
						tmp.reserve(scene.drawItems.size());

						for (const auto& item : scene.drawItems)
						{
							const rendern::MeshRHI* mesh = item.mesh ? &item.mesh->GetResource() : nullptr;
							if (!mesh || mesh->indexCount == 0) continue;

							const glm::mat4 model = item.transform.ToMatrix();

							InstanceData inst{};
							inst.r0 = model[0];
							inst.r1 = model[1];
							inst.r2 = model[2];
							inst.r3 = model[3];

							tmp[mesh].push_back(inst);
						}

						std::vector<InstanceData> instances;
						instances.reserve(scene.drawItems.size());

						std::vector<ShadowBatch> batches;
						batches.reserve(tmp.size());

						for (auto& [mesh, vec] : tmp)
						{
							if (!mesh || vec.empty()) continue;

							ShadowBatch b{};
							b.mesh = mesh;
							b.instanceOffset = static_cast<std::uint32_t>(instances.size());
							b.instanceCount = static_cast<std::uint32_t>(vec.size());

							instances.insert(instances.end(), vec.begin(), vec.end());
							batches.push_back(b);
						}

						if (!instances.empty())
						{
							const std::size_t bytes = instances.size() * sizeof(InstanceData);
							if (bytes > shadowInstanceBufferSizeBytes_)
							{
								throw std::runtime_error("ShadowPass: shadowInstanceBuffer_ overflow (increase shadowInstanceBufferSizeBytes_)");
							}
							device_.UpdateBuffer(shadowInstanceBuffer_, std::as_bytes(std::span{ instances }));
						}

						// --- Per-pass constants (одни на все draw calls) ---------------------------
						struct alignas(16) ShadowConstants
						{
							std::array<float, 16> uLightViewProj{};
						};

						const glm::mat4 lightViewProj = lightProj * lightView;

						ShadowConstants c{};
						const glm::mat4 lightViewProjT = glm::transpose(lightViewProj);
						std::memcpy(c.uLightViewProj.data(), glm::value_ptr(lightViewProjT), sizeof(float) * 16);
						ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &c, 1 }));

						// --- Draw instanced -------------------------------------------------------
						const std::uint32_t instStride = static_cast<std::uint32_t>(sizeof(InstanceData));

						for (const ShadowBatch& b : batches)
						{
							if (!b.mesh || b.instanceCount == 0) continue;

							ctx.commandList.BindInputLayout(b.mesh->layoutInstanced);

							ctx.commandList.BindVertexBuffer(0, b.mesh->vertexBuffer, b.mesh->vertexStrideBytes, 0);
							ctx.commandList.BindVertexBuffer(1, shadowInstanceBuffer_, instStride, b.instanceOffset * instStride);

							ctx.commandList.BindIndexBuffer(b.mesh->indexBuffer, b.mesh->indexType, 0);

							ctx.commandList.DrawIndexed(
								b.mesh->indexCount,
								b.mesh->indexType,
								0, 0,
								b.instanceCount,
								0);
						}
					});
			}

			// ---------------- Spot/Point shadow maps (arrays) ----------------
			struct SpotShadowRec
			{
				renderGraph::RGTextureHandle tex{};
				glm::mat4 viewProj{};
				std::uint32_t lightIndex = 0;
				float bias = 0.0020f;
			};

			struct PointShadowRec
			{
				renderGraph::RGTextureHandle cube{};
				renderGraph::RGTextureHandle depthTmp{};
				glm::vec3 pos{};
				float range = 10.0f;
				std::uint32_t lightIndex = 0;
				float bias = 0.0060f;
			};

			std::vector<SpotShadowRec> spotShadows;
			spotShadows.reserve(kMaxSpotShadows);

			std::vector<PointShadowRec> pointShadows;
			pointShadows.reserve(kMaxPointShadows);

			// Collect up to kMaxSpotShadows / kMaxPointShadows from scene.lights (index aligns with UploadLights()).
			for (std::uint32_t li = 0; li < static_cast<std::uint32_t>(scene.lights.size()); ++li)
			{
				if (li >= kMaxLights)
					break;

				const auto& l = scene.lights[li];

				if (l.type == LightType::Spot && spotShadows.size() < kMaxSpotShadows)
				{
					const rhi::Extent2D ext{ 1024, 1024 };
					const auto rg = graph.CreateTexture(renderGraph::RGTextureDesc{
						.extent = ext,
						.format = rhi::Format::D32_FLOAT,
						.usage = renderGraph::ResourceUsage::DepthStencil,
						.debugName = "SpotShadowMap"
					});

					const glm::vec3 up = glm::vec3(0, 1, 0);
					const glm::vec3 dir = glm::normalize(l.direction);
					const glm::mat4 v = glm::lookAt(l.position, l.position + dir, up);
					const float outer = std::max(1.0f, l.outerAngleDeg);
					const glm::mat4 p = glm::perspectiveRH_ZO(glm::radians(outer * 2.0f), 1.0f, 0.1f, std::max(1.0f, l.range));
					const glm::mat4 vp = p * v;

					SpotShadowRec rec{};
					rec.tex = rg;
					rec.viewProj = vp;
					rec.lightIndex = li;
					rec.bias = 0.0020f;
					spotShadows.push_back(rec);

					// Depth-only spot shadow pass (same shader/pso as directional shadow)
					rhi::ClearDesc clear{};
					clear.clearColor = false;
					clear.clearDepth = true;
					clear.depth = 1.0f;

					renderGraph::PassAttachments att{};
					att.useSwapChainBackbuffer = false;
					att.color = std::nullopt;
					att.depth = rg;
					att.clearDesc = clear;

					const std::string passName = "SpotShadowPass_" + std::to_string(static_cast<int>(spotShadows.size() - 1));

					graph.AddPass(passName, std::move(att),
						[this, &scene, vp](renderGraph::PassContext& ctx)
						{
							ctx.commandList.SetViewport(0, 0,
								static_cast<int>(ctx.passExtent.width),
								static_cast<int>(ctx.passExtent.height));

							ctx.commandList.SetState(shadowState_);
							ctx.commandList.BindPipeline(psoShadow_);

							// Same instance batching as ShadowPass
							struct alignas(16) InstanceDataRows
							{
								glm::vec4 r0{}, r1{}, r2{}, r3{};
							};

							struct ShadowBatch
							{
								const rendern::MeshRHI* mesh{};
								std::uint32_t instanceOffset = 0;
								std::uint32_t instanceCount = 0;
							};

							std::unordered_map<const rendern::MeshRHI*, std::vector<InstanceDataRows>> tmp;
							tmp.reserve(scene.drawItems.size());

							for (const auto& item : scene.drawItems)
							{
								const rendern::MeshRHI* mesh = item.mesh ? &item.mesh->GetResource() : nullptr;
								if (!mesh || mesh->indexCount == 0) continue;

								const glm::mat4 model = item.transform.ToMatrix();
								//const glm::mat4 mt = glm::transpose(model);

								InstanceDataRows inst{};
								inst.r0 = model[0];
								inst.r1 = model[1];
								inst.r2 = model[2];
								inst.r3 = model[3];

								tmp[mesh].push_back(inst);
							}

							std::vector<InstanceDataRows> instances;
							instances.reserve(scene.drawItems.size());

							std::vector<ShadowBatch> batches;
							batches.reserve(tmp.size());

							for (auto& [mesh, vec] : tmp)
							{
								if (!mesh || vec.empty()) continue;

								ShadowBatch b{};
								b.mesh = mesh;
								b.instanceOffset = static_cast<std::uint32_t>(instances.size());
								b.instanceCount = static_cast<std::uint32_t>(vec.size());

								instances.insert(instances.end(), vec.begin(), vec.end());
								batches.push_back(b);
							}

							if (!instances.empty())
							{
								const std::size_t bytes = instances.size() * sizeof(InstanceDataRows);
								if (bytes > shadowInstanceBufferSizeBytes_)
								{
									throw std::runtime_error("SpotShadowPass: shadowInstanceBuffer_ overflow (increase shadowInstanceBufferSizeBytes_)");
								}
								device_.UpdateBuffer(shadowInstanceBuffer_, std::as_bytes(std::span{ instances }));
							}

							struct alignas(16) ShadowConstantsLocal
							{
								std::array<float, 16> uLightViewProj{};
							};

							ShadowConstantsLocal c{};
							const glm::mat4 vpT = glm::transpose(vp);
							std::memcpy(c.uLightViewProj.data(), glm::value_ptr(vpT), sizeof(float) * 16);
							ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &c, 1 }));

							const std::uint32_t instStride = static_cast<std::uint32_t>(sizeof(InstanceDataRows));

							for (const ShadowBatch& b : batches)
							{
								if (!b.mesh || b.instanceCount == 0) continue;

								ctx.commandList.BindInputLayout(b.mesh->layoutInstanced);

								ctx.commandList.BindVertexBuffer(0, b.mesh->vertexBuffer, b.mesh->vertexStrideBytes, 0);
								ctx.commandList.BindVertexBuffer(1, shadowInstanceBuffer_, instStride, b.instanceOffset * instStride);

								ctx.commandList.BindIndexBuffer(b.mesh->indexBuffer, b.mesh->indexType, 0);

								ctx.commandList.DrawIndexed(
									b.mesh->indexCount,
									b.mesh->indexType,
									0, 0,
									b.instanceCount,
									0);
							}
						});
				}
				else if (l.type == LightType::Point && pointShadows.size() < kMaxPointShadows)
				{
					// Point shadows use a cubemap R32_FLOAT distance map (color) + a temporary D32 depth buffer.
					const rhi::Extent2D cubeExtent{ 512, 512 };
					const auto cube = graph.CreateTexture(renderGraph::RGTextureDesc{
						.extent = cubeExtent,
						.format = rhi::Format::R32_FLOAT,
						.usage = renderGraph::ResourceUsage::RenderTarget,
						.type = renderGraph::TextureType::Cube,
						.debugName = "PointShadowCube"
					});

					const auto depthTmp = graph.CreateTexture(renderGraph::RGTextureDesc{
						.extent = cubeExtent,
						.format = rhi::Format::D32_FLOAT,
						.usage = renderGraph::ResourceUsage::DepthStencil,
						.debugName = "PointShadowDepthTmp"
					});

					PointShadowRec rec{};
					rec.cube = cube;
					rec.depthTmp = depthTmp;
					rec.pos = l.position;
					rec.range = std::max(1.0f, l.range);
					rec.lightIndex = li;
					rec.bias = 0.0060f;
					pointShadows.push_back(rec);

					auto FaceView = [](const glm::vec3& p, int face) -> glm::mat4
					{
						// +X, -X, +Y, -Y, +Z, -Z
						static const glm::vec3 dirs[6] = {
							{ 1, 0, 0 },{-1, 0, 0 },{ 0, 1, 0 },{ 0,-1, 0 },{ 0, 0, 1 },{ 0, 0,-1 }
						};
						static const glm::vec3 ups[6] = {
							{ 0, 1, 0 },{ 0, 1, 0 },{ 0, 0,-1 },{ 0, 0, 1 },{ 0, 1, 0 },{ 0, 1, 0 }
						};
						return glm::lookAt(p, p + dirs[face], ups[face]);
					};

					const glm::mat4 proj90 = glm::perspectiveRH_ZO(glm::radians(90.0f), 1.0f, 0.1f, rec.range);

					for (int face = 0; face < 6; ++face)
					{
						const glm::mat4 vp = proj90 * FaceView(rec.pos, face);

						rhi::ClearDesc clear{};
						clear.clearColor = true;
						clear.clearDepth = true;
						clear.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // far
						clear.depth = 1.0f;

						renderGraph::PassAttachments att{};
						att.useSwapChainBackbuffer = false;
						att.color = cube;
						att.colorCubeFace = static_cast<std::uint32_t>(face);
						att.depth = depthTmp;
						att.clearDesc = clear;

						const std::string passName =
							"PointShadowPass_" + std::to_string(static_cast<int>(pointShadows.size() - 1)) +
							"_F" + std::to_string(face);

						graph.AddPass(passName, std::move(att),
							[this, &scene, vp, rec](renderGraph::PassContext& ctx)
							{
								ctx.commandList.SetViewport(0, 0,
									static_cast<int>(ctx.passExtent.width),
									static_cast<int>(ctx.passExtent.height));

								ctx.commandList.SetState(pointShadowState_);
								ctx.commandList.BindPipeline(psoPointShadow_);

								// Same instance batching as ShadowPass
								struct alignas(16) InstanceDataRows
								{
									glm::vec4 r0{}, r1{}, r2{}, r3{};
								};

								struct ShadowBatch
								{
									const rendern::MeshRHI* mesh{};
									std::uint32_t instanceOffset = 0;
									std::uint32_t instanceCount = 0;
								};

								std::unordered_map<const rendern::MeshRHI*, std::vector<InstanceDataRows>> tmp;
								tmp.reserve(scene.drawItems.size());

								for (const auto& item : scene.drawItems)
								{
									const rendern::MeshRHI* mesh = item.mesh ? &item.mesh->GetResource() : nullptr;
									if (!mesh || mesh->indexCount == 0) continue;

									const glm::mat4 model = item.transform.ToMatrix();

									InstanceDataRows inst{};
									inst.r0 = model[0];
									inst.r1 = model[1];
									inst.r2 = model[2];
									inst.r3 = model[3];

									tmp[mesh].push_back(inst);
								}

								std::vector<InstanceDataRows> instances;
								instances.reserve(scene.drawItems.size());

								std::vector<ShadowBatch> batches;
								batches.reserve(tmp.size());

								for (auto& [mesh, vec] : tmp)
								{
									if (!mesh || vec.empty()) continue;

									ShadowBatch b{};
									b.mesh = mesh;
									b.instanceOffset = static_cast<std::uint32_t>(instances.size());
									b.instanceCount = static_cast<std::uint32_t>(vec.size());

									instances.insert(instances.end(), vec.begin(), vec.end());
									batches.push_back(b);
								}

								if (!instances.empty())
								{
									const std::size_t bytes = instances.size() * sizeof(InstanceDataRows);
									if (bytes > shadowInstanceBufferSizeBytes_)
									{
										throw std::runtime_error("PointShadowPass: shadowInstanceBuffer_ overflow (increase shadowInstanceBufferSizeBytes_)");
									}
									device_.UpdateBuffer(shadowInstanceBuffer_, std::as_bytes(std::span{ instances }));
								}

								struct alignas(16) PointShadowConstants
								{
									std::array<float, 16> uFaceViewProj{};
									std::array<float, 4>  uLightPosRange{}; // xyz + range
									std::array<float, 4>  uMisc{};          // bias, ...
								};

								PointShadowConstants c{};
								const glm::mat4 vpT = glm::transpose(vp);
								std::memcpy(c.uFaceViewProj.data(), glm::value_ptr(vpT), sizeof(float) * 16);
								c.uLightPosRange = { rec.pos.x, rec.pos.y, rec.pos.z, rec.range };
								c.uMisc = { rec.bias, 0, 0, 0 };
								ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &c, 1 }));

								const std::uint32_t instStride = static_cast<std::uint32_t>(sizeof(InstanceDataRows));

								for (const ShadowBatch& b : batches)
								{
									if (!b.mesh || b.instanceCount == 0) continue;

									ctx.commandList.BindInputLayout(b.mesh->layoutInstanced);

									ctx.commandList.BindVertexBuffer(0, b.mesh->vertexBuffer, b.mesh->vertexStrideBytes, 0);
									ctx.commandList.BindVertexBuffer(1, shadowInstanceBuffer_, instStride, b.instanceOffset * instStride);

									ctx.commandList.BindIndexBuffer(b.mesh->indexBuffer, b.mesh->indexType, 0);

									ctx.commandList.DrawIndexed(
										b.mesh->indexCount,
										b.mesh->indexType,
										0, 0,
										b.instanceCount,
										0);
								}
							});
					}
				}
			}

			// Upload shadow metadata (t11). This is used by the stage-4 DX12 shader to pick the right shadow map per light.
			{
				ShadowDataSB sd{};

				for (std::size_t i = 0; i < spotShadows.size(); ++i)
				{
					const auto& s = spotShadows[i];
					sd.spotVPRows[i * 4 + 0] = s.viewProj[0];
					sd.spotVPRows[i * 4 + 1] = s.viewProj[1];
					sd.spotVPRows[i * 4 + 2] = s.viewProj[2];
					sd.spotVPRows[i * 4 + 3] = s.viewProj[3];
					sd.spotInfo[i] = glm::vec4(AsFloatBits(s.lightIndex), s.bias, 0, 0);
				}

				for (std::size_t i = 0; i < pointShadows.size(); ++i)
				{
					const auto& p = pointShadows[i];
					sd.pointPosRange[i] = glm::vec4(p.pos, p.range);
					sd.pointInfo[i] = glm::vec4(AsFloatBits(p.lightIndex), p.bias, 0, 0);
				}

				device_.UpdateBuffer(shadowDataBuffer_, std::as_bytes(std::span{ &sd, 1 }));
			}

			// ---------------- Main pass (swapchain) ----------------
			rhi::ClearDesc clearDesc{};
			clearDesc.clearColor = true;
			clearDesc.clearDepth = true;
			clearDesc.color = { 0.1f, 0.1f, 0.1f, 1.0f };

			graph.AddSwapChainPass("MainPass", clearDesc,
				[this, &scene, shadowRG, lightView, lightProj, spotShadows, pointShadows](renderGraph::PassContext& ctx)
				{
					const auto extent = ctx.passExtent;

					ctx.commandList.SetViewport(0, 0,
						static_cast<int>(extent.width),
						static_cast<int>(extent.height));

					ctx.commandList.SetState(state_);

					// Bind shadow map at slot 1 (t1)
					{
						const auto shadowTex = ctx.resources.GetTexture(shadowRG);
						ctx.commandList.BindTexture2D(1, shadowTex);
					}

					// Bind Spot shadow maps at t3..t6 and Point shadow cubemaps at t7..t10.
					for (std::size_t i = 0; i < spotShadows.size(); ++i)
					{
						const auto tex = ctx.resources.GetTexture(spotShadows[i].tex);
						ctx.commandList.BindTexture2D(3 + static_cast<std::uint32_t>(i), tex);
					}
					for (std::size_t i = 0; i < pointShadows.size(); ++i)
					{
						const auto tex = ctx.resources.GetTexture(pointShadows[i].cube);
						ctx.commandList.BindTextureCube(7 + static_cast<std::uint32_t>(i), tex);
					}

					// Bind shadow metadata SB at t11
					ctx.commandList.BindStructuredBufferSRV(11, shadowDataBuffer_);

					const float aspect = extent.height
						? (static_cast<float>(extent.width) / static_cast<float>(extent.height))
						: 1.0f;

					const glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(scene.camera.fovYDeg), aspect, scene.camera.nearZ, scene.camera.farZ);
					const glm::mat4 view = glm::lookAt(scene.camera.position, scene.camera.target, scene.camera.up);
					const glm::vec3 camPos = scene.camera.position;

					// Upload and bind lights (t2 StructuredBuffer SRV)
					const std::uint32_t lightCount = UploadLights(scene, camPos);
					ctx.commandList.BindStructuredBufferSRV(2, lightsBuffer_);

					// --- Batch build (proper packing) -------------------------------------------
					// 1) Gather per-batch instance lists
					std::unordered_map<BatchKey, BatchTemp, BatchKeyHash, BatchKeyEq> tmp;
					tmp.reserve(scene.drawItems.size());

					for (const auto& item : scene.drawItems)
					{
						const rendern::MeshRHI* mesh = item.mesh ? &item.mesh->GetResource() : nullptr;
						if (!mesh || mesh->indexCount == 0) continue;

						BatchKey key{};
						key.mesh = mesh;
						key.material = item.material;

						MaterialParams params{};
						if (item.material.id != 0)
						{
							params = scene.GetMaterial(item.material).params;
						}

						// IMPORTANT: BatchKey must include material parameters,
						// otherwise different materials get incorrectly merged.
						key.albedoDescIndex = params.albedoDescIndex;
						key.baseColor = params.baseColor;
						key.shininess = params.shininess;
						key.specStrength = params.specStrength;
						key.shadowBias = params.shadowBias;

						// Build instance data
						const glm::mat4 model = item.transform.ToMatrix();

						InstanceData inst{};
						inst.c0 = model[0];
						inst.c1 = model[1];
						inst.c2 = model[2];
						inst.c3 = model[3];

						auto& bucket = tmp[key];
						if (bucket.inst.empty())
						{
							bucket.materialHandle = item.material;
							bucket.material = params; // representative material for this batch
						}
						bucket.inst.push_back(inst);
					}

					// 2) Pack into one big contiguous instances[] + build batches with offsets
					std::vector<InstanceData> instances;
					instances.reserve(scene.drawItems.size());

					std::vector<Batch> batches;
					batches.reserve(tmp.size());

					for (auto& [key, bt] : tmp)
					{
						if (bt.inst.empty())
						{
							continue;
						}

						Batch b{};
						b.mesh = key.mesh;
						b.materialHandle = bt.materialHandle;
						b.material = bt.material;
						b.instanceOffset = static_cast<std::uint32_t>(instances.size());
						b.instanceCount = static_cast<std::uint32_t>(bt.inst.size());

						instances.insert(instances.end(), bt.inst.begin(), bt.inst.end());
						batches.push_back(b);
					}

					// 3) Upload instance buffer once
					if (!instances.empty())
					{
						const std::size_t bytes = instances.size() * sizeof(InstanceData);
						if (bytes > instanceBufferSizeBytes_)
						{
							throw std::runtime_error("DX12Renderer: instance buffer overflow (increase instanceBufferSizeBytes_)");
						}

						device_.UpdateBuffer(instanceBuffer_, std::as_bytes(std::span{ instances }));
					}

					// 4) (Optional debug)
					if (settings_.debugPrintDrawCalls)
					{
						static std::uint32_t frame = 0;
						if ((++frame % 60u) == 0u)
						{
							std::cout << "[DX12] MainPass draw calls: " << batches.size()
								<< " (instances total: " << instances.size() << ")\n";
						}
					}
					// --- End batch build --------------------------------------------------------

					const glm::mat4 viewProj = proj * view;
					const glm::mat4 lightViewProj = lightProj * lightView;

					constexpr std::uint32_t kFlagUseTex = 1u << 0;
					constexpr std::uint32_t kFlagUseShadow = 1u << 1;

					for (const Batch& b : batches)
					{
						if (!b.mesh || b.instanceCount == 0)
						{
							continue;
						}
						
						MaterialPerm perm = MaterialPerm::UseShadow;
						if (b.materialHandle.id != 0)
						{
							perm = EffectivePerm(scene.GetMaterial(b.materialHandle));
						}
						else
						{
							// Fallback: infer only from params.
							if (b.material.albedoDescIndex != 0)
								perm |= MaterialPerm::UseTex;
						}

						const bool useTex = HasFlag(perm, MaterialPerm::UseTex);
						const bool useShadow = HasFlag(perm, MaterialPerm::UseShadow);

						ctx.commandList.BindPipeline(MainPipelineFor(perm));
						ctx.commandList.BindTextureDesc(0, b.material.albedoDescIndex);

						std::uint32_t flags = 0;
						if (useTex)    flags |= kFlagUseTex;
						if (useShadow) flags |= kFlagUseShadow;

						// --- constants ---
						PerBatchConstants constatntsBatch{};
						const glm::mat4 viewProjT = glm::transpose(viewProj);
						const glm::mat4 lightViewProjT = glm::transpose(lightViewProj);
						std::memcpy(constatntsBatch.uViewProj.data(), glm::value_ptr(viewProjT), sizeof(float) * 16);
						std::memcpy(constatntsBatch.uLightViewProj.data(), glm::value_ptr(lightViewProjT), sizeof(float) * 16);

						constatntsBatch.uCameraAmbient = { camPos.x, camPos.y, camPos.z, 0.22f };
						constatntsBatch.uBaseColor = { b.material.baseColor.x, b.material.baseColor.y, b.material.baseColor.z, b.material.baseColor.w };

						const float shadowBias = (b.material.shadowBias != 0.0f) ? b.material.shadowBias : 0.0015f;
						constatntsBatch.uMaterialFlags = { b.material.shininess, b.material.specStrength, shadowBias, AsFloatBits(flags) };
						constatntsBatch.uCounts = { 
							static_cast<float>(lightCount), 
							static_cast<float>(spotShadows.size()), 
							static_cast<float>(pointShadows.size()), 
							0 };

						// --- IA (instanced) ---
						ctx.commandList.BindInputLayout(b.mesh->layoutInstanced);
						ctx.commandList.BindVertexBuffer(0, b.mesh->vertexBuffer, b.mesh->vertexStrideBytes, 0);

						const std::uint32_t instStride = static_cast<std::uint32_t>(sizeof(InstanceData));
						const std::uint32_t instOffsetBytes = b.instanceOffset * instStride;
						ctx.commandList.BindVertexBuffer(1, instanceBuffer_, instStride, instOffsetBytes);

						ctx.commandList.BindIndexBuffer(b.mesh->indexBuffer, b.mesh->indexType, 0);

						// bind constants to root param 0 (as before)
						ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &constatntsBatch, 1 }));
						ctx.commandList.DrawIndexed(b.mesh->indexCount, b.mesh->indexType, 0, 0, b.instanceCount, 0);
					}
				});

			graph.Execute(device_, swapChain);
			swapChain.Present();
		}

		void Shutdown()
		{
			if (instanceBuffer_)
			{
				device_.DestroyBuffer(instanceBuffer_);
			}
			if (shadowInstanceBuffer_)
			{
				device_.DestroyBuffer(shadowInstanceBuffer_);
			}
			if (lightsBuffer_)
			{
				device_.DestroyBuffer(lightsBuffer_);
			}
			if (shadowDataBuffer_)
			{
				device_.DestroyBuffer(shadowDataBuffer_);
			}
			psoCache_.ClearCache();
			shaderLibrary_.ClearCache();
		}

	private:
		rhi::PipelineHandle MainPipelineFor(MaterialPerm perm) const noexcept
		{
			const bool useTex = HasFlag(perm, MaterialPerm::UseTex);
			const bool useShadow = HasFlag(perm, MaterialPerm::UseShadow);
			const std::uint32_t idx = (useTex ? 1u : 0u) | (useShadow ? 2u : 0u);
			return psoMain_[idx];
		}

		static float AsFloatBits(std::uint32_t u) noexcept
		{
			float f{};
			std::memcpy(&f, &u, sizeof(u));
			return f;
		}

		std::uint32_t UploadLights(const Scene& scene, const glm::vec3& camPos)
		{
			std::vector<GPULight> gpu;
			gpu.reserve(std::min<std::size_t>(scene.lights.size(), kMaxLights));

			for (const auto& l : scene.lights)
			{
				if (gpu.size() >= kMaxLights)
					break;

				GPULight out{};

				out.p0 = { l.position.x, l.position.y, l.position.z, static_cast<float>(static_cast<std::uint32_t>(l.type)) };
				out.p1 = { l.direction.x, l.direction.y, l.direction.z, l.intensity };
				out.p2 = { l.color.x, l.color.y, l.color.z, l.range };

				const float cosOuter = std::cos(glm::radians(l.outerAngleDeg));
				const float cosInner = std::cos(glm::radians(l.innerAngleDeg));

				out.p3 = { cosInner, cosOuter, l.attLinear, l.attQuadratic };
				gpu.push_back(out);
			}

			// Small default rig if the scene didn't provide any lights
			if (gpu.empty())
			{
				GPULight dir{};
				const glm::vec3 dirFromLight = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f));
				dir.p0 = { 0,0,0, static_cast<float>(static_cast<std::uint32_t>(LightType::Directional)) };
				dir.p1 = { dirFromLight.x, dirFromLight.y, dirFromLight.z, 1.2f };
				dir.p2 = { 1.0f, 1.0f, 1.0f, 0.0f };
				dir.p3 = { 0,0,0,0 };
				gpu.push_back(dir);

				GPULight point{};
				point.p0 = { 2.5f, 2.0f, 1.5f, static_cast<float>(static_cast<std::uint32_t>(LightType::Point)) };
				point.p1 = { 0,0,0, 2.0f };
				point.p2 = { 1.0f, 0.95f, 0.8f, 12.0f };
				point.p3 = { 0,0, 0.12f, 0.04f };
				gpu.push_back(point);

				GPULight spot{};
				const glm::vec3 spotPos = camPos;
				const glm::vec3 spotDir = glm::normalize(glm::vec3(0, 0, 0) - camPos);
				spot.p0 = { spotPos.x, spotPos.y, spotPos.z, static_cast<float>(static_cast<std::uint32_t>(LightType::Spot)) };
				spot.p1 = { spotDir.x, spotDir.y, spotDir.z, 3.0f };
				spot.p2 = { 0.8f, 0.9f, 1.0f, 30.0f };
				spot.p3 = { std::cos(glm::radians(12.0f)), std::cos(glm::radians(20.0f)), 0.09f, 0.032f };
				gpu.push_back(spot);
			}

			device_.UpdateBuffer(lightsBuffer_, std::as_bytes(std::span{ gpu }));
			return static_cast<std::uint32_t>(gpu.size());
		}

		void CreateResources()
		{
			std::filesystem::path shaderPath;
			std::filesystem::path shadowPath;
			std::filesystem::path pointShadowPath;

			switch (device_.GetBackend())
			{
			case rhi::Backend::DirectX12:
				shaderPath = corefs::ResolveAsset("shaders\\GlobalShaderInstanced_dx12.hlsl");
				shadowPath = corefs::ResolveAsset("shaders\\ShadowDepth_dx12.hlsl");
				pointShadowPath = corefs::ResolveAsset("shaders\\ShadowPoint_dx12.hlsl");
				break;
			default:
				shaderPath = corefs::ResolveAsset("shaders\\VS.vert");
				shadowPath = corefs::ResolveAsset("shaders\\VS.vert");
				break;
			}

			// Main pipeline permutations (UseTex / UseShadow)
			{
				auto MakeDefines = [](bool useTex, bool useShadow) -> std::vector<std::string>
				{
					std::vector<std::string> d;
					if (useTex)    d.push_back("USE_TEX=1");
					if (useShadow) d.push_back("USE_SHADOW=1");
					return d;
				};

				for (std::uint32_t idx = 0; idx < 4; ++idx)
				{
					const bool useTex = (idx & 1u) != 0;
					const bool useShadow = (idx & 2u) != 0;
					const auto defs = MakeDefines(useTex, useShadow);

					const auto vs = shaderLibrary_.GetOrCreateShader(ShaderKey{
						.stage = rhi::ShaderStage::Vertex,
						.name = "VSMain",
						.filePath = shaderPath.string(),
						.defines = defs
					});
					const auto ps = shaderLibrary_.GetOrCreateShader(ShaderKey{
						.stage = rhi::ShaderStage::Pixel,
						.name = "PSMain",
						.filePath = shaderPath.string(),
						.defines = defs
					});

					std::string psoName = "PSO_Mesh";
					if (useTex) psoName += "_Tex";
					if (useShadow) psoName += "_Shadow";

					psoMain_[idx] = psoCache_.GetOrCreate(psoName, vs, ps);
				}

				state_.depth.testEnable = true;
				state_.depth.writeEnable = true;
				state_.depth.depthCompareOp = rhi::CompareOp::LessEqual;

				state_.rasterizer.cullMode = rhi::CullMode::Back;
				state_.rasterizer.frontFace = rhi::FrontFace::CounterClockwise;

				state_.blend.enable = false;
			}

			// Shadow pipeline (depth-only)
			if (device_.GetBackend() == rhi::Backend::DirectX12)
			{
				const auto vsShadow = shaderLibrary_.GetOrCreateShader(ShaderKey{
					.stage = rhi::ShaderStage::Vertex,
					.name = "VS_Shadow",
					.filePath = shadowPath.string(),
					.defines = {}
				});
				const auto psShadow = shaderLibrary_.GetOrCreateShader(ShaderKey{
					.stage = rhi::ShaderStage::Pixel,
					.name = "PS_Shadow",
					.filePath = shadowPath.string(),
					.defines = {}
				});

				psoShadow_ = psoCache_.GetOrCreate("PSO_Shadow", vsShadow, psShadow);

				shadowState_.depth.testEnable = true;
				shadowState_.depth.writeEnable = true;
				shadowState_.depth.depthCompareOp = rhi::CompareOp::LessEqual;

				// Disable culling for the shadow pass in stage-1 (avoid winding issues).
				shadowState_.rasterizer.cullMode = rhi::CullMode::None;
				shadowState_.rasterizer.frontFace = rhi::FrontFace::CounterClockwise;

				shadowState_.blend.enable = false;
			}


			// Point shadow pipeline (R32_FLOAT distance cubemap)
			if (device_.GetBackend() == rhi::Backend::DirectX12)
			{
				const auto vsPoint = shaderLibrary_.GetOrCreateShader(ShaderKey{
					.stage = rhi::ShaderStage::Vertex,
					.name = "VS_ShadowPoint",
					.filePath = pointShadowPath.string(),
					.defines = {}
				});
				const auto psPoint = shaderLibrary_.GetOrCreateShader(ShaderKey{
					.stage = rhi::ShaderStage::Pixel,
					.name = "PS_ShadowPoint",
					.filePath = pointShadowPath.string(),
					.defines = {}
				});
				psoPointShadow_ = psoCache_.GetOrCreate("PSO_PointShadow", vsPoint, psPoint);

				pointShadowState_.depth.testEnable = true;
				pointShadowState_.depth.writeEnable = true;
				pointShadowState_.depth.depthCompareOp = rhi::CompareOp::LessEqual;

				pointShadowState_.rasterizer.cullMode = rhi::CullMode::None;
				pointShadowState_.rasterizer.frontFace = rhi::FrontFace::CounterClockwise;

				pointShadowState_.blend.enable = false;
			}

			// DX12-only dynamic buffers
			if (device_.GetBackend() == rhi::Backend::DirectX12)
			{
				// Lights structured buffer (t2)
				{
					rhi::BufferDesc ld{};
					ld.bindFlag = rhi::BufferBindFlag::StructuredBuffer;
					ld.usageFlag = rhi::BufferUsageFlag::Dynamic;
					ld.sizeInBytes = sizeof(GPULight) * kMaxLights;
					ld.structuredStrideBytes = static_cast<std::uint32_t>(sizeof(GPULight));
					ld.debugName = "LightsSB";
					lightsBuffer_ = device_.CreateBuffer(ld);
				}


				// Shadow metadata structured buffer (t11) — holds spot VP rows + indices/bias, and point pos/range + indices/bias.
				{
					rhi::BufferDesc sd{};
					sd.bindFlag = rhi::BufferBindFlag::StructuredBuffer;
					sd.usageFlag = rhi::BufferUsageFlag::Dynamic;
					sd.sizeInBytes = sizeof(ShadowDataSB);
					sd.structuredStrideBytes = static_cast<std::uint32_t>(sizeof(ShadowDataSB));
					sd.debugName = "ShadowDataSB";
					shadowDataBuffer_ = device_.CreateBuffer(sd);
				}

				// Per-instance model matrices VB (slot1)
				{
					rhi::BufferDesc id{};
					id.bindFlag = rhi::BufferBindFlag::VertexBuffer;
					id.usageFlag = rhi::BufferUsageFlag::Dynamic;
					id.sizeInBytes = instanceBufferSizeBytes_;
					id.debugName = "InstanceVB";
					instanceBuffer_ = device_.CreateBuffer(id);
				}
				// Per-instance model matrices for ShadowPass (slot1)
				{
					rhi::BufferDesc id{};
					id.bindFlag = rhi::BufferBindFlag::VertexBuffer;
					id.usageFlag = rhi::BufferUsageFlag::Dynamic;
					id.sizeInBytes = shadowInstanceBufferSizeBytes_;
					id.debugName = "ShadowInstanceVB";
					shadowInstanceBuffer_ = device_.CreateBuffer(id);
				}
			}
		}

	private:
		static constexpr std::uint32_t kMaxLights = 64;
		static constexpr std::uint32_t kDefaultInstanceBufferSizeBytes = 1024u * 1024u; // 1 MB (~16k instances)

		rhi::IRHIDevice& device_;
		RendererSettings settings_{};

		ShaderLibrary shaderLibrary_;
		PSOCache psoCache_;

		// Main pass
		std::array<rhi::PipelineHandle, 4> psoMain_{}; // idx: (UseTex?1:0)|(UseShadow?2:0)
		rhi::GraphicsState state_{};

		rhi::BufferHandle instanceBuffer_{};
		std::uint32_t instanceBufferSizeBytes_{ kDefaultInstanceBufferSizeBytes };

		rhi::BufferHandle shadowInstanceBuffer_{};
		std::uint32_t shadowInstanceBufferSizeBytes_{ kDefaultInstanceBufferSizeBytes };

		// Shadow pass
		rhi::PipelineHandle psoShadow_{};
		rhi::GraphicsState shadowState_{};

		rhi::BufferHandle lightsBuffer_{};
		rhi::BufferHandle shadowDataBuffer_{};

		// Point shadow pass (R32_FLOAT distance cubemap)
		rhi::PipelineHandle psoPointShadow_{};
		rhi::GraphicsState pointShadowState_{};
	};
} // namespace rendern
