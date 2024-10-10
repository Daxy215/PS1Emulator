#include "VRAM.h"

#include <SDL_render.h>

#include "Gpu.h"

Emulator::VRAM::VRAM(Gpu* gpu) : gpu(gpu), color1555to8888LUT(2 * 32 * 32 * 32) {
    // Create a texture with the desired dimensions
    texture = SDL_CreateTexture(gpu->renderer->renderer, SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING, width, height);
    
    if (!texture) {
        std::cerr << "Failed to create texture: " << SDL_GetError() << '\n';
        return;
    }
    
    // Lock the texture to get a pointer to its raw pixel data
    pixels = nullptr;
    int pitch;
    
    if (SDL_LockTexture(texture, nullptr, reinterpret_cast<void**>(&pixels), &pitch) != 0) {
        std::cerr << "Failed to lock texture: " << SDL_GetError() << '\n';
        //throw std::runtime_error("Failed to lock texture");
        //return;
    }
    
    // Unlock the texture
    SDL_UnlockTexture(texture);
    
    // Init color 1555 to 8888 LUT
    for (int m = 0; m < 2; m++) {
        for (int r = 0; r < 32; r++) {
            for (int g = 0; g < 32; g++) {
                for (int b = 0; b < 32; b++) {
                    color1555to8888LUT[m << 15 | b << 10 | g << 5 | r] = m << 24 | r << 16 + 3 | g << 8 + 3| b << 3;
                }
            }
        }
    }
}

void Emulator::VRAM::store(uint32_t val) {
    uint32_t pixel1 = ((val >> 16) + gpu->dithering) / 8;
    uint32_t pixel0 = ((val & 0xFFFF) + gpu->dithering) / 8;
    
    pixel0 |= (gpu->forceSetMaskBit << 15);
    pixel1 |= (gpu->forceSetMaskBit << 15);
    
    // draw pixel 0
    drawPixel(pixel0);
    
    // draw pixel 1
    drawPixel(pixel1);
}

void Emulator::VRAM::beginTransfer(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t imgSize) {
    transferData = {x, y, width, height, imgSize};
}

void Emulator::VRAM::stepTransfer() {
    if(++transferData.x == transferData.originX + transferData.width) {
        transferData.x -= transferData.width;
        transferData.y++;
    }
}

void Emulator::VRAM::endTransfer() {
    // Draw the texture
    //SDL_Rect dstrect = { (int)transferData.x, (int)transferData.y, (int)width, (int)height };
    //SDL_Rect srcrect = { 0, 0, (int)width, (int)height};
    //SDL_RenderCopy(gpu->renderer->renderer, texture, nullptr, nullptr);
    
    // Update the screen
    //SDL_RenderPresent(gpu->renderer->renderer);
}

void Emulator::VRAM::drawPixel(uint32_t pixel) {
    if(!gpu->preserveMaskedPixels || (getPixelRGB888(transferData.x, transferData.y) >> 24) == 0) {
        uint32_t color = color1555to8888LUT[pixel];
        
        setPixel(transferData.x & 0x3FF, transferData.y & 0x1FF, color);
        //setPixel(transferData.x & 0x3FF, transferData.y & 0x1FF, pixel);
    }
    
    stepTransfer();
}

void Emulator::VRAM::setPixel(uint32_t x, uint32_t y, uint32_t color) {
    size_t index = y * width + x;
    pixels[index] = color;
}

uint32_t Emulator::VRAM::getPixelRGB888(uint32_t x, uint32_t y) {
    size_t index = y * width + x;
    return pixels[index];
}

uint32_t Emulator::VRAM::getPixelBGR555(uint32_t x, uint32_t y) {
    size_t index = y * width + x;
    uint32_t color = pixels[index];
    
    uint8_t m = static_cast<uint8_t>((color & 0xFF000000) >> 24);
    uint8_t r = static_cast<uint8_t>((color & 0x00FF0000) >> 16 + 3);
    uint8_t g = static_cast<uint8_t>((color & 0x0000FF00) >> 8 + 3);
    uint8_t b = static_cast<uint8_t>((color & 0x000000FF) >> 3);
    
    return static_cast<uint16_t>(m << 15 | b << 10 | g << 5 | r);
}
