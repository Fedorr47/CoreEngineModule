#include <gtest/gtest.h>

#include <string>
#include <unordered_map>

#include "TestSupport/TestThreadAffinity.h"

import core;

using namespace rendern;

namespace
{
    // Audit note for CR-54:
    // GameplaySceneSync currently exposes transform sync only (SyncGameplayTransformsToRuntime).
    // Node<->entity lifecycle mapping is currently exercised through GameplayRuntime::SpawnNodeBoundEntity.
    class GameplayRuntimeNodeBoundMappingHarness
    {
    public:
        struct MappingSnapshot
        {
            std::size_t validNodeBoundEntityCount{ 0 };
            std::size_t uniqueNodeIndexCount{ 0 };
            std::size_t duplicateNodeIndexCount{ 0 };
            std::unordered_map<int, EntityHandle> nodeToEntity{};
        };

        explicit GameplayRuntimeNodeBoundMappingHarness(const int nodeCount)
            : threadAffinityGuard_{}
            , levelAsset_(MakeLevelWithNodes(nodeCount))
        {
            runtime_.Initialize(levelAsset_, levelInstance_, scene_);
        }

        [[nodiscard]] GameplayUpdateContext Context()
        {
            GameplayUpdateContext ctx{};
            ctx.mode = GameplayRuntimeMode::Game;
            ctx.levelAsset = &levelAsset_;
            ctx.levelInstance = &levelInstance_;
            ctx.scene = &scene_;
            return ctx;
        }
        
        [[nodiscard]] GameplayRuntime& Runtime() noexcept
        {
            return runtime_;
        }

        [[nodiscard]] EntityHandle SyncNodeMapping(const int nodeIndex, const bool playerControlled = false)
        {
            return runtime_.SpawnNodeBoundEntity(Context(), nodeIndex, playerControlled);
        }
        
        [[nodiscard]] bool DestroyNodeMapping(const EntityHandle entity)
        {
            return runtime_.DestroyNodeBoundEntity(entity);
        }

        [[nodiscard]] bool IsEntityValid(const EntityHandle entity) const
        {
            return runtime_.GetWorld().IsEntityValid(entity);
        }

        [[nodiscard]] EntityHandle CreateUntrackedEntity()
        {
            return runtime_.GetWorld().CreateEntity();
        }

        [[nodiscard]] EntityHandle ControlledEntity() const
        {
            return runtime_.GetControlledEntity();
        }

        [[nodiscard]] MappingSnapshot CaptureNodeMappings() const
        {
            MappingSnapshot snapshot{};
            const GameplayWorld& world = runtime_.GetWorld();
            for (const EntityHandle entity : runtime_.GetNodeBoundEntities())
            {
                if (!world.IsEntityValid(entity))
                {
                    continue;
                }

                ++snapshot.validNodeBoundEntityCount;

                const GameplayNodeLinkComponent* link = world.TryGetNodeLink(entity);
                if (link == nullptr)
                {
                    continue;
                }

                const auto [_, inserted] = snapshot.nodeToEntity.emplace(link->nodeIndex, entity);
                if (!inserted)
                {
                    ++snapshot.duplicateNodeIndexCount;
                }
            }

            snapshot.uniqueNodeIndexCount = snapshot.nodeToEntity.size();
            return snapshot;
        }

        void MarkNodeDead(const int nodeIndex)
        {
            levelAsset_.nodes[static_cast<std::size_t>(nodeIndex)].alive = false;
        }

        void MarkNodeAlive(const int nodeIndex)
        {
            levelAsset_.nodes[static_cast<std::size_t>(nodeIndex)].alive = true;
        }

        void UpdateNodePosition(const int nodeIndex, const mathUtils::Vec3& position)
        {
            levelAsset_.nodes[static_cast<std::size_t>(nodeIndex)].transform.position = position;
        }

    private:
        static LevelAsset MakeLevelWithNodes(const int count)
        {
            LevelAsset asset{};
            asset.name = "GameplayRuntimeNodeBoundMappingFixture";
            for (int i = 0; i < count; ++i)
            {
                LevelNode node{};
                node.name = "Node_" + std::to_string(i);
                node.alive = true;
                node.visible = true;
                node.transform.position = { static_cast<float>(i), 0.0f, 0.0f };
                asset.nodes.push_back(node);
            }
            return asset;
        }

        InlineThreadOwnerRolesGuard threadAffinityGuard_{};
        LevelAsset levelAsset_{};
        LevelInstance levelInstance_{};
        Scene scene_{};
        GameplayRuntime runtime_{};
    };
}

