inline void ParseCameraSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- camera ---
	if (auto* camV = TryGet(jsonObject, "camera"))
	{
		const JsonObject& cd = camV->AsObject();
		Camera cam;
		if (auto* p = TryGet(cd, "position"))
		{
			auto a = ReadFloatArray(*p, 3, "camera.position");
			cam.position = { a[0], a[1], a[2] };
		}
		if (auto* t = TryGet(cd, "target"))
		{
			auto a = ReadFloatArray(*t, 3, "camera.target");
			cam.target = { a[0], a[1], a[2] };
		}
		if (auto* up = TryGet(cd, "up"))
		{
			auto a = ReadFloatArray(*up, 3, "camera.up");
			cam.up = { a[0], a[1], a[2] };
		}
		cam.fovYDeg = GetFloatOpt(cd, "fovYDeg", cam.fovYDeg);
		cam.nearZ = GetFloatOpt(cd, "nearZ", cam.nearZ);
		cam.farZ = GetFloatOpt(cd, "farZ", cam.farZ);
		out.camera = cam;
	}

}

inline void ParseLightSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- lights ---
	if (auto* lightsV = TryGet(jsonObject, "lights"))
	{
		for (const auto& lv : lightsV->AsArray())
		{
			const JsonObject& ld = lv.AsObject();
			Light l;
			const std::string type = GetStringOpt(ld, "type", "directional");
			if (type == "directional") l.type = LightType::Directional;
			else if (type == "point") l.type = LightType::Point;
			else if (type == "spot") l.type = LightType::Spot;
			else throw std::runtime_error("Level JSON: unknown light type: " + type);

			if (auto* p = TryGet(ld, "position"))
			{
				auto a = ReadFloatArray(*p, 3, "light.position");
				l.position = { a[0], a[1], a[2] };
			}
			if (auto* d = TryGet(ld, "direction"))
			{
				auto a = ReadFloatArray(*d, 3, "light.direction");
				l.direction = mathUtils::Normalize({ a[0], a[1], a[2] });
			}
			if (auto* c = TryGet(ld, "color"))
			{
				auto a = ReadFloatArray(*c, 3, "light.color");
				l.color = { a[0], a[1], a[2] };
			}
			l.intensity = GetFloatOpt(ld, "intensity", l.intensity);
			l.range = GetFloatOpt(ld, "range", l.range);
			l.innerHalfAngleDeg = GetFloatOpt(ld, "innerHalfAngleDeg", l.innerHalfAngleDeg);
			l.outerHalfAngleDeg = GetFloatOpt(ld, "outerHalfAngleDeg", l.outerHalfAngleDeg);
			l.attConstant = GetFloatOpt(ld, "attConstant", l.attConstant);
			l.attLinear = GetFloatOpt(ld, "attLinear", l.attLinear);
			l.attQuadratic = GetFloatOpt(ld, "attQuadratic", l.attQuadratic);

			out.lights.push_back(l);
		}
	}

}

