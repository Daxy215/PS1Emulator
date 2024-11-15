#pragma once

#ifndef GPU_H
#define GPU_H

#include <assert.h>
#include <stdint.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <GL/glew.h>

#include "vram.h"

// TODO; Please redo this shitty code WTF IS THIS ;-;

// https://psx-spx.consoledev.net/graphicsprocessingunitgpu/

namespace Emulator {
    class Renderer;
    class VRAM;
}

namespace Emulator {
    // Depth of the pixel values in a texture page
    enum class TextureDepth {
        T4Bit = 0,
        T8Bit = 1,
        T15Bit = 2,
    };
    
    // Interlaced output splits each frame in two fields
    enum class Field {
        Top = 0,
        Bottom = 1,
    };
    
    // Video output horizontal resolution
    struct HorizontalRes {
        uint32_t value;
        uint32_t hr1, hr2;
        
        static HorizontalRes fromFields(uint32_t hr1, uint32_t hr2) {
            uint32_t hr = (hr2 & 1) | ((hr1 & 3) << 1);
            return HorizontalRes{hr, hr1, hr2};
        }
        
        uint32_t intoStatus() const {
            return static_cast<uint32_t>(value) << 16;
        }
    };
    
    // Video output vertical resolution
    enum class VerticalRes {
        Y240Lines = 0,
        Y480Lines = 1,
    };
    
    // Video Modes
    enum class VMode {
        Ntsc = 0,
        Pal = 1,
    };
    
    // Display area color depth
    enum class DisplayDepth {
        D15Bits = 0,
        D24Bits = 1,
    };
    
    // Requested DMA direction
    enum class DmaDirection {
        Off = 0,
        Fifo = 1,
        CpuToGp0 = 2,
        VRamToCpu = 3,
    };
    
    // Buffer holding multi-word fixed-length GP0 command parameters
    struct CommandBuffer {
        // Command buffer: the longest possible command is GP0(0x3E)
        // which takes 12 parameters
        uint32_t buffer[12];
        
        // Number of words queued in buffer
        uint8_t len;
        
        CommandBuffer() : buffer(), len(0) {}
        
        uint32_t index(size_t index) const {
            if (index >= len) {
                throw std::out_of_range("Command buffer index out of range: " + std::to_string(index));
            }
            
            return buffer[index];
        }
        
        void clear() {
            len = 0;
        }
        
        void pushWord(uint32_t word) {
            buffer[static_cast<size_t>(len)] = word;
            len++;
        }
    };
    
    enum Gp0Mode {
        // Default mode: Handling commands
        Command,
        // Loading an image into VRAM
        VRam,
    };
    
    // GPU structure
    class Gpu {
    public:
        // TODO; Change this back to GLint
        struct Position {
            Position() = default;
            
            Position(float x, float y) : x(x), y(y) {
                
            }
            
            static Position fromGp0(uint32_t val) {
                float x = static_cast<float>(static_cast<int16_t>(val));
                float y = static_cast<float>(static_cast<int16_t>(val >> 16));
                
                return {x, y};
            }
            
            float x, y;
        };
        
        struct Color {
            Color() = default;
            
            Color(GLubyte r, GLubyte g, GLubyte b) : r(r), g(g), b(b) {
                
            }
            
            static Color fromGp0(uint32_t val) {
                uint8_t r = static_cast<uint8_t>(val);
                uint8_t g = static_cast<uint8_t>(val >> 8);
                uint8_t b = static_cast<uint8_t>(val >> 16);
                
                return {r, g, b};
            }
            
            GLubyte r, g, b;
        };
        