TEST(GameplayRuntimeNodeBoundMappingConsistency, ExplicitDestructionImmediatelyRemovesTrackingAndAllowsFreshSpawn)
{
    GameplayRuntimeNodeBoundMappingHarness harness(1);
    const EntityHandle first = harness.SyncNodeMapping(0, true);
    ASSERT_NE(first, kNullEntity);
    EXPECT_EQ(harness.ControlledEntity(), first);
    
    GameplayRuntime& runtime = harness.Runtime();
    GameplayWorld& world = runtime.GetWorld();
    world.AddAI(first);
    
    GameplayRoute route{
        .points = {
                            {{0.0f, 0.0f, 0.0f}},
                            {{1.0f, 0.0f, 0.0f}}},
                        .segmentAnnotations = {{}}};
    ASSERT_EQ(
        runtime.StartAIFollowRoute(first, std::move(route)),
        AIActionExecutionStatus::Running);
    
    const EntityHandle interactionObject = world.CreateEntity();
    world.AddInteractionPoint(
        interactionObject,
        GameplayInteractionPointComponent{});
    ASSERT_TRUE(runtime.TryReserveGameplayObject(interactionObject, first));
    ASSERT_TRUE(runtime.IsGameplayObjectReservedBy(interactionObject, first));
    
    EXPECT_TRUE(harness.DestroyNodeMapping(first));
    EXPECT_FALSE(harness.IsEntityValid(first));
    EXPECT_EQ(harness.ControlledEntity(), kNullEntity);
    EXPECT_EQ(
        runtime.GetAIActionStatus(first),
        AIActionExecutionStatus::NotStarted);
    EXPECT_FALSE(runtime.IsGameplayObjectReserved(interactionObject));
    EXPECT_TRUE(world.IsEntityValid(interactionObject));

    const auto afterDestroy = harness.CaptureNodeMappings();
    EXPECT_EQ(afterDestroy.validNodeBoundEntityCount, 0u);
    EXPECT_EQ(afterDestroy.uniqueNodeIndexCount, 0u);

    EXPECT_FALSE(harness.DestroyNodeMapping(first));
    const EntityHandle untracked = harness.CreateUntrackedEntity();
    EXPECT_FALSE(harness.DestroyNodeMapping(untracked));
    EXPECT_TRUE(harness.IsEntityValid(untracked));

    const EntityHandle second = harness.SyncNodeMapping(0);
    ASSERT_NE(second, kNullEntity);
    EXPECT_NE(second, first);
    const auto afterRespawn = harness.CaptureNodeMappings();
    EXPECT_EQ(afterRespawn.validNodeBoundEntityCount, 1u);
    EXPECT_EQ(afterRespawn.uniqueNodeIndexCount, 1u);
    EXPECT_EQ(afterRespawn.duplicateNodeIndexCount, 0u);
    EXPECT_EQ(afterRespawn.nodeToEntity.at(0), second);
}

TEST(GameplayRuntimeNodeBoundMappingConsistency, CreateSyncBuildsStableMappings)
{
    GameplayRuntimeNodeBoundMappingHarness harness(3);

    for (int nodeIndex = 0; nodeIndex < 3; ++nodeIndex)
    {
        ASSERT_NE(harness.SyncNodeMapping(nodeIndex), kNullEntity)
            << "Failed to create mapping for node index " << nodeIndex;
    }

    const auto snapshot = harness.CaptureNodeMappings();
    EXPECT_EQ(snapshot.validNodeBoundEntityCount, 3u);
    EXPECT_EQ(snapshot.uniqueNodeIndexCount, 3u);
    EXPECT_EQ(snapshot.duplicateNodeIndexCount, 0u)
        << "Duplicate nodeIndex mappings detected during initial sync";

    for (int nodeIndex = 0; nodeIndex < 3; ++nodeIndex)
    {
        const auto it = snapshot.nodeToEntity.find(nodeIndex);
        EXPECT_NE(it, snapshot.nodeToEntity.end()) << "Missing mapping for node index " << nodeIndex;
    }
}

TEST(GameplayRuntimeNodeBoundMappingConsistency, RepeatedSyncIsIdempotent)
{
    GameplayRuntimeNodeBoundMappingHarness harness(2);

    ASSERT_NE(harness.SyncNodeMapping(0), kNullEntity);
    ASSERT_NE(harness.SyncNodeMapping(1), kNullEntity);

    const auto before = harness.CaptureNodeMappings();
    ASSERT_EQ(before.validNodeBoundEntityCount, 2u);
    ASSERT_EQ(before.uniqueNodeIndexCount, 2u);
    ASSERT_EQ(before.duplicateNodeIndexCount, 0u);

    EXPECT_NE(harness.SyncNodeMapping(0), kNullEntity);
    EXPECT_NE(harness.SyncNodeMapping(1), kNullEntity);

    const auto after = harness.CaptureNodeMappings();
    EXPECT_EQ(after.validNodeBoundEntityCount, before.validNodeBoundEntityCount)
        << "Valid node-bound entity count grew on repeated sync with unchanged input";
    EXPECT_EQ(after.uniqueNodeIndexCount, before.uniqueNodeIndexCount)
        << "Unique mapped node index count grew on repeated sync";
    EXPECT_EQ(after.duplicateNodeIndexCount, 0u)
        << "Repeated sync produced duplicate nodeIndex mappings";

    for (const auto& [nodeIndex, entity] : before.nodeToEntity)
    {
        const auto it = after.nodeToEntity.find(nodeIndex);
        ASSERT_NE(it, after.nodeToEntity.end()) << "Mapping disappeared for node index " << nodeIndex;
        EXPECT_EQ(it->second, entity) << "Node mapping identity changed for node index " << nodeIndex;
    }
}

