# Shader Pragmas and Parameters

Fork Eater supports several custom `#pragma` directives to control how shader parameters (uniforms) and feature toggles are displayed in the UI.

## Support Pragmas

### Range Control

#### Named Range
`#pragma range(uniformName, min, max [, "label"])`
Sets the slider range for a specific uniform by name, with an optional display label.
*   `uniformName`: The name of the uniform.
*   `min`, `max`: Floating point values for the slider range.
*   `label` (Optional): The display name used in the Parameter Panel.

#### Positional Range
`#pragma range(min, max [, "label"])`
Sets the slider range for the *very next* uniform declared in the shader, with an optional display label.
*   `min`, `max`: Floating point values for the slider range.
*   `label` (Optional): The display name used in the Parameter Panel.

### Feature Toggles and Parameters

#### Switch (Boolean toggle)
`#pragma switch(NAME [, defaultValue] [, "offLabel" [, "onLabel"]])`
Creates a checkbox in the UI. When enabled, `#define NAME` is injected into the shader before compilation.
*   `NAME`: The preprocessor macro name.
*   `defaultValue` (Optional): `true`, `false`, `on`, or `off`. Defaults to `false`.
*   `offLabel` (Optional): The label displayed when the switch is **OFF**.
*   `onLabel` (Optional): The label displayed when the switch is **ON**. If not provided, `offLabel` is used for both states.

#### Slider (Compile-time Integer)
`#pragma slider(NAME, min, max [, "label"])`
Creates an **integer** slider in the UI. When the value changes, `#define NAME <value>` is injected into the shader, triggering a recompilation.
*   `NAME`: The preprocessor macro name.
*   `min`, `max`: **Integer** values for the slider range.
*   `label` (Optional): The display name used in the Parameter Panel.

### Labels and Grouping

#### Label
`#pragma label(NAME, "Display Label")`
Sets a display label for a uniform or compile-time parameter by name.

#### Grouping
`#pragma group("Group Name")`
Starts a new group in the Parameter Panel. All uniforms and parameters declared after this pragma will be indented under the group header until an `endgroup` pragma is encountered.

`#pragma endgroup`
Ends the current group.

## Examples

```glsl
#pragma group("Animation Settings")

// Switch with state-dependent labels
#pragma switch(USE_RED, true, "Red Channel: Disabled", "Red Channel: ENABLED")

// Compile-time integer slider
#pragma slider(ITERATIONS, 1, 10, "Quality Level")

// Uniform with explicit range and label
#pragma range(u_speed, 0.1, 5.0, "Animation Speed")
uniform float u_speed;

#pragma endgroup
```