        struct UV {
            static UV fromGp0(uint32_t val, uint32_t clut, uint32_t page, Gpu& gpu) {
                uint16_t depth = (gpu.textureDepth == TextureDepth::T4Bit) ? 4 : (gpu.textureDepth == TextureDepth::T8Bit) ? 8 : 16;
                
                uint16_t u = static_cast<uint16_t>((val) & 0xFF);
                uint16_t v = static_cast<uint16_t>((val >> 8) & 0xFF);
                
                uint16_t clutX = static_cast<uint16_t>((clut & 0x3F) << 4);
                uint16_t clutY = static_cast<uint16_t>((clut >> 6) & 0x1FF);
                
                uint16_t pageX = static_cast<uint16_t>((page & 0xF) << 6);
                uint16_t pageY = static_cast<uint16_t>(((page >> 4) & 1) << 8);
                
                float r = 16 / depth;
                float ux = (pageX * r + u) / (1024.0f * r);
                float vc = (pageY + v) / 512.0f;
                
                float dataX = (clutX << 16) | pageX;
                float dataY = (clutY << 16) | pageY;
                 
                return {ux, vc, dataX, dataY};
            }
            
            static UV fromGp0(uint32_t val, uint32_t clut, uint16_t pageX, uint16_t pageY, Gpu& gpu) {
                uint16_t depth = (gpu.textureDepth == TextureDepth::T4Bit) ? 4 : (gpu.textureDepth == TextureDepth::T8Bit) ? 8 : 16;
                
                uint16_t u = static_cast<uint16_t>((val) & 0xFF);
                uint16_t v = static_cast<uint16_t>((val >> 8) & 0xFF);
                
                uint16_t clutX = static_cast<uint16_t>((clut & 0x3F) << 4);
                uint16_t clutY = static_cast<uint16_t>((clut >> 6) & 0x1FF);
                
                float r = 16 / depth;
                float ux = (pageX * r + u) / (1024.0f * r);
                float vc = (pageY + v) / 512.0f;
                
                float dataX = (clutX << 16) | pageX;
                float dataY = (clutY << 16) | pageY;
                
                return {ux, vc, dataX, dataY};
            }
            
            float u = 0, v = 0;
            float dataX, dataY;
        };
        
        struct Attributes {
            Attributes() = default;
            
            Attributes(GLubyte isSemiTransparent, GLubyte blendTexture, GLubyte useTextures = 0)
                : isSemiTransparent(isSemiTransparent),
                  blendTexture(blendTexture),
                  useTextureMode(useTextures)
                  /*clutX(0),
                  clutY(0),
                  pageX(0),
                  pageY(0)*/ {
                
            }
            
            void setTextureParameters(uint32_t clut, uint32_t page) {
                uint16_t c = static_cast<uint16_t>(clut >> 16);
                uint16_t p = static_cast<uint16_t>(page >> 16);
                
                /*clutX = static_cast<uint16_t>((c & 0x3F) << 4);
                clutY = static_cast<uint16_t>((c >> 6) & 0x1FF);
                
                pageX = static_cast<uint16_t>((p & 0xF) << 6);
                pageY = static_cast<uint16_t>(((p >> 4) & 1) << 8);*/
            }
            
            bool useTextures() {
                return (useTextureMode != 0);
            }
            
            GLubyte isSemiTransparent = 0;
            GLubyte blendTexture = 0;
            
            /**
             * I'm too lazy to create an enum
             * 0 = No texture,
             * 1 = Texture,
             * 2 = Texture + color
             */
            GLubyte useTextureMode = 0;
            
            /*GLushort clutX = 0, clutY = 0;
            GLushort pageX = 0, pageY = 0;*/
        };
        
    public:
        Gpu();

        void step(uint32_t cycles);
        
        uint32_t status();
        
        // Handles writes to the GP0 command register
        void gp0(uint32_t val);
        
        // GP0(0xE1): command
        void gp0DrawMode(uint32_t val);
        
        // GP0(0XE3): Set Drawing Area top left
        void gp0DrawingAreaTopLeft(uint32_t val);
        
        // GP0(0xE4): Set Drawing Area bottom right
        void gp0DrawingAreaBottomRight(uint32_t val);
        
        // GP0(0xE5): Set Drawing Offset
        void gp0DrawingOffset(uint32_t val);
        
        // GP0(0xE2): Set Texture Window
        void gp0TextureWindow(uint32_t val);
        
