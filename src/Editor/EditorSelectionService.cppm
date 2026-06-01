module;

#include <cstddef>
#include <vector>

export module core:editor_selection_service;

import :EnTTHelpers;

export namespace rendern
{
    using EnTT_helpers::EntityHandle;
    using EnTT_helpers::kNullEntity;
    
    enum class EditorSelectedObjectKind
    {
        None,
        SceneNode,
        Light,
        DrawItem,
        SkinnedDrawItem,
        ParticleEmitter
    };
    
    struct EditorSelectedObjectId
    {
        EditorSelectedObjectKind kind;
        int index{-1}; // INDEX_NONE change it later
        
        [[nodiscard]] bool IsValid() const noexcept
        {
            return index >= 0 && kind != EditorSelectedObjectKind::None;
        }
        
        [[nodiscard]] friend bool operator==(
            const EditorSelectedObjectId& lhs, 
            const EditorSelectedObjectId& rhs) noexcept = default;       
    };
    
    /* Minimal editor/debug UI selection boundary.
        
          This service stores only lightweight IDs/handles. It does not own Scene,
          LevelInstance, gameplay runtime, renderer, RHI, RenderGraph, asset, or
          entity objects. Runtime entity state is intentionally tracked as an 
          observed entity handle rather than as the selected editor object so debug
          panels do not conflate gameplay-controlled runtime focus woth edotor
          selection. Usage follows the current single-threaded editor/debug UI flow;
          this type does not provide synchronization.
        */
    class EditorSelectionService
    {
    public:
        [[nodiscard]] bool HasSelection() const noexcept
        {
            return selectedObject_.IsValid();
        }
            
        [[nodiscard]] EditorSelectedObjectId GetSelectedObject() const noexcept
        {
            return selectedObject_;
        }
            
        [[nodiscard]] EditorSelectedObjectKind GetSelectedObjectKind() const noexcept
        {
            return selectedObject_.kind;
        }
            
        [[nodiscard]] int GetSelectedObjectIndex() const noexcept
        {
            return selectedObject_.index;
        }
            
        void ClearSelection() noexcept
        {
            selectedObject_ = {};
            selectedSceneNodes_.clear();
        }
        
        template <typename T>
        void SetSelectedObject(T&& objectId)
        {
            ClearSelection();
            if (!objectId.IsValid())
            {
                return;
            }
                
            selectedObject_ = objectId;
            if (objectId.kind == EditorSelectedObjectKind::SceneNode)
            {
                selectedSceneNodes_.push_back(objectId.index);
            }
        }
        
        void SelectSceneNode(const int nodeIndex)
        {
            SetSelectedObject(EditorSelectedObjectId{.kind = EditorSelectedObjectKind::SceneNode, .index = nodeIndex});
        }
        
        void ToggleSceneNode(const int nodeIndex)
        {
            if (nodeIndex < 0)
            {
                return;
            }
            
            if (selectedObject_.kind != EditorSelectedObjectKind::SceneNode)
            {
                ClearSelection();
            }
            
            for (std::size_t i = 0; i < selectedSceneNodes_.size(); ++i)
            {
                if (selectedSceneNodes_[i] == nodeIndex)
                {
                    selectedSceneNodes_.erase(selectedSceneNodes_.begin() + static_cast<std::vector<int>::difference_type>(i));
                    selectedObject_ = selectedSceneNodes_.empty()
                        ? EditorSelectedObjectId{}
                        : EditorSelectedObjectId{.kind = EditorSelectedObjectKind::SceneNode, .index = selectedSceneNodes_.back()};
                    return;
                }
            }
            
            selectedSceneNodes_.push_back(nodeIndex);
            selectedObject_ = EditorSelectedObjectId{.kind = EditorSelectedObjectKind::SceneNode, .index = nodeIndex};
        }
        
        [[nodiscard]] bool HasSelectedSceneNode() const noexcept
        {
            return selectedObject_.kind == EditorSelectedObjectKind::SceneNode && selectedObject_.index >= 0;
        }
        
        [[nodiscard]] int GetPrimarySceneNode() const noexcept
        {
            return HasSelectedSceneNode() ? selectedObject_.index : -1;
        }
        
        [[nodiscard]] const std::vector<int>& GetSelectedSceneNodes() const noexcept
        {
            return selectedSceneNodes_;
        }
        
        [[nodiscard]] bool IsSceneNodeSelected(const int nodeIndex) const noexcept
        {
            for (const int& selectedSceneNode : selectedSceneNodes_)
            {
                if (selectedSceneNode == nodeIndex)
                {
                    return true;
                }
            }
            
            return false;
        }
        
        void ClearObservedRuntimeEntity() noexcept
        {
            observedRuntimeEntity = kNullEntity;
        }
        
        void SetObservedRuntimeEntity(const EntityHandle entity) noexcept
        {
            observedRuntimeEntity = entity;
        }
        
        [[nodiscard]] bool HasObservedRuntimeEntity() const noexcept
        {
            return observedRuntimeEntity != kNullEntity;
        }
        
        [[nodiscard]] EntityHandle GetObservedRuntimeEntity() const noexcept
        {
            return observedRuntimeEntity;
        }
        
        void ClearAll() noexcept
        {
            ClearSelection();
            ClearObservedRuntimeEntity();
        }
            
    private:
        EditorSelectedObjectId  selectedObject_{};
        std::vector<int> selectedSceneNodes_{};
        EntityHandle observedRuntimeEntity{kNullEntity};
    };
}