inline void ParseParticleEmitterSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- particle emitters ---
	if (auto* emittersV = TryGet(jsonObject, "particleEmitters"))
	{
		for (const auto& ev : emittersV->AsArray())
		{
			const JsonObject& ed = ev.AsObject();
			ParticleEmitter emitter;
			emitter.name = GetStringOpt(ed, "name");
			emitter.textureId = GetStringOpt(ed, "textureId");
			emitter.enabled = GetBoolOpt(ed, "enabled", emitter.enabled);
			emitter.looping = GetBoolOpt(ed, "looping", emitter.looping);
			if (auto* p = TryGet(ed, "position"))
			{
				auto a = ReadFloatArray(*p, 3, "particleEmitters.position");
				emitter.position = { a[0], a[1], a[2] };
			}
			if (auto* p = TryGet(ed, "positionJitter"))
			{
				auto a = ReadFloatArray(*p, 3, "particleEmitters.positionJitter");
				emitter.positionJitter = { a[0], a[1], a[2] };
			}
			if (auto* p = TryGet(ed, "velocityMin"))
			{
				auto a = ReadFloatArray(*p, 3, "particleEmitters.velocityMin");
				emitter.velocityMin = { a[0], a[1], a[2] };
			}
			if (auto* p = TryGet(ed, "velocityMax"))
			{
				auto a = ReadFloatArray(*p, 3, "particleEmitters.velocityMax");
				emitter.velocityMax = { a[0], a[1], a[2] };
			}
			if (auto* c = TryGet(ed, "color"))
			{
				auto a = ReadFloatArray(*c, 4, "particleEmitters.color");
				emitter.color = { a[0], a[1], a[2], a[3] };
				emitter.colorBegin = emitter.color;
				emitter.colorEnd = emitter.color;
			}
			if (auto* c = TryGet(ed, "colorBegin"))
			{
				auto a = ReadFloatArray(*c, 4, "particleEmitters.colorBegin");
				emitter.colorBegin = { a[0], a[1], a[2], a[3] };
			}
			if (auto* c = TryGet(ed, "colorEnd"))
			{
				auto a = ReadFloatArray(*c, 4, "particleEmitters.colorEnd");
				emitter.colorEnd = { a[0], a[1], a[2], a[3] };
			}
			if (auto* s = TryGet(ed, "size"))
			{
				auto a = ReadFloatArray(*s, 2, "particleEmitters.size");
				emitter.sizeMin = a[0];
				emitter.sizeMax = a[1];
			}
			emitter.sizeMin = GetFloatOpt(ed, "sizeMin", emitter.sizeMin);
			emitter.sizeMax = GetFloatOpt(ed, "sizeMax", emitter.sizeMax);
			emitter.sizeBegin = GetFloatOpt(ed, "sizeBegin", emitter.sizeBegin);
			emitter.sizeEnd = GetFloatOpt(ed, "sizeEnd", emitter.sizeEnd);
			if (auto* s = TryGet(ed, "lifetime"))
			{
				auto a = ReadFloatArray(*s, 2, "particleEmitters.lifetime");
				emitter.lifetimeMin = a[0];
				emitter.lifetimeMax = a[1];
			}
			emitter.lifetimeMin = GetFloatOpt(ed, "lifetimeMin", emitter.lifetimeMin);
			emitter.lifetimeMax = GetFloatOpt(ed, "lifetimeMax", emitter.lifetimeMax);
			emitter.spawnRate = GetFloatOpt(ed, "spawnRate", emitter.spawnRate);
			emitter.burstCount = static_cast<std::uint32_t>(std::max(0.0f, GetFloatOpt(ed, "burstCount", static_cast<float>(emitter.burstCount))));
			emitter.duration = GetFloatOpt(ed, "duration", emitter.duration);
			emitter.startDelay = GetFloatOpt(ed, "startDelay", emitter.startDelay);
			emitter.maxParticles = static_cast<std::uint32_t>(std::max(0.0f, GetFloatOpt(ed, "maxParticles", static_cast<float>(emitter.maxParticles))));
			out.particleEmitters.push_back(std::move(emitter));
		}
	}

}

inline void ParseSkyboxSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- skybox ---
	if (auto* sb = TryGet(jsonObject, "skybox"))
	{
		// Accept either:
		//   - string: "SkyboxTexId"
		//   - object: { "textureId": "SkyboxTexId" }  (or "texture")
		//   - null
		if (sb->IsNull())
		{
			// ok
		}
		else if (sb->IsString())
		{
			out.skyboxTexture = sb->AsString();
		}
		else if (sb->IsObject())
		{
			const JsonObject& sbo = sb->AsObject();
			const JsonValue* t = TryGet(sbo, "textureId");
			if (!t) t = TryGet(sbo, "texture");
			if (!t) t = TryGet(sbo, "id");
			if (!t)
			{
				throw std::runtime_error("Level JSON: skybox object must contain 'textureId' (or 'texture')");
			}
			if (t->IsNull())
			{
				// ok
			}
			else if (t->IsString())
			{
				out.skyboxTexture = t->AsString();
			}
			else
			{
				throw std::runtime_error("Level JSON: skybox.textureId must be string or null");
			}
		}
		else
		{
			throw std::runtime_error("Level JSON: skybox must be string, object, or null");
		}
	}

}