        // GP0(0xE6): Set Mask Bit Setting
        void gp0MaskBitSetting(uint32_t val);
        
        // GP0(0x00): No operations
        void gp0Nop(uint32_t val) {
            // NOP
        }
        
        // TODO; Move to a utils class
        static inline bool get_bit(uint32_t num, int b) {
            return num & (1 << b);
        }
        
        // GP0(0x28): Momochrome Opaque Quadrilateral
        void gp0QuadMonoOpaque(uint32_t val);
        
        // TODO; Remove
        void vertexOrder(Position* p) {
            auto crossZ = (p[1].x - p[0].x) * (p[2].y - p[0].y) - (p[1].y - p[0].y) * (p[2].x - p[0].x);
            
            if(crossZ < 0) {
                std::swap(p[0], p[1]);
            }
        }
        
        // GP0(0x30): Shaded Opaque Triangle
        void gp0TriangleShadedOpaque(uint32_t val);
        
        // GP0(0x34): Shaded Texture Opaque Triangle
        void gp0TriangleTexturedShadedOpaque(uint32_t val);
        
        // GP0(0x38): gp0QuadShadedOpaque
        void gp0QuadShadedOpaque(uint32_t val);
        
        // GP0(0x3C): Shaded Texture Opaque Quad
        void gp0QuadTexturedShadedOpaque(uint32_t val);
        
        // Helper function
        void renderRectangle(Position position, Color color, UV uv, uint16_t width, uint16_t height);
        
        // y tf do these say "MonoOpaque"???????
        void gp0VarRectangleMonoOpaque(uint32_t val);
        void gp0VarTexturedRectangleMonoOpaque(uint32_t val);
        void gp0DotRectangleMonoOpaque(uint32_t val);
        void gp08RectangleMonoOpaque(uint32_t val);
        void gp08RectangleTexturedOpaqu(uint32_t val);
        void gp016RectangleMonoOpaque(uint32_t val);
        
        // TODO; Testing
        void gp0Rectangle(uint32_t val);
        
        // GP0(0x01): Clear Cache
        void gp0ClearCache(uint32_t val);
        
        // GP0(0x02): Full VRam
        void gp0FillVRam(uint32_t val);
        
        // GP0(0x20): 
        void gp0TriangleMonoOpaque(uint32_t val);
        
        void gp0TriangleTexturedOpaque(uint32_t val);
        
        void gp0TriangleRawTexturedOpaque(uint32_t val);
        
        // GP0(0x2C): Quad Raw Texture Blend Opqaue
        void gp0QuadTextureBlendOpaque(uint32_t val);
        
        // GP0(0x2D): Quad Raw Texture Blend Opqaue
        void gp0QuadRawTextureBlendOpaque(uint32_t val);
        
        // GP0(0xA0): Load Image
        // From CPU to VRAM
        void gp0ImageLoad(uint32_t val);
        
        // TODO; Remove
        uint32_t startX, startY, curX, curY, endX, endY;
        
        // GP0(0xC0): Load Store
        // From VRAM to CPU
        void gp0ImageStore(uint32_t val);
        
        void gp0VramToVram(uint32_t val) const;
        
        // Handles writes to the GP1 command register
        void gp1(uint32_t val);
        
        // GP1(0x00): soft rest
        void gp1Reset(uint32_t val);
        
        // GP1(0x80): Display Mode
        void gp1DisplayMode(uint32_t val);
        
        // GP1(0x03): Display Enable
        void gp1DisplayEnable(uint32_t val);
        
        // GP1(0x04): DMA Direction
        void gp1DmaDirection(uint32_t val);
        
        // GP1(0x05): Display VRAM Start
        void gp1DisplayVramStart(uint32_t val);
        
        // GP1(0x06): Display Horizontal Range
        void gp1DisplayHorizontalRange(uint32_t val);
        
        // GP1(0x07): Display Vertical Range
        void gp1DisplayVerticalRange(uint32_t val);
        
        // GP1(0x01): Reset Command Buffer
        void gp1ResetCommandBuffer(uint32_t val);
        
