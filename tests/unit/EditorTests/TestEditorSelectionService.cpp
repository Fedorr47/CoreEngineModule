module;

#include <gtest/gtest.h>

module core;

import :editor_selection_service;

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
