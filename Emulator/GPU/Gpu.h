#pragma once

#ifndef GPU_H
#define GPU_H

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <GL/glew.h>

#include "VRAM.h"
#include "Rendering/Renderer.h"

// TODO; Please redo this shitty code WTF IS THIS ;-;

// https://psx-spx.consoledev.net/graphicsprocessingunitgpu/

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
        
        static HorizontalRes fromFields(uint32_t hr1, uint32_t hr2) {
            uint32_t hr = (hr2 & 1) | ((hr1 & 3) << 1);
            return HorizontalRes{hr};
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
    
    struct Position {
        Position() = default;
        Position(float x, float y) : x(x), y(y) {
            
        }
        
        static Position fromGp0P(uint32_t val) {
            float x = static_cast<float>(static_cast<int16_t>(val));
            float y = static_cast<float>(static_cast<int16_t>(val >> 16));
            
            return {x, y};
        }
        
        float x, y;
    };

    struct Color {
        Color() {}
        
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
    
    // GPU structure
    class Gpu {
    public:
        Gpu()
            : pageBaseX(0),
              pageBaseY(0),
              semiTransparency(0),
              textureDepth(TextureDepth::T4Bit),
              dithering(false),
              drawToDisplay(false),
              forceSetMaskBit(false),
              preserveMaskedPixels(false),
              field(Field::Top),
              textureDisable(false),
              hres(HorizontalRes::fromFields(320, 240)),
              vres(VerticalRes::Y240Lines),
              vmode(VMode::Ntsc),
              displayDepth(DisplayDepth::D15Bits),
              interlaced(false),
              displayEnabled(false),
              interrupt(false),
              dmaDirection(DmaDirection::Off),
              renderer(new Renderer()),
              vram(new VRAM(this)) {
            
        }
        
        uint32_t status();
        
        // Handles writes to the GP0 command register
        void gp0(uint32_t val);
        
        // GP0(0xE1): command
        void gp0DrawMode(uint32_t val);
        
        // GP0(0XE3): Set Drawing Area top left
        void gp0DrawingAreaTopLeft(uint32_t val) {
            drawingAreaTop = static_cast<uint16_t>((val >> 10) & 0x3FF);
            drawingAreaLeft = static_cast<uint16_t>(val & 0x3FF);
        }
        
        // GP0(0xE4): Set Drawing Area bottom right
        void gp0DrawingAreaBottomRight(uint32_t val) {
            drawingAreaBottom = static_cast<uint16_t>((val >> 10) & 0x3FF);
            drawingAreaRight = static_cast<uint16_t>(val & 0x3FF);
            
            renderer->setDrawingArea(drawingAreaRight, drawingAreaBottom);
        }
        
        // GP0(0xE5): Set Drawing Offset
        void gp0DrawingOffset(uint32_t val) {
            uint16_t x = (static_cast<uint16_t>(val) & 0x7FF);
            uint16_t y = (static_cast<uint16_t>(val >> 11) & 0x7FF);
            
            // Values are 11bit two's complement-signed values,
            // we need to shift the value to 16 bits,
            // to force a sign extension
            drawingXOffset = static_cast<int16_t>(x << 5) >> 5;
            drawingYOffset = static_cast<int16_t>(y << 5) >> 5;
            
            // Update rendering offset
            renderer->setDrawingOffset(drawingXOffset, drawingYOffset);
            
            // XXX Temporary hack: force display when changing offset,
            // since we don't have proper timings
            renderer->display();
        }
        
        // GP0(0xE2): Set Texture Window
        void gp0TextureWindow(uint32_t val) {
            textureWindowXMask   = static_cast<uint8_t>(val & 0x1F);
            textureWindowYMask   = static_cast<uint8_t>((val >> 5) & 0x1F);
            textureWindowXOffset = static_cast<uint8_t>((val >> 10) & 0x1F);
            textureWindowYOffset = static_cast<uint8_t>((val >> 15) & 0x1F);
        }
        
        // GP0(0xE6): Set Mask Bit Setting
        void gp0MaskBitSetting(uint32_t val) {
            forceSetMaskBit = (val & 1) != 0;
            preserveMaskedPixels = (val & 2) != 0;
        }
        
        // GP0(0x00): No operations
        void gp0Nop(uint32_t val) {
            // NOP
        }
        
        static inline bool get_bit(uint32_t num, int b) {
            return num & (1 << b);
        }
        
        // GP0(0x28): Momochrome Opaque Quadrilateral
        void gp0QuadMonoOpaque(uint32_t val) {
            Position positions[] = {
                Position::fromGp0P(gp0Command.buffer[1]),
                Position::fromGp0P(gp0Command.buffer[2]),
                Position::fromGp0P(gp0Command.buffer[3]),
                Position::fromGp0P(gp0Command.buffer[4]),
            };
            
            // A single color repeated 4 times
            Color colors[] = {
                Color::fromGp0(gp0Command.buffer[0]),
                Color::fromGp0(gp0Command.buffer[0]),
                Color::fromGp0(gp0Command.buffer[0]),
                Color::fromGp0(gp0Command.buffer[0]),
            };
            
            renderer->pushQuad(positions, colors);
        }
        
        // TODO; Remove
        void vertexOrder(Position* p) {
            auto crossZ = (p[1].x - p[0].x) * (p[2].y - p[0].y) - (p[1].y - p[0].y) * (p[2].x - p[0].x);
            
            if(crossZ < 0) {
                std::swap(p[0], p[1]);
            }
        }
        
        // GP0(0x30): Shaded Opaque Triangle
        void gp0TriangleShadedOpaque(uint32_t val) {
            Position positions[] = {
                Position::fromGp0P(gp0Command.buffer[1]),
                Position::fromGp0P(gp0Command.buffer[3]),
                Position::fromGp0P(gp0Command.buffer[5]),
            };
            
            Color colors[] = {
                Color::fromGp0(gp0Command.index(0)),
                Color::fromGp0(gp0Command.index(2)),
                Color::fromGp0(gp0Command.index(4)),
            };
            
            renderer->pushTriangle(positions, colors);
        }
        
        // GP0(0x34): Shaded Texture Opaque Triangle
        void gp0TriangleTexturedShadedOpaque(uint32_t val) {
            Position positions[] = {
                Position::fromGp0P(gp0Command.buffer[1]),
                Position::fromGp0P(gp0Command.buffer[4]),
                Position::fromGp0P(gp0Command.buffer[7]),
            };
            
            Color colors[] = {
                Color::fromGp0(gp0Command.index(0)),
                Color::fromGp0(gp0Command.index(3)),
                Color::fromGp0(gp0Command.index(6)),
            };
            
            renderer->pushTriangle(positions, colors);
        }
        
        // GP0(0x38): gp0QuadShadedOpaque
        void gp0QuadShadedOpaque(uint32_t val) {
            Position positions[] = {
                Position::fromGp0P(gp0Command.buffer[1]),
                Position::fromGp0P(gp0Command.buffer[3]),
                Position::fromGp0P(gp0Command.buffer[5]),
                Position::fromGp0P(gp0Command.buffer[7]),
            };
            
            Color colors[] = {
                Color::fromGp0(gp0Command.buffer[0]),
                Color::fromGp0(gp0Command.buffer[2]),
                Color::fromGp0(gp0Command.buffer[4]),
                Color::fromGp0(gp0Command.buffer[6]),
            };
            
            renderer->pushQuad(positions, colors);
        }

        // GP0(0x3C): Shaded Texture Opaque Quad
        void gp0QuadTexturedShadedOpaque(uint32_t val) {
            Position positions[] = {
                Position::fromGp0P(gp0Command.index(1)),
                Position::fromGp0P(gp0Command.index(4)),
                Position::fromGp0P(gp0Command.index(7)),
                Position::fromGp0P(gp0Command.index(10)),
            };
            
            // TODO; Textures
            Color colors[] = {
                Color::fromGp0(gp0Command.index(0)),
                Color::fromGp0(gp0Command.index(3)),
                Color::fromGp0(gp0Command.index(6)),
                Color::fromGp0(gp0Command.index(9)),
            };
            
            renderer->pushQuad(positions, colors);
        }
        
        // Helper function
        void renderRectangle(Position position, Color color, uint16_t width, uint16_t height);
        
        void gp0VarRectangleMonoOpaque(uint32_t val);
        void gp0DotRectangleMonoOpaque(uint32_t val);
        void gp08RectangleMonoOpaque(uint32_t val);
        void gp016RectangleMonoOpaque(uint32_t val);
        
        // TODO; Testing
        void gp0Rectangle(uint32_t val) {
            uint32_t color = gp0Command.buffer[0] & 0xffffff;
            
            if(color != 0) {
                printf("");
            }
            
            uint32_t cmd = gp0Command.buffer[1];
            uint32_t x = (cmd & 0xffff) + drawingXOffset;
            uint32_t y = (cmd >> 16) + drawingYOffset;
            
            vram->setPixel(x, y, color);
        }
        
        // GP0(0x01): Clear Cache
        void gp0ClearCache(uint32_t val) {
            // TODO; Implement me
        }
        
        // GP0(0x02): Full VRam
        void gp0FillVRam(uint32_t val) {
            Color color = Color::fromGp0(gp0Command.index(0));
            uint32_t cords = gp0Command.buffer[1];
            uint32_t res = gp0Command.buffer[2];
            
            uint32_t xPos = (cords & 0xFFFF) & 0x3FF;
            uint32_t yPos = ((cords & 0xFFFF0000) >> 16) & 0x1FF;
            
            uint32_t width = (((res & 0xFFFF) - 1) & 0x3FF) + 1;
            uint32_t height = ((((res & 0xFFFF0000) >> 16) - 1) & 0x1FF) + 1;
            
            for(uint32_t y = yPos; y < (height + yPos); y++) {
                for(uint32_t x = xPos; x < (width + xPos); x++) {
                    vram->setPixel(x, y, (static_cast<uint32_t>(color.b) << 10) | (static_cast<uint32_t>(color.g) << 5) | static_cast<uint32_t>(color.r));
                }
            }
        }
        
        // GP0(0x20): 
        void gp0TriangleMonoOpaque(uint32_t val) {
            Position positions[] = {
                Position::fromGp0P(gp0Command.index(1)),
                Position::fromGp0P(gp0Command.index(2)),
                Position::fromGp0P(gp0Command.index(3)),
            };
            
            // Mono
            Color colors[] = {
                Color::fromGp0(gp0Command.index(0)),
                Color::fromGp0(gp0Command.index(0)),
                Color::fromGp0(gp0Command.index(0)),
            };
            
            renderer->pushTriangle(positions, colors);
            //renderer->display();
        }
        
        void gp0TriangleTexturedOpaque(uint32_t val) {
            Position positions[] = {
                Position::fromGp0P(gp0Command.index(1)),
                Position::fromGp0P(gp0Command.index(3)),
                Position::fromGp0P(gp0Command.index(5))
            };
            
            // This uses textures along side a color
            Color colors[] = {
                Color::fromGp0(gp0Command.index(0)),
                Color::fromGp0(gp0Command.index(0)),
                Color::fromGp0(gp0Command.index(0)),
            };
            
            renderer->pushTriangle(positions, colors);
        }
        
        void gp0TriangleRawTexturedOpaque(uint32_t val) {
            // Color is ignored for raw-textures
            Position positions[] = {
                Position::fromGp0P(gp0Command.index(1)),
                Position::fromGp0P(gp0Command.index(3)),
                Position::fromGp0P(gp0Command.index(5))
            };
            
            // TODO; Textures aren't currently supported
            Color colors[] = {
                {0x80, 0x00, 0x00},
                {0x80, 0x00, 0x00},
                {0x80, 0x00, 0x00},
            };
            
            renderer->pushTriangle(positions, colors);
        }

        // GP0(0x2C): Quad Raw Texture Blend Opqaue
        void gp0QuadTextureBlendOpaque(uint32_t val) {
            Position positions[] = {
                Position::fromGp0P(gp0Command.buffer[1]),
                Position::fromGp0P(gp0Command.buffer[3]),
                Position::fromGp0P(gp0Command.buffer[5]),
                Position::fromGp0P(gp0Command.buffer[7]),
            };
            
            // TODO; Textures aren't currently supported
            Color colors[] = {
                {0x80, 0x00, 0x00},
                {0x80, 0x00, 0x00},
                {0x80, 0x00, 0x00},
                {0x80, 0x00, 0x00},
            };
            
            renderer->pushQuad(positions, colors);
        }
        
        // GP0(0x2D): Quad Raw Texture Blend Opqaue
        void gp0QuadRawTextureBlendOpaque(uint32_t val) {
            Position positions[] = {
                Position::fromGp0P(gp0Command.buffer[1]),
                Position::fromGp0P(gp0Command.buffer[3]),
                Position::fromGp0P(gp0Command.buffer[5]),
                Position::fromGp0P(gp0Command.buffer[7]),
            };
            
            // TODO; Textures aren't currently supported
            // Uses a texture along side a color
            Color colors[] = {
                Color::fromGp0(gp0Command.index(0)),
                Color::fromGp0(gp0Command.index(0)),
                Color::fromGp0(gp0Command.index(0)),
                Color::fromGp0(gp0Command.index(0)),
            };
            
            renderer->pushQuad(positions, colors);
        }
        
        // GP0(0xA0): Load Image
        // From CPU to VRAM
        void gp0ImageLoad(uint32_t val) {
            uint32_t cords = gp0Command.buffer[1];
            uint32_t res = gp0Command.buffer[2];
            
            uint32_t x = (cords & 0xFFFF) & 0x3FF;
            uint32_t y = ((cords & 0xFFFF0000) >> 16) & 0x1FF;
            
            // 2nd  Source Coord      (YyyyXxxxh) ; write to GP0 port (as usual?)
            uint32_t width = (((res & 0xFFFF) - 1) & 0x3FF) + 1;
            uint32_t height = ((((res & 0xFFFF0000) >> 16) - 1) & 0x1FF) + 1;
            
            uint32_t imgSize = (width * height);
            
            // If we have an odd number of pixels we must round up
            // since we transfer 32bits at a time. There'll be 16bits
            // of padding in the last word
            imgSize = (imgSize + 1) & ~1;
            
            // Signal to VRAM to begin the transfer to the CPU
            vram->beginTransfer(x, y, width, height, imgSize);
            
            // Store number of words expected for this image
            gp0CommandRemaining = (imgSize / 2);
            
            // Put the GP0 state machine into the VRam mode
            gp0Mode = VRam;
            
            // Update signals
            canSendVRAMToCPU = true;
            canSendCPUToVRAM = false;
        }
        
        // TODO; Remove
        uint32_t startX, startY, curX, curY, endX, endY;
        
        // GP0(0xC0): Load Store
        // From VRAM to CPU
        void gp0ImageStore(uint32_t val) {
            uint32_t cords = gp0Command.buffer[1];
            uint32_t res = gp0Command.buffer[2];
            
            uint32_t x = (cords & 0xFFFF) & 0x3FF;
            uint32_t y = ((cords & 0xFFFF0000) >> 16) & 0x1FF;
            
            uint32_t width = (((res & 0xFFFF) - 1) & 0x3FF) + 1;
            uint32_t height = ((((res & 0xFFFF0000) >> 16) - 1) & 0x1FF) + 1;
            
            startX = curX = x;
            startY = curY = y;
            
            endX = startX + width;
            endY = startY + height;
            
            readMode = VRam;
            
            // Update signals
            canSendVRAMToCPU = false;
            canSendCPUToVRAM = true;
        }
        
        void gp0VramToVram(uint32_t val) {
            uint32_t cords = gp0Command.buffer[1];
            uint32_t dests = gp0Command.buffer[2];
            uint32_t res = gp0Command.buffer[3];
            
            uint32_t srcX = (cords & 0xFFFF) & 0x3FF;
            uint32_t srcY = ((cords & 0xFFFF0000) >> 16) & 0x1FF;
            
            uint32_t dstX = (dests & 0xFFFF) & 0x3FF;
            uint32_t dstY = ((dests & 0xFFFF0000) >> 16) & 0x1FF;
            
            uint32_t width = (((res & 0xFFFF) - 1) & 0x3FF) + 1;
            uint32_t height = ((((res & 0xFFFF0000) >> 16) - 1) & 0x1FF) + 1;
            
            bool dir = srcX < dstX;
            
            for(uint32_t y = 0; y < height; y++) {
                for(uint32_t x = 0; x < width; x++) {
                    uint32_t posX = (!dir) ? x : width - 1 - x;
                    
                    uint16_t color = vram->getPixelRGB888((srcX + posX), (srcY + y));
                    vram->setPixel(dstX + posX, dstY + y, color);
                }
            }
        }
        
        // Handles writes to the GP1 command register
        void gp1(uint32_t val);
        
        // GP1(0x00): soft rest
        void gp1Reset(uint32_t val);
        
        // GP1(0x80): Display Mode
        void gp1DisplayMode(uint32_t val);
        
        // GP1(0x03): Display Enable
        void gp1DisplayEnable(uint32_t val) {
            displayEnabled = (val & 1) == 0;
        }
        
        // GP1(0x04): DMA Direction
        void gp1DmaDirection(uint32_t val);
        
        // GP1(0x05): Display VRAM Start
        void gp1DisplayVramStart(uint32_t val) {
            displayVramXStart = static_cast<uint16_t>(val & 0x3FE);
            displayVramYStart = static_cast<uint16_t>((val >> 10) & 0x1FF);
        }
        
        // GP1(0x06): Display Horizontal Range
        void gp1DisplayHorizontalRange(uint32_t val) {
            displayHorizStart = static_cast<uint16_t>(val & 0xFFF);
            displayHorizEnd = static_cast<uint16_t>((val >> 12) & 0xFFF);
        }
        
        // GP1(0x07): Display Vertical Range
        void gp1DisplayVerticalRange(uint32_t val) {
            displayLineStart = static_cast<uint16_t>(val & 0x3FF);
            displayLineEnd = static_cast<uint16_t>((val >> 10) & 0x3FF);
        }
        
        // GP1(0x01): Reset Command Buffer
        void gp1ResetCommandBuffer(uint32_t val) {
            gp0Command.clear();
            //gp0CommandRemaining = 0;
            gp0Mode = Command; 
        }
        
        // GP1(0x02): Acknowledge Interrupt
        void gp1AcknowledgeIrq(uint32_t val) {
            interrupt = false;
        }
        
        // Retrieve value of the "read" register
        uint32_t read(uint32_t addr);
        
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
        
        // Signals
        // TODO; Rename to isReady instead of can. Would make more sense..
        bool canSendVRAMToCPU;
        bool canSendCPUToVRAM;
        bool canReceiveDMABlock;
        
        uint32_t _read = 0;
        
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
