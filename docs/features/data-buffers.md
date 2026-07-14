# Data Buffer Support

Fork Eater supports passing large structured datasets to shaders through the project manifest. These buffers can be defined in-line or loaded from external CSV files, and are automatically bound as either Texture Buffer Objects (TBOs), Uniform Buffer Objects (UBOs), or Uniform Arrays.

## Manifest Configuration

Buffers are defined in the `buffers` section of the `4k-eater` manifest file (located in your project root):

```json
{
  "name": "My Project",
  "buffers": [
    {
      "name": "u_colors",
      "type": "vec3",
      "file": "assets/palette.csv"
    },
    {
      "name": "u_config",
      "type": "float",
      "data": [1.0, 0.5, 0.2]
    },
    {
      "name": "u_shared_data",
      "type": "ubo",
      "dataType": "vec4",
      "file": "assets/data.csv"
    }
  ],
  "passes": [ ... ]
}
```

### Buffer Properties

- **`name`**: (String) The uniform or block name in the shader (e.g., `u_colors`).
- **`type`**: (String) Defines the buffer type:
  - Set to `float`, `vec2`, `vec3`, or `vec4` to create a **Texture Buffer Object (TBO)** (or Uniform Array).
  - Set to `ubo` to create a **Uniform Buffer Object (UBO)**.
- **`dataType`**: (String, Optional) Only used when `type` is `"ubo"`. Specifies the underlying data format of the elements inside the UBO (`float`, `vec2`, `vec3`, `vec4`). Defaults to `"float"`.
- **`striped`**: (Boolean, Optional) If `true`, indicates the source data is in planar / Structure of Arrays (SoA) layout (e.g. all X's, then all Y's, etc.) instead of interleaved Array of Structures (AoS) layout. 
- **`fixedPoint`**: (String, Optional) If specified, quantizes the floating-point values in the buffer to a fixed-point integer format using Q-notation (e.g., `"Q3.12"` or `"UQ4.12"`). Stored as compact integer arrays during export, and dynamically unpacked back to `float` on-the-fly via the generated C-header helper function.
- **`export`**: (Object, Optional) Configures the binary export options for the buffer (e.g., format, output path, coordinate striping, byte striping).
- **`file`**: (Optional, String) Path to an external CSV or whitespace-separated data file (relative to project root).
- **`data`**: (Optional, Array) In-line array of floating-point values.

> [!NOTE]
> If both `file` and `data` are provided, the `file` takes precedence.

> [!TIP]
> **Why Striped Data?** In demoscene 4K intros, storing coordinates/vectors in planar/SoA layout on disk or in the final executable enables significantly higher compression ratios under LZMA, Crinkler, or kpack, compared to interleaved AoS format.

---

## Exporting Buffer Headers & Striped Unpacking

To support production and build baking pipelines, Fork Eater provides a CLI option to export any manifest buffer (by its 0-based index in the `buffers` array) to a size-optimized C/C++ header:

```bash
./build/fork-eater <project_path> --export-buffer-header 0 -o build/buffer_corners.h
```

### Unpacking Function for Striped UBOs

If a buffer has `"striped": true` in the manifest, the static array inside the generated header remains tightly-packed and striped to preserve maximum compression. 

To bridge the gap to OpenGL UBOs at runtime, the generated header automatically provides a size-optimized, zero-dependency `static inline` helper function that reconstructs and pads the data:

```cpp
static inline const float* unpack_buffer_0();
```

This function:
1. Declares a persistent static destination array matching the exact size and alignment requirements of `std140` (every element aligned to a 16-byte boundary).
2. Interleaves the striped components into the destination array on the fly.
3. Fills in any necessary alignment padding with `0.0f`.
4. Returns a pointer to the unpacked array, ready to be passed directly to `glBufferData` or `glNamedBufferData`.

#### Example Generated Header Output

For a striped `vec2` buffer named `u_corners` at index 0 with a length of 2 elements, the exported header will look like:

```cpp
#ifndef BUFFER_0_H
#define BUFFER_0_H

#define BUFFER_0_LENGTH 2
#define BUFFER_0_NAME "u_corners"

static const float buffer_0[4] = {
    0.1f, 0.9f, 0.1f, 0.9f
};

static inline const float* unpack_buffer_0() {
    static float dest[BUFFER_0_LENGTH * 4];
    for (int i = 0; i < BUFFER_0_LENGTH; ++i) {
        dest[i * 4 + 0] = buffer_0[0 * BUFFER_0_LENGTH + i];
        dest[i * 4 + 1] = buffer_0[1 * BUFFER_0_LENGTH + i];
        dest[i * 4 + 2] = 0.0f;
        dest[i * 4 + 3] = 0.0f;
    }
    return dest;
}

#endif // BUFFER_0_H
```

