#include "VRAM.h"

#include <SDL_render.h>

#include "Gpu.h"

Emulator::VRAM::VRAM(Gpu* gpu) : gpu(gpu), color1555to8888LUT(2 * 32 * 32 * 32) {
    // Create a texture with the desired dimensions
    texture = SDL_CreateTexture(gpu->renderer->renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, MAX_WIDTH, MAX_HEIGHT);
    
    if (!texture) {
        std::cerr << "Failed to create texture: " << SDL_GetError() << '\n';
        return;
    }
    
    // Lock the texture to get a pointer to its raw pixel data
    pixels = nullptr;
    pitch = 0;
    
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
    // Lock the texture before drawing
    if (SDL_LockTexture(texture, nullptr, reinterpret_cast<void**>(&pixels), &pitch) != 0) {
        std::cerr << "Failed to lock texture: " << SDL_GetError() << '\n';
        return; // Handle the error appropriately
    }
    
    // Draw the texture
    SDL_Rect srcrect = { 0, 0, (int)transferData.width, (int)transferData.height };
    SDL_Rect dstrect = { (int)transferData.x, (int)transferData.y, (int)transferData.width, (int)transferData.height };
    //SDL_RenderCopy(gpu->renderer->renderer, texture, &srcrect, &dstrect);
    SDL_RenderCopy(gpu->renderer->renderer, texture, nullptr, nullptr);
    
    // Unlock the texture after drawing
    SDL_UnlockTexture(texture);
    
    // Update the screen
    SDL_RenderPresent(gpu->renderer->renderer);
    
    /*
    // Draw the texture
    SDL_Rect dstrect = { (int)transferData.x, (int)transferData.y, (int)transferData.width, (int)transferData.height };
    SDL_Rect srcrect = { 0, 0, (int)transferData.width, (int)transferData.height};
    SDL_RenderCopy(gpu->renderer->renderer, texture, &srcrect, &dstrect);
    
    //Update the screen
    SDL_RenderPresent(gpu->renderer->renderer);*/
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
    x %= MAX_WIDTH;
    y %= MAX_HEIGHT;
    
    size_t index = y * MAX_WIDTH + x;
    pixels[index] = color;
}

uint32_t Emulator::VRAM::getPixelRGB888(uint32_t x, uint32_t y) {
    size_t index = y * MAX_WIDTH + x;
    return pixels[index];
}

uint32_t Emulator::VRAM::getPixelBGR555(uint32_t x, uint32_t y) {
    size_t index = y * MAX_WIDTH + x;
    uint32_t color = pixels[index];
    
    uint8_t m = static_cast<uint8_t>((color & 0xFF000000) >> 24);
    uint8_t r = static_cast<uint8_t>((color & 0x00FF0000) >> 16 + 3);
    uint8_t g = static_cast<uint8_t>((color & 0x0000FF00) >> 8 + 3);
    uint8_t b = static_cast<uint8_t>((color & 0x000000FF) >> 3);
    
    return static_cast<uint16_t>(m << 15 | b << 10 | g << 5 | r);
}
