Shaders should be pre-processed for "#pragma include(<FILENAME>)" directives that includes other shader code into the shader.
Loops needs to be detected. Each shader file that is included needs to be watched for changes so that the root shader can be re-read
and compiled.
