#include "Gpu.h"

#include "Rendering/renderer.h"
#include "vram.h"

#include <ios>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

#include "../Memory/IRQ.h"

Emulator::Gpu::Gpu()
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

void Emulator::Gpu::step(uint32_t cycles) {
    /**
      * The PSone/PAL video clock is the cpu clock multiplied by 11/7.
      * CPU Clock   =  33.868800MHz (44100Hz*300h)
      * Video Clock =  53.222400MHz (44100Hz*300h*11/7) or 53.690000MHz in Ntsc mode?
      */
    _cycles += (cycles * 11 / 7);
    
    /**
     * Horizontal Timings
     * PAL:  3406 video cycles per scanline (or 3406.1 or so?)
     * NTSC: 3413 video cycles per scanline (or 3413.6 or so?)
     */
    uint32_t htiming = ((vmode == VMode::Pal) ? 3406 : 3413);
    
    /**
    * Vertical Timings
    * PAL:  314 scanlines per frame (13Ah)
    * NTSC: 263 scanlines per frame (107h)
    */
    uint32_t vtiming = ((vmode == VMode::Pal) ? 314 : 263);
    
    if(_cycles >= htiming) {
        _cycles -= htiming;
        _scanLine++;
        
        if(vres != VerticalRes::Y480Lines) {
            isOddLine = (_scanLine & 0x1);
        }
        
        if(_scanLine >= vtiming) {
            _scanLine = 0;
            
            if(interlaced && vres == VerticalRes::Y480Lines) {
                isOddLine = !isOddLine;
                field = static_cast<Field>(!isOddLine);
            }
            
            renderer->display();
            
            // TODO; This shouldn't always occur?
            IRQ::trigger(IRQ::VBlank);
        }
    }
    
    // Update timings for the timers
    
    /**
     * Dots per scanline are, depending on horizontal resolution, and on PAL/NTSC:
     * 256pix/PAL: 3406/10 = 340.6 dots      256pix/NTSC: 3413/10 = 341.3 dots
     * 320pix/PAL: 3406/8  = 425.75 dots     320pix/NTSC: 3413/8  = 426.625 dots
     * 512pix/PAL: 3406/5  = 681.2 dots      512pix/NTSC: 3413/5  = 682.6 dots
     * 640pix/PAL: 3406/4  = 851.5 dots      640pix/NTSC: 3413/4  = 853.25 dots
     * 368pix/PAL: 3406/7  = 486.5714 dots   368pix/NTSC: 3413/7  = 487.5714 dots
     */
    
    isInHBlank = _cycles < displayHorizStart || _cycles > displayHorizEnd;
    
    if(isInHBlank) {
        printf("");
    }
    
    isInVBlank = _scanLine < displayLineStart || _scanLine > displayLineEnd;
    
    dot = dotCycles[hres.hr2 << 2 | hres.hr1];
}

uint32_t Emulator::Gpu::status() {
    // https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#1f801814h-gpustat-gpu-status-register-r
    
    uint32_t status = 0;
    
    // Bits 0-3: Texture page X base (N * 64)
    status |= static_cast<uint32_t>(pageBaseX) << 0;
    
    // Bit 4: Texture page Y base 1 (N * 256)
    status |= static_cast<uint32_t>(pageBaseY) << 4;
    
    // Bits 5-6: Semi-transparency mode
    status |= static_cast<uint32_t>(semiTransparency) << 5; // Mask to 2 bits
    
    // Bits 7-8: Texture page color depth
    status |= (static_cast<uint32_t>(textureDepth)) << 7; // Mask to 2 bits
    
    // Bit 9: Dither 24bit to 15bit (0=Off, 1=Enabled)
    status |= static_cast<uint32_t>(dithering) << 9;
    
    // Bit 10: Drawing to display area (0=Prohibited, 1=Allowed)
    status |= static_cast<uint32_t>(drawToDisplay) << 10;
    
    // Bit 11: Set Mask-bit when drawing (0=No, 1=Yes)
    status |= (forceSetMaskBit ? 1 : 0) << 11;
    
    // Bit 12: Don't draw to masked areas (0=Always, 1=Not to masked areas)
    status |= static_cast<uint32_t>(preserveMaskedPixels) << 12;
    
    // Bit 13: Interlace Field (0 = Top, 1 = Bottom)
    status |= static_cast<uint32_t>(field) << 13;
    
    // Bit 14: ?
    
    status |= static_cast<uint32_t>(textureDisable) << 15;
    status |= hres.intoStatus();
    
    // Bit 19: Vertical resolution (0=240, 1=480)
    // Setting this to 1 locks the bios
    status |= (static_cast<uint32_t>(0/*vres*/)) << 19;
    
    // Bit 20: Video mode (0=NTSC, 1=PAL)
    status |= static_cast<uint32_t>(vmode) << 20;
    
    // Bit 21: Display area color depth (0=15bit, 1=24bit)
    status |= static_cast<uint32_t>(displayDepth) << 21;
    
    // Bit 22: Vertical Interlace (0=Off, 1=On)
    status |= static_cast<uint32_t>(interlaced) << 22;
    
    // Bit 23: Display enable (0=Enabled, 1=Disabled)
    status |= static_cast<uint32_t>(!displayEnabled) << 23;
    
    // Bit 24: Interrupt request (0=Off, 1=IRQ)
    status |= static_cast<uint32_t>(interrupt) << 24;
    
    // Bit 26: Ready to receive command word (0=No, 1=Ready)
    status |= (/*gp0CommandRemaining == 0 ? 1 : 0*/1) << 26;
    
    // Bit 27: Ready to send VRAM to CPU (0=No, 1=Ready)
    status |= (/*canSendVRAMToCPU*/true ? 1 : 0) << 27;
    
    // Bit 28: Ready to receive DMA block (0=No, 1=Ready)
    status |= (/*canReceiveDMABlock*/true ? 1 : 0) << 28;
    
    // Bits 29-30: DMA Direction (0=Off, 1=?, 2=CPUtoGP0, 3=GPUREADtoCPU)
    status |= (static_cast<uint32_t>(dmaDirection)) << 29;
    
    // Bit 31: Drawing even/odd lines in interlace mode (0=Even, 1=Odd)
    status |= isOddLine << 31;
    
    uint32_t dma = 0;
    if (dmaDirection == DmaDirection::Off) {
        dma = 0;
    } else if (dmaDirection == DmaDirection::Fifo) {
        dma = (gp0CommandRemaining == 0 ? 1 : 0);
    } else if (dmaDirection == DmaDirection::CpuToGp0) {
        dma = (status >> 28) & 1;
    } else if (dmaDirection == DmaDirection::VRamToCpu) {
        dma = (status >> 27) & 1;
    }
    
    status |= dma << 25;
    
    return status;
    //return 0x1c000000;
}

