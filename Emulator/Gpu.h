#pragma once

#ifndef GPU_H
#define GPU_H

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>

namespace Emulator {
    // Depth of the pixel values in a texture page
    enum class TextureDepth {
        T4Bit = 0,
        T8Bit = 1,
        T15Bit = 2,
    };

    // Interlaced output splits each frame in two fields
    enum class Field {
        Top = 1,
        Bottom = 0,
    };

    // Video output horizontal resolution
    struct HorizontalRes {
        uint8_t value;

        static HorizontalRes fromFields(uint8_t hr1, uint8_t hr2) {
            uint8_t hr = (hr2 & 1) | ((hr1 & 3) << 1);
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
        // Command buffer: the longuest possible command is GP0(0x3E)
        // which takes 12 paramaters
        uint32_t buffer[12];
        
        // Number of words queued in buffer
        uint8_t len;
        
        CommandBuffer() : buffer(), len(0) {}
        
        class Index {
        public:
            using Output = uint32_t;
            
            uint32_t* index(CommandBuffer& buffer, size_t index) const {
                if (index >= buffer.len) {
                    throw std::out_of_range("Command buffer index out of range: " + std::to_string(index));
                }
                
                return &buffer.buffer[index];
            }
        };
        
        void clear() {
            len = 0;
        }
        
        void pushWord(uint32_t word) {
            buffer[static_cast<size_t>(len)] = word;
            len++;
        }
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
              hres(HorizontalRes::fromFields(0, 0)),
              vres(VerticalRes::Y240Lines),
              vmode(VMode::Ntsc),
              displayDepth(DisplayDepth::D15Bits),
              interlaced(false),
              displayDisabled(true),
              interrupt(false),
              dmaDirection(DmaDirection::Off) {}
        
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
        
        void gp0DrawingBottomRight(uint32_t val) {
            drawingAreaBottom = static_cast<uint16_t>((val >> 10) & 0x3FF);
            drawingAreaRight = static_cast<uint16_t>(val & 0x3FF);
        }
        
        // GP0(0xE5): Set Drawing Offset
        void gp0DrawingOffset(uint32_t val) {
            uint16_t x = (static_cast<uint16_t>(val) & 0x7FF);
            uint16_t y = (static_cast<uint16_t>(val >> 11) & 0x7FF);
            
            // Values are 11bit two's complement signed values,
            // we need to shift the value to 16bits,
            // to force sing extension
            drawingXOffset = static_cast<int16_t>(x << 5) >> 5;
            drawingYOffset = static_cast<int16_t>(y << 5) >> 5;
        }
        
        // GP0(0xE2): Set Texture Window
        void gp0TextureWindow(uint32_t val) {
            textureWindowXMask = static_cast<uint8_t>(val & 0x1F);
            textureWindowYMask = static_cast<uint8_t>((val >> 5) & 0x1F);
            textureWindowYMask = static_cast<uint8_t>((val >> 10) & 0x1F);
            textureWindowYMask = static_cast<uint8_t>((val >> 15) & 0x1F);
        }
        
        // GP0(0xE6): Set Mask Bit Setting
        void gp0MaskBitSetting(uint32_t val) {
            forceSetMaskBit = (val & 1) != 0;
            preserveMaskedPixels = (val & 2) != 0;
        }
        
        // Handles writes to the GP1 command register
        void gp1(uint32_t val);
        
        // GP1(0x00): soft rest
        void gp1Reset(uint32_t val);
        
        // GP1(0x80): Display Mode
        void gp1DisplayMode(uint32_t val);
        
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
        bool displayDisabled;        // Disable the display
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
        
        // Buffer containing the current GP0 command
        CommandBuffer gp0Command;
        
        // Remaining words for the current GP0 command
        uint32_t gp0CommandRemaining;
        
        // Pointer to the method implementing the current GP command
        std::function<void(Gpu&)> Gp0CommandMethod;
    };
}
#endif // GPU_H
