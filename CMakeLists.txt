cmake_minimum_required(VERSION 3.16)
project(fork-eater VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)

# Find GLFW
pkg_check_modules(GLFW REQUIRED glfw3)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
include_directories(${GLFW_INCLUDE_DIRS})

# Dear ImGui source files
set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

# Project source files
set(SOURCES
    src/main.cpp
    src/ShaderManager.cpp
    src/FileWatcher.cpp
    src/ShaderEditor.cpp
    src/PreviewPanel.cpp
    src/MenuSystem.cpp
    src/LeftPanel.cpp
    src/FileManager.cpp
    src/ErrorPanel.cpp
    src/Timeline.cpp
    src/ShortcutManager.cpp
    src/ShaderProject.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    ${GLFW_LIBRARIES}
    ${OPENGL_LIBRARIES}
    GL
    -ldl
    -lpthread
)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE ${GLFW_CFLAGS_OTHER})

# Copy shaders to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)