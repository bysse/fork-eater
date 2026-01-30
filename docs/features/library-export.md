# Bundled Library Export

Fork Eater comes with several bundled GLSL libraries that can be included in your shaders using `#pragma include <filename.glsl>`. The Library Export feature allows you to copy these bundled libraries into your project's `libs/` folder for offline use, sharing, or customization.

## Usage

You can export libraries using the `--export-libs` command-line flag.

### Exporting for a new project

When creating a new project, add the `--export-libs` flag:

```bash
./fork-eater --new my-new-project --export-libs
```

This will create the project and immediately export all bundled libraries to `my-new-project/libs/`.

### Exporting for an existing project

To export libraries to an existing project:

```bash
./fork-eater path/to/project --export-libs
```

## Bundled Libraries

The following libraries are typically included:

*   `camera.glsl`: Raymarching camera and view setup utilities.
*   `hash.glsl`: Various hashing and noise primitive functions.
*   `hg.glsl`: Mercury (HG) SDF library for geometry primitives and operations.
*   `iq.glsl`: Inigo Quilez's standard SDF primitives and utilities.
*   `noise.glsl`: Simplex and Value noise functions.
*   `utils.glsl`: Common GLSL utility functions (rotation, lerping, etc.).

## Overriding Bundled Libraries

Once libraries are exported to your project's `libs/` folder, you can modify them. Fork Eater's preprocessor prioritizes local files. If you use `#pragma include "libs/camera.glsl"`, it will use your local modified version instead of the bundled one.
