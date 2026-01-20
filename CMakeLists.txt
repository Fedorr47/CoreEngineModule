cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

# Gate MUST be set before project()/enable_language(CXX)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
  "d0edc3af-4c50-42ea-a356-e2862fe7a444")

project(CoreEngineModule LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable `import std;` support for targets created after this line
set(CMAKE_CXX_MODULE_STD 1)

option(WITH_TESTS "Build tests" ON)

# ---------------------------------------------
# Fallback: submodules / FetchContent
# ---------------------------------------------

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/glfw/CMakeLists.txt")
  add_subdirectory(extern/glfw)
else()
  FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
  )
  FetchContent_MakeAvailable(glfw)
endif()

# GLM (header-only)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/glm/CMakeLists.txt")
  add_subdirectory(extern/glm)
else()
  FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
  )
  FetchContent_MakeAvailable(glm)
endif()

# GLEW
# NOTE: upstream git repo does not necessarily contain generated headers.
# Using release tarball guarantees include/GL/*.h exist.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/glew/build/cmake/CMakeLists.txt"
    AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/glew/include/GL/wglew.h")
  set(ONLY_LIBS  ON CACHE BOOL "" FORCE)
  set(BUILD_UTILS OFF CACHE BOOL "" FORCE)
  add_subdirectory(extern/glew/build/cmake)
else()	
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
  
  FetchContent_Declare(glew_src
    URL https://github.com/nigels-com/glew/releases/download/glew-2.3.0/glew-2.3.0.tgz
    SOURCE_SUBDIR build/cmake
  )		
  set(ONLY_LIBS   ON  CACHE BOOL "" FORCE)
  set(BUILD_UTILS OFF CACHE BOOL "" FORCE)
  
  FetchContent_MakeAvailable(glew_src)
endif()

add_library(CoreEngineModuleLib STATIC)

target_compile_features(CoreEngineModuleLib PUBLIC cxx_std_23)

set_property(TARGET CoreEngineModuleLib PROPERTY CXX_SCAN_FOR_MODULES ON)

target_sources(CoreEngineModuleLib
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES
      BASE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/src
      FILES
        src/Core.ixx

        src/Timer/GameTimer.ixx
        src/Timer/GameTimer.cppm

        src/Render/Render.ixx
        src/Render/RenderCore.cppm
        src/Render/RHI.cppm
        src/Render/RenderGraph.cppm
        src/Render/Bindless.cppm
        src/Render/GpuMemory.cppm
        src/Render/Renderer.cppm
        src/Render/SceneBridge.cppm
        src/Render/ShaderSystem.cppm
        src/Render/Sync.cppm
        src/Render/OpenGL/OpenGLRHI.cppm

        src/Assets/ResourceManager.ixx
        src/Assets/ResourceManager_core.cppm
        src/Assets/ResourceManager_texture.cppm

  PRIVATE
    src/Timer/GameTimer_impl.cpp
)

add_executable(app
  src/main.cpp
)

set(GLEW_INCLUDE_DIR "")

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/glew/include/GL/glew.h")
  set(GLEW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/glew/include")
elseif (DEFINED glew_src_SOURCE_DIR AND EXISTS "${glew_src_SOURCE_DIR}/include/GL/glew.h")
  set(GLEW_INCLUDE_DIR "${glew_src_SOURCE_DIR}/include")
endif()

target_include_directories(CoreEngineModuleLib PUBLIC "${GLEW_INCLUDE_DIR}")

target_compile_features(app PRIVATE cxx_std_23)
set_property(TARGET app PROPERTY CXX_SCAN_FOR_MODULES ON)

target_link_libraries(app 
    PUBLIC 
        ${GLFW_TARGET}
        ${GLM_TARGET} 
    PRIVATE 
        CoreEngineModuleLib)

if (WITH_TESTS)
  enable_testing()
  add_subdirectory(extern/googletest)
  add_subdirectory(tests)
endif()
