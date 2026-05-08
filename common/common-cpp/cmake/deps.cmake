# ============================================================================
# FetchContent declarations for common-cpp's source-buildable dependencies.
# ============================================================================
include(FetchContent)

# Make FetchContent quieter and more deterministic.
set(FETCHCONTENT_QUIET FALSE)

# ----------------------------------------------------------------------------
# GLFW — windowing
# ----------------------------------------------------------------------------
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)

# ----------------------------------------------------------------------------
# GLM — math
# ----------------------------------------------------------------------------
set(GLM_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_LIBRARY  OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)

# ----------------------------------------------------------------------------
# spdlog — logging
# ----------------------------------------------------------------------------
set(SPDLOG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL        OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
)

# ----------------------------------------------------------------------------
# nlohmann::json — serialization
# ----------------------------------------------------------------------------
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

# ----------------------------------------------------------------------------
# Vulkan Memory Allocator — VkBuffer / VkImage allocation
# ----------------------------------------------------------------------------
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        v3.1.0
    GIT_SHALLOW    TRUE
)

# ----------------------------------------------------------------------------
# shaderc — runtime GLSL → SPIR-V compilation (required by hot-reload)
# ----------------------------------------------------------------------------
# shaderc has a complex dep tree (glslang, SPIRV-Tools, SPIRV-Headers).
# Disable building its own tests and tools to keep build time reasonable.
set(SHADERC_SKIP_TESTS    ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_INSTALL  ON CACHE BOOL "" FORCE)
set(SHADERC_ENABLE_SHARED_CRT OFF CACHE BOOL "" FORCE)
# Don't let shaderc/glslang treat their own warnings as errors. Our top-level
# add_compile_options enables -Wshadow / -Wold-style-cast project-wide; those
# trip on shaderc's own sources (string_piece.h, ShaderLang.h, etc.). Turning
# off shaderc's -Werror lets those stay as warnings instead of build failures.
set(SHADERC_ENABLE_WERROR_COMPILE OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    shaderc
    GIT_REPOSITORY https://github.com/google/shaderc.git
    GIT_TAG        v2024.3
    GIT_SHALLOW    TRUE
)

# ----------------------------------------------------------------------------
# Dear ImGui — UI
# ----------------------------------------------------------------------------
# ImGui doesn't ship a CMakeLists. We FetchContent it then add a custom target
# in cmake/imgui.cmake.
FetchContent_Declare(
    imgui_src
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5-docking
    GIT_SHALLOW    TRUE
)

# ----------------------------------------------------------------------------
# Make all available
# ----------------------------------------------------------------------------
FetchContent_MakeAvailable(
    glfw
    glm
    spdlog
    nlohmann_json
    vma
    imgui_src
)

# shaderc requires its dependencies to be present at FetchContent_MakeAvailable
# time; populate them explicitly.
include(FetchContent)
FetchContent_GetProperties(shaderc)
if(NOT shaderc_POPULATED)
    FetchContent_Populate(shaderc)
    # shaderc bundles a script that pulls glslang, SPIRV-Tools, SPIRV-Headers
    # via its own utility. We invoke it before adding shaderc's CMake.
    if(EXISTS "${shaderc_SOURCE_DIR}/utils/git-sync-deps")
        find_package(Python3 COMPONENTS Interpreter REQUIRED)
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" utils/git-sync-deps
            WORKING_DIRECTORY "${shaderc_SOURCE_DIR}"
            RESULT_VARIABLE _shaderc_sync_result
            ERROR_VARIABLE  _shaderc_sync_err
        )
        if(NOT _shaderc_sync_result EQUAL 0)
            message(FATAL_ERROR "shaderc git-sync-deps failed: ${_shaderc_sync_err}\nRun it manually with:\n    cd build/_deps/shaderc-src && python3 utils/git-sync-deps\nthen re-run cmake.")
        endif()
    endif()
    # Suppress SPIRV-Tools / SPIRV-Headers install/export logic — we never
    # install them. Without these, glslang's install(EXPORT glslang-targets)
    # references SPIRV-Tools-opt which isn't in any export set, and CMake's
    # Generate step fails.
    set(SKIP_SPIRV_TOOLS_INSTALL    ON CACHE BOOL "" FORCE)
    set(SPIRV_HEADERS_SKIP_INSTALL  ON CACHE BOOL "" FORCE)
    set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)

    # Workaround for shaderc v2024.3: third_party/CMakeLists.txt does
    #     set(GLSLANG_ENABLE_INSTALL $<NOT:${SKIP_GLSLANG_INSTALL}>)
    # That is a normal (non-CACHE) set whose value is a generator expression;
    # generator expressions are NOT evaluated by configure-time if(), so the
    # literal string "$<NOT:ON>" reaches glslang and is treated as truthy by
    # if(GLSLANG_ENABLE_INSTALL). The directory-scope set also shadows our
    # GLSLANG_ENABLE_INSTALL cache override. Strip the offending line so
    # glslang's option() reads the cache (we set GLSLANG_ENABLE_INSTALL=OFF
    # below).
    set(GLSLANG_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(_shaderc_tp_cml "${shaderc_SOURCE_DIR}/third_party/CMakeLists.txt")
    file(READ "${_shaderc_tp_cml}" _shaderc_tp_text)
    string(REPLACE
        "set(GLSLANG_ENABLE_INSTALL $<NOT:\${SKIP_GLSLANG_INSTALL}>)"
        "# Patched out by gpu_sims/common-cpp/cmake/deps.cmake: shaderc's\n    # generator-expression-based set is broken under direct if() checks;\n    # GLSLANG_ENABLE_INSTALL is taken from the cache instead."
        _shaderc_tp_text "${_shaderc_tp_text}")
    file(WRITE "${_shaderc_tp_cml}" "${_shaderc_tp_text}")

    add_subdirectory("${shaderc_SOURCE_DIR}" "${shaderc_BINARY_DIR}" EXCLUDE_FROM_ALL)
endif()

# Provide the namespaced alias the main CMakeLists expects.
if(TARGET shaderc AND NOT TARGET shaderc::shaderc)
    add_library(shaderc::shaderc ALIAS shaderc)
endif()

# Custom ImGui target.
include(cmake/imgui.cmake)

# VMA wrapper target (header-only-ish library; needs one .cpp with the impl).
include(cmake/vma.cmake)