void Emulator::Gpu::gp0(uint32_t val) {
    if(gp0CommandRemaining == 0) {
        switch (uint8_t opcode = (val >> 24) & 0xFF) {
        case 0x00:
            
            gp0CommandRemaining = 1;
            Gp0CommandMethod = &Gpu::gp0Nop;
            
            break;
        case 0x01:
            
            gp0CommandRemaining = 1;
            Gp0CommandMethod = &Gpu::gp0ClearCache;
            
            break;
        case 0x2:
            
            gp0CommandRemaining = 3;
            Gp0CommandMethod = &Gpu::gp0FillVRam;
            
            break;
        case 0x20: {
            // GP0(20h) - Monochrome three-point polygon, opaque
            
            gp0CommandRemaining = 4;
            Gp0CommandMethod = &Gpu::gp0TriangleMonoOpaque;
            
            break;
        }
        case 0x22: {
            // GP0(22h) - Monochrome three-point polygon, semi-transparent
            
            curAttribute = {1, 0};
            
            gp0CommandRemaining = 4;
            Gp0CommandMethod = &Gpu::gp0TriangleMonoOpaque;
            
            break;
        }
        case 0x24: {
            // GP0(24h) - Textured three-point polygon, opaque, texture-blending
            
            curAttribute = {0, 1, 2};
            
            gp0CommandRemaining = 7;
            Gp0CommandMethod = &Gpu::gp0TriangleTexturedOpaque;
            
            break;
        }
        case 0x25: {
            // GP0(25h) - Textured three-point polygon, opaque, raw-texture
            
            curAttribute = {0, 0, 1};
            
            gp0CommandRemaining = 7;
            Gp0CommandMethod = &Gpu::gp0TriangleRawTexturedOpaque;
            
            break;
        }
        case 0x26: {
            // GP0(26h) - Textured three-point polygon, semi-transparent, texture-blending
            
            curAttribute = {1, 1, 2};
            
            gp0CommandRemaining = 7;
            Gp0CommandMethod = &Gpu::gp0TriangleTexturedOpaque;
            
            break;
        }
        case 0x27: {
            // GP0(27h) - Textured three-point polygon, semi-transparent, raw-texture
            
            curAttribute = {1, 0, 1};
            
            gp0CommandRemaining = 7;
            Gp0CommandMethod = &Gpu::gp0TriangleRawTexturedOpaque;
            
            break;
        }
        case 0x2C: {
            // GP0(2Ch) - Textured four-point polygon, opaque, texture-blending
            
            curAttribute = {0, 1, 1};
            
            gp0CommandRemaining = 9;
            Gp0CommandMethod = &Gpu::gp0QuadTextureBlendOpaque;
            
            break;
        }
        case 0x2D: {
            // GP0(2Dh) - Textured four-point polygon, opaque, raw-texture
            
            curAttribute = { 0, 0, 2 };
            
            gp0CommandRemaining = 9;
            Gp0CommandMethod = &Gpu::gp0QuadRawTextureBlendOpaque;
            
            break;
        }
        case 0x2E: {
            // GP0(2Eh) - Textured four-point polygon, semi-transparent, texture-blending
            
            curAttribute = {1, 1, 1};
            
            gp0CommandRemaining = 9;
            Gp0CommandMethod = &Gpu::gp0QuadTextureBlendOpaque;
            
            break;
        }
        case 0x2F: {
            // GP0(2Fh) - Textured four-point polygon, semi-transparent, raw-texture
            
            curAttribute = {1, 0, 2};
            gp0CommandRemaining = 9;
            Gp0CommandMethod = &Gpu::gp0QuadRawTextureBlendOpaque;
            
            break;
        }
        case 0x28:
            // GP0(28h) - Monochrome four-point polygon, opaque
            
            gp0CommandRemaining = 5;
            Gp0CommandMethod = &Gpu::gp0QuadMonoOpaque;
            
            break;
        case 0x2A: {
            // GP0(2Ah) - Monochrome four-point polygon, semi-transparent
            
            curAttribute = {1, 0};
            
            gp0CommandRemaining = 5;
            Gp0CommandMethod = &Gpu::gp0QuadMonoOpaque;
            
            break;
        }
        case 0x30: {
            // GP0(30h) - Shaded three-point polygon, opaque
            
            gp0CommandRemaining = 6;
            Gp0CommandMethod = &Gpu::gp0TriangleShadedOpaque;
            
            break;
        }
        case 0x32: {
            // GP0(32h) - Shaded three-point polygon, semi-transparent
            
            curAttribute = {1, 0};
            
            gp0CommandRemaining = 6;
            Gp0CommandMethod = &Gpu::gp0TriangleShadedOpaque;
            
            break;
        }
        case 0x34: {
            // GP0(34h) - Shaded Textured three-point polygon, opaque, texture-blending
            
            curAttribute = {0, 1};
            
            gp0CommandRemaining = 9;
            Gp0CommandMethod = &Gpu::gp0TriangleTexturedShadedOpaque;
            
            break;
        }
        case 0x36: {
            // GP0(36h) - Shaded Textured three-point polygon, semi-transparent, tex-blend
            
            curAttribute = {1, 1};
            
            gp0CommandRemaining = 9;
            Gp0CommandMethod = &Gpu::gp0TriangleTexturedShadedOpaque;
            
            break;
        }
        case 0x38:
            // GP0(38h) - Shaded four-point polygon, opaque
            
            gp0CommandRemaining = 8;
            Gp0CommandMethod = &Gpu::gp0QuadShadedOpaque;
            
            break;
        case 0x3A:
            // GP0(3Ah) - Shaded four-point polygon, semi-transparent
            
            curAttribute = {1, 0};
            
            gp0CommandRemaining = 8;
            Gp0CommandMethod = &Gpu::gp0QuadShadedOpaque;
            
            break;
        case 0x3C: {
            // GP0(3Ch) - Shaded Textured four-point polygon, opaque, texture-blending
            
            curAttribute = {0, 1, 2};
            
            gp0CommandRemaining = 12;
            Gp0CommandMethod = &Gpu::gp0QuadTexturedShadedOpaque;
            
            break;
        }
        case 0x3E: {
            // GP0(3Eh) - Shaded Textured four-point polygon, semi-transparent, tex-blend
            
            curAttribute = {1, 1, 2};
            
            gp0CommandRemaining = 12;
            Gp0CommandMethod = &Gpu::gp0QuadTexturedShadedOpaque;
            
            break;
        }
        case 0x60: {
            // GP0(60h) - Monochrome Rectangle (variable size) (opaque)
            
            gp0CommandRemaining = 3;
            Gp0CommandMethod = &Gpu::gp0VarRectangleMonoOpaque;
            
            break;
        }
        case 0x62: {
            // GP0(62h) - Monochrome Rectangle (variable size) (semi-transparent)
            
            curAttribute = {1, 0};
            
            gp0CommandRemaining = 3;
            Gp0CommandMethod = &Gpu::gp0VarRectangleMonoOpaque;
            
            break;
        }
        case 0x64: {
            // GP0(64h) - Textured Rectangle, variable size, opaque, texture-blending
            
            curAttribute = { 0, 1, 2 };
            
            gp0CommandRemaining = 4;
            Gp0CommandMethod = &Gpu::gp0VarTexturedRectangleMonoOpaque;
            
            break;
        }
        case 0x65: {
            // GP0(65h) - Textured Rectangle, variable size, opaque, raw-texture
            
            curAttribute = { 0, 0, 2};
            
            gp0CommandRemaining = 4;
            Gp0CommandMethod = &Gpu::gp0VarTexturedRectangleMonoOpaque;
            
            break;
        }
        case 0x66: {
            // GP0(66h) - Textured Rectangle, variable size, semi-transp, texture-blendin
            
            curAttribute = { 1, 1, 2 };
            
            gp0CommandRemaining = 4;
            Gp0CommandMethod = &Gpu::gp0VarTexturedRectangleMonoOpaque;
            
            break;
        }
        case 0x67: {
            // GP0(67h) - Textured Rectangle, variable size, semi-transp, raw-texture
            
            curAttribute = { 1, 0 };
            
            gp0CommandRemaining = 4;
            Gp0CommandMethod = &Gpu::gp0VarTexturedRectangleMonoOpaque;
            
            break;
        }
        case 0x68: {
            // GP0(68h) - Monochrome Rectangle (1x1) (Dot) (opaque)
            
            gp0CommandRemaining = 2;
            Gp0CommandMethod = &Gpu::gp0DotRectangleMonoOpaque;
            
            break;
        }
        case 0x6A: {
            // GP0(6Ah) - Monochrome Rectangle (1x1) (Dot) (semi-transparent)

            curAttribute = {1, 0};
            
            gp0CommandRemaining = 2;
            Gp0CommandMethod = &Gpu::gp0DotRectangleMonoOpaque;
            
            break;
        }
        case 0x70: {
            // GP0(70h) - Monochrome Rectangle (8x8) (opaque)
            
            gp0CommandRemaining = 2;
            Gp0CommandMethod = &Gpu::gp08RectangleMonoOpaque;
            
            break;
        }
        case 0x72: {
            // GP0(72h) - Monochrome Rectangle (8x8) (semi-transparent)
            
            curAttribute = {1, 0};
            
            gp0CommandRemaining = 2;
            Gp0CommandMethod = &Gpu::gp08RectangleMonoOpaque;
            
            break;
        }
        case 0x74: {
            // GP0(74h) - Textured Rectangle, 8x8, opaque, texture-blending
            
            curAttribute = {0, 1, 2};
            
            gp0CommandRemaining = 3;
            Gp0CommandMethod = &Gpu::gp08RectangleTexturedOpaqu;
            
            break;
        }
        case 0x75: {
            // GP0(75h) - Textured Rectangle, 8x8, opaque, raw-texture
            
            curAttribute = {0, 0, 2};
            
            gp0CommandRemaining = 3;
            Gp0CommandMethod = &Gpu::gp08RectangleTexturedOpaqu;
            
            break;
        }
        case 0x76: {
            // GP0(76h) - Textured Rectangle, 8x8, semi-transparent, texture-blending
            
            curAttribute = {1, 1, 2};
            
            gp0CommandRemaining = 3;
            Gp0CommandMethod = &Gpu::gp08RectangleTexturedOpaqu;
            
            break;
        }
        case 0x77: {
            // GP0(77h) - Textured Rectangle, 8x8, semi-transparent, raw-texture
            
            curAttribute = {1, 0, 2};
            
            gp0CommandRemaining = 3;
            Gp0CommandMethod = &Gpu::gp08RectangleTexturedOpaqu;
            
            break;
        }
        case 0x78: {
            // GP0(78h) - Monochrome Rectangle (16x16) (opaque)
            
            gp0CommandRemaining = 2;
            Gp0CommandMethod = &Gpu::gp016RectangleMonoOpaque;
            
            break;
        }
        case 0x7A: {
            // GP0(7Ah) - Monochrome Rectangle (16x16) (semi-transparent)
            
            curAttribute = {1, 0};
            
            gp0CommandRemaining = 2;
            Gp0CommandMethod = &Gpu::gp016RectangleMonoOpaque;
            
            break;
        }
        case 0x80: {
            gp0CommandRemaining = 4;
            Gp0CommandMethod = &Gpu::gp0VramToVram;
            
            break;
        }
        case 0xA0:
            gp0CommandRemaining = 3;
            Gp0CommandMethod = &Gpu::gp0ImageLoad;
            
            break;
        case 0xC0:
            gp0CommandRemaining = 2;
            Gp0CommandMethod = &Gpu::gp0ImageStore;
            
            break;
        case 0xE1:
            gp0CommandRemaining = 1;
            Gp0CommandMethod = &Gpu::gp0DrawMode;
            
            break;
        case 0xE2:
            gp0CommandRemaining = 1;
            Gp0CommandMethod = &Gpu::gp0TextureWindow;
            
            break;
        case 0xE3:
            gp0CommandRemaining = 1;
            Gp0CommandMethod = &Gpu::gp0DrawingAreaTopLeft;
            
            break;
        case 0xE4:
            gp0CommandRemaining = 1;
            Gp0CommandMethod = &Gpu::gp0DrawingAreaBottomRight;
            
            break;
        case 0xE5:
            gp0CommandRemaining = 1;
            Gp0CommandMethod = &Gpu::gp0DrawingOffset;
            
            break;
        case 0xE6:
            gp0CommandRemaining = 1;
            Gp0CommandMethod = &Gpu::gp0MaskBitSetting;
            
            break;
        default:
            printf("Unhandled GP0 command %x\n", opcode);
            std::cerr << "";
            
            return;
        }
        
        gp0Command.clear();
    }
    
    switch (gp0Mode) {
    case Command:
        gp0CommandRemaining--;
        gp0Command.pushWord(val);
        
        if(gp0CommandRemaining == 0) {
            // All of the parameters optioned; run the command
            Gp0CommandMethod(*this, val);
            
            // Reset current attributes
            curAttribute = {0, 0};
        }
        
        break;
    case VRam:
        // Draws 2 pixels at a time
        gp0CommandRemaining -= 1;
        
        if (gp0CommandRemaining == 0) {
            // Signal VRAM that image has finished loading.
            vram->endTransfer();
            
            
            // Upload texture to GPU
            //renderer->updateVramTextures(vram->texture4, vram->texture8, vram->texture16);
            
            // Load done, switch back to command mode
            gp0Mode = Gp0Mode::Command;
        } else {
            // Store pixel data into VRAM
            vram->store(val);
        }
        
        break;
    }
}