inline void ParseNodeSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- nodes ---
	if (auto* nodesV = TryGet(jsonObject, "nodes"))
	{
		for (const auto& nv : nodesV->AsArray())
		{
			const JsonObject& nd = nv.AsObject();
			LevelNode n;
			n.name = GetStringOpt(nd, "name");
			n.parent = static_cast<int>(GetFloatOpt(nd, "parent", -1.0f));
			n.visible = GetBoolOpt(nd, "visible", true);
			n.alive = GetBoolOpt(nd, "alive", true);
			if (auto* delV = TryGet(nd, "deleted"))
			{
				if (!delV->IsBool())
				{
					throw std::runtime_error("Level JSON: node.deleted must be bool");
				}
				n.alive = !delV->AsBool();
			}
			n.mesh = GetStringOpt(nd, "mesh");
			n.model = GetStringOpt(nd, "model");
			n.skinnedMesh = GetStringOpt(nd, "skinnedMesh");
			n.material = GetStringOpt(nd, "material");
			n.animation = GetStringOpt(nd, "animation");
			n.animationController = GetStringOpt(nd, "animationController");
			n.animationClip = GetStringOpt(nd, "animationClip");
			n.animationInPlace = GetBoolOpt(nd, "animationInPlace", true);
			n.animationRootMotionBone = GetStringOpt(nd, "animationRootMotionBone");
			n.animationAutoplay = GetBoolOpt(nd, "animationAutoplay", true);
			n.animationLoop = GetBoolOpt(nd, "animationLoop", true);
			n.animationPlayRate = GetFloatOpt(nd, "animationPlayRate", 1.0f);
			if (auto* overridesV = TryGet(nd, "materialOverrides"))
			{
				if (!overridesV->IsObject())
				{
					throw std::runtime_error("Level JSON: node.materialOverrides must be object");
				}
				for (const auto& [submeshKey, materialValue] : overridesV->AsObject())
				{
					if (!materialValue.IsString())
					{
						throw std::runtime_error("Level JSON: node.materialOverrides values must be strings");
					}

					char* end = nullptr;
					const unsigned long parsed = std::strtoul(submeshKey.c_str(), &end, 10);
					if (end == submeshKey.c_str() || *end != '\0')
					{
						throw std::runtime_error("Level JSON: node.materialOverrides keys must be unsigned integers");
					}

					n.materialOverrides.emplace(static_cast<std::uint32_t>(parsed), materialValue.AsString());
				}
			}

			if (auto* trV = TryGet(nd, "transform"))
			{
				const JsonObject& td = trV->AsObject();
				Transform t;
				if (auto* matV = TryGet(td, "matrix"))
				{
					t.useMatrix = true;
					t.matrix = ReadMat4_ColumnMajor16(*matV, "transform.matrix");
				}
				else
				{
					if (auto* p = TryGet(td, "position"))
					{
						auto a = ReadFloatArray(*p, 3, "transform.position");
						t.position = { a[0], a[1], a[2] };
					}
					if (auto* r = TryGet(td, "rotationDegrees"))
					{
						auto a = ReadFloatArray(*r, 3, "transform.rotationDegrees");
						t.rotationDegrees = { a[0], a[1], a[2] };
					}
					if (auto* s = TryGet(td, "scale"))
					{
						auto a = ReadFloatArray(*s, 3, "transform.scale");
						t.scale = { a[0], a[1], a[2] };
					}
				}
				n.transform = t;
			}

			out.nodes.push_back(std::move(n));
		}
	}
}
