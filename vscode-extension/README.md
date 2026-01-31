# Fork Eater VSCode Extension

A dedicated Language Server Protocol (LSP) extension for **Fork Eater** shader development in Visual Studio Code. This extension provides intelligent features for editing `.glsl`, `.vert`, and `.frag` files, tailored specifically for the Fork Eater environment.

## Features

### 1. Intelligent Auto-Completion

The extension understands the Fork Eater runtime environment and provides context-aware completions:

*   **System Uniforms:** Automatically suggests standard uniforms provided by the Fork Eater engine:
    *   `u_time`, `u_resolution`, `u_mouse`, `TexCoord`, `FragColor`
    *   Shadertoy compatibility aliases (`iTime`, `iResolution`, `iMouse`)
*   **Library Functions:** Uses a build-time index of system libraries (e.g., `hash.glsl`, `noise.glsl`) to suggest available functions without including the actual shader files in the extension.
*   **Uniform Declarations:** Snippets for quickly declaring standard uniforms (e.g., typing `uniform` suggests `float u_time;`).

### 2. Pragma Include Support

Enhanced support for Fork Eater's modular shader system:

*   **Path Completion:** Auto-completes paths inside `#pragma include(...)` directives.
*   **Context Aware:** Scans for includable files in:
    *   System libraries (`lib/...`) - Powered by build-time index.
    *   Current directory
    *   Project `shaders/` directory (if inside a Fork Eater project)
    *   Project `libs/` directory

### 3. Project Awareness

*   Detects `4k-eater.project` files to understand the project root.
*   Resolves relative paths correctly based on the project structure.

## Installation (Development)

This extension is currently part of the Fork Eater monorepo. To run it locally:

1.  **Prerequisites:**
    *   Node.js and npm installed.
    *   VS Code installed.

2.  **Build:**
    Navigate to the `vscode-extension` directory.

    **Option A: Using Make (Recommended)**
    This will automatically generate the library index, compile the client and server.
    ```bash
    make
    ```

    **Option B: Manual NPM Build**
    If you don't have `make` or want to run steps manually:
    ```bash
    # Generate library index (Required for completion features)
    node scripts/generate-libs-index.js

    # Install and Compile
    npm install
    npm run compile
    ```
    (Note: The `postinstall` script should automatically install dependencies for both `client` and `server` folders).

3.  **Run/Debug:**
    *   Open the `vscode-extension` folder in VS Code.
    *   Press `F5` to launch a new "Extension Development Host" window.
    *   Open any `.frag` or `.vert` file in the new window to test the features.

## Architecture

The extension follows the standard VS Code Client/Server architecture:

*   **Client (`client/`):** Runs in the VS Code extension host. It handles the activation and launches the language server.
*   **Server (`server/`):** Runs as a separate process (Node.js). It handles the heavy lifting of parsing, code analysis, and providing LSP responses (completions, etc.).
    *   **Library Indexing:** Instead of shipping GLSL files, a build script (`scripts/generate-libs-index.js`) creates a `libs-index.json` containing all available functions and files. This index is bundled with the server.

## Supported Files

*   `.glsl`
*   `.vert`
*   `.frag`
