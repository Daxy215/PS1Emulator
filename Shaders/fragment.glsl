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

uniform sampler2D texture_sample4;

uniform int texture_depth;

uint internalToPsxColor(vec4 c) {
    uint a = uint(floor(c.a + 0.5));
    uint r = uint(floor(c.r * 31.0 + 0.5));
    uint g = uint(floor(c.g * 31.0 + 0.5));
    uint b = uint(floor(c.b * 31.0 + 0.5));
    
    return (a << 15) | (b << 10) | (g << 5) | r;
}

vec4 read(int x, int y) {
    return texelFetch(texture_sample4, ivec2(x, y), 0);
}

vec4 clut4bit(ivec2 clut, ivec2 page) {
    int texX = int(UVs.x / 4.0) + page.x;
    int texY = int(UVs.y)       + page.y;
    
    uint index = internalToPsxColor(read(texX, texY));
    uint which = (index >> ((uint(UVs.x) & 3u) * 4u)) & 0xfu;
    
    return read(clut.x + int(which), clut.y);
}

vec4 clut8bit(ivec2 clut, ivec2 page) {
    int texX =  int(UVs.x / 2.0) + page.x;
    int texY =  int(UVs.y)       + page.y;
    
    uint index = internalToPsxColor(read(texX, texY));
    uint which = (index >> ((uint(UVs.x) & 1u) * 8u)) & 0xffu;
    
    return read(int(which) + clut.x, clut.y);
}

vec4 clut16bit(ivec2 page) {
    return read(int(UVs.x) + page.x, int(UVs.y) + page.y);
}

vec4 sample_texel() {
    /*
     * Extract clut and page
     */
    float clutX = int(UVs.z) >> 16;
    float clutY = int(UVs.w) >> 16;
    
    float pageX = int(UVs.z) & 0xFFFF;
    float pageY = int(UVs.w) & 0xFFFF;
    /*
    float index = texture(texture_sample4, vec2(UVs.x, UVs.y)).r;
    
    // The coordinates of the clut in the texture
    vec2 coords = vec2((clutX) / 1024.0, clutY / 512.0);
    
    // Fetch color based on index
    float clutEntryWidth = 1.0 / 16.0;
    
    float clutEntryX = coords.x + (index / 16.0) * clutEntryWidth;
    float clutEntryY = coords.y;
    
    vec4 clutColor = texture(texture_sample4, vec2(clutEntryX, clutEntryY));
    
    return clutColor;*/
    
    /**
     * T4Bit = 0,
     * T8Bit = 1,
     * T15Bit = 2,
     */
    if (texture_depth == 0) {
        return clut4bit(ivec2(clutX, clutY), ivec2(pageX, pageY));
    } else if(texture_depth == 1) {
        return clut8bit(ivec2(clutX, clutY), ivec2(pageX, pageY));
    } else if(texture_depth == 2) {
        return clut16bit(ivec2(pageX, pageY));
    }
    
    return vec4(1, 0, 0, 1);
}

void main() {
    // Apply transparency
    // TODO; Apply different level of transparency
    float alpha = (float(attr.x) == 1) ? 0.5 : 1.0;
    
    // TODO; Implement attr.z == 2, texture + color
    
    // This is for testing
    if(int(attr.z) == 6) {
        vec4 c = texture(texture_sample4, UVs.xy);
        fragColor = c;
        
        return;
    }
    
    if (int(attr.z) == 0) { 
        fragColor = vec4(color, alpha);
    } else if(int(attr.z) == 1) {
        fragColor = sample_texel();
    } else if(int(attr.z) == 2) {
        vec4 color = vec4(color, 1);
        vec4 c = mix(sample_texel(), color, 0.5f);
        
        fragColor = vec4(c.r, c.g, c.b, alpha);
    }
}
