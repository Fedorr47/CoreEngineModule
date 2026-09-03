cmake_minimum_required(VERSION 3.20)

# Also supports a dependency-free configure for architecture-only CI jobs.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(CoreEngineArchitectureChecks LANGUAGES NONE)
    enable_testing()
endif()

find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)

add_test(NAME AI.ImportBoundaries
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/check_ai_dependencies.py")
add_test(NAME AI.AssetComposition
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/check_goap_asset_composition.py")
set_tests_properties(AI.ImportBoundaries AI.AssetComposition PROPERTIES
    LABELS "ai;architecture"
    TIMEOUT 60)
