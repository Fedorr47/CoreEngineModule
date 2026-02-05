add_executable(CoreEngineModuleTests
  TestInputCore.cpp
  TestControllerBase.cpp
  TestCameraController.cpp
)

target_link_libraries(CoreEngineModuleTests
  PRIVATE
    CoreEngineModuleLib
    GTest::gtest_main
)

target_compile_features(CoreEngineModuleTests PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(CoreEngineModuleTests)
