# Test conventions: see tests/README.md for naming, layout, fixtures, and backend guidance.
add_executable(CoreEngineModuleTests
        "FakeRHI.h" 
        "FakeRHI.cpp"
        "FakeMeshIO.h"
        "FakeTextureIO.h" 
        "TestSupport/MathTestHelper.h"
        "TestSupport/TestFixtureLoader.h"
        "TestSupport/TestFixtureLoader.cpp"
        "TestSupport/TestGlobalVariables.h"
        "TestSupport/TestTempPath.h"
        "TestSupport/GameplayRouteTestHelper.h"
        "unit/InputTests/TestInputCore.cpp" 
        "unit/InputTests/TestControllerBase.cpp" 
        "unit/InputTests/TestCameraController.cpp"
        "unit/InputTests/TestGameplayInputBindings.cpp"
        "unit/Math/TestMathUtils.cpp" 
        "unit/GameplayTests/TestGameplayGraph.cpp" 
        "unit/GameplayTests/TestGameplayWorld.cpp"
        "unit/GameplayTests/TestGameplayInteractionPoint.cpp"
        "unit/GameplayTests/TestGameplayObjectReservationSystem.cpp"
        "unit/GameplayTests/TestGameplayActionPolicy.cpp"
        "unit/GameplayTests/TestGameplayRuntimeModeTransitions.cpp"
        "unit/GameplayTests/TestGameplayRuntimeNodeBoundMappingConsistency.cpp"
        "unit/GameplayTests/Traversal/TestDoorTraversalExecutor.cpp"
        "unit/GameplayTests/Traversal/TestJumpTraversalExecutor.cpp"
        "unit/GameplayTests/AI/TestAISystem.cpp"
        "unit/GameplayTests/AI/TestAIAgentWorldState.cpp"
        "unit/GameplayTests/AI/TestAIDecisionContracts.cpp"
        "unit/GameplayTests/AI/TestAIPlanner.cpp"
        "unit/GameplayTests/AI/TestAIGoalSelection.cpp"
        "unit/GameplayTests/AI/TestAIPlanExecution.cpp"
        "unit/GameplayTests/AI/TestAIActionBinding.cpp"
        "unit/GameplayTests/AI/TestAIDecisionRuntime.cpp"
        "unit/GameplayTests/AI/TestAIPlanExecutionBridge.cpp"
        "unit/GameplayTests/AI/TestAIActionRuntime.cpp"
        "unit/GameplayTests/AI/TestAIActionTask.cpp"
        "unit/GameplayTests/AI/TestAIFollowRouteAction.cpp"
        "unit/GameplayTests/AI/TestAIMoveToAction.cpp"
        "unit/GameplayTests/AI/TestAIMoveToActionBinding.cpp"
        "unit/GameplayTests/AI/TestAIGOAPMoveToVerticalSlice.cpp"
        "unit/GameplayTests/TestGameplayRequest.cpp"
        "unit/GameplayTests/TestGameplayRoute.cpp"
        "unit/GameplayTests/TestGameplaySteering.cpp"
        "unit/GameplayTests/TestGameplayRouteFollower.cpp"
        "unit/GameplayTests/TestGameplayRouteSearch.cpp"
        "unit/GameplayTests/TestGameplayAnimationBridge.cpp"
        "unit/AnimationTests/TestAnimationController.cpp"
        "unit/AnimationTests/TestAnimatorSamplingInvariants.cpp"
        "unit/RenderTests/LevelInstantiateTestHelper.h"
        "unit/RenderTests/TestLevelWorld.cpp"
        "unit/RenderTests/TestLevelInstantiateReferenceIntegrity.cpp"
        "unit/RenderTests/TestAssimpImportContracts.cpp"
        "unit/RenderTests/TestDescriptorBindlessInvariants.cpp"
        "unit/RenderTests/TestGpuStructPacking.cpp"
        "unit/RenderTests/TestRenderCommandQueue.cpp"
        "unit/RenderTests/TestRendererSettingsCommands.cpp"
        "unit/RenderTests/TestRendererSettingsViewModels.cpp"
        "unit/RenderTests/TestInlineRenderScheduler.cpp"
        "unit/RenderTests/TestThreadedRenderScheduler.cpp"
        "unit/ResourceTests/TestTextureStorage.cpp"
        "unit/ResourceTests/TestMeshStorage.cpp"
        "unit/ResourceTests/TestAssetManagerStartupValidation.cpp"
        "unit/TimerTests/TestTimerBasic.cpp"
        "unit/RenderTests/TestRenderGraphFakeRHI.cpp"
        "unit/CoreTests/TestJsonUtils.cpp"
        "unit/CoreTests/TestStringHashUtils.cpp"
        "unit/CoreTests/TestFileUtils.cpp"
        "unit/CoreTests/TestThreadAffinity.cpp"
        "unit/AppTests/TestAppBootstrapArguments.cpp"
        "unit/AppTests/TestAppLifecycleStreamingWorkerCount.cpp"
        "unit/AppTests/TestAppPhysicsStateLifecycle.cpp"
        "unit/AppTests/TestGameplayPhysicsCharacterIntegration.cpp"
        "unit/AppTests/TestAppDevelopmentScenarioRuntime.cpp"
        "unit/AppTests/TestDevelopmentScenario.cpp"
        "unit/EditorTests/TestEditorSelectionService.cpp"
        "unit/GameplayTests/TestAIFollowRouteActionRuntime.cpp"
        "unit/GameplayTests/TestGameplayTraversalLinkRegistry.cpp"
        "unit/GameplayTests/TestGameplayTraversalExecutorRegistry.cpp"
        "unit/GameplayTests/AI/TestAIFollowRouteTraversal.cpp"
        "unit/GameplayTests/TestGameplayAIGOAPAccessKeyDevelopmentScenario.cpp"
        "unit/GameplayTests/TestGameplayCharacterCameraOwnership.cpp"
        "unit/PhysicsTests/TestJoltRuntime.cpp"
        "unit/PhysicsTests/TestPhysicsTypes.cpp"
        "unit/PhysicsTests/TestJoltCollisionLayers.cpp"
        "unit/PhysicsTests/TestJoltPhysicsWorld.cpp"
        "unit/PhysicsTests/TestJoltShapeFactory.cpp"
        "unit/PhysicsTests/TestJoltBodyRegistry.cpp"
        "unit/PhysicsTests/TestJoltCharacterBackend.cpp"
        "unit/PhysicsTests/TestJoltBodyCreation.cpp"
        "unit/PhysicsTests/TestJoltFallingBody.cpp"
        "unit/PhysicsTests/TestJoltBodyMutation.cpp"
        "unit/PhysicsTests/TestLevelPhysicsSerialization.cpp"
        "unit/PhysicsTests/TestLevelPhysicsRuntime.cpp"
        "unit/NavigationTests/TestNavigation.cpp"
        "unit/PhysicsTests/TestJoltRayCast.cpp"
        "unit/PhysicsTests/TestJoltShapeQueries.cpp"
)

