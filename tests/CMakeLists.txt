# Test conventions: see tests/README.md for naming, layout, fixtures, and backend guidance.
add_executable(CoreEngineModuleTests
        "FakeRHI.h" 
        "FakeRHI.cpp"
        "FakeTextureIO.h" 
        "unit/Math/MathTestHelper.h" 
        "unit/InputTests/TestInputCore.cpp" 
        "unit/InputTests/TestControllerBase.cpp" 
        "unit/InputTests/TestCameraController.cpp" 
        "unit/Math/TestMathUtils.cpp" 
        "unit/GameplayTests/TestGameplayGraph.cpp" 
        "unit/GameplayTests/TestGameplayWorld.cpp" 
        "unit/AnimationTests/TestAnimationController.cpp" 
        "unit/RenderTests/TestLevelWorld.cpp" 
        "unit/ResourceTests/TestTextureStorage.cpp" 
        "unit/TimerTests/TestTimerBasic.cpp"
        "unit/RenderTests/TestRenderGraphFakeRHI.cpp"
        "unit/CoreTests/TestJsonUtils.cpp"
        "unit/CoreTests/TestStringHashUtils.cpp"
        "unit/CoreTests/TestFileUtils.cpp"
        "TestSupport/TestFixtureLoader.h"
        "TestSupport/TestFixtureLoader.cpp"
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