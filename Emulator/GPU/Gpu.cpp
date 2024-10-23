#include "Gpu.h"

#include <ios>
#include <iostream>
#include <stdexcept>
#include <string>

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
    status |= (false ? 1 : 0) << 31;

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
        uint8_t opcode = (val >> 24) & 0xFF;
        
        switch (opcode) {
        case 0x00:
            gp0CommandRemaining = 1;
            Gp0CommandMethod = &Gpu::gp0Nop;
            break;
        case 0x01:
            gp0CommandRemaining = 1;
            Gp0CommandMethod = &Gpu::gp0ClearCache;
            break;
        case 0x20: {
            // GP0(20h) - Monochrome three-point polygon, opaque
            
            gp0CommandRemaining = 4;
            Gp0CommandMethod = &Gpu::gp0TriangleMonoOpaque;
            break;
        }
        case 0x22: {
            // GP0(22h) - Monochrome three-point polygon, semi-transparent
            
            // For now just same implementation
            //TODO; Make this semi-transparent
            gp0CommandRemaining = 4;
            Gp0CommandMethod = &Gpu::gp0TriangleMonoOpaque;
            break;
        }
        case 0x28:
            gp0CommandRemaining = 5;
            Gp0CommandMethod = &Gpu::gp0QuadMonoOpaque;
            break;
        case 0x2A: {
            gp0CommandRemaining = 5;
            
            // For now, I don't really have transparency
            //TODO; Make this semi-transparent
            Gp0CommandMethod = &Gpu::gp0QuadMonoOpaque;
            break;
        }
        /*case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25:
        case 0x26: case 0x27: case 0x28: case 0x29: case 0x2A: case 0x2B:
        case 0x2C: case 0x2D: case 0x2E: case 0x2F: case 0x30: case 0x31:
        case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
        case 0x38: case 0x39:*/ case 0x30: {
            /*if(/*opcode == 0x28 || #1#opcode == 0x38) {
                printf("");
            }*/
            
            gp0CommandRemaining = 6;
            Gp0CommandMethod = &Gpu::gp0TriangleShadedOpaque;
            
            break;
        }
        case 0x32: {
            // TODO; Make this semi-transparent
            gp0CommandRemaining = 6;
            Gp0CommandMethod = &Gpu::gp0TriangleShadedOpaque;
            
            break;
        }
        case 0x38:
            gp0CommandRemaining = 8;
            Gp0CommandMethod = &Gpu::gp0QuadShadedOpaque;
            break;
        case 0x3A:
            // TODO; Make this semi-transparent
            gp0CommandRemaining = 8;
            Gp0CommandMethod = &Gpu::gp0QuadShadedOpaque;
            break;
        case 0x60: {
            // GP0(60h) - Monochrome Rectangle (variable size) (opaque)
            
            gp0CommandRemaining = 3;
            Gp0CommandMethod = &Gpu::gp0VarRectangleMonoOpaque;
            break;
        }
        case 0x62: {
            // GP0(62h) - Monochrome Rectangle (variable size) (semi-transparent)
            
            // TODO; Make this semi-transparent
            gp0CommandRemaining = 3;
            Gp0CommandMethod = &Gpu::gp0VarRectangleMonoOpaque;
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

            // TODO; Make this semi-transparent
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
            
            // TODO; Make this semi-transparent
            gp0CommandRemaining = 2;
            Gp0CommandMethod = &Gpu::gp08RectangleMonoOpaque;
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
            
            // TODO; Make this semi-transparent
            gp0CommandRemaining = 2;
            Gp0CommandMethod = &Gpu::gp016RectangleMonoOpaque;
            break;
        }
        case 0x80: {
            gp0CommandRemaining = 3;
            Gp0CommandMethod = &Gpu::gp0VramToVram;
            break;
        }
        case 0xA0:
            gp0CommandRemaining = 3;
            Gp0CommandMethod = &Gpu::gp0ImageLoad;
            break;
        case 0xC0:/* case 0xCA:*/
            gp0CommandRemaining = 2;
            Gp0CommandMethod = &Gpu::gp0ImageStore;
            break;
        case 0x2C:
            gp0CommandRemaining = 9;
            Gp0CommandMethod = &Gpu::gp0QuadTextureBlendOpaque;
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
            
            //throw std::runtime_error("Unhandled GP0 command " + std::to_string(opcode));
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
        }
        break;
    case VRam:
        // Draws 2 pixels at a time
        gp0CommandRemaining -= 1;
        
        if (gp0CommandRemaining == 0) {
            // Signal VRAM that image has finished loading.
            vram->endTransfer();
            
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
    
    // Dither 24bit to 15bit (0=Off/strip LSBs, 1=Dither Enabled) ;GPUSTAT.9
    dithering = ((val >> 9) & 1) != 0;
    drawToDisplay = ((val >> 10) & 1) != 0; // (0=Prohibited, 1=Allowed)
    textureDisable = ((val >> 11) & 1) != 0;
    rectangleTextureFlipX = ((val >> 12) & 1) != 0;
    rectangleTextureFlipY = ((val >> 13) & 1) != 0;
}

