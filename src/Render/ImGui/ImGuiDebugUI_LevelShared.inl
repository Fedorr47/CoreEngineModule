namespace rendern::ui::level_ui_detail
{
    enum class SceneHierarchyItemKind
    {
        None,
        SceneNode,
        ParticleEmitter,
        Light
    };

    enum class SceneHierarchyNodeSourceKind
    {
        Empty,
        Mesh,
        Model,
        SkinnedMesh
    };

    enum class SceneHierarchyLightKind
    {
        Unknown,
        Directional,
        Point,
        Spot
    };

    struct SceneHierarchyItemId
    {
        SceneHierarchyItemKind kind{ SceneHierarchyItemKind::None };
        int index{ -1 };

        [[nodiscard]] bool IsValid() const noexcept
        {
            return kind != SceneHierarchyItemKind::None && index >= 0;
        }

        [[nodiscard]] friend bool operator==(
            const SceneHierarchyItemId& lhs,
            const SceneHierarchyItemId& rhs) noexcept = default;
    };

    struct SceneHierarchyItemTypeFlags
    {
        SceneHierarchyNodeSourceKind nodeSource{ SceneHierarchyNodeSourceKind::Empty };
        SceneHierarchyLightKind lightKind{ SceneHierarchyLightKind::Unknown };
    };
    
    enum class SceneHierarchySelectionIntentMode
    {
        Replace,
        Toggle
    };

    struct SceneHierarchySelectionIntent
    {
        SceneHierarchyItemId itemId{};
        SceneHierarchySelectionIntentMode mode{ SceneHierarchySelectionIntentMode::Replace };
    };

    struct SceneHierarchyItemViewModel
    {
        // Stable only for this editor snapshot/current level arrays; not a persistent asset id.
        SceneHierarchyItemId id{};
        SceneHierarchyItemId parentId{};
        std::string displayName;
        SceneHierarchyItemTypeFlags typeFlags{};
        bool isSelected{ false };
        bool isVisibleOrEnabled{ true };
        // Copied display data only; childItemIndices is the traversal source.
        bool hasChildren{ false };
        std::vector<int> childItemIndices;
    };

    struct SceneHierarchyViewModel
    {
        // Per-frame ImGui/editor snapshot: copied UI data only, no Scene/Level/runtime ownership.
        // This does not define render visibility, thread-safety, or runtime/render snapshot lifetime.
        std::vector<SceneHierarchyItemViewModel> items;
        std::vector<int> sceneRootItemIndices;
        std::vector<int> particleEmitterItemIndices;
        std::vector<int> lightItemIndices;
    };
    
    struct LevelEditorUIState
    {
        rendern::EditorSelectionService selection;
        int selectedNode = -1;
        int prevSelectedNode = -2;
        int selectedParticleEmitter = -1;
        int prevSelectedParticleEmitter = -2;
        bool addAsChildOfSelection = false;
        bool importFlipUVs = true;
        bool importSceneCreateMaterialPlaceholders = true;
        bool importSceneSkeletonNodes = false;

        char nameBuf[128]{};
        char importPathBuf[512]{};
        char importAssetIdBuf[128]{};
        char savePathBuf[512]{};
        char saveStatusBuf[512]{};
        std::string cachedSourcePath;
        bool saveStatusIsError = false;

        bool animationGraphWindowOpen = true;
        bool animationGraphRequestFocus = false;
        bool animationGraphFsmFocusSelection = true;
        bool animationGraphShowTransitionLabels = false;
        std::string animationGraphSelectedStateName;
        bool animationRuntimeWindowOpen = true;
        int animationRuntimePinnedNodeIndex = -1;
        bool animationRuntimeObservedEntityUnavailable = false;
        float animationGraphFsmZoom = 1.0f;
        float animationGraphAssetZoom = 1.0f;
        float animationGraphBlend2DZoom = 1.0f;
        float animationRuntimeBlend2DZoom = 1.0f;
        ImVec2 animationGraphFsmPan{ 24.0f, 24.0f };
        ImVec2 animationGraphAssetPan{ 24.0f, 24.0f };
        ImVec2 animationGraphBlend2DPan{ 24.0f, 24.0f };
        ImVec2 animationRuntimeBlend2DPan{ 24.0f, 24.0f };
        std::unordered_map<std::string, ImVec2> animationGraphFsmNodePositions;
        std::unordered_map<std::string, ImVec2> animationGraphAssetNodePositions;
    };

    struct DerivedLists
    {
        std::vector<std::vector<int>> children;
        std::vector<int> roots;
        std::vector<std::string> meshIds;
        std::vector<std::string> modelIds;
        std::vector<std::string> skinnedMeshIds;
        std::vector<std::string> materialIds;
    };

    static LevelEditorUIState& GetState()
    {
        static LevelEditorUIState s{};
        return s;
    }

    static bool NodeAlive(const rendern::LevelAsset& level, int idx)
    {
        if (idx < 0)
            return false;
        const std::size_t i = static_cast<std::size_t>(idx);
        return i < level.nodes.size() && level.nodes[i].alive;
    }
    
    // Compatibility sync while Scene-owned editor selection migrates to EditorSelectionService.
    static void SyncEditorSelectionServiceWithScene(
        const rendern::Scene& scene,
        rendern::EditorSelectionService& editorSelection)
    {
        editorSelection.ClearSelection();
        for (const int selectedNodeIndex : scene.editorSelectedNodes)
        {
            editorSelection.ToggleSceneNode(selectedNodeIndex);
        }

        if (scene.editorSelectedNode >= 0 && !editorSelection.IsSceneNodeSelected(scene.editorSelectedNode))
        {
            editorSelection.ToggleSceneNode(scene.editorSelectedNode);
        }
    }

    static void SyncSavePathWithSource(rendern::LevelAsset& level, LevelEditorUIState& uiState)
    {
        if (uiState.cachedSourcePath != level.sourcePath)
        {
            uiState.cachedSourcePath = level.sourcePath;
            const std::string fallback = uiState.cachedSourcePath.empty()
                ? std::string("levels/edited.level.json")
                : uiState.cachedSourcePath;
            std::snprintf(uiState.savePathBuf, sizeof(uiState.savePathBuf), "%s", fallback.c_str());
        }
    }
    
    static SceneHierarchyLightKind ToSceneHierarchyLightKind(rendern::LightType type) noexcept
    {
        switch (type)
        {
        case rendern::LightType::Directional: return SceneHierarchyLightKind::Directional;
        case rendern::LightType::Point:       return SceneHierarchyLightKind::Point;
        case rendern::LightType::Spot:        return SceneHierarchyLightKind::Spot;
        }
        return SceneHierarchyLightKind::Unknown;
    }

    static const char* SceneHierarchyLightKindLabel(SceneHierarchyLightKind kind) noexcept
    {
        switch (kind)
        {
        case SceneHierarchyLightKind::Directional: return "Directional";
        case SceneHierarchyLightKind::Point:       return "Point";
        case SceneHierarchyLightKind::Spot:        return "Spot";
        case SceneHierarchyLightKind::Unknown:     return "Unknown";
        }
        return "Unknown";
    }

    static SceneHierarchyNodeSourceKind GetSceneHierarchyNodeSourceKind(const rendern::LevelNode& node) noexcept
    {
        if (!node.skinnedMesh.empty())
            return SceneHierarchyNodeSourceKind::SkinnedMesh;
        if (!node.model.empty())
            return SceneHierarchyNodeSourceKind::Model;
        if (!node.mesh.empty())
            return SceneHierarchyNodeSourceKind::Mesh;
        return SceneHierarchyNodeSourceKind::Empty;
    }

    static std::string BuildSceneHierarchyNodeLabel(const rendern::LevelNode& node, int nodeIndex)
    {
        char label[256]{};
        const char* name = node.name.empty() ? "<unnamed>" : node.name.c_str();
        if (!node.mesh.empty())
            std::snprintf(label, sizeof(label), "%d: %s  [mesh=%s]", nodeIndex, name, node.mesh.c_str());
        else
            std::snprintf(label, sizeof(label), "%d: %s", nodeIndex, name);
        return label;
    }

    static std::string BuildSceneHierarchyParticleEmitterLabel(const rendern::ParticleEmitter& emitter, int emitterIndex)
    {
        char label[256]{};
        const char* name = emitter.name.empty() ? "<unnamed emitter>" : emitter.name.c_str();
        std::snprintf(label, sizeof(label), "%d: %s%s", emitterIndex, name, emitter.enabled ? "" : "  [disabled]");
        return label;
    }

    static std::string BuildSceneHierarchyLightLabel(const rendern::Light& light, int lightIndex)
    {
        char label[256]{};
        std::snprintf(
            label,
            sizeof(label),
            "%d: %s%s",
            lightIndex,
            SceneHierarchyLightKindLabel(ToSceneHierarchyLightKind(light.type)),
            light.intensity > 0.00001f ? "" : "  [disabled]");
        return label;
    }

    static SceneHierarchyViewModel BuildSceneHierarchyViewModel(
        const rendern::LevelAsset& level,
        const DerivedLists& derived,
        const rendern::Scene& scene,
        const rendern::EditorSelectionService& editorSelection)
    {
        SceneHierarchyViewModel viewModel{};
        viewModel.items.reserve(level.nodes.size() + level.particleEmitters.size() + scene.lights.size());
        viewModel.sceneRootItemIndices.reserve(derived.roots.size());
        viewModel.particleEmitterItemIndices.reserve(level.particleEmitters.size());
        viewModel.lightItemIndices.reserve(scene.lights.size());

        auto appendNode = [&](auto&& self, int nodeIndex, SceneHierarchyItemId parentId) -> int
            {
                const std::size_t nodeArrayIndex = static_cast<std::size_t>(nodeIndex);
                const rendern::LevelNode& node = level.nodes[nodeArrayIndex];

                SceneHierarchyItemViewModel item{};
                item.id = SceneHierarchyItemId{ SceneHierarchyItemKind::SceneNode, nodeIndex };
                item.parentId = parentId;
                item.displayName = BuildSceneHierarchyNodeLabel(node, nodeIndex);
                item.typeFlags.nodeSource = GetSceneHierarchyNodeSourceKind(node);
                item.isSelected =
                    editorSelection.IsSceneNodeSelected(nodeIndex) ||
                    scene.EditorIsNodeSelected(nodeIndex);
                item.isVisibleOrEnabled = node.visible;
                item.hasChildren = nodeArrayIndex < derived.children.size() && !derived.children[nodeArrayIndex].empty();
                item.childItemIndices.reserve(item.hasChildren ? derived.children[nodeArrayIndex].size() : 0u);

                const int itemIndex = static_cast<int>(viewModel.items.size());
                viewModel.items.push_back(std::move(item));

                if (nodeArrayIndex < derived.children.size())
                {
                    for (const int childNodeIndex : derived.children[nodeArrayIndex])
                    {
                        const int childItemIndex = self(self, childNodeIndex, viewModel.items[static_cast<std::size_t>(itemIndex)].id);
                        viewModel.items[static_cast<std::size_t>(itemIndex)].childItemIndices.push_back(childItemIndex);
                    }
                }

                return itemIndex;
            };

        for (const int rootNodeIndex : derived.roots)
        {
            viewModel.sceneRootItemIndices.push_back(
                appendNode(appendNode, rootNodeIndex, SceneHierarchyItemId{}));
        }

        for (std::size_t emitterIndex = 0; emitterIndex < level.particleEmitters.size(); ++emitterIndex)
        {
            const rendern::ParticleEmitter& emitter = level.particleEmitters[emitterIndex];
            SceneHierarchyItemViewModel item{};
            item.id = SceneHierarchyItemId{ SceneHierarchyItemKind::ParticleEmitter, static_cast<int>(emitterIndex) };
            item.displayName = BuildSceneHierarchyParticleEmitterLabel(emitter, static_cast<int>(emitterIndex));
            item.isSelected = scene.EditorIsParticleEmitterSelected(static_cast<int>(emitterIndex));
            item.isVisibleOrEnabled = emitter.enabled;

            viewModel.particleEmitterItemIndices.push_back(static_cast<int>(viewModel.items.size()));
            viewModel.items.push_back(std::move(item));
        }

        for (std::size_t lightIndex = 0; lightIndex < scene.lights.size(); ++lightIndex)
        {
            const rendern::Light& light = scene.lights[lightIndex];
            SceneHierarchyItemViewModel item{};
            item.id = SceneHierarchyItemId{ SceneHierarchyItemKind::Light, static_cast<int>(lightIndex) };
            item.displayName = BuildSceneHierarchyLightLabel(light, static_cast<int>(lightIndex));
            item.typeFlags.lightKind = ToSceneHierarchyLightKind(light.type);
            item.isSelected = scene.EditorIsLightSelected(static_cast<int>(lightIndex));
            item.isVisibleOrEnabled = light.intensity > 0.00001f;

            viewModel.lightItemIndices.push_back(static_cast<int>(viewModel.items.size()));
            viewModel.items.push_back(std::move(item));
        }

        return viewModel;
    }

    static void BuildDerivedLists(const rendern::LevelAsset& level, DerivedLists& out)
    {
        const std::size_t ncount = level.nodes.size();
        out.children.clear();
        out.children.resize(ncount);
        out.roots.clear();
        out.roots.reserve(ncount);

        for (std::size_t i = 0; i < ncount; ++i)
        {
            const auto& n = level.nodes[i];
            if (!n.alive) continue;
            if (n.parent < 0) continue;
            if (!NodeAlive(level, n.parent)) continue;
            out.children[static_cast<std::size_t>(n.parent)].push_back(static_cast<int>(i));
        }

        for (std::size_t i = 0; i < ncount; ++i)
        {
            const auto& n = level.nodes[i];
            if (!n.alive) continue;
            if (n.parent < 0 || !NodeAlive(level, n.parent))
                out.roots.push_back(static_cast<int>(i));
        }

        out.meshIds.clear();
        out.meshIds.reserve(level.meshes.size());
        for (const auto& [id, _] : level.meshes) out.meshIds.push_back(id);
        std::sort(out.meshIds.begin(), out.meshIds.end());

        out.modelIds.clear();
        out.modelIds.reserve(level.models.size());
        for (const auto& [id, _] : level.models) out.modelIds.push_back(id);
        std::sort(out.modelIds.begin(), out.modelIds.end());

        out.skinnedMeshIds.clear();
        out.skinnedMeshIds.reserve(level.skinnedMeshes.size());
        for (const auto& [id, _] : level.skinnedMeshes) out.skinnedMeshIds.push_back(id);
        std::sort(out.skinnedMeshIds.begin(), out.skinnedMeshIds.end());

        out.materialIds.clear();
        out.materialIds.reserve(level.materials.size());
        for (const auto& [id, _] : level.materials) out.materialIds.push_back(id);
        std::sort(out.materialIds.begin(), out.materialIds.end());
    }

    static std::string SanitizeId(std::string s)
    {
        if (s.empty())
            s = "mesh";

        for (char& c : s)
        {
            const unsigned char uc = static_cast<unsigned char>(c);
            if (!(std::isalnum(uc) || c == '_' || c == '-'))
                c = '_';
        }
        return s;
    }

    static std::string MakeUniqueMeshId(const rendern::LevelAsset& level, std::string base)
    {
        std::string id = SanitizeId(std::move(base));
        if (id.empty())
            id = "mesh";

        if (!level.meshes.contains(id))
            return id;

        for (int suffix = 2; suffix < 10000; ++suffix)
        {
            std::string tryId = id + "_" + std::to_string(suffix);
            if (!level.meshes.contains(tryId))
                return tryId;
        }
        return id + "_x";
    }

    static std::string MakeUniqueModelId(const rendern::LevelAsset& level, std::string base)
    {
        std::string id = SanitizeId(std::move(base));
        if (id.empty())
            id = "model";

        if (!level.models.contains(id))
            return id;

        for (int suffix = 2; suffix < 10000; ++suffix)
        {
            std::string tryId = id + "_" + std::to_string(suffix);
            if (!level.models.contains(tryId))
                return tryId;
        }
        return id + "_x";
    }

    static void EnsureDefaultMesh(rendern::LevelAsset& level, std::string_view id, std::string_view relPath)
    {
        if (!level.meshes.contains(std::string(id)))
        {
            rendern::LevelMeshDef def{};
            def.path = std::string(relPath);
            def.debugName = std::string(id);
            level.meshes.emplace(std::string(id), std::move(def));
        }
    }

    static std::string MakeUniqueSkinnedMeshId(const rendern::LevelAsset& level, std::string base)
    {
        std::string id = SanitizeId(std::move(base));
        if (id.empty())
            id = "skinned";

        if (!level.skinnedMeshes.contains(id))
            return id;

        for (int suffix = 2; suffix < 10000; ++suffix)
        {
            std::string tryId = id + "_" + std::to_string(suffix);
            if (!level.skinnedMeshes.contains(tryId))
                return tryId;
        }
        return id + "_x";
    }

    static rendern::Transform ComputeSpawnTransform(const rendern::Scene& scene, const rendern::CameraController& camCtl)
    {
        rendern::Transform t{};
        t.position = scene.camera.position + camCtl.Forward() * 5.0f;
        t.rotationDegrees = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
        t.scale = mathUtils::Vec3(1.0f, 1.0f, 1.0f);
        return t;
    }

    static int ParentForNewNode(const rendern::LevelAsset& level, const LevelEditorUIState& uiState)
    {
        return (uiState.addAsChildOfSelection && NodeAlive(level, uiState.selectedNode)) ? uiState.selectedNode : -1;
    }

    static bool ParticleEmitterAlive(const rendern::LevelAsset& level, int idx)
    {
        return idx >= 0 && static_cast<std::size_t>(idx) < level.particleEmitters.size();
    }
}
