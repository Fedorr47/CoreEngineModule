inline void WriteMeshesSection_(std::ostringstream& ss, const LevelAsset& level)
{
	// meshes
	ss << "  \"meshes\": {";
	{
		auto keys = SortedStringKeys(level.meshes);
		for (std::size_t i = 0; i < keys.size(); ++i)
		{
			const auto& id = keys[i];
			const LevelMeshDef& md = level.meshes.at(id);
			if (i == 0) ss << "\n"; else ss << ",\n";
			ss << "    ";
			WriteJsonEscaped(ss, id);
			ss << ": {\"path\": ";
			WriteJsonEscaped(ss, md.path);
			if (!md.debugName.empty())
			{
				ss << ", \"debugName\": ";
				WriteJsonEscaped(ss, md.debugName);
			}
			if (!md.flipUVs)
			{
				ss << ", \"flipUVs\": false";
			}
			if (md.submeshIndex)
			{
				ss << ", \"submeshIndex\": " << *md.submeshIndex;
			}
			if (!md.bakeNodeTransforms)
			{
				ss << ", \"bakeNodeTransforms\": false";
			}
			ss << "}";
		}
		if (!keys.empty()) ss << "\n  ";
	}
	ss << "},\n";

}

inline void WriteModelsSection_(std::ostringstream& ss, const LevelAsset& level)
{
	// models
	ss << "  \"models\": {";
	{
		auto keys = SortedStringKeys(level.models);
		for (std::size_t i = 0; i < keys.size(); ++i)
		{
			const auto& id = keys[i];
			const LevelModelDef& md = level.models.at(id);
			if (i == 0) ss << "\n"; else ss << ",\n";
			ss << "    ";
			WriteJsonEscaped(ss, id);
			ss << ": {\"path\": ";
			WriteJsonEscaped(ss, md.path);
			ss << ", \"flipUVs\": ";
			WriteJsonBool(ss, md.flipUVs);
			if (!md.debugName.empty())
			{
				ss << ", \"debugName\": ";
				WriteJsonEscaped(ss, md.debugName);
			}
			ss << "}";
		}
		if (!keys.empty()) ss << "\n  ";
	}
	ss << "},\n";

}

inline void WriteSkinnedMeshesSection_(std::ostringstream& ss, const LevelAsset& level)
{
	// skinnedMeshes
	ss << "  \"skinnedMeshes\": {";
	{
		auto keys = SortedStringKeys(level.skinnedMeshes);
		for (std::size_t i = 0; i < keys.size(); ++i)
		{
			const auto& id = keys[i];
			const LevelSkinnedMeshDef& md = level.skinnedMeshes.at(id);
			if (i == 0) ss << "\n"; else ss << ",\n";
			ss << "    ";
			WriteJsonEscaped(ss, id);
			ss << ": {\"path\": ";
			WriteJsonEscaped(ss, md.path);
			ss << ", \"flipUVs\": ";
			WriteJsonBool(ss, md.flipUVs);
			if (!md.debugName.empty())
			{
				ss << ", \"debugName\": ";
				WriteJsonEscaped(ss, md.debugName);
			}
			if (md.submeshIndex)
			{
				ss << ", \"submeshIndex\": " << *md.submeshIndex;
			}
			ss << "}";
		}
		if (!keys.empty()) ss << "\n  ";
	}
	ss << "},\n";

}

inline void WriteAnimationsSection_(std::ostringstream& ss, const LevelAsset& level)
{
	// animations
	ss << "  \"animations\": {";
	{
		auto keys = SortedStringKeys(level.animations);
		for (std::size_t i = 0; i < keys.size(); ++i)
		{
			const auto& id = keys[i];
			const LevelAnimationDef& md = level.animations.at(id);
			if (i == 0) ss << "\n"; else ss << ",\n";
			ss << "    ";
			WriteJsonEscaped(ss, id);
			ss << ": {\"path\": ";
			WriteJsonEscaped(ss, md.path);
			ss << ", \"flipUVs\": ";
			WriteJsonBool(ss, md.flipUVs);
			if (!md.debugName.empty())
			{
				ss << ", \"debugName\": ";
				WriteJsonEscaped(ss, md.debugName);
			}
			ss << "}";
		}
		if (!keys.empty()) ss << "\n  ";
	}
	ss << "},\n";

}

inline void WriteAnimationControllerAssetsSection_(std::ostringstream& ss, const LevelAsset& level)
{
	// animationControllerAssets
	ss << "  \"animationControllerAssets\": {";
	{
		auto keys = SortedStringKeys(level.animationControllerAssetPaths);
		for (std::size_t i = 0; i < keys.size(); ++i)
		{
			const auto& id = keys[i];
			if (i == 0) ss << "\n"; else ss << ",\n";
			ss << "    ";
			WriteJsonEscaped(ss, id);
			ss << ": {\"path\": ";
			WriteJsonEscaped(ss, level.animationControllerAssetPaths.at(id));
			ss << "}";
		}
		if (!keys.empty()) ss << "\n  ";
	}
	ss << "},\n";

}

