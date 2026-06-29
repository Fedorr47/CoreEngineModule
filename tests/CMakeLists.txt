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
        "unit/InputTests/TestInputCore.cpp" 
        "unit/InputTests/TestControllerBase.cpp" 
        "unit/InputTests/TestCameraController.cpp" 
        "unit/Math/TestMathUtils.cpp" 
        "unit/GameplayTests/TestGameplayGraph.cpp" 
        "unit/GameplayTests/TestGameplayWorld.cpp"
        "unit/GameplayTests/TestGameplayActionPolicy.cpp"
        "unit/GameplayTests/TestGameplayRuntimeModeTransitions.cpp"
        "unit/GameplayTests/TestGameplayRuntimeNodeBoundMappingConsistency.cpp"
        "unit/AnimationTests/TestAnimationController.cpp"
        "unit/AnimationTests/TestAnimatorSamplingInvariants.cpp"
        "unit/RenderTests/LevelInstantiateTestHelper.h"
        "unit/RenderTests/TestLevelWorld.cpp"
        "unit/RenderTests/TestLevelInstantiateReferenceIntegrity.cpp"
        "unit/RenderTests/TestAssimpImportContracts.cpp"
        "unit/RenderTests/TestDescriptorBindlessInvariants.cpp"
        "unit/RenderTests/TestGpuStructPacking.cpp"
        "unit/RenderTests/TestRendererSettingsCommands.cpp"
        "unit/RenderTests/TestRendererSettingsViewModels.cpp"
        "unit/ResourceTests/TestTextureStorage.cpp"
        "unit/ResourceTests/TestMeshStorage.cpp"
        "unit/ResourceTests/TestAssetManagerStartupValidation.cpp"
        "unit/TimerTests/TestTimerBasic.cpp"
        "unit/RenderTests/TestRenderGraphFakeRHI.cpp"
        "unit/CoreTests/TestJsonUtils.cpp"
        "unit/CoreTests/TestStringHashUtils.cpp"
        "unit/CoreTests/TestFileUtils.cpp"
        "unit/AppTests/TestAppBootstrapArguments.cpp"
        "unit/AppTests/TestAppLifecycleStreamingWorkerCount.cpp"
        "unit/EditorTests/TestEditorSelectionService.cpp"
)

target_link_libraries(CoreEngineModuleTests
  PRIVATE
    CoreEngineModuleLib
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