inline void WriteCameraSection_(std::ostringstream& ss, const LevelAsset& level)
{
	// camera (always write if present)
	if (level.camera)
	{
		const Camera& cam = *level.camera;
		ss << "  \"camera\": {\"position\": ";
		WriteJsonVec3(ss, cam.position);
		ss << ", \"target\": ";
		WriteJsonVec3(ss, cam.target);
		ss << ", \"up\": ";
		WriteJsonVec3(ss, cam.up);
		ss << ", \"fovYDeg\": ";
		WriteJsonFloat(ss, cam.fovYDeg);
		ss << ", \"nearZ\": ";
		WriteJsonFloat(ss, cam.nearZ);
		ss << ", \"farZ\": ";
		WriteJsonFloat(ss, cam.farZ);
		ss << "},\n";
	}

}

inline void WriteLightsSection_(std::ostringstream& ss, const LevelAsset& level)
{
	// lights
	ss << "  \"lights\": [";
	for (std::size_t i = 0; i < level.lights.size(); ++i)
	{
		const Light& l = level.lights[i];
		if (i == 0) ss << "\n"; else ss << ",\n";
		ss << "    {";
		std::string_view type = "directional";
		if (l.type == LightType::Point) type = "point";
		else if (l.type == LightType::Spot) type = "spot";
		ss << "\"type\": ";
		WriteJsonEscaped(ss, type);
		ss << ", \"position\": ";
		WriteJsonVec3(ss, l.position);
		ss << ", \"direction\": ";
		WriteJsonVec3(ss, l.direction);
		ss << ", \"color\": ";
		WriteJsonVec3(ss, l.color);
		ss << ", \"intensity\": ";
		WriteJsonFloat(ss, l.intensity);
		ss << ", \"range\": ";
		WriteJsonFloat(ss, l.range);
		ss << ", \"innerHalfAngleDeg\": ";
		WriteJsonFloat(ss, l.innerHalfAngleDeg);
		ss << ", \"outerHalfAngleDeg\": ";
		WriteJsonFloat(ss, l.outerHalfAngleDeg);
		ss << ", \"attConstant\": ";
		WriteJsonFloat(ss, l.attConstant);
		ss << ", \"attLinear\": ";
		WriteJsonFloat(ss, l.attLinear);
		ss << ", \"attQuadratic\": ";
		WriteJsonFloat(ss, l.attQuadratic);
		ss << "}";
	}
	if (!level.lights.empty()) ss << "\n  ";
	ss << "],\n";

}

inline void WriteParticleEmittersSection_(std::ostringstream& ss, const LevelAsset& level)
{
	// particle emitters
	ss << "  \"particleEmitters\": [";
	for (std::size_t i = 0; i < level.particleEmitters.size(); ++i)
	{
		const ParticleEmitter& e = level.particleEmitters[i];
		if (i == 0) ss << "\n"; else ss << ",\n";
		ss << "    {";
		ss << "\"name\": ";
		WriteJsonEscaped(ss, e.name);
		ss << ", \"textureId\": ";
		WriteJsonEscaped(ss, e.textureId);
		ss << ", \"enabled\": ";
		WriteJsonBool(ss, e.enabled);
		ss << ", \"looping\": ";
		WriteJsonBool(ss, e.looping);
		ss << ", \"position\": ";
		WriteJsonVec3(ss, e.position);
		ss << ", \"positionJitter\": ";
		WriteJsonVec3(ss, e.positionJitter);
		ss << ", \"velocityMin\": ";
		WriteJsonVec3(ss, e.velocityMin);
		ss << ", \"velocityMax\": ";
		WriteJsonVec3(ss, e.velocityMax);
		ss << ", \"colorBegin\": ";
		WriteJsonVec4(ss, e.colorBegin);
		ss << ", \"colorEnd\": ";
		WriteJsonVec4(ss, e.colorEnd);
		ss << ", \"sizeBegin\": ";
		WriteJsonFloat(ss, e.sizeBegin > 0.0f ? e.sizeBegin : e.sizeMin);
		ss << ", \"sizeEnd\": ";
		WriteJsonFloat(ss, e.sizeEnd > 0.0f ? e.sizeEnd : e.sizeMax);
		ss << ", \"lifetime\": [";
		WriteJsonFloat(ss, e.lifetimeMin);
		ss << ", ";
		WriteJsonFloat(ss, e.lifetimeMax);
		ss << "]";
		ss << ", \"spawnRate\": ";
		WriteJsonFloat(ss, e.spawnRate);
		ss << ", \"burstCount\": ";
		ss << e.burstCount;
		ss << ", \"duration\": ";
		WriteJsonFloat(ss, e.duration);
		ss << ", \"startDelay\": ";
		WriteJsonFloat(ss, e.startDelay);
		ss << ", \"maxParticles\": ";
		ss << e.maxParticles;
		ss << "}";
	}
	if (!level.particleEmitters.empty()) ss << "\n  ";
	ss << "],\n";

}

