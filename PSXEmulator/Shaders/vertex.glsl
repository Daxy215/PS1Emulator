#version 330 core

in vec2 vertexPosition;
in uvec3 vertexColor;
in vec4 texCoords;

// Idk how to pass structs?
// Not really good with GLSL
in int attributes;

out vec3 color;
out vec4 UVs;

// Not sure how to forward data,
// to the Fragment shader right away..
//flat out uvec3 attr;
flat out int attr;
//flat out uint textureAttr[4];

uniform vec2 offset;

out vec2 VRAMPos;

const int TEXTURE_MODE_MASK = 0x1C;
const int TEXTURE_MODE_SHIFT = 2;

void main() {
    //vec2 position = vertexPosition + offset;
    vec2 position = vertexPosition;
    ivec2 area = ivec2(1024, 512);
    
    VRAMPos = position;
    
    // For testing
    /*uint isSemiTransparent = attributes & IS_SEMITRANSPARENT_MASK;
    uint blendTexture = (attributes & BLEND_TEXTURE_MASK) >> 1;*/
    int textureMode = (int(attributes) & TEXTURE_MODE_MASK) >> TEXTURE_MODE_SHIFT;
    
    if(textureMode == 3) {
        area = ivec2(uint(1024), uint(512));
    }
    
    /*
    * Converts VRAM coordinates (0; area.x, 0; area.y)
    * to OpenGL coordinates (-1;1, -1,1)
    */
    float xPos = (position.x / float(area.x)) * 2.0 - 1.0;
    
    // VRAM puts 0 at the top, OpenGL at the bottom..;
    float yPos = 1.0 - (position.y / float(area.y)) * 2.0;
    
    // Set the final position
    gl_Position = vec4(xPos, yPos, 0.0, 1.0);
    
    // Snap vertices to PS1's low precision
    //gl_Position.xy = floor(gl_Position.xy * 64.0 + 0.5) / 64.0;
    
    // Force manual depth sorting (PS1-style)
    //gl_Position.z = -1.0 + (vertexID % 1024) * 0.001; // Fake depth
    
    // Convert the components from [0;255] to [0;1]
    color = vec3(float(vertexColor.r) / 255.0,
                 float(vertexColor.g) / 255.0,
                 float(vertexColor.b) / 255.0);
    
    UVs = texCoords;
    attr = attributes;
}
