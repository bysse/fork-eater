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
#pragma switch(USE_FANCY_EFFECT, true, "Fancy Effect: OFF", "Fancy Effect: ON")
```

**Parameters:**
1.  **NAME**: The preprocessor macro name (e.g., `USE_FANCY_EFFECT`).
2.  **Default Value** (Optional): `true`, `false`, `on`, or `off`. Defaults to `false`.
3.  **Off Label** (Optional): Label displayed when the switch is disabled.
4.  **On Label** (Optional): Label displayed when the switch is enabled.

When the toggle is enabled, the preprocessor will inject `#define NAME` at the top of your shader source code before compilation. Toggling the switch triggers an automatic shader reload.

### Compile-time Parameters: `#pragma slider()`

For parameters that must be known at compile-time (such as loop bounds, array sizes, or constant values), you can use `#pragma slider()`. Sliders are **integer-only**.

**Usage:**

```glsl
#pragma slider(ITERATIONS, 1, 10, "Quality Level")
```

**Parameters:**
1.  **NAME**: The preprocessor macro name.
2.  **Min/Max**: The integer range for the UI slider.
3.  **Label** (Optional): A quoted string for the UI display.

Changing this slider will inject `#define NAME <value>` into the shader and trigger a recompilation.

### Uniform Range Control: `#pragma range()`

Control the range and display name of `uniform` variables in the UI.

**Usage:**

```glsl
// Positional: applies to the very next uniform
#pragma range(0.0, 1.0, "Effect Strength")
uniform float u_strength;

// Named: applies to a specific uniform by name
#pragma range(u_density, 0.0, 100.0, "Fog Density")
uniform float u_density;
```

### Labels: `#pragma label()`

Sets a display label for any uniform, switch, or slider by name. This is useful for keeping labels separate from range or switch definitions.

**Usage:**

```glsl
#pragma label(u_color, "Base Surface Color")
uniform vec3 u_color;
```

### Grouping Parameters: `#pragma group()`

You can group related parameters (uniforms and switches) in the UI using `#pragma group()`.

**Usage:**

```glsl
#pragma group("Color Settings")
uniform vec3 u_color;
uniform float u_intensity;
#pragma endgroup()

#pragma group("Effect Settings")
#pragma switch(ENABLE_NOISE)
uniform float u_noiseScale;
#pragma endgroup()
```

Parameters defined between `#pragma group("Name")` and `#pragma endgroup()` will be visually grouped under the specified label in the Parameters panel.

## System Uniforms

Fork Eater provides several built-in uniforms that are automatically available in your fragment shaders. For consistency with different shader sources, both `u_` (Unreal-style) and `i_` (Shadertoy-style) prefixes are supported.

- `uniform float u_time;` / `uniform float iTime;`
  - The current time in seconds since the application started.

- `uniform vec2 u_resolution;` / `uniform vec2 iResolution;`
  - The resolution of the viewport in pixels.

- `uniform vec4 u_mouse;` / `uniform vec4 iMouse;`
  - The mouse coordinates.
  - `u_mouse`:
    - `.xy`: The normalized mouse coordinates in the range **[-1, 1]** (Center is (0,0), Top-Right is (1,1)).
    - `.z`: 1.0 if the left mouse button is pressed, 0.0 otherwise.
    - `.w`: 0.0 (Reserved).
    - **Note**: These values are only updated when interacting with the preview panel.
  - `iMouse`:
    - `.xy`: Mouse position in **pixels** (Shadertoy compatible).
    - `.zw`: Click/Drag information (Shadertoy compatible).

- `uniform vec2 u_mouse_rel;`
  - Relative, integrated mouse coordinates.
  - Useful for infinite scrolling or navigation.
  - Values are normalized in the range **[-1, 1]** and wrap around.
  - Only updates when clicking and dragging in the preview panel.

## Development Workflow

Here is a typical workflow for creating and developing a new shader project from scratch.

### Creating a New Project

Open a terminal in the Fork Eater project root and use the `--new` command-line flag to create a new shader project. You can optionally specify a template with `-t`.

```bash
./run.sh --new project/MyCoolShader -t simple
```

You can also export all bundled GLSL libraries to your project's `libs/` folder using the `--export-libs` flag:

```bash
./run.sh --new project/MyCoolShader --export-libs
```

This is useful for seeing how the libraries work or if you want to modify them for your specific project.

### Project Structure

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