inline void WriteAnimationControllersSection_(std::ostringstream& ss, const LevelAsset& level)
{
	// animationControllers
	ss << "  \"animationControllers\": {";
	{
		auto keys = SortedStringKeys(level.animationControllers);
		bool firstController = true;
		for (std::size_t i = 0; i < keys.size(); ++i)
		{
			const auto& id = keys[i];
			if (level.animationControllerAssetPaths.contains(id))
			{
				continue;
			}
			const AnimationControllerAsset& controller = level.animationControllers.at(id);
			if (firstController) ss << "\n"; else ss << ",\n";
			firstController = false;
			ss << "    ";
			WriteJsonEscaped(ss, id);
			ss << ": {";
			ss << "\"defaultState\": ";
			WriteJsonEscaped(ss, controller.defaultState);
			if (!controller.notifyAssetPath.empty())
			{
				ss << ", \"notifyAsset\": ";
				WriteJsonEscaped(ss, controller.notifyAssetPath);
			}
			if (!controller.eventBindingsAssetPath.empty())
			{
				ss << ", \"eventBindingsAsset\": ";
				WriteJsonEscaped(ss, controller.eventBindingsAssetPath);
			}
			ss << ", \"parameters\": {";
			for (std::size_t pIndex = 0; pIndex < controller.parameters.size(); ++pIndex)
			{
				const AnimationParameterDesc& param = controller.parameters[pIndex];
				if (pIndex == 0) ss << "\n"; else ss << ",\n";
				ss << "      ";
				WriteJsonEscaped(ss, param.name);
				ss << ": {\"type\": ";
				WriteJsonEscaped(ss, AnimationParameterTypeToJsonString_(param.defaultValue.type));
				if (param.defaultValue.type != AnimationParameterType::Trigger ||
					param.defaultValue.triggerValue || param.defaultValue.boolValue)
				{
					ss << ", \"default\": ";
					WriteAnimationParameterLiteral_(ss, param.defaultValue);
				}
				ss << "}";
			}
			if (!controller.parameters.empty()) ss << "\n    ";
			ss << "}, \"states\": {";
			for (std::size_t sIndex = 0; sIndex < controller.states.size(); ++sIndex)
			{
				const AnimationStateDesc& state = controller.states[sIndex];
				if (sIndex == 0) ss << "\n"; else ss << ",\n";
				ss << "      ";
				WriteJsonEscaped(ss, state.name);
				ss << ": {";
				if (!state.blend1D.empty())
				{
					ss << "\"blend1D\": {\"parameter\": ";
					WriteJsonEscaped(ss, state.blendParameter);
					ss << ", \"points\": [";
					for (std::size_t pointIndex = 0; pointIndex < state.blend1D.size(); ++pointIndex)
					{
						const AnimationBlend1DPoint& point = state.blend1D[pointIndex];
						if (pointIndex == 0) ss << "\n"; else ss << ",\n";
						ss << "        {\"clip\": ";
						WriteJsonEscaped(ss, point.clipName);
						ss << ", \"value\": " << point.value << "}";
					}
					if (!state.blend1D.empty()) ss << "\n      ";
					ss << "]}";
				}
				else
				{
					ss << "\"clip\": ";
					WriteJsonEscaped(ss, state.clipName);
				}
				if (!state.clipSourceAssetId.empty())
				{
					ss << ", \"clipSourceAssetId\": ";
					WriteJsonEscaped(ss, state.clipSourceAssetId);
				}
				if (controller.notifyAssetPath.empty() && !state.notifies.empty())
				{
					ss << ", \"notifies\": [";
					for (std::size_t notifyIndex = 0; notifyIndex < state.notifies.size(); ++notifyIndex)
					{
						const AnimationNotifyDesc& notify = state.notifies[notifyIndex];
						if (notifyIndex == 0) ss << "\n"; else ss << ",\n";
						ss << "        {\"id\": ";
						WriteJsonEscaped(ss, notify.id);
						ss << ", \"time\": " << notify.timeNormalized;
						if (notify.fireOnEnter)
						{
							ss << ", \"fireOnEnter\": true";
						}
						ss << "}";
					}
					if (!state.notifies.empty()) ss << "\n      ";
					ss << "]";
				}
				if (!state.tags.empty())
				{
					ss << ", \"tags\": [";
					for (std::size_t tagIndex = 0; tagIndex < state.tags.size(); ++tagIndex)
					{
						if (tagIndex != 0) ss << ", ";
						WriteJsonEscaped(ss, state.tags[tagIndex]);
					}
					ss << "]";
				}
				if (!state.looping)
				{
					ss << ", \"loop\": false";
				}
				if (std::fabs(state.playRate - 1.0f) > 1e-6f)
				{
					ss << ", \"playRate\": " << state.playRate;
				}
				ss << "}";
			}
			if (!controller.states.empty()) ss << "\n    ";
			ss << "}, \"transitions\": [";
			for (std::size_t tIndex = 0; tIndex < controller.transitions.size(); ++tIndex)
			{
				const AnimationTransitionDesc& transition = controller.transitions[tIndex];
				if (tIndex == 0) ss << "\n"; else ss << ",\n";
				ss << "      {\"from\": ";
				WriteJsonEscaped(ss, transition.fromState);
				ss << ", \"to\": ";
				WriteJsonEscaped(ss, transition.toState);
				if (transition.hasExitTime)
				{
					ss << ", \"exitTime\": " << transition.exitTimeNormalized;
				}
				if (std::fabs(transition.blendDurationSeconds - 0.15f) > 1e-6f)
				{
					ss << ", \"blendDuration\": " << transition.blendDurationSeconds;
				}
				if (transition.priority != 0)
				{
					ss << ", \"priority\": " << transition.priority;
				}
				if (!transition.conditions.empty())
				{
					ss << ", \"conditions\": [";
					for (std::size_t cIndex = 0; cIndex < transition.conditions.size(); ++cIndex)
					{
						const AnimationConditionDesc& condition = transition.conditions[cIndex];
						if (cIndex == 0) ss << "\n"; else ss << ",\n";
						ss << "        {\"parameter\": ";
						WriteJsonEscaped(ss, condition.parameter);
						ss << ", \"op\": ";
						WriteJsonEscaped(ss, AnimationConditionOpToJsonString_(condition.op));
						if (condition.op != AnimationConditionOp::IfTrue &&
							condition.op != AnimationConditionOp::IfFalse &&
							condition.op != AnimationConditionOp::Triggered)
						{
							ss << ", \"value\": ";
							WriteAnimationParameterLiteral_(ss, condition.value);
						}
						ss << "}";
					}
					if (!transition.conditions.empty()) ss << "\n      ";
					ss << "]";
				}
				ss << "}";
			}
			if (!controller.transitions.empty()) ss << "\n    ";
			ss << "]";
			if (controller.eventBindingsAssetPath.empty() && !controller.eventBindings.empty())
			{
				ss << ", \"eventBindings\": [";
				for (std::size_t bindingIndex = 0; bindingIndex < controller.eventBindings.size(); ++bindingIndex)
				{
					const AnimationEventBindingDesc& binding = controller.eventBindings[bindingIndex];
					if (bindingIndex == 0) ss << "\n"; else ss << ",\n";
					ss << "      {\"animationEvent\": ";
					WriteJsonEscaped(ss, binding.animationEventId);
					ss << ", \"gameplayEvent\": ";
					WriteJsonEscaped(ss, binding.gameplayEventId);
					ss << "}";
				}
				if (!controller.eventBindings.empty()) ss << "\n    ";
				ss << "]";
			}
			ss << "}";
		}
		if (!keys.empty()) ss << "\n  ";
	}
	ss << "},\n";

}

