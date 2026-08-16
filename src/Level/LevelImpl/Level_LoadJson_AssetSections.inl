inline void ParseMeshSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- meshes ---
	if (auto* meshesV = TryGet(jsonObject, "meshes"))
	{
		const JsonObject& meshesO = meshesV->AsObject();
		for (const auto& [id, defV] : meshesO)
		{
			const JsonObject& md = defV.AsObject();
			LevelMeshDef def;
			def.path = GetStringOpt(md, "path");
			def.debugName = GetStringOpt(md, "debugName");
			def.flipUVs = GetBoolOpt(md, "flipUVs", true);
			def.bakeNodeTransforms = GetBoolOpt(md, "bakeNodeTransforms", true);
			if (auto* submeshV = TryGet(md, "submeshIndex"))
			{
				if (!submeshV->IsNumber())
				{
					throw std::runtime_error("Level JSON: meshes." + id + ".submeshIndex must be number");
				}
				def.submeshIndex = static_cast<std::uint32_t>(submeshV->AsNumber());
			}
			if (def.path.empty())
			{
				throw std::runtime_error("Level JSON: meshes." + id + ".path is required");
			}
			out.meshes.emplace(id, std::move(def));
		}
	}

}

inline void ParseModelSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- models ---
	if (auto* modelsV = TryGet(jsonObject, "models"))
	{
		const JsonObject& modelsO = modelsV->AsObject();
		for (const auto& [id, defV] : modelsO)
		{
			const JsonObject& md = defV.AsObject();
			LevelModelDef def;
			def.path = GetStringOpt(md, "path");
			def.debugName = GetStringOpt(md, "debugName");
			def.flipUVs = GetBoolOpt(md, "flipUVs", true);
			if (def.path.empty())
			{
				throw std::runtime_error("Level JSON: models." + id + ".path is required");
			}
			out.models.emplace(id, std::move(def));
		}
	}

}

inline void ParseTextureSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- textures ---
	if (auto* texV = TryGet(jsonObject, "textures"))
	{
		const JsonObject& texO = texV->AsObject();
		for (const auto& [id, defV] : texO)
		{
			const JsonObject& td = defV.AsObject();
			LevelTextureDef def;

			const std::string kind = GetStringOpt(td, "kind", "tex2d");
			if (kind == "tex2d")
			{
				def.kind = LevelTextureKind::Tex2D;
				def.props.dimension = TextureDimension::Tex2D;
				def.props.filePath = GetStringOpt(td, "path");
				if (def.props.filePath.empty())
				{
					throw std::runtime_error("Level JSON: textures." + id + ".path is required for tex2d");
				}
			}
			else if (kind == "cube")
			{
				def.kind = LevelTextureKind::Cube;
				def.props.dimension = TextureDimension::Cube;

				const std::string source = GetStringOpt(td, "source", "cross");
				if (source == "cross")
				{
					def.cubeSource = LevelCubeSource::Cross;
					def.props.cubeFromCross = true;
					def.props.filePath = GetStringOpt(td, "cross");
					if (def.props.filePath.empty())
					{
						throw std::runtime_error("Level JSON: textures." + id + ".cross is required for cube/cross");
					}
				}
				else if (source == "auto")
				{
					def.cubeSource = LevelCubeSource::AutoFaces;
					def.baseOrDir = GetStringOpt(td, "baseOrDir");
					def.preferBase = GetStringOpt(td, "preferBase");
					if (def.baseOrDir.empty())
					{
						throw std::runtime_error("Level JSON: textures." + id + ".baseOrDir is required for cube/auto");
					}
				}
				else if (source == "faces")
				{
					def.cubeSource = LevelCubeSource::Faces;
					const JsonObject& facesO = GetReq(td, "faces").AsObject();
					def.facePaths[0] = GetStringOpt(facesO, "px");
					def.facePaths[1] = GetStringOpt(facesO, "nx");
					def.facePaths[2] = GetStringOpt(facesO, "py");
					def.facePaths[3] = GetStringOpt(facesO, "ny");
					def.facePaths[4] = GetStringOpt(facesO, "pz");
					def.facePaths[5] = GetStringOpt(facesO, "nz");

					for (const auto& p : def.facePaths)
					{
						if (p.empty())
						{
							throw std::runtime_error("Level JSON: textures." + id + ".faces must define px/nx/py/ny/pz/nz");
						}
					}
				}
				else
				{
					throw std::runtime_error("Level JSON: textures." + id + ".source must be cross|auto|faces");
				}
			}
			else
			{
				throw std::runtime_error("Level JSON: textures." + id + ".kind must be tex2d|cube");
			}

			// Common props
			def.props.srgb = GetBoolOpt(td, "srgb", true);
			def.props.generateMips = GetBoolOpt(td, "mips", true);
			def.props.flipY = GetBoolOpt(td, "flipY", false);
			def.props.isNormalMap = GetBoolOpt(td, "normalMap", GetBoolOpt(td, "isNormalMap", false));

			out.textures.emplace(id, std::move(def));
		}
	}

}