void Emulator::Gpu::gp0DrawMode(uint32_t val) {
    // https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#clut-attribute-color-lookup-table-aka-palette
    
    pageBaseX = static_cast<uint8_t>((val & 0xF));
    pageBaseY = static_cast<uint8_t>((val >> 4) & 1);
    semiTransparency = static_cast<uint8_t>((val >> 5) & 3);
    
    switch ((val >> 7) & 3) {
    case 0:
        textureDepth = TextureDepth::T4Bit;
        break;
    case 1:
        textureDepth = TextureDepth::T8Bit;
        break;
    case 2: case 3:
        // Texture page colors setting 3 (reserved) is same as setting 2 (15bit).
        textureDepth = TextureDepth::T15Bit;
        break;
    default:
        throw std::runtime_error("Unhandled texture depth " + std::to_string((val >> 7) & 3));
    }
     
    // Upload texture depth to GPU
    renderer->setTextureDepth(static_cast<int>(textureDepth));
    
    // Dither 24bit to 15bit (0=Off/strip LSBs, 1=Dither Enabled) ;GPUSTAT.9
    dithering = ((val >> 9) & 1) != 0;
    drawToDisplay = ((val >> 10) & 1) != 0; // (0=Prohibited, 1=Allowed)
    textureDisable = ((val >> 11) & 1) != 0;
    rectangleTextureFlipX = ((val >> 12) & 1) != 0;
    rectangleTextureFlipY = ((val >> 13) & 1) != 0;
}

