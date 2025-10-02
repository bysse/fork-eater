The parameters window should be the same as the uniforms panel. the build in uniforms iTime, iResolution and iMouse should nor be
modifiable by the user. They are set by the tool.
---
When an embedded shader library is included it should be written to the ./lib/ folder relative to the shader project root.
Whenever a shader from the library is about to be included, the ./lib folder should be checked first, to see if the file is
already there. Change the wat to include library shader to "#pragma include(lib/filename)" as well.
---
Add the pragma command "#pragma switch(SOME_FLAG)" which will create a toggle in the parameter panel. If it's enable a "define SOME_FLAG"
will be inserted into the source (after the #version directive) at compile time 