### Unpacking Function for Fixed-Point Buffers

When a buffer specifies `"fixedPoint"` configuration (e.g. `"fixedPoint": "Q3.12"`), it is exported as a compact integer array to save space in the compiled executable binary. To keep things transparent to the graphics pipeline, the generated C-header helper function (`unpack_buffer_X()`) automatically scales these integers back to floating-point values on-the-fly when called:

```cpp
#ifndef BUFFER_0_H
#define BUFFER_0_H

#include <stdint.h>

#define BUFFER_0_LENGTH 2
#define BUFFER_0_NAME "u_colors"

// Compact signed 16-bit integer fixed-point array
static const int16_t buffer_0[6] = {
    4096, -2048, 1024, 0, 6144, -8192
};

static inline const float* unpack_buffer_0() {
    static float dest[6];
    const float scale = 1.0f / 4096.0f; // 1 / (1 << 12)
    for (int i = 0; i < BUFFER_0_LENGTH; ++i) {
        dest[i * 3 + 0] = (float)buffer_0[i * 3 + 0] * scale;
        dest[i * 3 + 1] = (float)buffer_0[i * 3 + 1] * scale;
        dest[i * 3 + 2] = (float)buffer_0[i * 3 + 2] * scale;
    }
    return dest;
}

#endif // BUFFER_0_H
```

### Exporting TZD Compressed Buffers

To export buffers using the custom compressed TZD format, add an `"export"` block to any buffer definition in the manifest:

```json
{
  "name": "u_corners",
  "type": "vec2",
  "file": "assets/corners.csv",
  "export": {
    "format": "tzd",
    "stripedCoordinates": true,
    "stripedBytes": true
  }
}
```

#### TZD Bitwise Layout (16-bit)
*   **Bit 15**: Type indicator (`1` = Integer, `0` = Float). Integers are detected automatically (having only zeroes as decimals).
*   **Bit 14**: Sign bit (`1` = Negative, `0` = Positive).
*   **Bits 13–0**: Payload value (absolute value for integers, or absolute fraction scaled by $16384.0$ for float coordinates in the range $[-1.0, 1.0]$).

#### Striping and Ordering
*   `stripedCoordinates`: If `true`, groups components by dimension (all X components first, then all Y components) before exporting.
*   `stripedBytes`: If `true`, splits the 16-bit words into High (MSB) and Low (LSB) bytes and groups all High bytes together first, followed by all Low bytes (e.g. `[high X, high Y, low X, low Y]`).

#### Exporting to C-Header
To export the buffer to a C-header file:
```bash
./build/fork-eater <project_path> --export-buffer-header INDEX -o build/buffer_corners.h
```

If `stripedBytes` is `true`, it is generated as a `uint8_t` array. If `false`, it is generated as a `uint16_t` array. The header automatically includes a zero-dependency `static inline const float* unpack_buffer_X()` helper function to reconstruct and decode the coordinates back into floats on-the-fly.

#### Example Generated TZD Header Output

For a striped, byte-striped `vec2` buffer `u_corners` containing `[(1.0, -0.5), (-0.75, 0.0)]`, the generated header is:

```cpp
#ifndef BUFFER_0_H
#define BUFFER_0_H

#include <stdint.h>

#define BUFFER_0_LENGTH 2
#define BUFFER_0_NAME "u_corners"

// Groups high bytes [0x80, 0x70, 0x60, 0x80] and low bytes [0x01, 0x00, 0x00, 0x00]
static const uint8_t buffer_0[8] = {
    0x80, 0x70, 0x60, 0x80, 0x01, 0x00, 0x00, 0x00
};

static inline const float* unpack_buffer_0() {
    static float dest[4]; // Tightly-packed 4 floats (2 elements of vec2)
    for (int i = 0; i < BUFFER_0_LENGTH; ++i) {
        {
            uint16_t p = ((uint16_t)buffer_0[0 * BUFFER_0_LENGTH + i] << 8) | buffer_0[2 * BUFFER_0_LENGTH + 0 * BUFFER_0_LENGTH + i];
            float val = (p & 0x8000) ? (float)(p & 0x3FFF) : ((float)(p & 0x3FFF) / 16384.0f);
            dest[i * 2 + 0] = (p & 0x4000) ? -val : val;
        }
        {
            uint16_t p = ((uint16_t)buffer_0[1 * BUFFER_0_LENGTH + i] << 8) | buffer_0[2 * BUFFER_0_LENGTH + 1 * BUFFER_0_LENGTH + i];
            float val = (p & 0x8000) ? (float)(p & 0x3FFF) : ((float)(p & 0x3FFF) / 16384.0f);
            dest[i * 2 + 1] = (p & 0x4000) ? -val : val;
        }
    }
    return dest;
}

#endif // BUFFER_0_H
```