void Emulator::Gpu::gp0DrawingAreaTopLeft(uint32_t val) {
    drawingAreaTop = static_cast<uint16_t>((val >> 10) & 0x3FF);
    drawingAreaLeft = static_cast<uint16_t>(val & 0x3FF);
}

void Emulator::Gpu::gp0DrawingAreaBottomRight(uint32_t val) {
    drawingAreaBottom = static_cast<uint16_t>((val >> 10) & 0x3FF);
    drawingAreaRight = static_cast<uint16_t>(val & 0x3FF);
    
    renderer->setDrawingArea(drawingAreaRight, drawingAreaBottom);
}

void Emulator::Gpu::gp0DrawingOffset(uint32_t val) {
    uint16_t x = (static_cast<uint16_t>(val) & 0x7FF);
    uint16_t y = (static_cast<uint16_t>(val >> 11) & 0x7FF);
    
    // Values are 11bit two's complement-signed values,
    // we need to shift the value to 16 bits,
    // to force a sign extension
    drawingXOffset = static_cast<int16_t>(x << 5) >> 5;
    drawingYOffset = static_cast<int16_t>(y << 5) >> 5;
    
    // Update rendering offset
    renderer->setDrawingOffset(drawingXOffset, drawingYOffset);
}

void Emulator::Gpu::gp0TextureWindow(uint32_t val) {
    textureWindowXMask   = static_cast<uint8_t>(val & 0x1F);
    textureWindowYMask   = static_cast<uint8_t>((val >> 5) & 0x1F);
    textureWindowXOffset = static_cast<uint8_t>((val >> 10) & 0x1F);
    textureWindowYOffset = static_cast<uint8_t>((val >> 15) & 0x1F);
}

