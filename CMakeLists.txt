cmake_minimum_required(VERSION 4.3.0 FATAL_ERROR)

# ------------------------------------------------------------
# Experimental: import std;
# Must be set BEFORE project()/enable_language(CXX)
# ------------------------------------------------------------
option(ENABLE_IMPORT_STD "Enable CMake experimental support for `import std;`" ON)

if(ENABLE_IMPORT_STD)
  set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
          "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
endif()

# Modules dyn-deps (needed for correct build order with Ninja/MSVC)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)

project(CoreEngineModule LANGUAGES C CXX)

message(STATUS "CMake version     = ${CMAKE_VERSION}")
message(STATUS "Generator         = ${CMAKE_GENERATOR}")
message(STATUS "CXX compiler      = ${CMAKE_CXX_COMPILER}")
message(STATUS "CXX compiler ID   = ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CXX compiler ver  = ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "MSVC toolset      = ${MSVC_TOOLSET_VERSION}")
message(STATUS "IMPORT_STD        = ${CMAKE_CXX_COMPILER_IMPORT_STD}")

include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Modules
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

if(ENABLE_IMPORT_STD)
  message(STATUS "CMAKE_CXX_COMPILER_IMPORT_STD=${CMAKE_CXX_COMPILER_IMPORT_STD}")

  if(NOT DEFINED CMAKE_CXX_COMPILER_IMPORT_STD)
    message(FATAL_ERROR
            "Current toolchain does not expose CMAKE_CXX_COMPILER_IMPORT_STD. "
            "Check that Rider uses CMake 4.3.0+, Ninja, and a compatible MSVC toolset.")
  endif()

  if(NOT "23" IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD)
    message(FATAL_ERROR
            "Current C++ toolchain does not provide `import std;` for C++23.")
  endif()
endif()

function(core_enable_import_std target_name)
  if(ENABLE_IMPORT_STD)
    set_property(TARGET ${target_name} PROPERTY CXX_MODULE_STD ON)
  endif()
endfunction()

option(USE_SUBMODULES "Prefer extern/* submodules if present" ON)
option(WITH_TESTS "Build tests" ON)
option(CORE_TEST_SANITIZER_PROFILE "Enable sanitizer-oriented test profile." OFF)
option(CORE_ENABLE_ASAN "Enable AddressSanitizer instrumentation for supported toolchains." OFF)
option(CORE_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer instrumentation for supported toolchains." OFF)

if(CORE_TEST_SANITIZER_PROFILE)
  set(CORE_ENABLE_ASAN ON CACHE BOOL "" FORCE)
  if(MSVC)
    message(STATUS "CORE_TEST_SANITIZER_PROFILE: MSVC toolchain detected; enabling ASan-only profile.")
    set(CORE_ENABLE_UBSAN OFF CACHE BOOL "" FORCE)
  else()
    set(CORE_ENABLE_UBSAN ON CACHE BOOL "" FORCE)
  endif()
endif()

set(CORE_SANITIZERS_ACTIVE FALSE)
set(_core_sanitizer_compile_flags "")
set(_core_sanitizer_link_flags "")

if(CORE_ENABLE_ASAN OR CORE_ENABLE_UBSAN)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    if(CORE_ENABLE_ASAN)
      list(APPEND _core_sanitizer_compile_flags -fsanitize=address -fno-omit-frame-pointer)
      list(APPEND _core_sanitizer_link_flags -fsanitize=address)
    endif()
    if(CORE_ENABLE_UBSAN)
      list(APPEND _core_sanitizer_compile_flags -fsanitize=undefined)
      list(APPEND _core_sanitizer_link_flags -fsanitize=undefined)
    endif()
    set(CORE_SANITIZERS_ACTIVE TRUE)
  elseif(MSVC OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    if(CORE_ENABLE_UBSAN)
      message(WARNING "CORE_ENABLE_UBSAN is not supported on MSVC toolchains; disabling UBSan.")
      set(CORE_ENABLE_UBSAN OFF CACHE BOOL "" FORCE)
    endif()

    if(CORE_ENABLE_ASAN)
      list(APPEND _core_sanitizer_compile_flags /fsanitize=address)
      list(APPEND _core_sanitizer_link_flags /fsanitize=address)
      set(CORE_SANITIZERS_ACTIVE TRUE)
    endif()
  else()
    message(WARNING
            "Sanitizers requested, but compiler '${CMAKE_CXX_COMPILER_ID}' is not supported by this profile. "
            "Sanitizer instrumentation will be disabled.")
    set(CORE_ENABLE_ASAN OFF CACHE BOOL "" FORCE)
    set(CORE_ENABLE_UBSAN OFF CACHE BOOL "" FORCE)
  endif()

  if(CORE_ENABLE_ASAN OR CORE_ENABLE_UBSAN)
    if(NOT WITH_TESTS)
      message(FATAL_ERROR "Sanitizer options require WITH_TESTS=ON so the test target is built.")
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      if(NOT CMAKE_BUILD_TYPE)
        message(WARNING
                "Sanitizer profile is most deterministic with a single-config generator and explicit "
                "CMAKE_BUILD_TYPE=Debug or RelWithDebInfo.")
      endif()
    endif()
  endif()
endif()

if(CORE_SANITIZERS_ACTIVE)
  list(REMOVE_DUPLICATES _core_sanitizer_compile_flags)
  list(REMOVE_DUPLICATES _core_sanitizer_link_flags)

  add_library(core_test_sanitizers INTERFACE)
  target_compile_options(core_test_sanitizers INTERFACE ${_core_sanitizer_compile_flags})
  target_link_options(core_test_sanitizers INTERFACE ${_core_sanitizer_link_flags})

  message(STATUS "Sanitizers active: ASAN=${CORE_ENABLE_ASAN}; UBSAN=${CORE_ENABLE_UBSAN}")
else()
  if(CORE_ENABLE_ASAN OR CORE_ENABLE_UBSAN OR CORE_TEST_SANITIZER_PROFILE)
    message(STATUS "Sanitizers requested but no supported sanitizer flags were activated.")
  endif()
endif()

# ------------------------------------------------------------
# --- Backend switch (cache) ---
# ------------------------------------------------------------
set(CORE_RENDER_BACKEND "GL" CACHE STRING "Render backend: GL or DX12")
set_property(CACHE CORE_RENDER_BACKEND PROPERTY STRINGS "GL" "DX12")

if(CORE_RENDER_BACKEND STREQUAL "DX12" AND NOT WIN32)
  message(FATAL_ERROR "DX12 backend is Windows-only")
endif()

# ------------------------------------------------------------
# EnTT (header-only ECS)
# ------------------------------------------------------------
# We use EnTT only for Level runtime objects at first (hybrid approach).
FetchContent_Declare(entt
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.13.1
)
# Avoid building EnTT tests/docs when fetched as a subproject.
set(ENTT_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENTT_BUILD_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(entt)

# ------------------------------------------------------------
# Jolt Physics
# ------------------------------------------------------------

# Jolt must not override the parent project's compiler and linker settings.
set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)

# Build configuration.
set(JPH_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

# CoreEngine currently uses single precision and its default allocator.
set(DOUBLE_PRECISION OFF CACHE BOOL "" FORCE)
set(CROSS_PLATFORM_DETERMINISTIC OFF CACHE BOOL "" FORCE)
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF CACHE BOOL "" FORCE)
set(CPP_EXCEPTIONS_ENABLED OFF CACHE BOOL "" FORCE)
set(CPP_RTTI_ENABLED OFF CACHE BOOL "" FORCE)
set(DISABLE_CUSTOM_ALLOCATOR OFF CACHE BOOL "" FORCE)

# These features are not needed for the initial CPU physics integration.
set(DEBUG_RENDERER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
set(DEBUG_RENDERER_IN_DISTRIBUTION OFF CACHE BOOL "" FORCE)
set(PROFILER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
set(PROFILER_IN_DISTRIBUTION OFF CACHE BOOL "" FORCE)
set(ENABLE_OBJECT_STREAM OFF CACHE BOOL "" FORCE)

# Jolt 5.6 compute backends are unrelated to our rigid-body and character goals.
set(JPH_USE_DX12 OFF CACHE BOOL "" FORCE)
set(JPH_USE_VK OFF CACHE BOOL "" FORCE)
set(JPH_USE_MTL OFF CACHE BOOL "" FORCE)
set(JPH_USE_CPU_COMPUTE OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG        v5.6.0
        SOURCE_SUBDIR  Build
)

FetchContent_MakeAvailable(JoltPhysics)


# Pin the upstream release and disable its demo, tests, and examples.
set(RECASTNAVIGATION_DEMO OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_TESTS OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(RecastNavigation
        GIT_REPOSITORY https://github.com/recastnavigation/recastnavigation.git
        GIT_TAG        v1.6.0
        GIT_SHALLOW    TRUE)

block()
  # RecastNavigation v1.6.0 declares CMake 3.1.
  # CMake 4.x no longer supports policy compatibility below 3.5.
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
  FetchContent_MakeAvailable(RecastNavigation)
endblock()

if(TARGET Jolt AND NOT TARGET Jolt::Jolt)
  add_library(Jolt::Jolt ALIAS Jolt)
endif()

if(CORE_RENDER_BACKEND STREQUAL "GL")
  # ------------------------------------------------------------
  # GLFW
  # ------------------------------------------------------------
  if(USE_SUBMODULES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/glfw/CMakeLists.txt")
    add_subdirectory(extern/glfw EXCLUDE_FROM_ALL)
  else()
    FetchContent_Declare(glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG        3.4
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
  endif()

  if(TARGET glfw AND NOT TARGET GLFW::glfw)
    add_library(GLFW::glfw ALIAS glfw)
  endif()
endif()

if(CORE_RENDER_BACKEND STREQUAL "GL")
  # ------------------------------------------------------------
  # GLM (header-only)
  # ------------------------------------------------------------
  if(USE_SUBMODULES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/glm/CMakeLists.txt")
    add_subdirectory(extern/glm EXCLUDE_FROM_ALL)
  else()
    FetchContent_Declare(glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG        1.0.1
    )
    FetchContent_MakeAvailable(glm)
  endif()

  if(NOT TARGET glm::glm)
    add_library(glm::glm INTERFACE IMPORTED)
    if(DEFINED glm_SOURCE_DIR AND EXISTS "${glm_SOURCE_DIR}/glm/glm.hpp")
      set_target_properties(glm::glm PROPERTIES
              INTERFACE_INCLUDE_DIRECTORIES "${glm_SOURCE_DIR}"
      )
    else()
      message(FATAL_ERROR "glm::glm target not found and glm_SOURCE_DIR is unknown")
    endif()
  endif()
endif()

# ------------------------------------------------------------
# OpenGL (only needed for GL backend)
# ------------------------------------------------------------
set(OPENGL_TARGET "")
if(CORE_RENDER_BACKEND STREQUAL "GL")
  find_package(OpenGL REQUIRED)
  if(TARGET OpenGL::GL)
    set(OPENGL_TARGET OpenGL::GL)
  elseif(TARGET OpenGL::OpenGL)
    set(OPENGL_TARGET OpenGL::OpenGL)
  else()
    message(FATAL_ERROR "No suitable OpenGL target found (expected OpenGL::GL or OpenGL::OpenGL)")
  endif()
endif()

if(CORE_RENDER_BACKEND STREQUAL "GL")
  # ------------------------------------------------------------
  # GLEW (build/find ALWAYS, link only for GL)
  # ------------------------------------------------------------
  set(GLEW_USE_STATIC_LIBS TRUE)
  find_package(GLEW QUIET)

  set(GLEW_HEADERS_DIR "")

  if(GLEW_FOUND AND TARGET GLEW::GLEW)
    # ok
  else()
    FetchContent_Declare(glew_src
            URL https://github.com/nigels-com/glew/releases/download/glew-2.3.0/glew-2.3.0.tgz
            SOURCE_SUBDIR build/cmake
    )
    set(ONLY_LIBS ON CACHE BOOL "" FORCE)
    set(BUILD_UTILS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glew_src)

    if(TARGET glew_s AND NOT TARGET GLEW::GLEW)
      add_library(GLEW::GLEW ALIAS glew_s)
    elseif(TARGET glew AND NOT TARGET GLEW::GLEW)
      add_library(GLEW::GLEW ALIAS glew)
    elseif(TARGET libglew_static AND NOT TARGET GLEW::GLEW)
      add_library(GLEW::GLEW ALIAS libglew_static)
    elseif(TARGET libglew_shared AND NOT TARGET GLEW::GLEW)
      add_library(GLEW::GLEW ALIAS libglew_shared)
    endif()

    if(DEFINED glew_src_SOURCE_DIR AND EXISTS "${glew_src_SOURCE_DIR}/include/GL/glew.h")
      set(GLEW_HEADERS_DIR "${glew_src_SOURCE_DIR}/include")
    endif()
  endif()

  # If you are in GL backend, GLEW must exist.
  if(CORE_RENDER_BACKEND STREQUAL "GL" AND NOT TARGET GLEW::GLEW)
    message(FATAL_ERROR "GLEW target not available for GL backend.")
  endif()
endif()

# ------------------------------------------------------------
# STB
# ------------------------------------------------------------
FetchContent_Declare(stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG 28d546d5eb77d4585506a20480f4de2e706dff4c
)
FetchContent_MakeAvailable(stb)

add_library(stb_headers INTERFACE)
target_include_directories(stb_headers INTERFACE "${stb_SOURCE_DIR}")

# ------------------------------------------------------------
# Assimp
# ------------------------------------------------------------
FetchContent_Declare(assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v6.0.4
)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT ON CACHE BOOL "" FORCE)

set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(assimp)

if(TARGET assimp AND NOT TARGET assimp::assimp)
  add_library(assimp::assimp ALIAS assimp)
endif()

# ------------------------------------------------------------
# ImGui (DX12 + Win32 only)
# ------------------------------------------------------------
if(WIN32 AND CORE_RENDER_BACKEND STREQUAL "DX12")
  set(IMGUI_DIR "")
  set(_want_imgui_tag "v1.91.0-docking")
  set(_imgui_submodule_dir "${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui")

  # Prefer extern/imgui only if it is the Docking branch.
  # Otherwise you'll get compile errors such as:
  #   ImGuiConfigFlags_DockingEnable not found
  # because upstream master without docking doesn't define it.
  set(_imgui_submodule_is_docking FALSE)
  if(USE_SUBMODULES AND EXISTS "${_imgui_submodule_dir}/imgui.cpp" AND EXISTS "${_imgui_submodule_dir}/imgui.h")
    file(READ "${_imgui_submodule_dir}/imgui.h" _imgui_h_contents)
    string(FIND "${_imgui_h_contents}" "ImGuiConfigFlags_DockingEnable" _dock_flag_pos)
    if(NOT _dock_flag_pos EQUAL -1)
      set(_imgui_submodule_is_docking TRUE)
    endif()
  endif()

  if(_imgui_submodule_is_docking)
    set(IMGUI_DIR "${_imgui_submodule_dir}")
  else()
    if(USE_SUBMODULES AND EXISTS "${_imgui_submodule_dir}/imgui.cpp")
      message(WARNING "extern/imgui is present but NOT the docking branch; ignoring it and fetching ${_want_imgui_tag} instead.")
    endif()
    FetchContent_Declare(imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG        ${_want_imgui_tag}
    )
    FetchContent_MakeAvailable(imgui)
    set(IMGUI_DIR "${imgui_SOURCE_DIR}")
  endif()

  # Hard error if we still didn't get docking.
  if(EXISTS "${IMGUI_DIR}/imgui.h")
    file(READ "${IMGUI_DIR}/imgui.h" _imgui_h_final)
    string(FIND "${_imgui_h_final}" "ImGuiConfigFlags_DockingEnable" _dock_flag_final_pos)
    if(_dock_flag_final_pos EQUAL -1)
      message(FATAL_ERROR "Dear ImGui must be built from the docking branch/tag (e.g. ${_want_imgui_tag}).")
    endif()
  endif()

  add_library(imgui STATIC
          "${IMGUI_DIR}/imgui.cpp"
          "${IMGUI_DIR}/imgui_draw.cpp"
          "${IMGUI_DIR}/imgui_tables.cpp"
          "${IMGUI_DIR}/imgui_widgets.cpp"
          "${IMGUI_DIR}/imgui_demo.cpp"
          "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
          "${IMGUI_DIR}/backends/imgui_impl_dx12.cpp"
  )

  target_include_directories(imgui PUBLIC
          "${IMGUI_DIR}"
          "${IMGUI_DIR}/backends"
  )

  # Also allow imgui.cpp to see a project-level imconfig.h (if you have one in the repo root),
  # so that both the library and the app compile with the same config and struct layouts.
  target_include_directories(imgui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

  # Make sure the application picks the same ImGui headers that the imgui target was built with.
  # Otherwise IMGUI_CHECKVERSION() can assert on mismatched struct layout.
  set(IMGUI_INCLUDE_DIRS "${IMGUI_DIR}" "${IMGUI_DIR}/backends")

  # IMPORTANT: Do NOT set IMGUI_DISABLE_OBSOLETE_FUNCTIONS only for the imgui target.
  # That macro can change public struct layouts (ImGuiIO, etc.) and will trip IMGUI_CHECKVERSION()
  # if the application includes imgui.h without the same macro.
  # Keep defaults. If you ever need it, set it PUBLIC so it propagates to app.

  # Backend needs D3D12/DXGI. (dxguid for IID definitions)
  target_link_libraries(imgui PUBLIC d3d12 dxgi dxguid)
endif()

# ------------------------------------------------------------
# CoreEngineModuleLib
# ------------------------------------------------------------
add_library(CoreEngineModuleLib STATIC)
target_compile_features(CoreEngineModuleLib PUBLIC cxx_std_23)
core_enable_import_std(CoreEngineModuleLib)
if(CORE_SANITIZERS_ACTIVE)
  target_link_libraries(CoreEngineModuleLib PRIVATE core_test_sanitizers)
endif()

target_compile_definitions(CoreEngineModuleLib
        PUBLIC
        $<$<STREQUAL:${CORE_RENDER_BACKEND},DX12>:CORE_USE_DX12=1>
        $<$<STREQUAL:${CORE_RENDER_BACKEND},GL>:CORE_USE_GL=1>
)

# If you vendor d3dx12.h in extern/, allow includes like "d3dx12.h" or "extern/d3dx12.h"
target_include_directories(CoreEngineModuleLib
        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(COMMON_MODULES
        Core/Core.ixx

        Core/Math/MathUtils.cppm
        Core/Math/Geometry.cppm

        Physics/PhysicsTypes.cppm
        Physics/Jolt/JoltShapeFactory.cppm
        Physics/Jolt/JoltBodyRegistry.cppm
        Physics/Jolt/JoltCharacterRegistry.cppm

        Navigation/Navigation.cppm
        App/NavigationDebugDraw.cppm
        App/LevelNavigation.cppm

        Core/Hash/HashUtils.cppm
        Core/Json/JsonUtils.cppm
        Core/IO/FileUtils.cppm

        Core/String/StringUtils.cppm

        Core/ThreadAffinity/ThreadAffinity.cppm


        Input/Input.cppm
        Input/InputCore.cppm
        Input/ControllerBase.cppm
        Input/Win32Input.cppm

        ECS/EnTTHelpers.cppm

        Gameplay/Components/GameplayEntityComponents.cppm
        Gameplay/AI/AIAgentWorldState.cppm
        Gameplay/AI/AIActionContracts.cppm
        Gameplay/AI/GameplayAIComponents.cppm
        Gameplay/Input/GameplayInputComponents.cppm
        Gameplay/Interaction/GameplayInteractionComponents.cppm
        Gameplay/Character/GameplayCharacterComponents.cppm
        Gameplay/Camera/GameplayCameraComponents.cppm
        Gameplay/Action/GameplayActionComponents.cppm
        Gameplay/Input/GameplayInputBindings.cppm
        Gameplay/Animation/GameplayAnimationComponents.cppm
        Gameplay/GameplayWorld.cppm
        Gameplay/GameplayWorld_impl.cppm
        Gameplay/AI/AISystem.cppm
        Gameplay/AI/AIDecisionContracts.cppm
        Gameplay/AI/AIGoalSelection.cppm
        Gameplay/AI/AIActionRuntime.cppm
        Gameplay/AI/AIPlanner.cppm
        Gameplay/AI/AIPlanExecution.cppm
        Gameplay/AI/AIDebugViewModel.cppm
        Gameplay/AI/AIActionBinding.cppm
        Gameplay/AI/AIPlanExecutionBridge.cppm
        Gameplay/AI/AIDecisionRuntime.cppm
        Gameplay/AI/AIActionTask.cppm
        Gameplay/AI/GameplayRequest.cppm
        Gameplay/AI/GameplaySteering.cppm
        Gameplay/AI/GameplayRoute.cppm
        Gameplay/AI/GameplayRouteFollower.cppm
        Gameplay/AI/GameplayRouteSearch.cppm
        Gameplay/Traversal/GameplayTraversalLink.cppm
        Gameplay/Traversal/GameplayTraversalLinkRegistry.cppm
        Gameplay/Traversal/GameplayTraversalExecutor.cppm
        Gameplay/Traversal/GameplayTraversalExecutorRegistry.cppm
        Gameplay/Traversal/DoorTraversalExecutor.cppm
        Gameplay/Traversal/JumpTraversalExecutor.cppm
        Gameplay/AI/AIFollowRouteActionRuntime.cppm
        Gameplay/AI/AIFollowRouteAction.cppm
        Gameplay/AI/AIMoveToAction.cppm
        Gameplay/AI/AIMoveToActionBinding.cppm
        Gameplay/Input/GameplayInput.cppm
        Gameplay/Camera/GameplayFollowCamera.cppm
        Gameplay/Character/CharacterController.cppm
        Gameplay/Character/CharacterMovement.cppm
        Gameplay/Combat/CombatSystem.cppm
        Gameplay/Interaction/InteractionSystem.cppm
        Gameplay/Interaction/GameplayInteractionPoint.cppm
        Gameplay/Interaction/GameplayObjectReservationSystem.cppm
        Gameplay/Events/GameplayWorldEvent.cppm
        Gameplay/Pickup/GameplayPickupSystem.cppm
        Gameplay/Animation/GameplayAnimationBridge.cppm
        Gameplay/Animation/GameplayAnimationBridgeSystem.cppm
        Gameplay/Graph/GameplayGraph.cppm
        Gameplay/Graph/GameplayGraphAssets.cppm
        Gameplay/Runtime/GameplayRuntimeCommon.cppm
        Gameplay/Runtime/GameplayBootstrap.cppm
        Gameplay/Runtime/GameplaySceneSync.cppm
        Gameplay/Runtime/GameplayRuntime.cppm
        Gameplay/AI/GameplayAIDecisionContracts.cppm
        Gameplay/AI/GameplayGOAPDecision.cppm
        Gameplay/AI/GameplayGOAPDefinitionAsset.cppm
        Gameplay/AI/GameplayAIAccessKeyObservation.cppm
        Gameplay/AI/GameplayAIAccessKeyDecision.cppm
        Gameplay/AI/GameplayAIDecision.cppm

        Timer/GameTimer.ixx
        Timer/GameTimer.cppm

        Render/Render.ixx
        Render/RenderCore.cppm
        Render/RenderCommandQueue.cppm
        Render/RenderScheduler.cppm
        Render/RHI.cppm
        Render/RenderGraph.cppm
        Render/Debug/DebugColorUtils.cppm
        Render/Debug/DebugDraw.cppm
        Render/Debug/DebugText.cppm
        Render/Bindless.cppm
        Render/GpuMemory.cppm
        Render/RendererSettings.cppm
        Render/Renderer.cppm
        Render/Frame/RenderFrameView.cppm
        Render/Frame/RenderSceneExtractor.cppm

        Render/Sync.cppm
        Render/FileSystem.cppm

        Render/Shader/ShaderFiles.cppm
        Render/Shader/ShaderSystem.cppm

        Render/Scene/SceneBridge.cppm
        Scene/Scene.cppm
        Level/Level.cppm
        Level/LevelECS.cppm
        Level/LevelECS_impl.cppm
        Editor/EditorSelectionService.cppm
        Editor/EditorSceneState.cppm
        Editor/EditorCommands.cppm
        Editor/EditorGizmo.cppm
        Editor/EditorRotateGizmo.cppm
        Editor/EditorScaleGizmo.cppm

        Editor/Picking.cppm
        Render/Scene/Visibility.cppm

        Render/Scene/CameraController.cppm

        Animation/Runtime/Animator.cppm
        Animation/Runtime/AnimationController.cppm
        Animation/Tooling/AnimationWorkspace.cppm

        Render/Model/ObjLoader.cppm
        Render/Model/AssimpLoader.cppm
        Render/Model/AssimpSceneLoader.cppm
        Render/Model/Mesh/Mesh.cppm
        Animation/Assets/Skeleton.cppm
        Animation/Assets/AnimationClip.cppm
        Render/Model/SkinnedMesh.cppm

        Render/Decoders/TextureDecoderSTB.cppm

        Assets/ResourceManager.ixx
        Assets/ResourceManager_core.cppm
        Assets/ResourceManager_texture.cppm
        Assets/ResourceManager_mesh.cppm
        Assets/AssetManager.cppm
)

set(GL_MODULES
        Render/OpenGL/RenderGL.ixx
        Render/OpenGL/OpenGLCore.cppm
        Render/OpenGL/OpenGLRHI.cppm
        Render/OpenGL/OpenGLRenderer.cppm
)

set(DX12_MODULES
        Render/DirectX12/RenderDX12.ixx
        Render/DirectX12/DirectX12Core.cppm
        Render/DirectX12/DirectX12RHI.cppm
        Render/DirectX12/DirectX12Renderer.cppm
        Render/DirectX12/DebugDrawRendererDX12.cppm
        Render/DirectX12/DebugTextRendererDX12.cppm
        Render/DirectX12/DirectX12TextureUploader.cppm
        Render/DirectX12/CommonDX12Structs.cppm
        Render/ImGui/ImGuiDebugUI.cppm
)

set(CORE_MODULES ${COMMON_MODULES})
if(CORE_RENDER_BACKEND STREQUAL "DX12")
  list(APPEND CORE_MODULES ${DX12_MODULES})
else()
  list(APPEND CORE_MODULES ${GL_MODULES})
endif()
list(REMOVE_DUPLICATES CORE_MODULES)

set(CORE_MODULES_ABS "")
foreach(f IN LISTS CORE_MODULES)
  list(APPEND CORE_MODULES_ABS "${SRC_DIR}/${f}")
endforeach()

target_sources(CoreEngineModuleLib
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES ${CORE_MODULES_ABS}
)

target_sources(CoreEngineModuleLib
        PRIVATE
        "${SRC_DIR}/Core/ThreadAffinity/ThreadAffinity.cpp"
        "${SRC_DIR}/Physics/Jolt/JoltRuntime.cpp"
        "${SRC_DIR}/Physics/Jolt/JoltCollisionLayers.cpp"
        "${SRC_DIR}/Physics/Jolt/JoltPhysicsWorld.cpp"
        "${SRC_DIR}/Physics/LevelPhysicsRuntime.cpp"
        "${SRC_DIR}/Navigation/Recast/Navigation.cpp"
        "${SRC_DIR}/App/Win32AppShell.cpp"
        "${SRC_DIR}/App/DebugUiHost.cpp"

        "${SRC_DIR}/App/Development/AppDevelopmentScenarioRuntime.cpp"
        "${SRC_DIR}/App/Development/DevelopmentScenario.cpp"
        
        "${SRC_DIR}/App/AppArguments.cpp"
        "${SRC_DIR}/App/AppBootstrap.cpp"
        "${SRC_DIR}/App/GameplayPhysicsCharacterIntegration.cpp"
        "${SRC_DIR}/App/AppLifecycle.cpp"
)

# Common deps
target_link_libraries(CoreEngineModuleLib
        PUBLIC
        stb_headers
        assimp::assimp

        PRIVATE
        Jolt::Jolt
        Recast
        Detour
        DebugUtils
)

# GL-only deps (link AFTER CoreEngineModuleLib exists)
if(CORE_RENDER_BACKEND STREQUAL "GL")
  if(GLEW_HEADERS_DIR)
    target_include_directories(CoreEngineModuleLib PUBLIC "${GLEW_HEADERS_DIR}")
  endif()

  target_link_libraries(CoreEngineModuleLib PUBLIC GLFW::glfw glm::glm GLEW::GLEW ${OPENGL_TARGET})

  target_compile_definitions(CoreEngineModuleLib
          PUBLIC
          $<$<TARGET_EXISTS:glew_s>:GLEW_STATIC>
          $<$<TARGET_EXISTS:libglew_static>:GLEW_STATIC>
  )
endif()

# DX12-only deps
if(WIN32 AND CORE_RENDER_BACKEND STREQUAL "DX12")
  target_include_directories(CoreEngineModuleLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/extern")
  target_link_libraries(CoreEngineModuleLib PUBLIC d3d12 dxgi dxguid comdlg32)
  if(TARGET imgui)
    target_link_libraries(CoreEngineModuleLib PUBLIC imgui)
  endif()
endif()

# ------------------------------------------------------------
# App
# ------------------------------------------------------------
add_executable(app src/main.cpp)
target_compile_features(app PRIVATE cxx_std_23)
core_enable_import_std(app)
if(CORE_SANITIZERS_ACTIVE)
  target_link_libraries(app PRIVATE core_test_sanitizers)
endif()

add_custom_command(TARGET app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:app>/assets"
)

target_link_libraries(app PRIVATE CoreEngineModuleLib)

if(CORE_RENDER_BACKEND STREQUAL "DX12")
  # Put ImGui include dirs BEFORE project/extern include dirs to avoid picking up a stray imgui.h from elsewhere.
  target_include_directories(app SYSTEM BEFORE PRIVATE ${IMGUI_INCLUDE_DIRS})
endif()

if(WIN32)
  find_library(D3DCOMPILER_LIB NAMES d3dcompiler_47 d3dcompiler REQUIRED)
  target_link_libraries(CoreEngineModuleLib PRIVATE ${D3DCOMPILER_LIB})
endif()

# EnTT (header-only) include propagation
if(TARGET EnTT::EnTT)
  target_link_libraries(CoreEngineModuleLib PUBLIC EnTT::EnTT)
else()
  # Fallback: just include directory if alias target is not present for some reason.
  if(DEFINED entt_SOURCE_DIR)
    target_include_directories(CoreEngineModuleLib PUBLIC "${entt_SOURCE_DIR}/src")
  endif()
endif()

# ------------------------------------------------------------
# Tests
# ------------------------------------------------------------
if(WITH_TESTS)
  enable_testing()

  if(USE_SUBMODULES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/googletest/CMakeLists.txt")
    add_subdirectory(extern/googletest EXCLUDE_FROM_ALL)
  else()
    FetchContent_Declare(googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()

  add_subdirectory(tests)
endif()