        // GP1(0x02): Acknowledge Interrupt
        void gp1AcknowledgeIrq(uint32_t val);
        
        float getRefreshRate() const;
        
        // Retrieve value of the "read" register
        uint32_t read();
        
    public:
        uint8_t pageBaseX;           // Texture page base X coordinate (4 bits, 64 byte increment)
        uint8_t pageBaseY;           // Texture page base Y coordinate (1 bit, 256 line increment)
        uint8_t semiTransparency;    // Semi-transparency value
        TextureDepth textureDepth;   // Texture page color depth
        bool dithering;              // Enable dithering from 24 to 15 bits RGB
        bool drawToDisplay;          // Allow drawing to the display area
        bool forceSetMaskBit;        // Force "mask" bit of the pixel to 1 when writing to VRAM
        bool preserveMaskedPixels;   // Don't draw to pixels which have the "mask" bit set
        Field field;                 // Currently displayed field
        bool textureDisable;         // When true all textures are disabled
        HorizontalRes hres;          // Video output horizontal resolution
        VerticalRes vres;            // Video output vertical resolution
        VMode vmode;                 // Video mode
        DisplayDepth displayDepth;   // Display depth
        bool interlaced;             // Output interlaced video signal instead of progressive
        bool displayEnabled;         // Whether the display is on or not
        bool interrupt;              // True when the interrupt is active
        DmaDirection dmaDirection;   // DMA request direction
        bool rectangleTextureFlipX;  // Mirror textured rectangles along the x axis
        bool rectangleTextureFlipY;  // Mirror textured rectangles along the Y axis
        
        uint8_t textureWindowXMask; // Texture window x mask (8 pixel steps)
        uint8_t textureWindowYMask; // Texture window y mask (8 pixel steps)
        uint8_t textureWindowXOffset; // Texture window x offset (8 pixel steps)
        uint8_t textureWindowYOffset; // Texture window y offset (8 pixel steps)
        uint16_t drawingAreaLeft; // Left-most column of drawing area
        uint16_t drawingAreaTop; // Top-most line of drawing area
        uint16_t drawingAreaRight; // Right-most column of drawing area
        uint16_t drawingAreaBottom; // Bottom-most line of drawing area
        int16_t drawingXOffset; // Horizontal drawing offset applied to all vertices
        int16_t drawingYOffset; // Vertical drawing offset applied to all vertices
        uint16_t displayVramXStart; // First column of the display area in VRAM
        uint16_t displayVramYStart; // First line of the display area in VRAM
        uint16_t displayHorizStart; // Display output horizontal start relative to HSYNC
        uint16_t displayHorizEnd; // Display output horizontal end relative to HSYNC
        uint16_t displayLineStart; // Display output first line relative to VSYNC
        uint16_t displayLineEnd; // Display output last line relative to VSYNC
        
        uint32_t _read = 0;
        
        const float ntscVideoClock = 53693175.0f / 60.0f;
        const float palVideoClock = 53203425.0f / 60.0f;
        
        uint32_t _scanLine = 0;
        uint32_t _cycles = 0;
        
        bool isInHBlank = false;
        bool isInVBlank = false;
        uint32_t dot = 1;
        
        bool isOddLine = false;
        
        // 10, 8, 5, 4, 7
        uint32_t dotCycles[5] = { 10, 8, 5, 4, 7};
        
    private:
        Attributes curAttribute = {0, 0, 0};
        
    public:
        // Buffer containing the current GP0 command
        CommandBuffer gp0Command;
        
        // Remaining words for the current GP0 command
        uint32_t gp0CommandRemaining = 0;
        
        // Current mode for GP0
        Gp0Mode gp0Mode = Command;
        Gp0Mode readMode = Command;
        
        // Pointer to the method implementing the current GP command
        std::function<void(Gpu&, uint32_t)> Gp0CommandMethod;
        
    public:
        Renderer* renderer;
        VRAM* vram;
    };
}
#endif // GPU_H