inline void WriteTexturesSection_(std::ostringstream& ss, const LevelAsset& level)
{
	// textures
	ss << "  \"textures\": {";
	{
		auto keys = SortedStringKeys(level.textures);
		for (std::size_t i = 0; i < keys.size(); ++i)
		{
			const auto& id = keys[i];
			const LevelTextureDef& td = level.textures.at(id);
			if (i == 0) ss << "\n"; else ss << ",\n";
			ss << "    ";
			WriteJsonEscaped(ss, id);
			ss << ": {";
			if (td.kind == LevelTextureKind::Tex2D)
			{
				ss << "\"kind\": \"tex2d\", \"path\": ";
				WriteJsonEscaped(ss, td.props.filePath);
				ss << ", \"srgb\": ";
				WriteJsonBool(ss, td.props.srgb);
				ss << ", \"mips\": ";
				WriteJsonBool(ss, td.props.generateMips);
				ss << ", \"flipY\": ";
				WriteJsonBool(ss, td.props.flipY);
				if (td.props.isNormalMap)
				{
					ss << ", \"normalMap\": true";
				}
			}
			else
			{
				ss << "\"kind\": \"cube\"";
				if (td.cubeSource == LevelCubeSource::Cross)
				{
					ss << ", \"source\": \"cross\", \"cross\": ";
					WriteJsonEscaped(ss, td.props.filePath);
				}
				else if (td.cubeSource == LevelCubeSource::AutoFaces)
				{
					ss << ", \"source\": \"auto\", \"baseOrDir\": ";
					WriteJsonEscaped(ss, td.baseOrDir);
					if (!td.preferBase.empty())
					{
						ss << ", \"preferBase\": ";
						WriteJsonEscaped(ss, td.preferBase);
					}
				}
				else // Faces
				{
					ss << ", \"source\": \"faces\", \"faces\": {";
					ss << "\"px\": ";
					WriteJsonEscaped(ss, td.facePaths[0]);
					ss << ", \"nx\": ";
					WriteJsonEscaped(ss, td.facePaths[1]);
					ss << ", \"py\": ";
					WriteJsonEscaped(ss, td.facePaths[2]);
					ss << ", \"ny\": ";
					WriteJsonEscaped(ss, td.facePaths[3]);
					ss << ", \"pz\": ";
					WriteJsonEscaped(ss, td.facePaths[4]);
					ss << ", \"nz\": ";
					WriteJsonEscaped(ss, td.facePaths[5]);
					ss << "}";
				}
				ss << ", \"srgb\": ";
				WriteJsonBool(ss, td.props.srgb);
				ss << ", \"mips\": ";
				WriteJsonBool(ss, td.props.generateMips);
				ss << ", \"flipY\": ";
				WriteJsonBool(ss, td.props.flipY);
				if (td.props.isNormalMap)
				{
					ss << ", \"normalMap\": true";
				}
			}
			ss << "}";
		}
		if (!keys.empty()) ss << "\n  ";
	}
	ss << "},\n";

}

