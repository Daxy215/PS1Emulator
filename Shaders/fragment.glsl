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
flat in int attr;

out vec4 fragColor;

uniform sampler2D texture_sample4;

//uniform int texture_depth;
uniform vec4 textureWindow;

const int IS_SEMITRANSPARENT_MASK = 0x1;
const int BLEND_TEXTURE_MASK = 0x2;
const int TEXTURE_MODE_MASK = 0x1C;
const int TEXTURE_MODE_SHIFT = 2;
const int TEXTURE_DEPTH_SHIFT = 5;
const int TEXTURE_DEPTH_MASK = 0x3;

vec2 calculateTexel() {
    uvec2 texel = uvec2(uint(UVs.x) % 256u, uint(UVs.y) % 256u);
    
    /*uvec2 mask = uvec2((fragTextureWindow) & 0x1fu, (fragTextureWindow >> 5) & 0x1fu);
    uvec2 offset = uvec2((fragTextureWindow >> 10) & 0x1fu, (fragTextureWindow >> 15) & 0x1fu);*/
    
    uvec2 mask = uvec2(textureWindow.x, textureWindow.y);
    uvec2 offset = uvec2(textureWindow.z, textureWindow.w);
    
    texel.x = (texel.x & ~(mask.x * 8u)) | ((offset.x & mask.x) * 8u);
    texel.y = (texel.y & ~(mask.y * 8u)) | ((offset.y & mask.y) * 8u);
    
    return vec2(texel);
}

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

vec4 clut4bit(vec2 coords, ivec2 clut, ivec2 page) {
    int texX = int(coords.x / 4.0) + page.x;
    int texY = int(coords.y)       + page.y;
    
    uint index = internalToPsxColor(read(texX, texY));
    uint which = (index >> ((uint(coords.x) & 3u) * 4u)) & 0xfu;
    
    return read(clut.x + int(which), clut.y);
}

vec4 clut8bit(vec2 coords, ivec2 clut, ivec2 page) {
    int texX =  int(coords.x / 2.0) + page.x;
    int texY =  int(coords.y)       + page.y;
    
    uint index = internalToPsxColor(read(texX, texY));
    uint which = (index >> ((uint(coords.x) & 1u) * 8u)) & 0xffu;
    
    return read(int(which) + clut.x, clut.y);
}

vec4 clut16bit(vec2 coords, ivec2 page) {
    return read(int(coords.x) + page.x, int(coords.y) + page.y);
}

vec4 sample_texel(int textureDepth) {
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
    
    vec2 texel = calculateTexel();
    
    /**
     * T4Bit = 0,
     * T8Bit = 1,
     * T15Bit = 2,
     */
    if (textureDepth == 0) {
        return clut4bit(texel, ivec2(clutX, clutY), ivec2(pageX, pageY));
    } else if(textureDepth == 1) {
        return clut8bit(texel, ivec2(clutX, clutY), ivec2(pageX, pageY));
    } else if(textureDepth == 2) {
        return clut16bit(texel, ivec2(pageX, pageY));
    }
    
    return vec4(1, 0, 0, 1);
}

void main() {
    int isSemiTransparent = int(attr) & IS_SEMITRANSPARENT_MASK;
    int blendTexture = (int(attr) & BLEND_TEXTURE_MASK) >> 1;
    int textureMode = (int(attr) & TEXTURE_MODE_MASK) >> TEXTURE_MODE_SHIFT;
    
    // Apply transparency
    // TODO; Apply different level of transparency
    float alpha = (isSemiTransparent == 1) ? 0.5 : 1.0;
    
    // TODO; Implement texture blend?
    
    // This is for testing
    if(textureMode == 3) {
        vec4 c = texture(texture_sample4, UVs.xy);
        fragColor = c;
        
        return;
    }
    
    if (textureMode == 0) {
        fragColor = vec4(color, alpha);
    } else if(textureMode == 1 || textureMode == 2) {
        int textureDepth = (attr >> TEXTURE_DEPTH_SHIFT) & TEXTURE_DEPTH_MASK;
        
        vec4 samp = sample_texel(textureDepth);
        
        if(internalToPsxColor(samp) == 0u) {
            discard;
        }
        
        if(textureMode == 2) {
            vec4 f = vec4(color, 1);
            vec4 c = mix(samp, f, 0.5f);
            
            fragColor = vec4(c.r, c.g, c.b, alpha);
        } else {
            // TODO; Check if alpha is needed?
            fragColor = samp;
        }
    }
}
