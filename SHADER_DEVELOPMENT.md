# Shader Development Guide

This guide provides detailed information on the advanced features available for shader development in Fork Eater, including preprocessor directives and the shader development workflow.

## Preprocessor Directives (`#pragma`)

Fork Eater's shader preprocessor supports special `#pragma` directives to enhance shader modularity and control.

### Including Files: `#pragma include()`

You can include other GLSL files into your shader source, which is useful for creating reusable utility libraries.

#### Standard Includes

To include a file relative to the current shader's directory, use:

```glsl
#pragma include("my_utils.glsl")
```

#### Library Includes

Fork Eater supports a special `lib/` directory within your shader project for shared library files. To include a file from this directory, use the `lib/` prefix:

```glsl
#pragma include("lib/hg.glsl")
```

**Feature:** If a library file (e.g., `hg.glsl`, `iq.glsl`) is requested and does not exist in the project's local `lib/` directory, the application will automatically extract it from its embedded collection of common shader libraries and place it there for you.

### Feature Toggles: `#pragma switch()`

You can define conditional compilation flags directly from your shader code that will appear as on/off toggles in the "Parameters" panel of the UI.

**Usage:**

```glsl
#pragma switch(USE_FANCY_EFFECT)
```

This directive does two things:
1.  It creates a checkbox toggle labeled "USE_FANCY_EFFECT" in the Parameters panel.
2.  When the toggle is enabled, the preprocessor will inject `#define USE_FANCY_EFFECT` at the top of your shader source code before compilation.

You can then use this define in your shader code for conditional logic:

```glsl
#ifdef USE_FANCY_EFFECT
    // Apply the fancy effect
    color = pow(color, vec3(2.2));
#else
    // Normal rendering
    color = color;
#endif
```

Toggling the switch in the UI will trigger an automatic shader reload with the define either present or absent.

## System Uniforms

Fork Eater provides several built-in uniforms that are automatically available in your fragment shaders. For consistency with different shader sources, both `u_` (Unreal-style) and `i_` (Shadertoy-style) prefixes are supported.

- `uniform float u_time;` / `uniform float iTime;`
  - The current time in seconds since the application started.

- `uniform vec2 u_resolution;` / `uniform vec2 iResolution;`
  - The resolution of the viewport in pixels.

- `uniform vec4 u_mouse;` / `uniform vec4 iMouse;`
  - The mouse coordinates, normalized to the range [0, 1].
  - `.xy`: The normalized mouse coordinates (from bottom-left).
  - `.z`: 1.0 if the left mouse button is pressed, 0.0 otherwise.
  - `.w`: 1.0 if the right mouse button is pressed, 0.0 otherwise. (Currently `.w` is the same as `.z`)

## Development Workflow

Here is a typical workflow for creating and developing a new shader project from scratch.

### 1. Create a New Project

Open a terminal in the Fork Eater project root and use the `--new` command-line flag to create a new shader project. You can optionally specify a template with `-t`.

```bash
# Create a new project named "MyCoolShader" using the "simple" template
./run.sh --new project/MyCoolShader -t simple
```

This will create a new directory `project/MyCoolShader` containing a `4k-eater.project` manifest and a `shaders/` subdirectory with the template files.

### 2. Open the Project

Launch Fork Eater and provide the path to your new project directory.

```bash
./run.sh project/MyCoolShader
```

The application will open, load your shader, and you should see the output in the preview panel.

### 3. Edit and Hot-Reload

Open the `.frag` or `.vert` files from your new project's `shaders/` directory in your favorite text editor.

Make any changes to the GLSL code and save the file. Fork Eater's file watcher will detect the change and automatically recompile the shader, instantly updating the preview. Any compilation errors will be shown in the error panel.

### 4. Tweak Parameters

If your shader uses custom uniforms (e.g., `uniform float myValue;`) or `#pragma switch()` directives, they will appear in the **Parameters** panel on the right.

- **Uniforms:** Adjust the sliders to tweak values in real-time.
- **Switches:** Toggle the checkboxes to enable or disable features. This will trigger a shader reload.

This iterative loop of coding in your editor and tweaking parameters in the UI allows for a fast and efficient shader development experience.
