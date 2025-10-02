Add support for setting values of non-system uniforms. float and vec2 can be represented as one and two sliders in the parameter panel.
The values set should be stored in a separate file so that they can be read during project loading.
---
When an embedded shader library is included it should be written to the ./lib/ folder relative to the shader project root.
Whenever a shader from the library is about to be included, the ./lib folder should be checked first, to see if the file is
already there. Change the wat to include library shader to "#pragma include(lib/filename)" as well.