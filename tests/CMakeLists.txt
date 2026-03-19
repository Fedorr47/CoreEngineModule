add_executable(CoreEngineModuleTests
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
)

target_link_libraries(CoreEngineModuleTests
  PRIVATE
    CoreEngineModuleLib
    GTest::gtest_main
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