inline void WriteMaterialsSection_(std::ostringstream& ss, const LevelAsset& level)
{
	// materials
	ss << "  \"materials\": {";
	{
		auto keys = SortedStringKeys(level.materials);
		for (std::size_t i = 0; i < keys.size(); ++i)
		{
			const auto& id = keys[i];
			const LevelMaterialDef& md = level.materials.at(id);
			const MaterialParams& p = md.material.params;

			if (i == 0) ss << "\n"; else ss << ",\n";
			ss << "    ";
			WriteJsonEscaped(ss, id);
			ss << ": {";
			ss << "\"baseColor\": ";
			WriteJsonVec4(ss, p.baseColor);
			ss << ", \"shininess\": ";
			WriteJsonFloat(ss, p.shininess);
			ss << ", \"specStrength\": ";
			WriteJsonFloat(ss, p.specStrength);
			ss << ", \"shadowBias\": ";
			WriteJsonFloat(ss, p.shadowBias);
			ss << ", \"metallic\": ";
			WriteJsonFloat(ss, p.metallic);
			ss << ", \"roughness\": ";
			WriteJsonFloat(ss, p.roughness);
			ss << ", \"ao\": ";
			WriteJsonFloat(ss, p.ao);
			ss << ", \"emissiveStrength\": ";
			WriteJsonFloat(ss, p.emissiveStrength);
			ss << ", \"heightScale\": ";
			WriteJsonFloat(ss, p.heightScale);
			if (md.material.envSource == EnvSource::ReflectionCapture)
			{
				ss << ", \"envSource\": \"reflectionCapture\"";
			}

			// flags as array (parser expects array)
			ss << ", \"flags\": [";
			bool firstFlag = true;
			auto emitFlag = [&](std::string_view f)
				{
					if (!firstFlag) ss << ", ";
					WriteJsonEscaped(ss, f);
					firstFlag = false;
				};
			if (HasFlag(md.material.permFlags, MaterialPerm::UseTex)) emitFlag("useTex");
			if (HasFlag(md.material.permFlags, MaterialPerm::UseShadow)) emitFlag("useShadow");
			if (HasFlag(md.material.permFlags, MaterialPerm::Skinning)) emitFlag("skinning");
			if (HasFlag(md.material.permFlags, MaterialPerm::Transparent)) emitFlag("transparent");
			if (HasFlag(md.material.permFlags, MaterialPerm::PlanarMirror)) emitFlag("planarMirror");
			ss << "]";

			// texture bindings
			ss << ", \"textures\": {";
			{
				auto tkeys = SortedStringKeys(md.textureBindings);
				for (std::size_t ti = 0; ti < tkeys.size(); ++ti)
				{
					const auto& slot = tkeys[ti];
					const std::string& texId = md.textureBindings.at(slot);
					if (ti == 0) ss << "\n"; else ss << ",\n";
					ss << "      ";
					WriteJsonEscaped(ss, slot);
					ss << ": ";
					WriteJsonEscaped(ss, texId);
				}
				if (!tkeys.empty()) ss << "\n    ";
			}
			ss << "}";

			ss << "}";
		}
		if (!keys.empty()) ss << "\n  ";
	}
	ss << "},\n";

}
