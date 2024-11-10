#version 330 core

in vec3 color;
flat in vec2 UVs;

/**
 * 0 = isSemiTransparent
 * 1 = blendTexture
 * 2 = useTextures
 */
flat in uvec3 attr;

/**
 * ClutX, ClutY,
 * PageX, PageY
 */
//flat in uint textureAttr[4];

out vec4 fragColor;

uniform int texture_depth;

/*uniform sampler2D texture_sample16;
uniform sampler2D texture_sample8;
uniform sampler2D texture_sample4;*/
uniform sampler2D texture_sample4;

vec4 split_colors(int data) {
    vec4 color;
    
    color.r = (data << 3) & 0xf8;
    color.g = (data >> 2) & 0xf8;
    color.b = (data >> 7) & 0xf8;
    color.a = 255.0f;
    
    return color;
}

vec4 sample_texel() {
    vec4 index = texture(texture_sample4, UVs);
    
    // For now, let's assume that,
    // the clut is always at [320, 480]
    //clut4[int(index.r * 255)]
    //int texel = int(texture(texture_sample4, uvec2(320 + int(index.r * 255), 480)).r * 255);
    
    //return index / vec4(255.0f);
    //return split_colors(texel) / vec4(255.0f);
    return index;
}

void main() {
    // vec4 texelColor = vec4(0, 0, 0, 0);
    //if(int(attr.z) == 1) {
        //texelColor = sample_texel();
    //}
    
    // Apply transparenty
    float alpha = (float(attr.x) == 1) ? 0.5 : 1.0;
    
    if (int(attr.z) == 0) { 
        fragColor = vec4(color, alpha);
    } else {
        fragColor = sample_texel();
    }
}
