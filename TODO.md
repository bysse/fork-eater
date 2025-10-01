
Change all system uniforms to be ShaderToy compatible. Make sure the templates are changed. These should be supported:

uniform vec3      iResolution;           // viewport resolution (in pixels)
uniform float     iTime;                 // shader playback time (in seconds)
uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click
---
Each shader file that is included by the preprocessor needs to be watched for changes so that the root shader can be re-read
and compiled.
---
Add a basic raymarching shader template
---
Add support for embedding shader libraries into the binary that can be included by the #pragma include .
