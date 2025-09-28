All shader passes should render to a framebuffer / texture. 
The size of the last pass is set "the screen size" which is selected from a menu where the use can choose from 1280x720 and 1920x1080.
All other passes set their output size in the project manifest where the default is the screen size.
---
The tool should monitor the project manifest for changes and reload if changes are made.
If a project has been opened successfully the tool must not exit if there are errors in the manifest
during reload. It should print those errors in the log / console and ignore the update.
---
The project should keep track of the fps / render time of the shader and display it as a part of the timeline
The FPS is expressed as a color where red is below 20 and green is above 30, (grey is unknown) these limit should be easily adjusted.
That means that the tool needs to keep track of the FPS in a buffer and display it. But the buffer could be worst case
per 250 ms or quantified in some other convenient way. 
---
The tool needs a way to cope with super low FPS:es. If an FPS lower than 10 is detected the shader will only 
render 50% of the screen each frame. If it's still goes down <= 10 is will half again to do 25% chunks. 
This type of mode most be clearly visible in the UI and the FPS bar.
---
Shaders should be pre-processed for "#pragma include(<FILENAME>)" directives that includes other shader code into the shader.
Loops needs to be detected. Each shader file that is included needs to be watched for changes so that the root shader can be re-read
and compiled.