inline void ParseAnimationSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- animations ---
	if (auto* animationsV = TryGet(jsonObject, "animations"))
	{
		const JsonObject& animationsO = animationsV->AsObject();
		for (const auto& [id, defV] : animationsO)
		{
			const JsonObject& md = defV.AsObject();
			LevelAnimationDef def;
			def.path = GetStringOpt(md, "path");
			def.debugName = GetStringOpt(md, "debugName");
			def.flipUVs = GetBoolOpt(md, "flipUVs", true);
			if (def.path.empty())
			{
				throw std::runtime_error("Level JSON: animations." + id + ".path is required");
			}
			out.animations.emplace(id, std::move(def));
		}
	}

}

inline void ParseExternalAnimationControllerAssetSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- external animationControllerAssets ---
	if (auto* controllerAssetsV = TryGet(jsonObject, "animationControllerAssets"))
	{
		const JsonObject& controllerAssetsO = controllerAssetsV->AsObject();
		for (const auto& [id, defV] : controllerAssetsO)
		{
			const JsonObject& ad = defV.AsObject();
			const std::string path = GetStringOpt(ad, "path");
			if (path.empty())
			{
				throw std::runtime_error("Level JSON: animationControllerAssets." + id + ".path is required");
			}
			out.animationControllerAssetPaths[id] = path;
			out.animationControllers.insert_or_assign(id, LoadExternalAnimationControllerAssetFromJson_(path, id));
		}
	}

}

inline void ParseAnimationControllerSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- animationControllers ---
	if (auto* controllersV = TryGet(jsonObject, "animationControllers"))
	{
		const JsonObject& controllersO = controllersV->AsObject();
		for (const auto& [id, defV] : controllersO)
		{
			const JsonObject& cd = defV.AsObject();
			out.animationControllers.insert_or_assign(
				id,
				ParseAnimationControllerAssetObject_(cd, id, "Level JSON: animationControllers." + id));
		}
	}

}

inline void ParseAnimationProfileAssetSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	if (auto* assetsValue = TryGet(jsonObject, "animationProfileAssets"))
	{
		for (const auto& [id, value] : assetsValue->AsObject())
		{
			const std::string path = GetStringOpt(value.AsObject(), "path");
			if (path.empty())
			{
				throw std::runtime_error("Level JSON: animationProfileAssets." + id + ".path is required");
			}
			out.animationProfileAssetPaths[id] = path;
			out.animationProfiles.insert_or_assign(id, LoadExternalAnimationProfileAssetFromJson_(path, id));
		}
	}
	if (auto* profilesValue = TryGet(jsonObject, "animationProfiles"))
	{
		for (const auto& [id, value] : profilesValue->AsObject())
		{
			out.animationProfiles.insert_or_assign(id,
				ParseAnimationProfileAssetObject_(value.AsObject(), id, "Level JSON: animationProfiles." + id));
		}
	}
}