void Emulator::Gpu::renderRectangle(Position position, Color color, uint16_t width, uint16_t height) {
    Position positions[4] = {
        position,
        {position.x + width, position.y},
        {position.x + width, position.y + height},
        {position.x, position.y + height},
    };
    
    Color colors[4] = { color, color, color, color };
    
    renderer->pushRectangle(positions, colors);
}

void Emulator::Gpu::gp0VarRectangleMonoOpaque(uint32_t val) {
    Color color = Color::fromGp0(gp0Command.index(0));
    Position position = Position::fromGp0P(gp0Command.index(1));
    
    uint32_t sizeData = gp0Command.index(2);
    
    uint16_t width = sizeData & 0xFFFF;
    uint16_t height = sizeData >> 16;
    
    renderRectangle(position, color, width, height);
}

void Emulator::Gpu::gp0DotRectangleMonoOpaque(uint32_t val) {
    Color color = Color::fromGp0(gp0Command.index(0));
    Position position = Position::fromGp0P(gp0Command.index(1));
    
    renderRectangle(position, color, 1, 1);
}

void Emulator::Gpu::gp08RectangleMonoOpaque(uint32_t val) {
    Color color = Color::fromGp0(gp0Command.index(0));
    Position position = Position::fromGp0P(gp0Command.index(1));
    
    renderRectangle(position, color, 8, 8);
}

void Emulator::Gpu::gp016RectangleMonoOpaque(uint32_t val) {
    Color color = Color::fromGp0(gp0Command.index(0));
    Position position = Position::fromGp0P(gp0Command.index(1));
    
    renderRectangle(position, color, 16, 16);
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
    uint8_t hr2 = static_cast<uint8_t>((val >> 6) & 1);
    
    hres = HorizontalRes::fromFields(hr1, hr2);
    
    vres = (val & 0x4) != 0 ? VerticalRes::Y480Lines : VerticalRes::Y240Lines;
    vmode = (val & 0x8) != 0 ? VMode::Pal : VMode::Ntsc;
    displayDepth = (val & 0x10) != 0 ? DisplayDepth::D15Bits : DisplayDepth::D24Bits;
    interlaced = (val & 0x20) != 0;
    
    field = static_cast<Field>(interlaced);
    
    if ((val & 0x80) != 0) {
        std::cout << "Unsupported display mode: " << std::hex << val << '\n';
    }
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

uint32_t Emulator::Gpu::read(uint32_t addr) {
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
    
    data |= vram->getPixelRGB888(curX % vram->MAX_WIDTH, curY % vram->MAX_HEIGHT);
    step();
    data |= vram->getPixelRGB888(curX % vram->MAX_WIDTH, curY % vram->MAX_HEIGHT) << 16;
    step();
    
    return data;
}
