#pragma once

#include <stdint.h>
#include <vector>

// https://www.reddit.com/r/EmuDev/comments/fmhtcn/article_the_ps1_gpu_texture_pipeline_and_how_to/

namespace Emulator {
    class Gpu;
    
    class VRAM {
    public:
        VRAM(Gpu* gpu);
        
        void endTransfer();
        
        // Pixel stuff...
        void drawPixel(uint32_t pixel);
        
        void writePixel(uint32_t x, uint32_t y, uint16_t pixel);
        void setPixel(uint32_t x, uint32_t y, uint32_t color);
        
        uint16_t getPixel(uint32_t x, uint32_t y) const;
        
        uint16_t getPixel4(uint32_t x, uint32_t y, uint32_t clutX, uint32_t clutY, uint32_t pageX, uint32_t pageY);
        uint16_t getPixel8(uint32_t x, uint32_t y, uint32_t clutX, uint32_t clutY, uint32_t pageX, uint32_t pageY);
        uint16_t getPixel16(uint32_t x, uint32_t y, uint32_t pageX, uint32_t pageY);
        
        uint16_t RGB555_to_RGB565(uint16_t color);
        
    public:
        // https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#vram-overview-vram-addressing
        const int32_t MAX_WIDTH  = 1024;
        const int32_t MAX_HEIGHT = 512;
        
    private:
        Gpu* gpu;
        
    public:
        // Maybe move this to a struct?
        uint32_t pbo4, pbo8, pbo16;
        unsigned int texture4, texture8, texture16;
        
        uint8_t* ptr4;
        uint8_t* ptr8;
        uint16_t* ptr16;
    };
}