void Emulator::Gpu::gp0MaskBitSetting(uint32_t val) {
    forceSetMaskBit = (val & 1) != 0;
    preserveMaskedPixels = (val & 2) != 0;
}

void Emulator::Gpu::gp0QuadMonoOpaque(uint32_t val) {
    Position positions[] = {
        Position::fromGp0(gp0Command.buffer[1]),
        Position::fromGp0(gp0Command.buffer[2]),
        Position::fromGp0(gp0Command.buffer[3]),
        Position::fromGp0(gp0Command.buffer[4]),
    };
    
    // A single color repeated 4 times
    Color colors[] = {
        Color::fromGp0(gp0Command.buffer[0]),
        Color::fromGp0(gp0Command.buffer[0]),
        Color::fromGp0(gp0Command.buffer[0]),
        Color::fromGp0(gp0Command.buffer[0]),
    };
    
    renderer->pushQuad(positions, colors, {}, curAttribute);
}

void Emulator::Gpu::gp0TriangleShadedOpaque(uint32_t val) {
    Position positions[] = {
        Position::fromGp0(gp0Command.buffer[1]),
        Position::fromGp0(gp0Command.buffer[3]),
        Position::fromGp0(gp0Command.buffer[5]),
    };
    
    Color colors[] = {
        Color::fromGp0(gp0Command.index(0)),
        Color::fromGp0(gp0Command.index(2)),
        Color::fromGp0(gp0Command.index(4)),
    };
    
    renderer->pushTriangle(positions, colors, {}, curAttribute);
}

void Emulator::Gpu::gp0TriangleTexturedShadedOpaque(uint32_t val) {
    Position positions[] = {
        Position::fromGp0(gp0Command.buffer[1]),
        Position::fromGp0(gp0Command.buffer[4]),
        Position::fromGp0(gp0Command.buffer[7]),
    };
    
    Color colors[] = {
        Color::fromGp0(gp0Command.index(0)),
        Color::fromGp0(gp0Command.index(3)),
        Color::fromGp0(gp0Command.index(6)),
    };
    
    renderer->pushTriangle(positions, colors, {}, curAttribute);
}

void Emulator::Gpu::gp0QuadShadedOpaque(uint32_t val) {
    Position positions[] = {
        Position::fromGp0(gp0Command.buffer[1]),
        Position::fromGp0(gp0Command.buffer[3]),
        Position::fromGp0(gp0Command.buffer[5]),
        Position::fromGp0(gp0Command.buffer[7]),
    };
    
    Color colors[] = {
        Color::fromGp0(gp0Command.buffer[0]),
        Color::fromGp0(gp0Command.buffer[2]),
        Color::fromGp0(gp0Command.buffer[4]),
        Color::fromGp0(gp0Command.buffer[6]),
    };
    
    renderer->pushQuad(positions, colors, {}, curAttribute);
}

void Emulator::Gpu::gp0QuadTexturedShadedOpaque(uint32_t val) {
    Position positions[] = {
        Position::fromGp0(gp0Command.index(1)),
        Position::fromGp0(gp0Command.index(4)),
        Position::fromGp0(gp0Command.index(7)),
        Position::fromGp0(gp0Command.index(10)),
    };
    
    Color colors[] = {
        Color::fromGp0(gp0Command.index(0)),
        Color::fromGp0(gp0Command.index(3)),
        Color::fromGp0(gp0Command.index(6)),
        Color::fromGp0(gp0Command.index(9)),
    };
    
    uint16_t c = static_cast<uint16_t>(gp0Command.index(2) >> 16);
    uint16_t p = static_cast<uint16_t>(gp0Command.index(4) >> 16);
    
    UV uvs[] = {
        UV::fromGp0(gp0Command.index(2), c, p, *this),
        UV::fromGp0(gp0Command.index(5), c, p, *this),
        UV::fromGp0(gp0Command.index(8), c, p, *this),
        UV::fromGp0(gp0Command.index(11), c, p,*this),
    };
    
    renderer->pushQuad(positions, colors, uvs, curAttribute);
}

