#pragma once
#include <cstdint>
#include <SDL_render.h>
#include <vector>

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
        void setPixel(uint32_t x, uint32_t y, uint32_t color) const;
        
        uint16_t getPixelRGB888(uint32_t x, uint32_t y) const;
        uint32_t getPixelBGR555(uint32_t x, uint32_t y);

    public:
        TransferData transferData = {};
        
        // https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#vram-overview-vram-addressing
        const int32_t MAX_WIDTH  = 1024;
        const int32_t MAX_HEIGHT = 512;
        
        SDL_Texture* texture;
        
        Gpu* gpu;
    public:
        uint16_t* pixels;
        int pitch;
        
        std::pmr::vector<uint32_t> color1555to8888LUT;
    };
}
