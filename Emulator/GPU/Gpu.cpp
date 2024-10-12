#include "Gpu.h"

#include <ios>
#include <iostream>
#include <stdexcept>
#include <string>

uint32_t Emulator::Gpu::status() {
    // https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#1f801814h-gpustat-gpu-status-register-r
    
    uint32_t r = 0;
    
    r |= static_cast<uint32_t>(pageBaseX) << 0;
    r |= static_cast<uint32_t>(pageBaseY) << 4;
    r |= static_cast<uint32_t>(semiTransparency) << 5;
    r |= static_cast<uint32_t>(textureDepth) << 7;
    r |= static_cast<uint32_t>(dithering) << 9;
    r |= static_cast<uint32_t>(drawToDisplay) << 10;
    r |= static_cast<uint32_t>(forceSetMaskBit) << 11;
    r |= static_cast<uint32_t>(preserveMaskedPixels) << 12;
    r |= static_cast<uint32_t>(field) << 13;
    
    // Bit 14: not supported
    r |= static_cast<uint32_t>(textureDisable) << 15;
    r |= hres.intoStatus();
    r |= static_cast<uint32_t>(vres) << 19;
    r |= static_cast<uint32_t>(vmode) << 20;
    r |= static_cast<uint32_t>(displayDepth) << 21;
    r |= static_cast<uint32_t>(interlaced) << 22;
    r |= static_cast<uint32_t>(displayDisabled) << 23;
    r |= static_cast<uint32_t>(interrupt) << 24;
    
    //TODO;
    //r |= hres.intoStatus();
    
    // XXX Temporary hack: If we don't emulate bit 31 correctly,
    // setting 'vres' to 1 locks the BIOS:
    // r |= static_cast<uint32_t>(vmode) << 19;
    //r |= static_cast<uint32_t>(vmode) << 20;
    
    // Not sure about that,
    // I'm guessing that it's the signal
    // checked by the DMA in when sending
    // data in Request synchronization mode.
    // For now I blindly follow the Nocash spec.
    uint32_t dmaRequest = 0;
    
    switch (dmaDirection) {
    case DmaDirection::Off:
        dmaRequest = 0;
        break;
    case DmaDirection::Fifo:
        dmaRequest = 1;
        break;
    case DmaDirection::CpuToGp0:
        dmaRequest = (r >> 28) & 1;
        break;
    case DmaDirection::VRamToCpu:
        dmaRequest = (r >> 27) & 1;
        break;
    }
    
    r |= dmaRequest << 25;
    
    // For now we pretend that the GPU is always ready:
    // Ready to receive command
    r |= 1 << 26;
    
    // Ready to send VRAM to CPU
    r |= canSendVRAMToCPU << 27;
    
    // Ready to receive DMA block
    r |= canReceiveDMABlock << 28;
    r |= static_cast<uint32_t>(dmaDirection) << 29;
    
    // Bit 31 should change depending on the currently drawn line
    // (whether it's even, odd or in the vblank apparently).
    // Let's not bother with it for now.
    r |= 0 << 31;
    
    return r;
}

void Emulator::Gpu::gp0(uint32_t val) {
    if(gp0CommandRemaining == 0) {
        // Start a new GP0 command
        uint32_t opcode = (val >> 24) & 0xFF;
        uint32_t code = val >> 29;
        
        /*if(opcode != code) {
            printf("Really?\n");
            std::cerr << "";
        }*/
        
        switch (opcode) {
        case 0x00:
            gp0CommandRemaining = 1;
            Gp0CommandMethod = &Gpu::gp0Nop;
            break;
        case 0x01:
            gp0CommandRemaining = 1;
            Gp0CommandMethod = &Gpu::gp0ClearCache;
            break;
        case 0x28:
            gp0CommandRemaining = 5;
            Gp0CommandMethod = &Gpu::gp0QuadMonoOpaque;
            break;
        case 0x30:
            gp0CommandRemaining = 6;
            Gp0CommandMethod = &Gpu::gp0TriangleShadedOpaque;
            break;
        case 0x38:
            gp0CommandRemaining = 8;
            Gp0CommandMethod = &Gpu::gp0QuadShadedOpaque;
            break;
        case 0xA0:
            gp0CommandRemaining = 3;
            Gp0CommandMethod = &Gpu::gp0ImageLoad;
            break;
        case 0xC0:/* case 0xCA:*/
            gp0CommandRemaining = 3;
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
    
    dmaDirection = DmaDirection::Off;
    
    displayDisabled = false;
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

uint32_t Emulator::Gpu::read() {
    // Not implemented for now
    
    //printf("Size; %d\n", gp0CommandRemaining);
    
    return 0; // 0x1f801810
}