void Emulator::Gpu::renderRectangle(Position position, Color color, UV uv, uint16_t width, uint16_t height) {
    Position positions[4] = {
        position,
        {position.x + width, position.y},
        {position.x + width, position.y + height},
        {position.x, position.y + height},
    };
    
    Color colors[4] = { color, color, color, color };
    UV uvs[4] = { uv, uv, uv, uv };
    
    renderer->pushRectangle(positions, colors, uvs, curAttribute);
}

void Emulator::Gpu::gp0VarRectangleMonoOpaque(uint32_t val) {
    Color color = Color::fromGp0(gp0Command.index(0));
    Position position = Position::fromGp0(gp0Command.index(1));
    
    uint32_t sizeData = gp0Command.index(2);
    
    uint16_t width = sizeData & 0xFFFF;
    uint16_t height = sizeData >> 16;
    
    renderRectangle(position, color, {}, width, height);
}

// 65
void Emulator::Gpu::gp0VarTexturedRectangleMonoOpaque(uint32_t val) {
    Color color = Color::fromGp0(gp0Command.index(0));
    Position position = Position::fromGp0(gp0Command.index(1));
    
    uint16_t c = static_cast<uint16_t>(gp0Command.index(2) >> 16);
    
    // Text page
    UV uv = UV::fromGp0(gp0Command.index(2), c, pageBaseX, pageBaseY * 255, *this);
    
    uint32_t sizeData = gp0Command.index(3);
    
    uint16_t width = sizeData & 0xFFFF;
    uint16_t height = sizeData >> 16;
    
    renderRectangle(position, color, uv, width, height);
}

void Emulator::Gpu::gp0DotRectangleMonoOpaque(uint32_t val) {
    Color color = Color::fromGp0(gp0Command.index(0));
    Position position = Position::fromGp0(gp0Command.index(1));
    
    renderRectangle(position, color, {}, 1, 1);
}

void Emulator::Gpu::gp08RectangleMonoOpaque(uint32_t val) {
    Color color = Color::fromGp0(gp0Command.index(0));
    Position position = Position::fromGp0(gp0Command.index(1));
    
    renderRectangle(position, color, {}, 8, 8);
}

void Emulator::Gpu::gp08RectangleTexturedOpaqu(uint32_t val) {
    Color color = Color::fromGp0(gp0Command.index(0));
    Position position = Position::fromGp0(gp0Command.index(1));
    
    uint16_t c = static_cast<uint16_t>(gp0Command.index(2) >> 16);
    uint16_t p = static_cast<uint16_t>(gp0Command.index(4) >> 16);
    
    UV uv = UV::fromGp0(gp0Command.index(2), c, p, *this);
    
    renderRectangle(position, color, uv, 8, 8);
}

void Emulator::Gpu::gp016RectangleMonoOpaque(uint32_t val) {
    Color color = Color::fromGp0(gp0Command.index(0));
    Position position = Position::fromGp0(gp0Command.index(1));
    
    renderRectangle(position, color, {}, 16, 16);
}

void Emulator::Gpu::gp0Rectangle(uint32_t val) {
    uint32_t color = gp0Command.buffer[0] & 0xffffff;
    
    if(color != 0) {
        printf("");
    }
    
    uint32_t cmd = gp0Command.buffer[1];
    uint32_t x = (cmd & 0xffff) + drawingXOffset;
    uint32_t y = (cmd >> 16) + drawingYOffset;
    
    vram->setPixel(x, y, color);
}

void Emulator::Gpu::gp0ClearCache(uint32_t val) {
    // TODO;
    /*memset(vram->pixels16, 0, vram->MAX_WIDTH * vram->MAX_HEIGHT * sizeof(uint16_t));
    memset(vram->pixels8, 0,  vram->MAX_WIDTH * vram->MAX_HEIGHT * sizeof(uint8_t));
    memset(vram->pixels4, 0,  vram->MAX_WIDTH * vram->MAX_HEIGHT * sizeof(uint8_t));*/
}