inline void WriteSkyboxSection_(std::ostringstream& ss, const LevelAsset& level)
{
	// skybox: write as string or null (keep loader happy and stable)
	ss << "  \"skybox\": ";
	if (level.skyboxTexture && !level.skyboxTexture->empty())
	{
		WriteJsonEscaped(ss, *level.skyboxTexture);
	}
	else
	{
		ss << "null";
	}
	ss << ",\n";

}

inline void WriteNodesSection_(std::ostringstream& ss, const LevelAsset& level, const AliveNodeRemap_& remap)
{
	// nodes (alive only)
	ss << "  \"nodes\": [";
	for (std::size_t ni = 0; ni < remap.newToOld.size(); ++ni)
	{
		const LevelNode& n = level.nodes[static_cast<std::size_t>(remap.newToOld[ni])];
		if (ni == 0) ss << "\n"; else ss << ",\n";
		ss << "    {";
		ss << "\"name\": ";
		WriteJsonEscaped(ss, n.name);

		int parent = -1;
		if (n.parent >= 0)
		{
			const std::size_t op = static_cast<std::size_t>(n.parent);
			if (op < remap.oldToNew.size() && remap.oldToNew[op] >= 0)
				parent = remap.oldToNew[op];
		}
		ss << ", \"parent\": " << parent;
		ss << ", \"visible\": ";
		WriteJsonBool(ss, n.visible);

		if (!n.model.empty())
		{
			ss << ", \"model\": ";
			WriteJsonEscaped(ss, n.model);
		}
		if (!n.mesh.empty())
		{
			ss << ", \"mesh\": ";
			WriteJsonEscaped(ss, n.mesh);
		}
		if (!n.skinnedMesh.empty())
		{
			ss << ", \"skinnedMesh\": ";
			WriteJsonEscaped(ss, n.skinnedMesh);
		}
		if (!n.material.empty())
		{
			ss << ", \"material\": ";
			WriteJsonEscaped(ss, n.material);
		}
		if (!n.materialOverrides.empty())
		{
			ss << ", \"materialOverrides\": {";
			bool firstOverride = true;
			for (const auto& [submeshIndex, materialId] : n.materialOverrides)
			{
				if (!firstOverride) ss << ", ";
				WriteJsonEscaped(ss, std::to_string(submeshIndex));
				ss << ": ";
				WriteJsonEscaped(ss, materialId);
				firstOverride = false;
			}
			ss << "}";
		}
		if (!n.animation.empty())
		{
			ss << ", \"animation\": ";
			WriteJsonEscaped(ss, n.animation);
		}
		if (!n.animationController.empty())
		{
			ss << ", \"animationController\": ";
			WriteJsonEscaped(ss, n.animationController);
		}
		if (!n.animationClip.empty())
		{
			ss << ", \"animationClip\": ";
			WriteJsonEscaped(ss, n.animationClip);
		}
		if (!n.animationInPlace)
		{
			ss << ", \"animationInPlace\": false";
		}
		if (!n.animationRootMotionBone.empty())
		{
			ss << ", \"animationRootMotionBone\": ";
			WriteJsonEscaped(ss, n.animationRootMotionBone);
		}
		if (!n.animationAutoplay)
		{
			ss << ", \"animationAutoplay\": false";
		}
		if (!n.animationLoop)
		{
			ss << ", \"animationLoop\": false";
		}
		if (std::fabs(n.animationPlayRate - 1.0f) > 1e-6f)
		{
			ss << ", \"animationPlayRate\": " << n.animationPlayRate;
		}
		ss << ", \"transform\": {";
		if (n.transform.useMatrix)
		{
			ss << "\"matrix\": ";
			WriteJsonMat4ColMajor16(ss, n.transform.matrix);
		}
		else
		{
			ss << "\"position\": ";
			WriteJsonVec3(ss, n.transform.position);
			ss << ", \"rotationDegrees\": ";
			WriteJsonVec3(ss, n.transform.rotationDegrees);
			ss << ", \"scale\": ";
			WriteJsonVec3(ss, n.transform.scale);
		}
		ss << "}";

		ss << "}";
	}
	if (!remap.newToOld.empty()) ss << "\n  ";
	ss << "]\n";
}
