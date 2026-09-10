#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/LevelPhysicsRuntime.h"

import core;
import std;

#include "AppDebugVisualization.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "AppLifecycle.h"

namespace appDebugVisualization
{
    using appLifecycle::AppState;

    static void UpdateGameplayMovementDebug(AppState& app)
    {
        auto& runtimeState = app.runtimeState;
        auto& graphicState = app.graphicsState;
        runtimeState.scene.gameplayMovementDebug.Clear();

        if (!runtimeState.gameplayRuntime || !graphicState.rendererSettings.drawGameplayMovementDebug)
        {
            return;
        }

        const rendern::EntityHandle controlledEntity = runtimeState.gameplayRuntime->GetControlledEntity();
        const auto& entities = runtimeState.gameplayRuntime->GetNodeBoundEntities();
        runtimeState.scene.gameplayMovementDebug.samples.reserve(entities.size());

        for (const rendern::EntityHandle entity : entities)
        {
            if (graphicState.rendererSettings.drawGameplayMovementDebugOnlyControlled
                && controlledEntity != rendern::kNullEntity
                && entity != controlledEntity)
            {
                continue;
            }

            const rendern::GameplayTransformComponent* transform = runtimeState.gameplayRuntime->GetWorld().TryGetTransform(entity);
            const rendern::GameplayCharacterMotorComponent* motor = runtimeState.gameplayRuntime->GetWorld().TryGetCharacterMotor(entity);
            if (transform == nullptr || motor == nullptr)
            {
                continue;
            }

            const rendern::GameplayLocomotionComponent* locomotion = runtimeState.gameplayRuntime->GetWorld().TryGetLocomotion(entity);
            const bool isRunning = locomotion != nullptr && locomotion->isRunning;
            const float targetSpeed = isRunning ? motor->maxRunSpeed : motor->maxWalkSpeed;

            const float yawRad = mathUtils::DegToRad(transform->rotationDegrees.y);
            const mathUtils::Vec3 facingForward(std::sin(yawRad), 0.0f, std::cos(yawRad));

            rendern::GameplayMovementDebugSample sample{};
            sample.entity = entity;
            sample.origin = transform->position;
            sample.velocity = motor->velocity;
            sample.targetVelocity = motor->desiredMoveWorld * targetSpeed;
            sample.desiredMoveWorld = motor->desiredMoveWorld;
            sample.facingForward = facingForward;
            sample.forwardSpeed = locomotion != nullptr ? locomotion->forwardSpeed : 0.0f;
            sample.rightSpeed = locomotion != nullptr ? locomotion->rightSpeed : 0.0f;
            sample.planarSpeed = locomotion != nullptr ? locomotion->planarSpeed : mathUtils::Length(motor->velocity);
            sample.controlled = entity == controlledEntity;
            runtimeState.scene.gameplayMovementDebug.samples.push_back(sample);
        }
    }

    static float AnimationRuntimeGetNormalizedTime(const rendern::AnimatorState& animator)
    {
        if (animator.clip == nullptr || animator.clip->ticksPerSecond <= 0.0f)
        {
            return 0.0f;
        }

        const float durationSeconds = animator.clip->durationTicks / animator.clip->ticksPerSecond;
        if (durationSeconds <= 1e-6f)
        {
            return 0.0f;
        }

        const float normalized = animator.timeSeconds / durationSeconds;
        if (animator.looping)
        {
            const float wrapped = normalized - std::floor(normalized);
            return std::clamp(wrapped, 0.0f, 1.0f);
        }

        return std::clamp(normalized, 0.0f, 1.0f);
    }