inline void ParseSkinnedMeshSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- skinnedMeshes ---
	if (auto* skinnedV = TryGet(jsonObject, "skinnedMeshes"))
	{
		const JsonObject& skinnedO = skinnedV->AsObject();
		for (const auto& [id, defV] : skinnedO)
		{
			const JsonObject& md = defV.AsObject();
			LevelSkinnedMeshDef def;
			def.path = GetStringOpt(md, "path");
			def.debugName = GetStringOpt(md, "debugName");
			def.flipUVs = GetBoolOpt(md, "flipUVs", true);
			if (auto* smi = TryGet(md, "submeshIndex"))
			{
				if (!smi->IsNumber())
				{
					throw std::runtime_error("Level JSON: skinnedMeshes." + id + ".submeshIndex must be number");
				}
				def.submeshIndex = static_cast<std::uint32_t>(smi->AsNumber());
			}
			if (def.path.empty())
			{
				throw std::runtime_error("Level JSON: skinnedMeshes." + id + ".path is required");
			}
			out.skinnedMeshes.emplace(id, std::move(def));
		}
	}


}

inline void ParseMaterialSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- materials ---
	if (auto* matsV = TryGet(jsonObject, "materials"))
	{
		const JsonObject& matsO = matsV->AsObject();
		for (const auto& [id, defV] : matsO)
		{
			const JsonObject& md = defV.AsObject();
			LevelMaterialDef def;

			if (auto* bc = TryGet(md, "baseColor"))
			{
				auto a = ReadFloatArray(*bc, 4, "baseColor");
				def.material.params.baseColor = { a[0], a[1], a[2], a[3] };
			}

			def.material.params.shininess = GetFloatOpt(md, "shininess", def.material.params.shininess);
			def.material.params.specStrength = GetFloatOpt(md, "specStrength", def.material.params.specStrength);
			def.material.params.shadowBias = GetFloatOpt(md, "shadowBias", def.material.params.shadowBias);

			def.material.params.metallic = GetFloatOpt(md, "metallic", def.material.params.metallic);
			def.material.params.roughness = GetFloatOpt(md, "roughness", def.material.params.roughness);
			def.material.params.ao = GetFloatOpt(md, "ao", def.material.params.ao);
			def.material.params.emissiveStrength = GetFloatOpt(md, "emissiveStrength", def.material.params.emissiveStrength);
			def.material.params.heightScale = GetFloatOpt(md, "heightScale", def.material.params.heightScale);

			if (auto* flagsV = TryGet(md, "flags"))
			{
				def.material.permFlags = ParsePermFlags(*flagsV);
			}

			if (auto* envV = TryGet(md, "envSource"))
			{
				def.material.envSource = ParseEnvSourceOrThrow(*envV, id);
			}
			else if (auto* envV2 = TryGet(md, "env"))
			{
				def.material.envSource = ParseEnvSourceOrThrow(*envV2, id);
			}

			if (auto* texBindV = TryGet(md, "textures"))
			{
				const JsonObject& tbo = texBindV->AsObject();
				for (const auto& [slot, tv] : tbo)
				{
					if (!tv.IsString())
					{
						throw std::runtime_error("Level JSON: materials." + id + ".textures values must be strings");
					}
					def.textureBindings.emplace(slot, tv.AsString());
				}
			}

			out.materials.emplace(id, std::move(def));
		}
	}

}

inline GameplayActionPolicyGroup ParseGameplayActionGroup_(const std::string& value)
{
	if (value == "Input") return GameplayActionPolicyGroup::Input;
	if (value == "Combat") return GameplayActionPolicyGroup::Combat;
	if (value == "Interaction") return GameplayActionPolicyGroup::Interaction;
	if (value == "Any") return GameplayActionPolicyGroup::Any;
	if (value == "None") return GameplayActionPolicyGroup::None;
	throw std::runtime_error("Level JSON: unknown gameplay action group '" + value + "'.");
}

inline GameplayActionRequestSource ParseGameplayActionSource_(const std::string& value)
{
	if (value == "Input") return GameplayActionRequestSource::Input;
	if (value == "Combat") return GameplayActionRequestSource::Combat;
	if (value == "Interaction") return GameplayActionRequestSource::Interaction;
	if (value == "AnimationEvent") return GameplayActionRequestSource::AnimationEvent;
	if (value == "Script") return GameplayActionRequestSource::Script;
	if (value == "None") return GameplayActionRequestSource::None;
	throw std::runtime_error("Level JSON: unknown gameplay action source '" + value + "'.");
}

