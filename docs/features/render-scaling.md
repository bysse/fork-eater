# Render Scaling Feature Improvements

## Problem Statement

The existing render scaling feature is not functioning as expected. Specifically, the following issues have been identified:

1.  **Incorrect Chunk Snippet Injection:** The chunk rendering logic (uniforms and discard statements) is being injected into fragment shaders even when the `RenderScaleMode` is set to `Resolution`, leading to unintended visual artifacts and incorrect behavior.
2.  **Non-Functional Chunk-Based Scaling:** The chunk-based scaling mode, when selected, does not apply its intended effects. This is likely a direct consequence of the incorrect injection logic mentioned above.
3.  **Compiler Errors (Intermittent during development):** During the process of addressing the above issues, compiler errors arose due to incorrect parameter passing in shader loading/reloading functions and issues with shader library include path resolution.

## Intention for Changes

The primary goal of these modifications is to ensure the render scaling feature functions correctly and reliably across both "Resolution" and "Chunk" modes, while maintaining code integrity and resolving compilation issues.

### Key Intentions:

1.  **Conditional Chunk Logic Injection:**
    *   **Modification:** The `ShaderPreprocessor::preprocess` method will be updated to receive the current `RenderScaleMode`.
    *   **Outcome:** Chunk rendering uniforms (`u_progressive_fill`, `u_render_phase`, `u_renderChunkFactor`, `u_time_offset`) and the associated `discard` logic will *only* be injected into fragment shaders when `RenderScaleMode::Chunk` is actively selected. This will prevent erroneous behavior in "Resolution" mode.

2.  **Dynamic Shader Reloading on Mode Change:**
    *   **Modification:** A new callback mechanism (`onRenderScaleModeChanged`) will be introduced in the `Settings` class, which will be triggered whenever the `RenderScaleMode` is altered.
    *   **Integration:** The `ShaderEditor` will subscribe to this callback. Upon a mode change, it will clear all currently loaded shaders via `ShaderManager::clearShaders()` and then reload the entire project's shaders via `ShaderProject::loadShadersIntoManager()`, ensuring the new `RenderScaleMode` is passed down the pipeline for correct preprocessing.
    *   **Outcome:** Shaders will be dynamically recompiled with the correct (or absent) chunk rendering logic instantly when the render scale mode is changed in the UI, providing a seamless and accurate user experience.

3.  **Consistent `RenderScaleMode` Propagation:**
    *   **Modification:** The `RenderScaleMode` parameter will be consistently added to the signatures of `ShaderManager::loadShader()` and `ShaderManager::reloadShader()`.
    *   **Call Site Updates:** All call sites for `loadShader()` and `reloadShader()` across `ShaderProject`, `FileManager`, and `ParameterPanel` will be updated to pass the current `RenderScaleMode` from the `Settings` singleton.
    *   **Outcome:** The `ShaderPreprocessor` will always receive the accurate `RenderScaleMode` when processing shaders, ensuring that the conditional injection logic functions as intended.

4.  **Robust Shader Library Include Resolution:**
    *   **Modification:** The `ShaderPreprocessor::preprocessRecursive` method will be refined to correctly resolve include paths for shader libraries, particularly those referenced with `lib/` prefixes (e.g., `#pragma include("lib/utils.glsl")`). This will involve ensuring the preprocessor correctly utilizes the `EmbeddedLibraries` namespace to find embedded shader libraries.
    *   **Specific Fix:** The `test_project/shaders/test_lib.frag` include directive will be updated from `lib/utils.glsl` to ensure it correctly points to the embedded library resource.
    *   **Outcome:** All shader `#pragma include` directives, especially for shared libraries, will be resolved correctly, eliminating "file not found" errors during shader compilation.