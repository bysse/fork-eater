The tool needs a way to cope with super low FPS:es. If an FPS lower than 10 is detected the shader will only
render 50% of the screen each frame. If it's still goes down <= 10 is will half again to do 25% chunks.
This type of mode most be clearly visible in the UI and the FPS bar.
---
Shaders should be pre-processed for "#pragma include(<FILENAME>)" directives that includes other shader code into the shader.
Loops needs to be detected. Each shader file that is included needs to be watched for changes so that the root shader can be re-read
and compiled.