void Emulator::Gpu::gp0FillVRam(uint32_t val) {
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

void Emulator::Gpu::gp0TriangleMonoOpaque(uint32_t val) {
    Position positions[] = {
        Position::fromGp0(gp0Command.index(1)),
        Position::fromGp0(gp0Command.index(2)),
        Position::fromGp0(gp0Command.index(3)),
    };
    
    // Mono
    Color colors[] = {
        Color::fromGp0(gp0Command.index(0)),
        Color::fromGp0(gp0Command.index(0)),
        Color::fromGp0(gp0Command.index(0)),
    };
    
    renderer->pushTriangle(positions, colors, {}, curAttribute);
}

void Emulator::Gpu::gp0TriangleTexturedOpaque(uint32_t val) {
    Position positions[] = {
        Position::fromGp0(gp0Command.index(1)),
        Position::fromGp0(gp0Command.index(3)),
        Position::fromGp0(gp0Command.index(5))
    };
    
    // This uses textures along side a color
    Color colors[] = {
        Color::fromGp0(gp0Command.index(0)),
        Color::fromGp0(gp0Command.index(0)),
        Color::fromGp0(gp0Command.index(0)),
    };
    
    uint16_t c = static_cast<uint16_t>(gp0Command.index(2) >> 16);
    uint16_t p = static_cast<uint16_t>(gp0Command.index(4) >> 16);
    
    UV uvs[] = {
        UV::fromGp0(gp0Command.index(2), c, p, *this),
        UV::fromGp0(gp0Command.index(4), c, p, *this),
        UV::fromGp0(gp0Command.index(6), c, p, *this),
    };
    
    renderer->pushTriangle(positions, colors, uvs, curAttribute);
}

void Emulator::Gpu::gp0TriangleRawTexturedOpaque(uint32_t val) {
    // Color is ignored for raw-textures
    Position positions[] = {
        Position::fromGp0(gp0Command.index(1)),
        Position::fromGp0(gp0Command.index(3)),
        Position::fromGp0(gp0Command.index(5))
    };
    
    Color colors[] = {
        {0x80, 0x00, 0x00},
       {0x80, 0x00, 0x00},
       {0x80, 0x00, 0x00},
    };
    
    uint16_t c = static_cast<uint16_t>(gp0Command.index(2) >> 16);
    uint16_t p = static_cast<uint16_t>(gp0Command.index(4) >> 16);
    
    UV uvs[] = {
        UV::fromGp0(gp0Command.index(2), c, p, *this),
        UV::fromGp0(gp0Command.index(4), c, p, *this),
        UV::fromGp0(gp0Command.index(6), c, p, *this),
    };
    
    renderer->pushTriangle(positions, colors, uvs, curAttribute);
}

// 2C
void Emulator::Gpu::gp0QuadTextureBlendOpaque(uint32_t val) {
    Position positions[] = {
        Position::fromGp0(gp0Command.buffer[1]),
        Position::fromGp0(gp0Command.buffer[3]),
        Position::fromGp0(gp0Command.buffer[5]),
        Position::fromGp0(gp0Command.buffer[7]),
    };
    
    Color colors[] = {
        Color::fromGp0(gp0Command.index(0)),
        Color::fromGp0(gp0Command.index(0)),
        Color::fromGp0(gp0Command.index(0)),
        Color::fromGp0(gp0Command.index(0)),
    };
    
    uint16_t c = static_cast<uint16_t>(gp0Command.index(2) >> 16);
    uint16_t p = static_cast<uint16_t>(gp0Command.index(4) >> 16);
    
    UV uvs[] = {
        UV::fromGp0(gp0Command.index(2), c, p, *this),
        UV::fromGp0(gp0Command.index(4), c, p, *this),
        UV::fromGp0(gp0Command.index(6), c, p, *this),
        UV::fromGp0(gp0Command.index(8), c, p, *this),
    };
    
    renderer->pushQuad(positions, colors, uvs, curAttribute);
}

void Emulator::Gpu::gp0QuadRawTextureBlendOpaque(uint32_t val) {
    Position positions[] = {
        Position::fromGp0(gp0Command.buffer[1]),
        Position::fromGp0(gp0Command.buffer[3]),
        Position::fromGp0(gp0Command.buffer[5]),
        Position::fromGp0(gp0Command.buffer[7]),
    };
    
    // TODO; Textures aren't currently supported
    // Uses a texture along side a color
    Color colors[] = {
        Color::fromGp0(gp0Command.index(0)),
        Color::fromGp0(gp0Command.index(0)),
        Color::fromGp0(gp0Command.index(0)),
        Color::fromGp0(gp0Command.index(0)),
    };
    
    uint16_t c = static_cast<uint16_t>(gp0Command.index(2) >> 16);
    uint16_t p = static_cast<uint16_t>(gp0Command.index(4) >> 16);
    
    UV uvs[] = {
        UV::fromGp0(gp0Command.index(2), c, p, *this),
        UV::fromGp0(gp0Command.index(4), c, p, *this),
        UV::fromGp0(gp0Command.index(6), c, p, *this),
        UV::fromGp0(gp0Command.index(8), c, p, *this),
    };
    
    renderer->pushQuad(positions, colors, uvs, curAttribute);
}

void Emulator::Gpu::gp0ImageLoad(uint32_t val) {
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
}

void Emulator::Gpu::gp0ImageStore(uint32_t val) {
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
}

void Emulator::Gpu::gp0VramToVram(uint32_t val) const {
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
            
            uint16_t color = vram->getPixel((srcX + posX), (srcY + y));
            vram->setPixel(dstX + posX, dstY + y, color);
        }
    }
}

void Emulator::Gpu::gp1(uint32_t val) {
    uint32_t opcode = (val >> 24) & 0xFF;
    
    switch (opcode) {
    case 0x00:
        gp1Reset(val);
        break;
    case 0x01:
        gp1ResetCommandBuffer(val);
        break;
    case 0x02:
        gp1AcknowledgeIrq(val);
        break;
    case 0x03:
        gp1DisplayEnable(val);
        break;
    case 0x04:
        gp1DmaDirection(val);
        break;
    case 0x05:
        gp1DisplayVramStart(val);
        break;
    case 0x06:
        gp1DisplayHorizontalRange(val);
        break;
    case 0x07:
        gp1DisplayVerticalRange(val);
        break;
    case 0x08:
        gp1DisplayMode(val);
        break;
    case 0x10: {
        readMode = Command;
        
        switch (val % 8) {
            case 2: {
                _read = textureWindowXMask | (textureWindowYMask << 5)
                    | (textureWindowXOffset << 10) | (textureWindowYOffset << 15);
                
                break;
            }
            case 3: {
                _read = drawingAreaLeft | (drawingAreaTop << 10);
                
                break;
            }
            case 4: {
                _read = drawingAreaRight | (drawingAreaBottom << 10);
                
                break;
            }
            case 5: {
                _read = (static_cast<uint32_t>(drawingXOffset) & 0x7FF)
                    | ((static_cast<uint32_t>(drawingYOffset) & 0x7F) << 11);
                
                break;
            }
            case 7: {
                _read = 2; // GPU VERSION
                
                break;
            }
            case 8: {
                _read = 0;
                
                break;
            }
            default: {
                printf("Unknown GP1 read value %d", (val % 8));
                std::cerr << "";
                
                break;
            }
        }
        
        break;
    }
    default:
        std::cerr << "ERROR; Unhandled GPU command " << std::to_string(opcode) << '\n'; 
        break;
    }
}