inline GameplayActionExecutorKind ParseGameplayActionExecutor_(const std::string& value)
{
	if (value == "Jump") return GameplayActionExecutorKind::Jump;
	if (value == "CombatAttack") return GameplayActionExecutorKind::CombatAttack;
	if (value == "Interact") return GameplayActionExecutorKind::Interact;
	if (value == "None") return GameplayActionExecutorKind::None;
	throw std::runtime_error("Level JSON: unknown gameplay action executor '" + value + "'.");
}

inline std::uint32_t ParseGameplayActionGates_(const JsonObject& object)
{
	std::uint32_t gates = 0u;
	const JsonValue* gatesValue = TryGet(object, "gates");
	if (gatesValue == nullptr) return gates;
	for (const JsonValue& value : gatesValue->AsArray())
	{
		const std::string& gate = value.AsString();
		if (gate == "RequireGrounded") gates |= GameplayActionPolicyGateMask(GameplayActionPolicyGate::RequireGrounded);
		else if (gate == "RequireAirborne") gates |= GameplayActionPolicyGateMask(GameplayActionPolicyGate::RequireAirborne);
		else if (gate == "RequireNotBusy") gates |= GameplayActionPolicyGateMask(GameplayActionPolicyGate::RequireNotBusy);
		else if (gate == "RequireBusy") gates |= GameplayActionPolicyGateMask(GameplayActionPolicyGate::RequireBusy);
		else if (gate == "RequireNoPending") gates |= GameplayActionPolicyGateMask(GameplayActionPolicyGate::RequireNoPending);
		else if (gate == "RequireNoBuffered") gates |= GameplayActionPolicyGateMask(GameplayActionPolicyGate::RequireNoBuffered);
		else throw std::runtime_error("Level JSON: unknown gameplay action gate '" + gate + "'.");
	}
	return gates;
}

inline void ParseGameplayActionsSection_(LevelAsset& out, const JsonObject& object)
{
	if (const JsonValue* actions = TryGet(object, "gameplayActions"))
	{
		for (const JsonValue& value : actions->AsArray())
		{
			const JsonObject& action = value.AsObject();
			out.gameplayActions.push_back({
				GameplayActionId{ GetStringOpt(action, "id") },
				ParseGameplayActionGroup_(GetStringOpt(action, "group", "None")),
				ParseGameplayActionSource_(GetStringOpt(action, "source", "None")),
				ParseGameplayActionExecutor_(GetStringOpt(action, "executor", "None")),
				GetIntOpt(action, "priority", 0), ParseGameplayActionGates_(action) });
		}
	}
	if (const JsonValue* bindings = TryGet(object, "gameplayActionAnimationBindings"))
	{
		for (const JsonValue& value : bindings->AsArray())
		{
			const JsonObject& binding = value.AsObject();
			out.gameplayActionAnimationBindings.push_back({
				GameplayActionId{ GetStringOpt(binding, "actionId") },
				GetStringOpt(binding, "triggerParameter") });
		}
	}
	if (out.gameplayActions.empty()) out.gameplayActions = MakeDefaultGameplayActionDefinitions();
	if (out.gameplayActionAnimationBindings.empty())
		out.gameplayActionAnimationBindings = MakeDefaultGameplayActionAnimationBindings();
	std::string diagnostic;
	if (!ValidateGameplayActionDefinitions(out.gameplayActions, diagnostic) ||
		!ValidateGameplayActionAnimationBindings(out.gameplayActions, out.gameplayActionAnimationBindings, diagnostic))
		throw std::runtime_error("Level JSON: " + diagnostic);
}