    static void UpdateAnimationRuntimeDebug(AppState& app)
    {
        auto& runtimeState = app.runtimeState;
        auto& graphicState = app.graphicsState;
        auto& contentState = app.contentState;
        runtimeState.scene.animationRuntimeDebug.Clear();

        if (!runtimeState.gameplayRuntime 
            || !graphicState.rendererSettings.drawAnimationRuntimeOverlay 
            || runtimeState.gameplayMode != rendern::GameplayRuntimeMode::Game)
        {
            return;
        }

        if (!contentState.levelAsset || !runtimeState.levelInstance)
        {
            return;
        }

        const rendern::EntityHandle controlledEntity = runtimeState.gameplayRuntime->GetControlledEntity();
        const auto& entities = runtimeState.gameplayRuntime->GetNodeBoundEntities();
        runtimeState.scene.animationRuntimeDebug.samples.reserve(entities.size());

        for (const rendern::EntityHandle entity : entities)
        {
            if (graphicState.rendererSettings.drawAnimationRuntimeOverlayOnlyControlled
                && controlledEntity != rendern::kNullEntity
                && entity != controlledEntity)
            {
                continue;
            }

            const auto& world = runtimeState.gameplayRuntime->GetWorld();
            const rendern::GameplayTransformComponent* transform = world.TryGetTransform(entity);
            const rendern::GameplayNodeLinkComponent* nodeLink = world.TryGetNodeLink(entity);
            const rendern::GameplayAnimationLinkComponent* animationLink = world.TryGetAnimationLink(entity);
            const rendern::GameplayAnimationStateComponent* animState = world.TryGetAnimationState(entity);
            const rendern::GameplayAnimationNotifyStateComponent* notifyState = world.TryGetAnimationNotifyState(entity);
            if (transform == nullptr || nodeLink == nullptr || animationLink == nullptr || animState == nullptr)
            {
                continue;
            }

            if (nodeLink->nodeIndex < 0 || static_cast<std::size_t>(nodeLink->nodeIndex) >= contentState.levelAsset->nodes.size())
            {
                continue;
            }

            const rendern::LevelNode& node = contentState.levelAsset->nodes[static_cast<std::size_t>(nodeLink->nodeIndex)];
            rendern::SkinnedDrawItem* skinnedItem = runtimeState.levelInstance->GetSkinnedDrawItem(runtimeState.scene, animationLink->skinnedDrawIndex);
            if (skinnedItem == nullptr)
            {
                continue;
            }

            const rendern::AnimationControllerRuntime& runtimeController = skinnedItem->controller;
            const float secondaryWeight = std::clamp(runtimeController.blendSecondaryAlpha, 0.0f, 1.0f);
            const float tertiaryWeight = std::clamp(runtimeController.blendTertiaryAlpha, 0.0f, 1.0f);
            const float primaryWeight = std::max(0.0f, 1.0f - secondaryWeight - tertiaryWeight);
            const float transitionAlpha = (runtimeController.transitionDurationSeconds > 1e-6f)
                ? std::clamp(runtimeController.transitionElapsedSeconds / runtimeController.transitionDurationSeconds, 0.0f, 1.0f)
                : (runtimeController.transitionActive ? 1.0f : 0.0f);

            rendern::AnimationRuntimeDebugSample sample{};
            sample.entity = entity;
            sample.origin = transform->position;
            sample.nodeName = node.name;
            sample.controllerAssetId = animState->controllerAssetId;
            sample.currentStateName = animState->currentStateName;
            sample.previousStateName = animState->previousStateName;
            sample.requestedStateName = runtimeController.requestedStateName;
            sample.modeName = animState->modeName;
            sample.primaryClipName = animState->primaryClipName;
            sample.secondaryClipName = animState->secondaryClipName;
            sample.tertiaryClipName = animState->tertiaryClipName;
            sample.blendParameterNameX = animState->blendParameterNameX;
            sample.blendParameterNameY = animState->blendParameterNameY;
            sample.blendParameterValueX = animState->blendParameterValueX;
            sample.blendParameterValueY = animState->blendParameterValueY;
            sample.lastNotifyId = notifyState != nullptr ? notifyState->lastNotifyId : std::string{};
            sample.normalizedTime = AnimationRuntimeGetNormalizedTime(skinnedItem->animator);
            sample.primaryWeight = primaryWeight;
            sample.secondaryWeight = secondaryWeight;
            sample.tertiaryWeight = tertiaryWeight;
            sample.transitionAlpha = transitionAlpha;
            sample.transitionActive = runtimeController.transitionActive;
            sample.controlled = entity == controlledEntity;
            runtimeState.scene.animationRuntimeDebug.samples.push_back(std::move(sample));
        }
    }
    
