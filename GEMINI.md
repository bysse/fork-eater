# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview

Fork Eater is a real-time OpenGL/GLSL shader editor built with C++, GLFW, and Dear ImGui. It provides hot reloading capabilities and a multi-pane interface for creating and editing vertex and fragment shaders with live preview.

## Development Commands

### Build System
```bash
# Full build (clones dependencies if needed)
./build.sh

# Manual build process
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

### Running the Application
```bash
# Run from project root
./run.sh

# Or run directly from build directory
cd build && ./fork-eater

# Test mode (for CI/automated testing)
./fork-eater --test [exit_code]

# Dump framebuffer to image
./fork-eater --dump-framebuffer <pass_name> <output_path.png>

# Help command
./fork-eater --help
```

### Testing
```bash
# Run comprehensive exit functionality tests
./test_exit.sh
```
All test runs must be done with the `--test` flag set, so that the process exits.

## Architecture

### Core Components

The application follows a modular architecture with these key classes:

- **Application**: Main application class handling GLFW window, OpenGL context, and ImGui initialization
- **ShaderManager**: Handles OpenGL shader compilation, linking, uniform management, and program lifecycle
- **FileWatcher**: Linux inotify-based file monitoring for hot reloading of shader files
- **ShaderEditor**: Main GUI coordinator that orchestrates all UI components
- **ShaderPreprocessor**: Handles `#pragma include` directives to allow for modular shader code.

### GUI System (Dear ImGui-based)

The interface is composed of specialized panel components:

- **PreviewPanel**: Real-time shader rendering viewport
- **MenuSystem**: File menu and main menu bar
- **LeftPanel**: Shader list, file browser, and shader management
- **ErrorPanel**: Compilation error display and logging
- **Timeline**: Time-based controls for shader animations
- **ShortcutManager**: Keyboard shortcut handling and help system
- **ParameterPanel**: Dynamically generated panel for controlling shader uniforms.

### Shader Pipeline

Shaders use OpenGL 3.3 Core Profile GLSL. The system automatically provides these uniforms:
- `u_time`: Time in seconds since startup
- `u_resolution`: Viewport resolution as vec2

### File Structure

```
├── src/               # Main application source
│   ├── main.cpp       # Application entry and GLFW setup
│   └── [Component].cpp # Individual GUI and system components
├── include/           # Header files for all components
├── shaders/           # GLSL shader files (.vert/.frag)
├── external/          # Third-party dependencies (ImGui cloned here)
└── build/             # CMake build output
```

## Dependencies

### Required System Packages
```bash
# Ubuntu/Debian
sudo apt install build-essential cmake pkg-config libglfw3-dev libgl1-mesa-dev

# Fedora/RHEL
sudo dnf install gcc-c++ cmake pkg-config glfw-devel mesa-libGL-devel

# Arch Linux
sudo pacman -S base-devel cmake pkg-config glfw-x11 mesa
```

### External Libraries
- **Dear ImGui**: Automatically cloned to `external/imgui/` by build script
- **GLFW**: System package for windowing and OpenGL context
- **OpenGL 3.3+**: Graphics API for shader execution

## Key Implementation Details

### Hot Reloading
The FileWatcher class uses Linux inotify to monitor shader files for changes. When a file is modified, it triggers automatic recompilation through ShaderManager callbacks.

### Shader Compilation
ShaderManager handles the complete OpenGL shader pipeline:
1. File reading and preprocessing
2. Vertex/fragment shader compilation
3. Program linking and validation
4. Uniform location caching
5. Error reporting with line numbers

### GUI Layout
The interface uses ImGui docking to create a flexible multi-pane layout. The main layout is managed in ShaderEditor::renderMainLayout() with configurable panel sizes.

### Exit Handling
The application supports multiple exit methods:
- ESC key for immediate exit
- Window close button
- File menu exit option
- Command-line test modes with custom exit codes

### Shader Libraries

The project supports embedding shader libraries into the binary. These libraries can be included in other shaders using the `#pragma include(filename)` directive. The `CMakeLists.txt` file handles the embedding of all files in the `libs/` directory. The `ShaderPreprocessor` class then handles the inclusion of these libraries.

### Shader structure
A shader must reside in its own directory with a manifest file named "4k-eater". The manifest includes the shader's name, lists all shader passes with their source shaders, the timeline length, and a BPM value to scale the timeline for music beat synchronization.

### Uniform Management

The application automatically detects non-system uniforms (float, vec2, vec3, vec4) in fragment shaders and displays corresponding sliders in the `ParameterPanel`. The values of these uniforms are automatically saved to a `uniforms.json` file in the project directory and are reloaded when the project is opened.

## Development Notes