inline int ParseGameplayInputKey_(const std::string& value)
{
	if (value.size() == 1 && ((value[0] >= 'A' && value[0] <= 'Z') ||
		(value[0] >= '0' && value[0] <= '9'))) return value[0];
	if (value == "Space") return 0x20;
	if (value == "Shift") return 0x10;
	if (value == "Control") return 0x11;
	if (value == "Enter") return 0x0D;
	if (value == "Escape") return 0x1B;
	if (value == "Tab") return 0x09;
	if (value == "Delete") return 0x2E;
	if (value == "MouseLeft") return kGameplayMouseLeft;
	if (value == "MouseRight") return kGameplayMouseRight;
	if (value == "MouseMiddle") return kGameplayMouseMiddle;
	if (value == "MouseX1") return kGameplayMouseX1;
	if (value == "MouseX2") return kGameplayMouseX2;
	if (value.size() >= 2 && value[0] == 'F')
	{
		int number = 0;
		for (std::size_t i = 1; i < value.size(); ++i)
		{
			if (value[i] < '0' || value[i] > '9')
				throw std::runtime_error("Level JSON: unknown gameplay input identifier '" + value + "'.");
			number = number * 10 + (value[i] - '0');
		}
		if (number >= 1 && number <= 12 && value == "F" + std::to_string(number)) return 0x6F + number;
	}
	throw std::runtime_error("Level JSON: unknown gameplay input identifier '" + value + "'.");
}

inline void ParseGameplayInputSection_(LevelAsset& out, const JsonObject& object)
{
	const JsonValue* sectionValue = TryGet(object, "gameplayKeyboardMouseBindings");
	if (sectionValue == nullptr) return; // Default member value preserves old levels.
	const JsonObject& section = sectionValue->AsObject();
	const JsonObject& moveX = section.at("moveX").AsObject();
	const JsonObject& moveY = section.at("moveY").AsObject();
	out.gameplayKeyboardMouseBindings.moveX = {
		ParseGameplayInputKey_(GetStringOpt(moveX, "negative")),
		ParseGameplayInputKey_(GetStringOpt(moveX, "positive")) };
	out.gameplayKeyboardMouseBindings.moveY = {
		ParseGameplayInputKey_(GetStringOpt(moveY, "negative")),
		ParseGameplayInputKey_(GetStringOpt(moveY, "positive")) };
	out.gameplayKeyboardMouseBindings.run.key = ParseGameplayInputKey_(GetStringOpt(section, "run"));
	const auto validateKeyboardBinding = [](const int key, const std::string_view field)
	{
		if (!IsSupportedGameplayKeyboardKey(key))
			throw std::runtime_error("Level JSON: gameplay input '" + std::string(field) +
				"' must be a supported keyboard key.");
		if (IsGameplayActionBindingKeyReserved(key))
			throw std::runtime_error("Level JSON: gameplay input '" + std::string(field) +
				"' uses a reserved application hotkey.");
	};
	validateKeyboardBinding(out.gameplayKeyboardMouseBindings.moveX.negativeKey, "moveX.negative");
	validateKeyboardBinding(out.gameplayKeyboardMouseBindings.moveX.positiveKey, "moveX.positive");
	validateKeyboardBinding(out.gameplayKeyboardMouseBindings.moveY.negativeKey, "moveY.negative");
	validateKeyboardBinding(out.gameplayKeyboardMouseBindings.moveY.positiveKey, "moveY.positive");
	validateKeyboardBinding(out.gameplayKeyboardMouseBindings.run.key, "run");
	out.gameplayKeyboardMouseBindings.actions.clear();
	for (const JsonValue& value : section.at("actions").AsArray())
	{
		const JsonObject& binding = value.AsObject();
		out.gameplayKeyboardMouseBindings.actions.push_back({
			ParseGameplayInputKey_(GetStringOpt(binding, "input")),
			GameplayActionId{ GetStringOpt(binding, "actionId") } });
	}
	std::string diagnostic;
	for (std::size_t i = 0; i < out.gameplayKeyboardMouseBindings.actions.size(); ++i)
	{
		const auto& binding = out.gameplayKeyboardMouseBindings.actions[i];
		if (IsGameplayActionBindingKeyReserved(binding.key))
			throw std::runtime_error("Level JSON: gameplay input uses a reserved application input.");
		if (!ValidateGameplayInputAction(out.gameplayActions, binding.action, diagnostic))
			throw std::runtime_error("Level JSON: " + diagnostic);
		for (std::size_t j = i + 1; j < out.gameplayKeyboardMouseBindings.actions.size(); ++j)
			if (binding.key == out.gameplayKeyboardMouseBindings.actions[j].key)
				throw std::runtime_error("Level JSON: duplicate gameplay input binding '" +
					GetStringOpt(section.at("actions").AsArray()[i].AsObject(), "input") + "'.");
	}
}
