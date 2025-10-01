cmake_minimum_required(VERSION 3.16)
project(fork-eater VERSION 1.0.0 LANGUAGES CXX C)

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
    src/Logger.cpp
    src/Settings.cpp
    src/ShaderManager.cpp
    src/FileWatcher.cpp
    src/ShaderEditor.cpp
    src/PreviewPanel.cpp
    src/MenuSystem.cpp
    src/LeftPanel.cpp
    src/FileManager.cpp
    src/Timeline.cpp
    src/ShortcutManager.cpp
    src/ShaderProject.cpp
    src/ShaderTemplates.cpp
    src/Framebuffer.cpp
    src/ShaderPreprocessor.cpp
)

# GLAD library
add_library(glad external/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external)

# --- Embed Shader Templates ---
file(GLOB_RECURSE TEMPLATE_FILES "templates/*")
set(GENERATED_TEMPLATES_HEADER ${CMAKE_CURRENT_BINARY_DIR}/GeneratedShaderTemplates.h)

set(GENERATED_CONTENT "#pragma once\n#include <map>\n#include <string>\n#include <utility>\n\n")
set(GENERATED_CONTENT "${GENERATED_CONTENT}namespace EmbeddedTemplates {\n")
set(GENERATED_CONTENT "${GENERATED_CONTENT}std::map<std::string, std::pair<const char*, size_t>> g_templates;\n")
set(GENERATED_CONTENT "${GENERATED_CONTENT}void initialize() {\n")

foreach(TEMPLATE_FILE ${TEMPLATE_FILES})
    file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${TEMPLATE_FILE})
    file(READ ${TEMPLATE_FILE} FILE_CONTENT HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," FILE_CONTENT ${FILE_CONTENT})

    string(REPLACE "/" "_" VAR_NAME ${REL_PATH})
    string(REPLACE "." "_" VAR_NAME ${VAR_NAME})
    string(TOUPPER ${VAR_NAME} VAR_NAME)

    set(GENERATED_CONTENT "${GENERATED_CONTENT}    static const unsigned char ${VAR_NAME}[] = { ${FILE_CONTENT} };\n")
    set(GENERATED_CONTENT "${GENERATED_CONTENT}    g_templates[\"${REL_PATH}\"] = { reinterpret_cast<const char*>(${VAR_NAME}), sizeof(${VAR_NAME}) };\n")
endforeach()

set(GENERATED_CONTENT "${GENERATED_CONTENT}}\n")
set(GENERATED_CONTENT "${GENERATED_CONTENT}}\n")

file(WRITE ${GENERATED_TEMPLATES_HEADER} "${GENERATED_CONTENT}")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
# --- End Embed Shader Templates ---

# --- Embed Shader Libraries ---
file(GLOB_RECURSE LIB_FILES "libs/*")
set(GENERATED_LIBS_HEADER ${CMAKE_CURRENT_BINARY_DIR}/GeneratedShaderLibraries.h)

set(GENERATED_LIB_CONTENT "#pragma once\n#include <map>\n#include <string>\n#include <utility>\n\n")
set(GENERATED_LIB_CONTENT "${GENERATED_LIB_CONTENT}namespace EmbeddedLibraries {\n")
set(GENERATED_LIB_CONTENT "${GENERATED_LIB_CONTENT}std::map<std::string, std::pair<const char*, size_t>> g_libs;\n")
set(GENERATED_LIB_CONTENT "${GENERATED_LIB_CONTENT}void initialize() {\n")

foreach(LIB_FILE ${LIB_FILES})
    file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs ${LIB_FILE})
    file(READ ${LIB_FILE} FILE_CONTENT HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," FILE_CONTENT ${FILE_CONTENT})

    string(REPLACE "/" "_" VAR_NAME ${REL_PATH})
    string(REPLACE "." "_" VAR_NAME ${VAR_NAME})
    string(TOUPPER ${VAR_NAME} VAR_NAME)

    set(GENERATED_LIB_CONTENT "${GENERATED_LIB_CONTENT}    static const unsigned char ${VAR_NAME}[] = { ${FILE_CONTENT} };\n")
    set(GENERATED_LIB_CONTENT "${GENERATED_LIB_CONTENT}    g_libs[\"${REL_PATH}\"] = { reinterpret_cast<const char*>(${VAR_NAME}), sizeof(${VAR_NAME}) };\n")
endforeach()

set(GENERATED_LIB_CONTENT "${GENERATED_LIB_CONTENT}}\n")
set(GENERATED_LIB_CONTENT "${GENERATED_LIB_CONTENT}}\n")

file(WRITE ${GENERATED_LIBS_HEADER} "${GENERATED_LIB_CONTENT}")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
# --- End Embed Shader Libraries ---


# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    glad
    ${GLFW_LIBRARIES}
    ${OPENGL_LIBRARIES}
    GL
    -ldl
    -lpthread
)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE ${GLFW_CFLAGS_OTHER})

# Copy shaders to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/templates DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/project DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)