To export multiple buffers configured with an optional `"outputFile"` in their `"export"` blocks, you can run:
```bash
./build/fork-eater <project_path> --export-buffers
```

---

## Shader Implementation

### 1. Using Texture Buffer Objects (TBOs) - Recommended for Large Data
TBOs allow for millions of elements and are highly efficient. Use the `samplerBuffer` uniform type and `texelFetch` to access the tightly packed data.

```glsl
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform samplerBuffer u_colors;

void main() {
    // Fetch the 3rd color from the buffer (index 2)
    vec3 color = texelFetch(u_colors, 2).rgb;
    FragColor = vec4(color, 1.0);
}
```

---

### 2. Using Uniform Buffer Objects (UBOs) - Best for Shared & Structured Blocks
UBOs are perfect for sharing block data across different shader passes without manually setting uniforms.

#### Automatic Macro Injection
When you define a buffer, the preprocessor automatically injects an upper-case length helper define after the `#version` directive:
`#define <UPPERCASE_BUFFER_NAME>_LENGTH <element_count>`

For example, a buffer named `u_shared_data` of type `ubo` with 12 float elements and `dataType` of `vec4` (total 3 elements) will have this injected:
```glsl
#define U_SHARED_DATA_LENGTH 3
```

#### Naming Conventions
To map the UBO, Fork Eater searches the compiled shader program for a uniform block matching:
- Either the exact buffer name: `u_shared_data`
- Or the buffer name with `_block` appended: `u_shared_data_block`

#### GLSL Example
```glsl
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

// Declaring the Uniform Block using the auto-defined length
layout (std140) uniform u_shared_data_block {
    vec4 dataPoints[U_SHARED_DATA_LENGTH];
};

void main() {
    // Access elements normally from the uniform block
    vec4 firstPoint = dataPoints[0];
    FragColor = vec4(firstPoint.rgb, 1.0);
}
```

> [!WARNING]
> ### Critical `std140` Alignment Gotcha
> GLSL uniform blocks with `std140` layout follow strict alignment rules that introduce padding:
> - `float` values are aligned to 4 bytes.
> - `vec2` values are aligned to 8 bytes.
> - `vec3` and `vec4` values are aligned to 16 bytes.
> - **Arrays of any type (including `float` or `vec3` arrays)**: Each element is padded to a **16-byte boundary (the size of a `vec4`)**.
> 
> *Pro-Tip:* If you define a UBO with `dataType: "vec3"`, each `vec3` element inside the shader array will be padded by the compiler to 16 bytes (as if it were a `vec4`). Therefore, your CSV data must be padded accordingly (4 values per line instead of 3), or you should declare `dataType: "vec4"` to avoid layout mismatches. For tightly-packed large array data with no padding, prefer using **Texture Buffer Objects (TBOs)**.

---

### 3. Using Uniform Arrays - For Small, Fixed Data
For smaller datasets, you can use standard GLSL uniform arrays. Fork Eater will automatically detect the array and populate it if the name matches a manifest buffer.

```glsl
#version 330 core
out vec4 FragColor;

uniform float u_config[3];

void main() {
    float param = u_config[0];
    FragColor = vec4(vec3(param), 1.0);
}
```

---

## CSV File Format

The CSV parser is lightweight and supports:
- Comma-separated values (`,`)
- Space-separated values
- Tab-separated values
- Comments starting with `#`
- Multiple values per line (automatically flattened)

Example `assets/palette.csv`:
```csv
# RGB Palette
1.0, 0.0, 0.0
0.0, 1.0, 0.0
0.0, 0.0, 1.0
```

## Hot-Reloading

Fork Eater monitors all buffer files specified in the manifest. Modifying and saving an external CSV file will trigger an automatic project reload, instantly updating your shader with the new data.

## Key Implementation Details

- **Binding Strategy**:
  - **TBOs** are bound to texture units starting from unit 10 to avoid conflicts with shader pass inputs.
  - **UBOs** are bound to sequential uniform buffer binding points starting from unit 0.
- **Resource Management**: `ShaderManager` automatically handles the creation, update, and deletion of OpenGL buffer and texture objects.
- **Data Validation**: The system logs the number of values loaded and reports errors if files are missing.
