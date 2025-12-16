#pragma once

#include <cstdint>
#include <vector>
#include <bits/move.h>
#include <GL/glew.h>

// https://www.reddit.com/r/EmuDev/comments/fmhtcn/article_the_ps1_gpu_texture_pipeline_and_how_to/

namespace Emulator {
    class Gpu;
    typedef unsigned int GLuint;
    
    class VRAM {
        public:
            explicit VRAM(Gpu& gpu);
            ~VRAM();
            
            void endTransfer();
            void flushRegion(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
            
            // Pixel stuff...
            //void drawPixel(uint32_t pixel);
            
            void writePixel(uint32_t x, uint32_t y, uint16_t pixel);
            void setPixel(uint32_t x, uint32_t y, uint32_t color);
            
            [[nodiscard]] uint16_t getPixel(uint32_t x, uint32_t y) const;
            
            uint16_t getPixel4(uint32_t x, uint32_t y, uint32_t clutX, uint32_t clutY, uint32_t pageX, uint32_t pageY);
            uint16_t getPixel8(uint32_t x, uint32_t y, uint32_t clutX, uint32_t clutY, uint32_t pageX, uint32_t pageY);
            uint16_t getPixel16(uint32_t x, uint32_t y, uint32_t pageX, uint32_t pageY);
            
            uint16_t RGB555_to_RGB565(uint16_t color);
            
            /*void swapBuffers() {
                std::swap(vramReadTex, vramWriteTex);
            }*/
            
            void copyToTexture(uint32_t x, uint32_t y, uint32_t dx, uint32_t dy, uint32_t width, uint32_t height, GLuint tex) {
                glCopyImageSubData(
                    this->tex, GL_TEXTURE_2D, 0,
                    x, y, 0,
                    tex, GL_TEXTURE_2D, 0,
                    dx, dy, 0,
                    width, height, 1
                );
            }
            
            void reset();

        private:
            void markTile(uint32_t x, uint32_t y);
            
        public:
            // https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#vram-overview-vram-addressing
            const int32_t MAX_WIDTH  = 1024;
            const int32_t MAX_HEIGHT = 512;
            
        public:
            GLuint tex;
            GLuint pbo;
            
            static constexpr uint32_t TILE_SIZE = 32;
            uint32_t tilesX;
            uint32_t tilesY;
            
            //uint16_t* ptr16;
            uint16_t* gpuPtr;
            
            std::vector<uint8_t> tileDirty;
            
        private:
            Gpu& gpu;
    };
}