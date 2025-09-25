# Fork Eater

A real-time OpenGL/GLSL shader editor with hot reloading capabilities built with C++, GLFW, and Dear ImGui.

## Features

- **Real-time shader editing** with syntax-highlighted text editor
- **Hot reloading** - shaders automatically recompile when files change
- **Multi-pane interface** with dockable windows:
  - Shader list and management
  - Text editor for vertex and fragment shaders
  - Compilation log with error reporting
  - Live preview window
- **File watching** using Linux inotify for automatic recompilation
- **Cross-platform** design (currently Linux-focused)

## Prerequisites

Before building, make sure you have the following dependencies installed:

### Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential cmake pkg-config
sudo apt install libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev
```

### Fedora/RHEL:
```bash
sudo dnf install gcc-c++ cmake pkg-config
sudo dnf install glfw-devel mesa-libGL-devel mesa-libGLU-devel
```

### Arch Linux:
```bash
sudo pacman -S base-devel cmake pkg-config
sudo pacman -S glfw-x11 mesa glu
```

## Building

1. **Clone Dear ImGui** (required for the GUI):
```bash
cd external
git clone https://github.com/ocornut/imgui.git
```

2. **Create build directory and configure**:
```bash
mkdir build && cd build
cmake ..
```

3. **Build the project**:
```bash
make -j$(nproc)
```

4. **Run the shader editor**:
```bash
./fork-eater
```

## Project Structure

```
fork-eater/
├── build/              # Build output directory
├── external/           # Third-party dependencies
│   └── imgui/         # Dear ImGui library (clone here)
├── include/           # Header files
│   ├── ShaderManager.h
│   ├── FileWatcher.h
│   └── ShaderEditor.h
├── shaders/           # GLSL shader files
│   ├── basic.vert
│   └── basic.frag
├── src/               # Source files
│   ├── main.cpp
│   ├── ShaderManager.cpp
│   ├── FileWatcher.cpp
│   └── ShaderEditor.cpp
├── CMakeLists.txt     # Build configuration
├── build.sh          # Build script
├── run.sh            # Run script
└── README.md
```

## Usage

### Basic Operations

1. **Launch the editor**: Run `./fork-eater` from the build directory
2. **Create new shader**: Use "File → New Shader" or click "New Shader" in the shader list
3. **Edit shaders**: Select a shader from the list to edit vertex and fragment code
4. **Live preview**: Changes are reflected in real-time in the preview window
5. **Save shaders**: Use "File → Save" or the "Save" button in the editor

### Interface Windows

- **Shader List**: Shows all loaded shaders, click to select/edit
- **Shader Editor**: Text editor for vertex and fragment shader code
- **Compile Log**: Shows compilation results, errors, and warnings
- **Preview**: Real-time rendering of the current shader

### Hot Reloading

When "Auto Reload" is enabled (default), the editor monitors shader files for changes and automatically recompiles them. This allows you to use external text editors while seeing changes in real-time.

### Keyboard Shortcuts

- `Ctrl+N`: Create new shader
- `Ctrl+S`: Save current shader
- `Escape`: Exit application
- `F1`: Toggle ImGui demo window (for learning the interface)

## Shader Format

Shaders use OpenGL 3.3 Core Profile GLSL:

### Vertex Shader Template
```glsl
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
```

### Fragment Shader Template
```glsl
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform float u_time;
uniform vec2 u_resolution;

void main()
{
    vec2 uv = TexCoord;
    vec3 col = 0.5 + 0.5 * cos(u_time + uv.xyx + vec3(0, 2, 4));
    FragColor = vec4(col*0.0, 1.0);
}
```

### Available Uniforms

The editor automatically provides these uniforms to fragment shaders:
- `u_time`: Time in seconds since startup
- `u_resolution`: Viewport resolution as vec2

## Architecture

### Core Components

1. **ShaderManager**: Handles OpenGL shader compilation, linking, and management
2. **FileWatcher**: Uses Linux inotify to monitor file changes for hot reloading  
3. **ShaderEditor**: Dear ImGui-based interface for editing and preview
4. **Application**: SDL2 window management and main loop

### Dependencies

- **GLFW**: Lightweight cross-platform windowing and OpenGL context management
- **OpenGL 3.3+**: Graphics rendering and shader support
- **Dear ImGui**: Immediate mode GUI for the editor interface
- **Linux inotify**: File system monitoring for hot reloading

## Development

### Adding New Features

The codebase is structured for easy extension:

- Add new shader uniform types in `ShaderManager::setUniform()`
- Extend file watching to other platforms by implementing platform-specific backends in `FileWatcher`
- Add new GUI panels by creating methods in `ShaderEditor`
- Support additional shader types by extending `ShaderManager`

### Building Debug Version

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

### Common Issues

1. **ImGui not found**: Make sure to clone ImGui into `external/imgui/`
2. **GLFW linking errors**: Install GLFW development packages for your distribution
3. **OpenGL context errors**: Ensure you have working OpenGL drivers
4. **File watching not working**: FileWatcher currently requires Linux with inotify support

## License

This project is provided as-is for educational and development purposes. 

Dear ImGui is licensed under the MIT License.
GLFW is licensed under the zlib License.

## Contributing

Feel free to submit issues and pull requests. Areas for improvement:
- Cross-platform file watching (Windows, macOS)
- Better text editor with syntax highlighting
- Shader uniform GUI controls
- Export/import functionality
- Additional shader examples