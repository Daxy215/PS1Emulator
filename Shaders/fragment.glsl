#version 330 core

in vec3 color;

/*
 * Because I'm too lazy,
 * I decided to add clut position,
 * to this, so, x, y are the uvs,
 * and z and w are the clut positions.
 * 
 * Because I'm even more lazy,
 * I split the data into a single float,
 * so:
 * UVs.z, has clutX AND pageX
 * UVs.w, has clutY AND pageY
 */
in vec4 UVs;

/**
 * 0 = isSemiTransparent
 * 1 = blendTexture
 * 2 = useTextures ( 0 = No texture, 1 = Only texture, 2 = Texture + Color)
 */
flat in uvec3 attr;

out vec4 fragColor;

/*uniform sampler2D texture_sample16;
uniform sampler2D texture_sample8;*/
uniform sampler2D texture_sample4;

uniform int texture_depth;

/*vec4 sample_texel() {
    if (texture_depth == 4) { // 4
        vec4 index = texture(texture_sample4, UVs);
    
        // For now, let's assume that,
        // the clut is always at [320, 480]
        //clut4[int(index.r * 255)];
        int texel = int(texelFetch(texture_sample4, uvec2(320 + int(index.r * 255), 480)).r * 255);
        
        return split_colors(texel) / vec4(255.0f);
    } else if (texture_depth == 8) { // 8
        vec4 index = texture(texture_sample8, UVs);
        *//*int texel = clut8[int(index.r * 255)];*//*
        int texel = int(texture(texture_sample8, uvec2(320 + int(index.r * 255), 480)).r * 255);
          
        return split_colors(texel) / vec4(255.0f);
    } else { // 16
        int texel = int(texture(texture_sample16, UVs).r * 255);
        return split_colors(texel) / vec4(255.0f);
    }
}*/

vec4 sample_texel() {
    return texture2D(texture_sample4, UVs.xy, 0.0);
    
    /**
     * Extract clut and page
     */
    float clutX = int(UVs.z) >> 16;
    float clutY = int(UVs.w) >> 16;
    
    float pageX = int(UVs.z) & 0xFFFF;
    float pageY = int(UVs.w) & 0xFFFF;
    
    float index = texture(texture_sample4, vec2(UVs.x, UVs.y)).r;
    
    // The coordinates of the clut in the texture
    vec2 coords = vec2((clutX) / 1024.0, clutY / 512.0);
    
    // Fetch color based on index
    float clutEntryWidth = 1.0 / 16.0;
    
    float clutEntryX = coords.x + (index / 16.0) * clutEntryWidth;
    float clutEntryY = coords.y;
    
    vec4 clutColor = texture(texture_sample4, vec2(clutEntryX, clutEntryY));
    
    return clutColor;
    
    /**
     * T4Bit = 0,
     * T8Bit = 1,
     * T15Bit = 2,
     */
    if (texture_depth == 0) {
        
    } else if(texture_depth == 1) {
        return vec4(1, 0, 0, 1);
        vec4 index = texture(texture_sample4, UVs.xy);
        int texelIndex = int(index.r * 255.0);
        
        // The coordinates of the clut in the texture
        vec2 coords = vec2((clutX + float(texelIndex)) / 1024.0, clutY / 512.0);
        
        return texture2D(texture_sample4, coords, 0.0);
    } else if(texture_depth == 2) {
        return vec4(1, 0, 0, 1);
        return texture2D(texture_sample4, UVs.xy, 0.0);
    }
    
    return vec4(1, 0, 0, 1);
}

void main() {
    // Apply transparency
    // TODO; Apply different level of transparency
    float alpha = (float(attr.x) == 1) ? 0.5 : 1.0;
    
    // TODO; Implement attr.z == 2, texture + color
    
    if (int(attr.z) == 0) { 
        fragColor = vec4(color, alpha);
    } else {
        fragColor = sample_texel();
    }
}