void Emulator::Gpu::gp1Reset(uint32_t val) {
    interrupt = false;
    
    pageBaseX = 0;
    pageBaseY = 0;
    semiTransparency = 0;
    textureDepth = TextureDepth::T4Bit;
    textureWindowXMask = 0;
    textureWindowYMask = 0;
    textureWindowXOffset = 0;
    textureWindowYOffset = 0;
    dithering = false;
    drawToDisplay = false;
    textureDisable = false;
    rectangleTextureFlipX = false;
    rectangleTextureFlipY = false;
    drawingAreaLeft = 0;
    drawingAreaTop = 0;
    drawingAreaRight = 0;
    drawingAreaBottom = 0;
    drawingXOffset = 0;
    drawingYOffset = 0;
    forceSetMaskBit = false;
    preserveMaskedPixels = false;
    
    _read = 0;
    
    dmaDirection = DmaDirection::Off;
    
    displayEnabled = false;
    displayVramXStart = 0;
    displayVramYStart = 0;
    hres = HorizontalRes::fromFields(0, 0);
    vres = VerticalRes::Y240Lines;
    
    vmode = VMode::Ntsc;
    interlaced = true;
    displayHorizStart = 0x200;
    displayHorizEnd = 0xC00;
    displayLineStart = 0x10;
    displayLineEnd = 0x100;
    displayDepth = DisplayDepth::D15Bits;
    
    // XXX should also clear the command FIFO when we implement it
    // XXX Should also invalidate GPU chance if we ever implement it
}

void Emulator::Gpu::gp1DisplayMode(uint32_t val) {
    // https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#gp108h-display-mode
    
    uint8_t hr1 = static_cast<uint8_t>(val & 3);
    uint8_t hr2 = static_cast<uint8_t>((val >> 6)/* & 1*/);
    
    // Resolutions: 256, 320, 512, 640, 368
    hres = HorizontalRes::fromFields(hr1, hr2);
    
    vres = (val & 0x4) != 0 ? VerticalRes::Y480Lines : VerticalRes::Y240Lines;
    vmode = (val & 0x8) != 0 ? VMode::Pal : VMode::Ntsc;
    displayDepth = (val & 0x10) != 0 ? DisplayDepth::D15Bits : DisplayDepth::D24Bits;
    interlaced = (val & 0x20) != 0;
    
    field = static_cast<Field>(interlaced);
    
    // Reverse flag?
    if ((val & 0x80) != 0) {
        std::cout << "Unsupported display mode: " << std::hex << val << '\n';
    }
}

void Emulator::Gpu::gp1DisplayEnable(uint32_t val) {
    displayEnabled = (val & 1) == 0;
}

void Emulator::Gpu::gp1DmaDirection(uint32_t val) {
    switch (val & 3) {
    case 0:
        dmaDirection = DmaDirection::Off;
        break;
    case 1:
        dmaDirection = DmaDirection::Fifo;
        break;
    case 2:
        dmaDirection = DmaDirection::CpuToGp0;
        break;
    case 3:
        dmaDirection = DmaDirection::VRamToCpu;
        break;
    default:
        dmaDirection = DmaDirection::Off;
        
        throw std::runtime_error("This shouldn't happen.. GP1 Direction; " + std::to_string(val & 3));
        break;
    }
}

void Emulator::Gpu::gp1DisplayVramStart(uint32_t val) {
    displayVramXStart = static_cast<uint16_t>(val & 0x3FE);
    displayVramYStart = static_cast<uint16_t>((val >> 10) & 0x1FF);
}

void Emulator::Gpu::gp1DisplayHorizontalRange(uint32_t val) {
    displayHorizStart = static_cast<uint16_t>(val & 0xFFF);
    displayHorizEnd = static_cast<uint16_t>((val >> 12) & 0xFFF);
}

void Emulator::Gpu::gp1DisplayVerticalRange(uint32_t val) {
    displayLineStart = static_cast<uint16_t>(val & 0x3FF);
    displayLineEnd = static_cast<uint16_t>((val >> 10) & 0x3FF);
}

void Emulator::Gpu::gp1ResetCommandBuffer(uint32_t val) {
    gp0Command.clear();
    gp0CommandRemaining = 0;
    gp0Mode = Command;
}

void Emulator::Gpu::gp1AcknowledgeIrq(uint32_t val) {
    interrupt = false;
    
    IRQ::trigger(IRQ::GPU);
}

float Emulator::Gpu::getRefreshRate() const {
    if (vmode == VMode::Ntsc) {
        if (interlaced) {
            return 59.940f; // NTSC interlaced
        } else {
            return 59.826f; // NTSC progressive
        }
    } else {
        if (interlaced) {
            return 50.000f; // PAL interlaced
        } else {
            return 49.761f; // PAL progressive
        }
    }
}

uint32_t Emulator::Gpu::read() {
    if(readMode == Command)
        return _read;
    
    const auto step = [&]() {
        if (++curX >= endX) {
            curX = startX;
            
            if (++curY >= endY) {
                readMode = Command;
            }
        }
    };
    
    uint32_t data = 0;
    
    data |= vram->getPixel(curX % vram->MAX_WIDTH, curY % vram->MAX_HEIGHT);
    step();
    data |= vram->getPixel(curX % vram->MAX_WIDTH, curY % vram->MAX_HEIGHT) << 16;
    step();
    
    if(data != 0) {
        printf("");
    }
    
    return data;
}
