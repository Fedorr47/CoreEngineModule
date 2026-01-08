add_executable(ResourceManagerTests
  "TestTextureStorage.cpp")

target_link_libraries(ResourceManagerTests
  PRIVATE
    ResourceManagerLib
    GTest::gtest_main
)

target_compile_features(ResourceManagerTests PRIVATE cxx_std_23)
set_property(TARGET ResourceManagerTests PROPERTY CXX_SCAN_FOR_MODULES ON)

include(GoogleTest)
gtest_discover_tests(ResourceManagerTests)