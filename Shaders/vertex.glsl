#version 330 core

in ivec2 vertexPosition;
in uvec3 vertexColor;

out vec3 color;

// Drawing offset
uniform ivec2 offset;

void main() {
    ivec2 position = vertexPosition;
    
    /*
    * Converts VRAM coordinates (0;1023, 0;511)
    * to OpenGL coordinates (-1;1, -1,1)
    */
    float xPos = ((position.x / 1024.0) * 2.0) - 1.0; //(float(position.x) / 512) - 1.0;
    
    // VRAM puts 0 at the top, OpenGL at the bottom..
    float yPos = 1.0 - (position.y / 512.0) * 2; //1.0 - (float(position.y) / 256);
    
    gl_Position.xyzw = vec4(xPos, yPos, 0.0, 1.0);
    
    // Convert the components from [0;255] to [0;1]
    color = vec3(float(vertexColor.r) / 255,
                 float(vertexColor.g) / 255,
                 float(vertexColor.b) / 255);
}