    void UpdateRuntimeDebugSamples(appLifecycle::AppState& app)
    {
        UpdateGameplayMovementDebug(app);
        UpdateAnimationRuntimeDebug(app);
    }

    void ComposeExternalDebugGeometry(AppState& app)
	{
	    auto& runtime = app.runtimeState;
	    runtime.scene.externalDebugLines.clear();
        runtime.scene.externalDebugTriangles.clear();
        runtime.scene.externalDebugCapsules.clear();
        runtime.scene.externalDebugBoxes.clear();
        runtime.scene.externalDebugSpheres.clear();
        runtime.scene.externalDebugArrows.clear();
        if (runtime.gameplayRuntime)
        {
            runtime.gameplayRuntime->SetSteeringDebugEnabled(
                app.graphicsState.rendererSettings.drawAIPlannedPathDebug);
        }
        const auto& physicsDebug = app.graphicsState.rendererSettings;
	    const bool drawAnyCharacterPhysics = physicsDebug.drawPhysicsCharacters
	        || physicsDebug.drawPhysicsCharacterGround
	        || physicsDebug.drawPhysicsCharacterVelocity
	        || physicsDebug.drawPhysicsCharacterBlocked;
	    if (drawAnyCharacterPhysics && runtime.gameplayRuntime && app.physicsState.joltPhysicsWorld)
	    {
	        constexpr std::uint32_t capsuleColor = 0xffe0b060u;
	        constexpr std::uint32_t walkableColor = 0xff55dd55u;
	        constexpr std::uint32_t steepColor = 0xff44aaffu;
	        constexpr std::uint32_t desiredColor = 0xff33ccffu;
	        constexpr std::uint32_t actualColor = 0xffffcc33u;
	        constexpr std::uint32_t blockedColor = 0xff3333ffu;
	        constexpr float pointHalfSize = 0.08f;
	        constexpr float normalLength = 0.5f;
	        constexpr float zeroVectorEpsilon = 1e-4f;
	        rendern::GameplayWorld& world = runtime.gameplayRuntime->GetWorld();
	        for (const rendern::EntityHandle entity : runtime.gameplayRuntime->GetNodeBoundEntities())
	        {
	            const auto* binding = world.TryGetPhysicsCharacter(entity);
	            if (binding == nullptr)
	            {
	                continue;
	            }
	            const auto state = app.physicsState.joltPhysicsWorld->GetCharacterDebugState(
	                binding->character);
	            if (!state)
	            {
	                continue;
	            }
	            if (physicsDebug.drawPhysicsCharacters)
	            {
	                runtime.scene.externalDebugCapsules.push_back({
	                    state->position, state->collider.radius,
	                    state->collider.cylinderHeight, capsuleColor});
	            }
	            if (physicsDebug.drawPhysicsCharacterGround && state->ground.bIsSupported)
	            {
	                const std::uint32_t color = state->ground.bIsWalkable
	                    ? walkableColor : steepColor;
	                const auto& point = state->ground.position;
	                runtime.scene.externalDebugLines.push_back({
	                    point - mathUtils::Vec3{pointHalfSize, 0.0f, 0.0f},
	                    point + mathUtils::Vec3{pointHalfSize, 0.0f, 0.0f}, color});
	                runtime.scene.externalDebugLines.push_back({
	                    point - mathUtils::Vec3{0.0f, 0.0f, pointHalfSize},
	                    point + mathUtils::Vec3{0.0f, 0.0f, pointHalfSize}, color});
	                runtime.scene.externalDebugLines.push_back({
	                    point, point + state->ground.normal * normalLength, color});
	            }
	            if (physicsDebug.drawPhysicsCharacterVelocity)
	            {
	                const float scale = physicsDebug.physicsCharacterVelocityScale;
	                if (mathUtils::Length(state->desiredVelocity) > zeroVectorEpsilon)
	                {
	                    runtime.scene.externalDebugArrows.push_back({state->position,
	                        state->position + state->desiredVelocity * scale, desiredColor});
	                }
	                if (mathUtils::Length(state->actualVelocity) > zeroVectorEpsilon)
	                {
	                    runtime.scene.externalDebugArrows.push_back({state->position,
	                        state->position + state->actualVelocity * scale, actualColor});
	                }
	            }
	            if (physicsDebug.drawPhysicsCharacterBlocked)
	            {
	                const auto* movement = world.TryGetCharacterMovementState(entity);
	                if (movement != nullptr && movement->physicallyBlocked)
	                {
	                    const float markerRadius = state->collider.radius * 1.25f;
	                    runtime.scene.externalDebugCapsules.push_back({state->position,
	                        markerRadius, state->collider.cylinderHeight, blockedColor});
	                }
	            }
	        }
	    }
        constexpr std::uint32_t staticBodyColor = 0xff55cc88u;
	    constexpr std::uint32_t dynamicBodyColor = 0xff44aaffu;
	    constexpr std::uint32_t kinematicBodyColor = 0xffffaa44u;
	    if (runtime.gameplayMode == rendern::GameplayRuntimeMode::Editor
	        && physicsDebug.drawPhysicsBodies && app.contentState.levelAsset)
	    {
	        for (const rendern::LevelNode& node : app.contentState.levelAsset->nodes)
	        {
	            const auto descriptor = physics::TryResolveLevelPhysicsBodyDescriptor(node);
	            if (!descriptor.has_value())
	            {
	                continue;
	            }
	            const std::uint32_t bodyColor =
	                descriptor->motionType == physics::PhysicsMotionType::Static
	                ? staticBodyColor : descriptor->motionType == physics::PhysicsMotionType::Dynamic
	                ? dynamicBodyColor : kinematicBodyColor;
	            std::visit([&](const auto& shape)
	            {
	                using Shape = std::decay_t<decltype(shape)>;
	                if constexpr (std::is_same_v<Shape, physics::BoxShapeDescriptor>)
	                {
	                    runtime.scene.externalDebugBoxes.push_back({ descriptor->transform.position,
	                        shape.halfExtents, descriptor->transform.rotationQuaternion, bodyColor });
	                }
	                else if constexpr (std::is_same_v<Shape, physics::SphereShapeDescriptor>)
	                {
	                    runtime.scene.externalDebugSpheres.push_back({
	                        descriptor->transform.position, shape.radius, bodyColor });
	                }
	                else if constexpr (std::is_same_v<Shape, physics::CapsuleShapeDescriptor>)
	                {
	                    runtime.scene.externalDebugCapsules.push_back({ descriptor->transform.position,
	                        shape.radius, shape.cylinderHeight, bodyColor,
	                        descriptor->transform.rotationQuaternion });
	                }
	            }, descriptor->shape);
	        }
	    }
        const bool drawAnyBodyPhysics = physicsDebug.drawPhysicsBodies
	        || physicsDebug.drawPhysicsBodyAabbs
	        || physicsDebug.drawPhysicsBodyVelocity;
        if (runtime.gameplayMode == rendern::GameplayRuntimeMode::Game
           && drawAnyBodyPhysics && app.physicsState.joltPhysicsWorld)
	    {
	        constexpr std::uint32_t aabbColor = 0xffcc66ccu;
	        constexpr std::uint32_t velocityColor = 0xff33ddffu;
	        constexpr float zeroVectorEpsilon = 1e-4f;
	        for (const physics::PhysicsBodyDebugState& state :
	            app.physicsState.joltPhysicsWorld->BuildBodyDebugStates())
	        {
	            const std::uint32_t bodyColor = state.motionType == physics::PhysicsMotionType::Static
	                ? staticBodyColor : state.motionType == physics::PhysicsMotionType::Dynamic
                    ? dynamicBodyColor : kinematicBodyColor;
	            if (physicsDebug.drawPhysicsBodies)
	            {
	                std::visit([&](const auto& shape)
	                {
	                    using Shape = std::decay_t<decltype(shape)>;
	                    if constexpr (std::is_same_v<Shape, physics::BoxShapeDescriptor>)
	                    {
	                        runtime.scene.externalDebugBoxes.push_back({ state.transform.position,
	                            shape.halfExtents, state.transform.rotationQuaternion, bodyColor });
	                    }
	                    else if constexpr (std::is_same_v<Shape, physics::SphereShapeDescriptor>)
	                    {
	                        runtime.scene.externalDebugSpheres.push_back({
	                            state.transform.position, shape.radius, bodyColor });
	                    }
	                    else if constexpr (std::is_same_v<Shape, physics::CapsuleShapeDescriptor>)
	                    {
	                        runtime.scene.externalDebugCapsules.push_back({ state.transform.position,
	                            shape.radius, shape.cylinderHeight, bodyColor,
	                            state.transform.rotationQuaternion });
	                    }
	                }, state.shape);
	            }
	            if (physicsDebug.drawPhysicsBodyAabbs)
	            {
	                const mathUtils::Vec3 center = (state.aabb.minimum + state.aabb.maximum) * 0.5f;
	                const mathUtils::Vec3 halfExtents = (state.aabb.maximum - state.aabb.minimum) * 0.5f;
	                runtime.scene.externalDebugBoxes.push_back({ center, halfExtents,
	                    { 0.0f, 0.0f, 0.0f, 1.0f }, aabbColor });
	            }
	            if (physicsDebug.drawPhysicsBodyVelocity
	                && state.motionType != physics::PhysicsMotionType::Static
	                && mathUtils::Length(state.linearVelocity) > zeroVectorEpsilon)
	            {
	                runtime.scene.externalDebugArrows.push_back({ state.transform.position,
	                    state.transform.position + state.linearVelocity * physicsDebug.physicsBodyVelocityScale,
	                    velocityColor });
	            }
	        }
	    }
	    if (app.graphicsState.rendererSettings.drawNavigationMesh
               && runtime.navigationState == appLifecycle::AppRuntimeState::NavigationState::Ready
               && runtime.navigationProfiles)
	    {
	        app::debugDraw::AppendNavigationGeometry(
                   runtime.navigationDebugGeometry, runtime.scene);
	    }
        if (app.graphicsState.rendererSettings.drawNavigationPathDebug && runtime.gameplayRuntime)
        {
            constexpr std::uint32_t navigationPathColor = 0xffff40a6u;
            constexpr float navigationPathHalfWidth = 0.03f;
            for (const auto& entry : runtime.gameplayRuntime->GetNavigationDebugRegistry().Routes())
            {
                const rendern::GameplayRoute& route = entry.second;
                for (std::size_t index = 0; index + 1u < route.points.size(); ++index)
                {
                    const mathUtils::Vec3 start = route.points[index].worldPosition;
                    const mathUtils::Vec3 end = route.points[index + 1u].worldPosition;
                    
                    runtime.scene.externalDebugLines.push_back({
                        start,
                        end,
                        navigationPathColor});
                    
                    const mathUtils::Vec3 segment = end - start;
                    const mathUtils::Vec3 planarSegment{
                        segment.x,
                        0.0f,
                        segment.z
                    };
                    const float planarLength = mathUtils::Length(planarSegment);
                    if (planarLength <= 1e-5f)
                    {
                        continue;
                    }
                   
                    const mathUtils::Vec3 side{
                        -planarSegment.z / planarLength,
                        0.0f,
                        planarSegment.x / planarLength
                    };
                    const mathUtils::Vec3 offset = side * navigationPathHalfWidth;
                   
                    runtime.scene.externalDebugLines.push_back({
                        start + offset,
                        end + offset,
                        navigationPathColor});
                   
                    runtime.scene.externalDebugLines.push_back({
                        start - offset,
                        end - offset,
                        navigationPathColor});
                }
            }
        }
	    if (app.graphicsState.rendererSettings.drawAIPlannedPathDebug
               && runtime.gameplayRuntime)
	    {
	        constexpr std::uint32_t ordinaryColor = 0xff33ccffu;
	        constexpr std::uint32_t traversalColor = 0xffff66ffu;
	        for (const rendern::GameplayAIPlannedPathDebugAgentView& agent :
                   runtime.gameplayRuntime->BuildAIPlannedPathDebugAgentViews())
	        {
	            for (const rendern::GameplayAIDebugPlannedRouteStep& step :
                       agent.plannedPath.routeSteps)
	            {
	                if (!step.route.has_value())
	                {
	                    continue;
	                }
	                for (std::size_t index = 0; index + 1u < step.route->points.size(); ++index)
	                {
	                    const bool traversal = index < step.route->segmentAnnotations.size()
                               && step.route->segmentAnnotations[index].traversalLink.has_value();
	                    runtime.scene.externalDebugLines.push_back({
                               step.route->points[index].worldPosition,
                               step.route->points[index + 1u].worldPosition,
                               traversal ? traversalColor : ordinaryColor});
	                }
	            }
	        }
	    }
        
        // Steering telemetry shares the existing optional AI debug toggle and line renderer.
        if (app.graphicsState.rendererSettings.drawAIPlannedPathDebug && runtime.gameplayRuntime)
        {
            constexpr std::uint32_t baseColor = 0xffb0b0b0u;
            constexpr std::uint32_t feelerColor = 0xff33ccffu;
            constexpr std::uint32_t blockedColor = 0xff3333ffu;
            constexpr std::uint32_t normalColor = 0xffffff33u;
            constexpr std::uint32_t finalColor = 0xff33ff33u;
            const auto appendProbe = [&](const rendern::GameplayObstacleProbeDebugState& probe)
            {
                if (!probe.queried)
                {
                    return;
                }
                const auto end = probe.request.origin +
                    probe.request.direction * probe.request.maximumDistance;
                runtime.scene.externalDebugLines.push_back({probe.request.origin, end,
                    probe.hit ? blockedColor : feelerColor});
                if (probe.hit)
                {
                    constexpr float marker = 0.08f;
                    runtime.scene.externalDebugLines.push_back({
                        probe.hitPosition + mathUtils::Vec3{-marker, 0.0f, 0.0f},
                        probe.hitPosition + mathUtils::Vec3{marker, 0.0f, 0.0f}, blockedColor});
                    runtime.scene.externalDebugLines.push_back({probe.hitPosition,
                        probe.hitPosition + probe.hitNormal * 0.35f, normalColor});
                }
            };
            for (const auto& entry :
                runtime.gameplayRuntime->GetSteeringDebugRegistry().States())
            {
                const rendern::GameplaySteeringDebugState& state = entry.second;
                const auto& snapshot = state.avoidance;
                if (!snapshot.evaluated)
                {
                    continue;
                }
                const float directionLength = snapshot.forward.request.maximumDistance > 0.0f
                    ? snapshot.forward.request.maximumDistance : 1.0f;
                runtime.scene.externalDebugLines.push_back({snapshot.probeOrigin,
                    snapshot.probeOrigin + snapshot.baseMovement.moveWorld * directionLength,
                    baseColor});
                appendProbe(snapshot.forward);
                appendProbe(snapshot.left);
                appendProbe(snapshot.right);
                runtime.scene.externalDebugLines.push_back({snapshot.probeOrigin,
                    snapshot.probeOrigin + snapshot.finalMovement.moveWorld * directionLength,
                    finalColor});
            }
        }
	}
        
}
