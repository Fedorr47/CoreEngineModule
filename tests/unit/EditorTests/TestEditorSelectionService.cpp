#include <gtest/gtest.h>

import core;

using namespace rendern;

TEST(EditorSelectionService, StartsEmptyAndClearsSelectedEditorObject)
{
    EditorSelectionService selection{};

    EXPECT_FALSE(selection.HasSelection());
    EXPECT_FALSE(selection.HasSelectedSceneNode());
    EXPECT_EQ(selection.GetSelectedObjectKind(), EditorSelectedObjectKind::None);
    EXPECT_EQ(selection.GetSelectedObjectIndex(), -1);
    EXPECT_EQ(selection.GetPrimarySceneNode(), -1);
    EXPECT_TRUE(selection.GetSelectedSceneNodes().empty());

    selection.SelectSceneNode(7);
    ASSERT_TRUE(selection.HasSelection());
    EXPECT_TRUE(selection.HasSelectedSceneNode());
    EXPECT_EQ(selection.GetPrimarySceneNode(), 7);
    ASSERT_EQ(selection.GetSelectedSceneNodes().size(), 1u);
    EXPECT_EQ(selection.GetSelectedSceneNodes().front(), 7);

    selection.ClearSelection();
    EXPECT_FALSE(selection.HasSelection());
    EXPECT_FALSE(selection.HasSelectedSceneNode());
    EXPECT_EQ(selection.GetPrimarySceneNode(), -1);
    EXPECT_TRUE(selection.GetSelectedSceneNodes().empty());
}

TEST(EditorSelectionService, SeparatesObservedRuntimeEntityFromEditorSelection)
{
    EditorSelectionService selection{};
    constexpr EntityHandle runtimeEntity{ 42u };

    selection.SetObservedRuntimeEntity(runtimeEntity);
    EXPECT_TRUE(selection.HasObservedRuntimeEntity());
    EXPECT_EQ(selection.GetObservedRuntimeEntity(), runtimeEntity);
    EXPECT_FALSE(selection.HasSelection());

    selection.SetSelectedObject(EditorSelectedObjectId{ .kind = EditorSelectedObjectKind::Light, .index = 2 });
    EXPECT_TRUE(selection.HasSelection());
    EXPECT_EQ(selection.GetSelectedObject(), (EditorSelectedObjectId{ .kind = EditorSelectedObjectKind::Light, .index = 2 }));
    EXPECT_FALSE(selection.HasSelectedSceneNode());
    EXPECT_EQ(selection.GetObservedRuntimeEntity(), runtimeEntity);

    selection.SetSelectedObject(EditorSelectedObjectId{ .kind = EditorSelectedObjectKind::DrawItem, .index = 4 });
    EXPECT_EQ(selection.GetSelectedObject(), (EditorSelectedObjectId{ .kind = EditorSelectedObjectKind::DrawItem, .index = 4 }));

    selection.SetSelectedObject(EditorSelectedObjectId{ .kind = EditorSelectedObjectKind::SkinnedDrawItem, .index = 1 });
    EXPECT_EQ(selection.GetSelectedObject(), (EditorSelectedObjectId{ .kind = EditorSelectedObjectKind::SkinnedDrawItem, .index = 1 }));

    selection.SetSelectedObject(EditorSelectedObjectId{ .kind = EditorSelectedObjectKind::ParticleEmitter, .index = 3 });
    EXPECT_EQ(selection.GetSelectedObject(), (EditorSelectedObjectId{ .kind = EditorSelectedObjectKind::ParticleEmitter, .index = 3 }));
    EXPECT_EQ(selection.GetObservedRuntimeEntity(), runtimeEntity);

    selection.ClearSelection();
    EXPECT_FALSE(selection.HasSelection());
    EXPECT_TRUE(selection.HasObservedRuntimeEntity());
    EXPECT_EQ(selection.GetObservedRuntimeEntity(), runtimeEntity);

    selection.ClearObservedRuntimeEntity();
    EXPECT_FALSE(selection.HasObservedRuntimeEntity());
    EXPECT_EQ(selection.GetObservedRuntimeEntity(), kNullEntity);
}

TEST(EditorSelectionService, ToggleSceneNodeMaintainsPrimaryNodeAndSelectionSet)
{
    EditorSelectionService selection{};

    selection.ToggleSceneNode(3);
    selection.ToggleSceneNode(5);

    EXPECT_TRUE(selection.HasSelectedSceneNode());
    EXPECT_EQ(selection.GetPrimarySceneNode(), 5);
    EXPECT_TRUE(selection.IsSceneNodeSelected(3));
    EXPECT_TRUE(selection.IsSceneNodeSelected(5));
    ASSERT_EQ(selection.GetSelectedSceneNodes().size(), 2u);

    selection.ToggleSceneNode(5);
    EXPECT_TRUE(selection.HasSelectedSceneNode());
    EXPECT_EQ(selection.GetPrimarySceneNode(), 3);
    EXPECT_TRUE(selection.IsSceneNodeSelected(3));
    EXPECT_FALSE(selection.IsSceneNodeSelected(5));

    selection.ToggleSceneNode(3);
    EXPECT_FALSE(selection.HasSelection());
    EXPECT_TRUE(selection.GetSelectedSceneNodes().empty());
}

TEST(EditorCommands, SelectSceneNodeRoutesThroughSelectionServiceAndMirrorsScene)
{
    EditorSelectionService selection{};
    Scene scene{};

    editor_commands::SelectSceneNode(selection, scene, 4);

    EXPECT_TRUE(selection.HasSelectedSceneNode());
    EXPECT_EQ(selection.GetPrimarySceneNode(), 4);
    ASSERT_EQ(selection.GetSelectedSceneNodes().size(), 1u);
    EXPECT_EQ(selection.GetSelectedSceneNodes().front(), 4);
    EXPECT_EQ(scene.editorSelectedNode, 4);
    ASSERT_EQ(scene.editorSelectedNodes.size(), 1u);
    EXPECT_EQ(scene.editorSelectedNodes.front(), 4);
}

TEST(EditorCommands, ToggleSceneNodePreservesExistingSceneSelectionWhileUsingService)
{
    EditorSelectionService selection{};
    Scene scene{};
    scene.EditorSetSelectionSingle(2);

    editor_commands::ToggleSceneNodeSelection(selection, scene, 5);

    EXPECT_TRUE(selection.IsSceneNodeSelected(2));
    EXPECT_TRUE(selection.IsSceneNodeSelected(5));
    EXPECT_EQ(selection.GetPrimarySceneNode(), 5);
    EXPECT_TRUE(scene.EditorIsNodeSelected(2));
    EXPECT_TRUE(scene.EditorIsNodeSelected(5));
    EXPECT_EQ(scene.editorSelectedNode, 5);
}