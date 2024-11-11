#pragma once

#include <cstdint>

// https://www.reddit.com/r/EmuDev/comments/fmhtcn/article_the_ps1_gpu_texture_pipeline_and_how_to/

namespace Emulator {
    class Gpu;
    
    struct TransferData {
        TransferData() = default;
        
        TransferData(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t size)
            : x(x),
              y(y),
              originX(x),
              originY(y),
              width(width),
              height(height),
              size(size) {
            
        }
        
        uint32_t x, y, originX, originY, width, height, size;
    };
    
    class VRAM {
    public:
        VRAM(Gpu* gpu);
        
        void store(uint32_t val);
        void beginTransfer(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t imgSize);
        void stepTransfer();
        void endTransfer();
        
        // Pixel stuff...
        void drawPixel(uint32_t pixel);
        void setPixel(uint32_t x, uint32_t y, uint32_t color);
        
        uint16_t getPixel(uint32_t x, uint32_t y) const;
        
        uint16_t getPixel4(uint32_t x, uint32_t y, uint32_t clutX, uint32_t clutY, uint32_t pageX, uint32_t pageY);
        uint16_t getPixel8(uint32_t x, uint32_t y, uint32_t clutX, uint32_t clutY, uint32_t pageX, uint32_t pageY);
        uint16_t getPixel16(uint32_t x, uint32_t y, uint32_t pageX, uint32_t pageY);

        uint16_t RGB555_to_RGB565(uint16_t color);
        
    private:
        TransferData transferData = {};
        
    public:
        // https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#vram-overview-vram-addressing
        const int32_t MAX_WIDTH  = 1024;
        const int32_t MAX_HEIGHT = 512;
        
    private:
        Gpu* gpu;
        
    public:
        // Maybe move this to a struct?
        uint32_t pbo4, pbo8, pbo16;
        uint32_t texture4, texture8, texture16;
        unsigned int textureID = 0;
        
        uint8_t* ptr4;
        uint8_t* ptr8;
        uint16_t* ptr16;
    };
}