target_link_libraries(CoreEngineModuleTests
  PRIVATE
    CoreEngineModuleLib 
    Jolt::Jolt
    GTest::gtest_main
)

if(TARGET core_test_sanitizers)
  target_link_libraries(CoreEngineModuleTests PRIVATE core_test_sanitizers)
endif()

target_compile_definitions(CoreEngineModuleTests
        PRIVATE
        CORE_TEST_FIXTURE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

target_compile_features(CoreEngineModuleTests PRIVATE cxx_std_23)

target_include_directories(CoreEngineModuleTests
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}" 
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

if (WIN32)
  add_custom_command(TARGET CoreEngineModuleTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:CoreEngineModuleTests>
            $<TARGET_FILE_DIR:CoreEngineModuleTests>
    COMMAND_EXPAND_LISTS
  )
endif()

include(GoogleTest)
gtest_discover_tests(CoreEngineModuleTests
  DISCOVERY_MODE PRE_TEST
)

# Physics smoke start
add_executable(CoreEnginePhysicsLevelSmokeTests
        "integration/PhysicsLevelSmoke/TestPhysicsLevelSmoke.cpp"
)

target_link_libraries(CoreEnginePhysicsLevelSmokeTests
        PRIVATE
        CoreEngineModuleLib
        GTest::gtest_main
)

target_compile_features(CoreEnginePhysicsLevelSmokeTests PRIVATE cxx_std_23)
core_enable_import_std(CoreEnginePhysicsLevelSmokeTests)

target_include_directories(CoreEnginePhysicsLevelSmokeTests
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

if(TARGET core_test_sanitizers)
  target_link_libraries(CoreEnginePhysicsLevelSmokeTests PRIVATE core_test_sanitizers)
endif()

add_custom_command(TARGET CoreEnginePhysicsLevelSmokeTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:CoreEnginePhysicsLevelSmokeTests>/assets"
)

if(WIN32)
  add_custom_command(TARGET CoreEnginePhysicsLevelSmokeTests POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_RUNTIME_DLLS:CoreEnginePhysicsLevelSmokeTests>
          $<TARGET_FILE_DIR:CoreEnginePhysicsLevelSmokeTests>
          COMMAND_EXPAND_LISTS
  )
endif()

gtest_discover_tests(CoreEnginePhysicsLevelSmokeTests
        DISCOVERY_MODE PRE_TEST
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:CoreEnginePhysicsLevelSmokeTests>"
        PROPERTIES
        LABELS "physics;smoke;integration"
        TIMEOUT 30
)
#Physics smoke end

# Character physics smoke start
add_executable(CoreEngineCharacterPhysicsSmokeTests
        "integration/CharacterPhysicsSmoke/TestCharacterPhysicsSmoke.cpp"
)

target_link_libraries(CoreEngineCharacterPhysicsSmokeTests
        PRIVATE
        CoreEngineModuleLib
        GTest::gtest_main
)

target_compile_features(CoreEngineCharacterPhysicsSmokeTests PRIVATE cxx_std_23)
core_enable_import_std(CoreEngineCharacterPhysicsSmokeTests)

target_include_directories(CoreEngineCharacterPhysicsSmokeTests
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

if(TARGET core_test_sanitizers)
  target_link_libraries(CoreEngineCharacterPhysicsSmokeTests PRIVATE core_test_sanitizers)
endif()

add_custom_command(TARGET CoreEngineCharacterPhysicsSmokeTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:CoreEngineCharacterPhysicsSmokeTests>/assets"
)

if(WIN32)
  add_custom_command(TARGET CoreEngineCharacterPhysicsSmokeTests POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_RUNTIME_DLLS:CoreEngineCharacterPhysicsSmokeTests>
          $<TARGET_FILE_DIR:CoreEngineCharacterPhysicsSmokeTests>
          COMMAND_EXPAND_LISTS
  )
endif()

gtest_discover_tests(CoreEngineCharacterPhysicsSmokeTests
        DISCOVERY_MODE PRE_TEST
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:CoreEngineCharacterPhysicsSmokeTests>"
        PROPERTIES
        LABELS "physics;character;smoke;integration"
        TIMEOUT 30
)
# Character physics smoke end

# AI physics smoke start
add_executable(CoreEngineAIPhysicsSmokeTests
        "integration/AIPhysicsSmoke/TestAIPhysicsSmoke.cpp"
)

target_link_libraries(CoreEngineAIPhysicsSmokeTests
        PRIVATE
        CoreEngineModuleLib
        GTest::gtest_main
)

target_compile_features(CoreEngineAIPhysicsSmokeTests PRIVATE cxx_std_23)
core_enable_import_std(CoreEngineAIPhysicsSmokeTests)

target_include_directories(CoreEngineAIPhysicsSmokeTests
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

if(TARGET core_test_sanitizers)
  target_link_libraries(CoreEngineAIPhysicsSmokeTests PRIVATE core_test_sanitizers)
endif()

add_custom_command(TARGET CoreEngineAIPhysicsSmokeTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:CoreEngineAIPhysicsSmokeTests>/assets"
)

if(WIN32)
  add_custom_command(TARGET CoreEngineAIPhysicsSmokeTests POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_RUNTIME_DLLS:CoreEngineAIPhysicsSmokeTests>
          $<TARGET_FILE_DIR:CoreEngineAIPhysicsSmokeTests>
          COMMAND_EXPAND_LISTS
  )
endif()

gtest_discover_tests(CoreEngineAIPhysicsSmokeTests
        DISCOVERY_MODE PRE_TEST
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:CoreEngineAIPhysicsSmokeTests>"
        PROPERTIES
        LABELS "physics;character;ai;smoke;integration"
        TIMEOUT 30
)
# AI physics smoke end

if(WIN32 AND CORE_RENDER_BACKEND STREQUAL "DX12")
  add_executable(CoreEngineModuleDx12SmokeTests
          "integration/Dx12Smoke/TestDx12BackendSmoke.cpp"
  )

  target_link_libraries(CoreEngineModuleDx12SmokeTests
          PRIVATE
          CoreEngineModuleLib
          GTest::gtest_main
  )

  target_compile_features(CoreEngineModuleDx12SmokeTests PRIVATE cxx_std_23)
  core_enable_import_std(CoreEngineModuleDx12SmokeTests)

  target_include_directories(CoreEngineModuleDx12SmokeTests
          PRIVATE
          "${CMAKE_CURRENT_SOURCE_DIR}"
          "${CMAKE_CURRENT_SOURCE_DIR}/../src"
  )

  if(TARGET core_test_sanitizers)
    target_link_libraries(CoreEngineModuleDx12SmokeTests PRIVATE core_test_sanitizers)
  endif()

  add_custom_command(TARGET CoreEngineModuleDx12SmokeTests POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_RUNTIME_DLLS:CoreEngineModuleDx12SmokeTests>
          $<TARGET_FILE_DIR:CoreEngineModuleDx12SmokeTests>
          COMMAND_EXPAND_LISTS
  )

  gtest_discover_tests(CoreEngineModuleDx12SmokeTests
          DISCOVERY_MODE PRE_TEST
          PROPERTIES
          LABELS "dx12;smoke"
  )
else()
  message(STATUS "CoreEngineModuleDx12SmokeTests not registered: requires WIN32 and CORE_RENDER_BACKEND=DX12.")
endif()

# ------------------------------------------------------------
# Jolt dependency smoke test
# ------------------------------------------------------------

add_executable(CoreEngineJoltSmokeTests
        "integration/JoltSmoke/TestJoltDependencySmoke.cpp"
)

target_link_libraries(CoreEngineJoltSmokeTests
        PRIVATE
        Jolt::Jolt
        GTest::gtest_main
)

target_compile_features(CoreEngineJoltSmokeTests
        PRIVATE
        cxx_std_23
)

if(TARGET core_test_sanitizers)
  target_link_libraries(CoreEngineJoltSmokeTests
          PRIVATE
          core_test_sanitizers
  )
endif()

include(GoogleTest)

gtest_discover_tests(CoreEngineJoltSmokeTests
        DISCOVERY_MODE PRE_TEST
        PROPERTIES
        LABELS "jolt;smoke"
)
