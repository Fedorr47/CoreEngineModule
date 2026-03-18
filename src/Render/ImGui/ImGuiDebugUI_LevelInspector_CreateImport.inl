    static std::string MakeUniqueParticleEmitterName(const rendern::LevelAsset& level, std::string_view base)
    {
        auto NameExists = [&](std::string_view candidate) noexcept
            {
                for (const rendern::ParticleEmitter& emitter : level.particleEmitters)
                {
                    if (emitter.name == candidate)
                    {
                        return true;
                    }
                }
                return false;
            };

        std::string result = std::string(base.empty() ? std::string_view("ParticleEmitter") : base);
        if (!NameExists(result))
        {
            return result;
        }

        for (int suffix = 2; suffix <= 9999; ++suffix)
        {
            std::string candidate = result + std::to_string(suffix);
            if (!NameExists(candidate))
            {
                return candidate;
            }
        }

        return result + "_Copy";
    }

    static void EnsureDemoSmokeTexture(rendern::LevelAsset& level, AssetManager& assets)
    {
        auto it = level.textures.find(kDemoSmokeTextureId);
        if (it == level.textures.end())
        {
            rendern::LevelTextureDef def{};
            def.kind = rendern::LevelTextureKind::Tex2D;
            def.props.dimension = TextureDimension::Tex2D;
            def.props.filePath = kDemoSmokeTexturePath;
            def.props.srgb = true;
            def.props.generateMips = true;
            def.props.flipY = false;
            it = level.textures.emplace(kDemoSmokeTextureId, std::move(def)).first;
        }

        TextureProperties props = it->second.props;
        if (props.filePath.empty())
        {
            props.dimension = TextureDimension::Tex2D;
            props.filePath = kDemoSmokeTexturePath;
            props.srgb = true;
            props.generateMips = true;
            props.flipY = false;
            it->second.props = props;
        }

        assets.LoadTextureAsync(kDemoSmokeTextureId, std::move(props));
    }

    static rendern::ParticleEmitter MakeDemoSmokeEmitter(const rendern::LevelAsset& level, const rendern::Scene& scene, const rendern::CameraController& camCtl)
    {
        rendern::ParticleEmitter emitter{};
        emitter.name = MakeUniqueParticleEmitterName(level, "SmokeSoft");
        emitter.textureId = kDemoSmokeTextureId;
        emitter.position = ComputeSpawnTransform(scene, camCtl).position;
        emitter.positionJitter = mathUtils::Vec3(0.18f, 0.04f, 0.18f);
        emitter.velocityMin = mathUtils::Vec3(-0.08f, 0.28f, -0.08f);
        emitter.velocityMax = mathUtils::Vec3(0.08f, 0.62f, 0.08f);
        emitter.colorBegin = mathUtils::Vec4(0.95f, 0.95f, 0.95f, 0.55f);
        emitter.colorEnd = mathUtils::Vec4(0.55f, 0.58f, 0.60f, 0.0f);
        emitter.sizeMin = 0.28f;
        emitter.sizeMax = 0.42f;
        emitter.sizeBegin = 0.24f;
        emitter.sizeEnd = 0.95f;
        emitter.lifetimeMin = 1.2f;
        emitter.lifetimeMax = 2.1f;
        emitter.spawnRate = 18.0f;
        emitter.burstCount = 8u;
        emitter.maxParticles = 256u;
        emitter.looping = true;
        emitter.duration = 0.0f;
        emitter.startDelay = 0.0f;
        return emitter;
    }

    static void DrawCreateImportSection(
        rendern::LevelAsset& level,
        rendern::LevelInstance& levelInst,
        AssetManager& assets,
        rendern::Scene& scene,
        rendern::CameraController& camCtl,
        LevelEditorUIState& st)
    {
        ImGui::Text("Create / Import");
        ImGui::Checkbox("Add as child of selected", &st.addAsChildOfSelection);

        const int parentForNew = ParentForNewNode(level, st);

        if (ImGui::Button("Add Cube"))
        {
            EnsureDefaultMesh(level, "cube", "models/cube.obj");
            const int newIdx = levelInst.AddNode(level, scene, assets, "cube", "", parentForNew, ComputeSpawnTransform(scene, camCtl), "Cube");
            st.selectedNode = newIdx;
            st.selectedParticleEmitter = -1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Quad"))
        {
            EnsureDefaultMesh(level, "quad", "models/quad.obj");
            rendern::Transform t = ComputeSpawnTransform(scene, camCtl);
            t.scale = mathUtils::Vec3(3.0f, 1.0f, 3.0f);
            const int newIdx = levelInst.AddNode(level, scene, assets, "quad", "", parentForNew, t, "Quad");
            st.selectedNode = newIdx;
            st.selectedParticleEmitter = -1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Planar Mirror"))
        {
            EnsureDefaultMesh(level, "quad", "models/quad.obj");

            const std::string matId = "planar_mirror";
            if (!level.materials.contains(matId))
            {
                rendern::LevelMaterialDef def{};
                def.material.permFlags = rendern::MaterialPerm::PlanarMirror;
                def.material.params.baseColor = mathUtils::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
                def.material.params.metallic = 1.0f;
                def.material.params.roughness = 0.02f;
                level.materials.emplace(matId, std::move(def));
            }

            levelInst.EnsureMaterial(level, scene, matId);

            rendern::Transform t = ComputeSpawnTransform(scene, camCtl);
            t.rotationDegrees.x = 90.0f;
            t.scale = mathUtils::Vec3(3.0f, 1.0f, 3.0f);
            const int newIdx = levelInst.AddNode(level, scene, assets, "quad", matId, parentForNew, t, "PlanarMirror");
            st.selectedNode = newIdx;
            st.selectedParticleEmitter = -1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Empty"))
        {
            const int newIdx = levelInst.AddNode(level, scene, assets, "", "", parentForNew, ComputeSpawnTransform(scene, camCtl), "Empty");
            st.selectedNode = newIdx;
            st.selectedParticleEmitter = -1;
        }

        if (ImGui::Button("Add Particle Emitter"))
        {
            rendern::ParticleEmitter emitter{};
            emitter.name = MakeUniqueParticleEmitterName(level, "ParticleEmitter");
            emitter.position = ComputeSpawnTransform(scene, camCtl).position;
            const int newIdx = levelInst.AddParticleEmitter(level, scene, emitter);
            st.selectedNode = -1;
            st.selectedParticleEmitter = newIdx;
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Demo Smoke Emitter"))
        {
            EnsureDemoSmokeTexture(level, assets);
            const int newIdx = levelInst.AddParticleEmitter(level, scene, MakeDemoSmokeEmitter(level, scene, camCtl));
            st.selectedNode = -1;
            st.selectedParticleEmitter = newIdx;
        }
        ImGui::TextDisabled("Demo texture: %s", kDemoSmokeTexturePath);

        ImGui::Spacing();
        ImGui::InputText("OBJ path", st.importPathBuf, sizeof(st.importPathBuf));
        ImGui::InputText("Asset id (optional)", st.importAssetIdBuf, sizeof(st.importAssetIdBuf));
        ImGui::Checkbox("Flip UVs on import", &st.importFlipUVs);

        const auto makeImportBaseId = [&](const char* fallback) -> std::string
            {
                std::string base = std::string(st.importAssetIdBuf);
                if (base.empty())
                {
                    base = std::filesystem::path(std::string(st.importPathBuf)).stem().string();
                }
                if (base.empty())
                {
                    base = fallback;
                }
                return base;
            };

        if (ImGui::Button("Import mesh into library"))
        {
            const std::string pathStr = std::string(st.importPathBuf);
            if (!pathStr.empty())
            {
                const std::string meshId = MakeUniqueMeshId(level, makeImportBaseId("mesh"));

                rendern::LevelMeshDef def{};
                def.path = pathStr;
                def.debugName = meshId;
                def.flipUVs = st.importFlipUVs;
                level.meshes.emplace(meshId, std::move(def));

                try
                {
                    rendern::MeshProperties p{};
                    p.filePath = pathStr;
                    p.debugName = meshId;
                    p.flipUVs = st.importFlipUVs;
                    assets.LoadMeshAsync(meshId, std::move(p));
                }
                catch (...)
                {
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Create object from path"))
        {
            const std::string pathStr = std::string(st.importPathBuf);
            if (!pathStr.empty())
            {
                const std::string base = makeImportBaseId("mesh");
                const std::string meshId = level.meshes.contains(base) ? base : MakeUniqueMeshId(level, base);

                if (!level.meshes.contains(meshId))
                {
                    rendern::LevelMeshDef def{};
                    def.path = pathStr;
                    def.debugName = meshId;
                    def.flipUVs = st.importFlipUVs;
                    level.meshes.emplace(meshId, std::move(def));
                }

                const int newIdx = levelInst.AddNode(level, scene, assets, meshId, "", parentForNew, ComputeSpawnTransform(scene, camCtl), meshId);
                st.selectedNode = newIdx;
                st.selectedParticleEmitter = -1;
            }
        }

        ImGui::Spacing();
        ImGui::SeparatorText("Skinned Import");
        if (ImGui::Button("Import skinned mesh into library"))
        {
            const std::string pathStr = std::string(st.importPathBuf);
            if (!pathStr.empty())
            {
                const std::string skinnedId = MakeUniqueSkinnedMeshId(level, makeImportBaseId("skinned"));
                rendern::LevelSkinnedMeshDef def{};
                def.path = pathStr;
                def.debugName = skinnedId;
                def.flipUVs = st.importFlipUVs;
                level.skinnedMeshes.emplace(skinnedId, std::move(def));
                levelInst.ImportSkinnedMaterials(level, scene, assets, skinnedId, pathStr, st.importFlipUVs);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Create skinned object"))
        {
            const std::string pathStr = std::string(st.importPathBuf);
            if (!pathStr.empty())
            {
                const std::string base = makeImportBaseId("skinned");
                const std::string skinnedId = level.skinnedMeshes.contains(base) ? base : MakeUniqueSkinnedMeshId(level, base);
                if (!level.skinnedMeshes.contains(skinnedId))
                {
                    rendern::LevelSkinnedMeshDef def{};
                    def.path = pathStr;
                    def.debugName = skinnedId;
                    def.flipUVs = st.importFlipUVs;
                    level.skinnedMeshes.emplace(skinnedId, std::move(def));
                }

                const std::string defaultMaterialId = levelInst.ImportSkinnedMaterials(level, scene, assets, skinnedId, pathStr, st.importFlipUVs);
                const int newIdx = levelInst.AddNode(level, scene, assets, "", defaultMaterialId, parentForNew, ComputeSpawnTransform(scene, camCtl), skinnedId);
                levelInst.SetNodeSkinnedMesh(level, scene, assets, newIdx, skinnedId);
                st.selectedNode = newIdx;
                st.selectedParticleEmitter = -1;
            }
        }

        ImGui::Spacing();
        ImGui::SeparatorText("Model / Scene Import");
        ImGui::Checkbox("Import skeleton/bone nodes (scene debug only)", &st.importSceneSkeletonNodes);

        if (ImGui::Button("Import model into library"))
        {
            const std::string pathStr = std::string(st.importPathBuf);
            if (!pathStr.empty())
            {
                const std::string modelId = MakeUniqueModelId(level, makeImportBaseId("model"));
                rendern::LevelModelDef def{};
                def.path = pathStr;
                def.debugName = modelId;
                def.flipUVs = st.importFlipUVs;
                level.models.emplace(modelId, std::move(def));
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Create model object"))
        {
            const std::string pathStr = std::string(st.importPathBuf);
            if (!pathStr.empty())
            {
                const std::string base = makeImportBaseId("model");
                const std::string modelId = level.models.contains(base) ? base : MakeUniqueModelId(level, base);
                if (!level.models.contains(modelId))
                {
                    rendern::LevelModelDef def{};
                    def.path = pathStr;
                    def.debugName = modelId;
                    def.flipUVs = st.importFlipUVs;
                    level.models.emplace(modelId, std::move(def));
                }

                const int newIdx = levelInst.AddNode(level, scene, assets, "", "", parentForNew, ComputeSpawnTransform(scene, camCtl), modelId);
                levelInst.SetNodeModel(level, scene, assets, newIdx, modelId);
                st.selectedNode = newIdx;
                st.selectedParticleEmitter = -1;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Import model scene as nodes"))
        {
            const std::string pathStr = std::string(st.importPathBuf);
            if (!pathStr.empty())
            {
                const std::string base = makeImportBaseId("model");
                const std::string modelId = level.models.contains(base) ? base : MakeUniqueModelId(level, base);
                if (!level.models.contains(modelId))
                {
                    rendern::LevelModelDef def{};
                    def.path = pathStr;
                    def.debugName = modelId;
                    def.flipUVs = st.importFlipUVs;
                    level.models.emplace(modelId, std::move(def));
                }

                try
                {
                    const int firstIdx = levelInst.ImportModelSceneAsNodes(
                        level,
                        scene,
                        assets,
                        modelId,
                        parentForNew,
                        st.importSceneCreateMaterialPlaceholders,
                        st.importSceneSkeletonNodes,
                        true);
                    st.selectedNode = firstIdx;
                    st.selectedParticleEmitter = -1;
                }
                catch (...)
                {
                }
            }
        }
    }

