
Change all system uniforms to be ShaderToy compatible. Make sure the templates are changed. These should be supported:

uniform vec3      iResolution;           // viewport resolution (in pixels)
uniform float     iTime;                 // shader playback time (in seconds)
uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click
---
Each shader file that is included needs to be watched for changes so that the root shader can be re-read
and compiled.
