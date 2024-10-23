#version 330 core

in vec2 vertexPosition;
in uvec3 vertexColor;

out vec3 color;

// Drawing offset
uniform ivec2 offset;

uniform ivec2 drawingArea;

void main() {
    // Apply the offset
    vec2 position = vertexPosition + offset;
    
    /*
    * Converts VRAM coordinates (0; drawingArea.x, 0; drawingArea.y)
    * to OpenGL coordinates (-1;1, -1,1)
    */
    float xPos = ((position.x / float(drawingArea.x)) * 2.0) - 1.0; 
    
    // VRAM puts 0 at the top, OpenGL at the bottom..
    float yPos = 1.0 - ((position.y / float(drawingArea.y)) * 2.0);
    
    // Set the final position
    gl_Position.xyzw = vec4(xPos, yPos, 0.0, 1.0);
    
    // Convert the components from [0;255] to [0;1]
    color = vec3(float(vertexColor.r) / 255.0,
                 float(vertexColor.g) / 255.0,
                 float(vertexColor.b) / 255.0);
}
