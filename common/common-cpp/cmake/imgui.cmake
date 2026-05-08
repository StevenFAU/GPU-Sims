# ============================================================================
# Custom CMake target for Dear ImGui.
#
# ImGui ships sources but no CMakeLists. We pick the docking-branch sources
# we need (core + GLFW backend + Vulkan backend) and build a single static lib.
# ============================================================================

set(IMGUI_DIR "${imgui_src_SOURCE_DIR}")

if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
    message(FATAL_ERROR "ImGui sources not found at ${IMGUI_DIR}")
endif()

add_library(imgui STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
)

target_include_directories(imgui PUBLIC
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
)

target_link_libraries(imgui PUBLIC
    glfw
    Vulkan::Vulkan
)

# Use ImGui's own VK loader functions; we have prototypes available.
target_compile_definitions(imgui PUBLIC
    IMGUI_DEFINE_MATH_OPERATORS
)