### Adding New Shader Uniforms
Extend `ShaderManager::setUniform()` methods to support additional uniform types. The system currently supports float, int, bool, and vector types.

### Cross-Platform Support
FileWatcher currently requires Linux with inotify. To add Windows/macOS support, implement platform-specific file watching backends in the FileWatcher class.

### GUI Extensions
New interface panels can be added by:
1. Creating a new class similar to existing panels
2. Adding it to ShaderEditor's member variables
3. Integrating its render method in renderMainLayout()
4. Setting up any necessary callbacks in setupCallbacks()

### Shader Examples
Basic shaders are provided in the `templates/` directory. The system expects paired `.vert` and `.frag` files following OpenGL 3.3 Core Profile syntax.

A raymarching example with a depth of field pass is available in `templates/raymarching`.

## Documentation

All project documentation is now organized in the `docs/` directory, with `docs/README.md` serving as the central hub. Feature-specific documentation can be found in `docs/features/`.

When a new feature is implemented or an existing one is modified, its corresponding documentation file in `docs/features/` must be updated to reflect the changes. If a new feature is added, a new Markdown file should be created in `docs/features/` and linked from `docs/README.md`.

## Shader Project Structure

### Project Manifest (4k-eater file)

Shader projects are organized using a JSON manifest file called `4k-eater` (no extension) in the project root. This file defines:

```json
{
  "name": "Project Name",
  "description": "Project description",
  "version": "1.0",
  "timelineLength": 120.0,
  "bpm": 128.0,
  "beatsPerBar": 4,
  "passes": [
    {
      "name": "main",
      "vertexShader": "basic.vert",
      "fragmentShader": "colorful.frag",
      "enabled": true
    }
  ]
}
```

**Manifest Properties:**
- `name`: Human-readable project name
- `description`: Project description
- `version`: Project version (semantic versioning)
- `timelineLength`: Duration in seconds for timeline/animation
- `bpm`: Beats per minute for music synchronization
- `beatsPerBar`: Musical time signature (typically 4)
- `passes`: Array of shader rendering passes

**Pass Properties:**
- `name`: Unique pass identifier (used in ShaderManager)
- `vertexShader`: Path to vertex shader file (relative to shaders/ directory)
- `fragmentShader`: Path to fragment shader file (relative to shaders/ directory)
- `enabled`: Boolean to enable/disable the pass
- `inputs`: (Optional) Array of input textures for multi-pass rendering
- `output`: (Optional) Output buffer name for multi-pass rendering

### Project Directory Structure

```
project-name/
├── 4k-eater              # Project manifest (JSON, no extension)
├── shaders/              # GLSL shader files
│   ├── basic.vert       # Vertex shaders (.vert)
│   ├── colorful.frag    # Fragment shaders (.frag)
│   └── effect.frag
└── assets/              # Additional project assets
    └── textures/
```

### Shader File Rules

**Vertex Shaders (.vert):**
- Must use OpenGL 3.3 Core Profile (`#version 330 core`)
- Standard vertex attributes:
  - `layout (location = 0) in vec2 aPos;` - Vertex position
  - `layout (location = 1) in vec2 aTexCoord;` - Texture coordinates
- Must output `vec2 TexCoord;` for fragment shader

**Fragment Shaders (.frag):**
- Must use OpenGL 3.3 Core Profile (`#version 330 core`)
- Automatic uniforms provided by system:
  - `uniform float u_time;` - Time in seconds since startup
  - `uniform vec2 u_resolution;` - Viewport resolution
- Must output `out vec4 FragColor;` for final color
- Input `in vec2 TexCoord;` from vertex shader
- Can use `gl_FragCoord.xy / u_resolution.xy` for normalized coordinates

### Music Synchronization

The project manifest supports music synchronization through:
- `bpm`: Beats per minute for timing calculations
- `beatsPerBar`: Time signature (usually 4)
- Timeline calculations available in ShaderProjectManifest:
  - `getBeatsPerSecond()`: Convert BPM to beats/sec
  - `secondsToBeats(seconds)`: Convert time to beat count
  - `beatsToSeconds(beats)`: Convert beats to time

### Multi-Pass Rendering

Shader passes can be chained for complex effects:
1. Each pass renders to a texture or screen
2. Subsequent passes can use previous outputs as input textures
3. Pass execution order follows array sequence in manifest
4. Use `enabled: false` to temporarily disable passes

### Creating New Projects

Use ShaderProject class methods:
- `createNew(projectPath, name)`: Initialize new project with defaults
- `loadFromDirectory(projectPath)`: Load existing project
- `saveToDirectory(projectPath)`: Save project manifest
- `loadShadersIntoManager(shaderManager)`: Load all enabled passes into renderer