TEST(GameplayRuntimeNodeBoundMappingConsistency, UpdatingExistingNodePreservesEntityIdentity)
{
    GameplayRuntimeNodeBoundMappingHarness harness(1);

    ASSERT_NE(harness.SyncNodeMapping(0), kNullEntity);
    const auto before = harness.CaptureNodeMappings();
    ASSERT_EQ(before.uniqueNodeIndexCount, 1u);
    ASSERT_EQ(before.duplicateNodeIndexCount, 0u);

    harness.UpdateNodePosition(0, { 7.0f, 1.0f, -3.0f });
    EXPECT_NE(harness.SyncNodeMapping(0), kNullEntity);

    const auto after = harness.CaptureNodeMappings();
    ASSERT_EQ(after.validNodeBoundEntityCount, before.validNodeBoundEntityCount)
        << "Node-bound entity count changed after update-only node mutation";
    ASSERT_EQ(after.uniqueNodeIndexCount, before.uniqueNodeIndexCount)
        << "Unique mapping count changed after update-only node mutation";
    ASSERT_EQ(after.duplicateNodeIndexCount, 0u)
        << "Update flow introduced duplicate nodeIndex mappings";
    EXPECT_EQ(after.nodeToEntity.at(0), before.nodeToEntity.at(0))
        << "Unchanged node identity remapped to different entity";
}

TEST(GameplayRuntimeNodeBoundMappingConsistency, RemovingNodeRemovesOrInvalidatesMapping)
{
    GameplayRuntimeNodeBoundMappingHarness harness(2);

    ASSERT_NE(harness.SyncNodeMapping(0), kNullEntity);
    ASSERT_NE(harness.SyncNodeMapping(1), kNullEntity);
    const auto before = harness.CaptureNodeMappings();
    ASSERT_EQ(before.uniqueNodeIndexCount, 2u);

    harness.MarkNodeDead(1);
    EXPECT_EQ(harness.SyncNodeMapping(1), kNullEntity);

    const auto after = harness.CaptureNodeMappings();
    EXPECT_EQ(after.validNodeBoundEntityCount, 1u)
        << "Node-bound entity count did not decrease after removing node 1";
    EXPECT_EQ(after.uniqueNodeIndexCount, 1u)
        << "Unique mapped node index count did not decrease after removing node 1";
    EXPECT_EQ(after.duplicateNodeIndexCount, 0u);
    EXPECT_EQ(after.nodeToEntity.find(1), after.nodeToEntity.end())
        << "Removed node 1 still has a mapping";

    const auto keepIt = after.nodeToEntity.find(0);
    ASSERT_NE(keepIt, after.nodeToEntity.end()) << "Remaining node lost its mapping after unrelated removal";
    EXPECT_EQ(keepIt->second, before.nodeToEntity.at(0))
        << "Remaining mapping changed identity after unrelated removal";
}

TEST(GameplayRuntimeNodeBoundMappingConsistency, RemoveAndRecreateNodeUsesCorrectIdentityContract)
{
    GameplayRuntimeNodeBoundMappingHarness harness(2);

    ASSERT_NE(harness.SyncNodeMapping(0), kNullEntity);
    ASSERT_NE(harness.SyncNodeMapping(1), kNullEntity);
    const auto baseline = harness.CaptureNodeMappings();
    ASSERT_EQ(baseline.uniqueNodeIndexCount, 2u);

    harness.MarkNodeDead(1);
    EXPECT_EQ(harness.SyncNodeMapping(1), kNullEntity);

    const auto afterRemove = harness.CaptureNodeMappings();
    ASSERT_EQ(afterRemove.validNodeBoundEntityCount, 1u);
    ASSERT_EQ(afterRemove.uniqueNodeIndexCount, 1u);
    ASSERT_EQ(afterRemove.duplicateNodeIndexCount, 0u);
    EXPECT_EQ(afterRemove.nodeToEntity.find(1), afterRemove.nodeToEntity.end())
        << "Removed node retained a stale mapping";
    EXPECT_EQ(afterRemove.nodeToEntity.at(0), baseline.nodeToEntity.at(0))
        << "Unrelated node mapping changed after removal";

    harness.MarkNodeAlive(1);
    EXPECT_NE(harness.SyncNodeMapping(1), kNullEntity)
        << "Recreated live node failed to produce mapping";

    const auto afterRecreate = harness.CaptureNodeMappings();
    EXPECT_EQ(afterRecreate.validNodeBoundEntityCount, 2u);
    EXPECT_EQ(afterRecreate.uniqueNodeIndexCount, 2u);
    EXPECT_EQ(afterRecreate.duplicateNodeIndexCount, 0u)
        << "Recreate flow introduced duplicate nodeIndex mappings";
    EXPECT_EQ(afterRecreate.nodeToEntity.at(0), baseline.nodeToEntity.at(0))
        << "Unrelated mapping changed after recreation";
    EXPECT_NE(afterRecreate.nodeToEntity.find(1), afterRecreate.nodeToEntity.end())
        << "Recreated node has no current mapping";
}
