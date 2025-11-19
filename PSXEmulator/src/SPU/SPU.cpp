#include "SPU.h"

#include <cassert>
#include <iostream>
#include <sstream>

#define LOG

Emulator::SPU::SPU () : buffer(441000*8) {
    
}

uint32_t Emulator::SPU::load(uint32_t addr, uint32_t val) {
    /**
     * 1F801C00h..1F801D7Fh - Voice 0..23 Registers (eight 16bit regs per voice)
     * 1F801D80h..1F801D87h - SPU Control (volume)
     * 1F801D88h..1F801D9Fh - Voice 0..23 Flags (six 1bit flags per voice)
     * 1F801DA2h..1F801DBFh - SPU Control (memory, control, etc.)
     * 1F801DC0h..1F801DFFh - Reverb configuration area
     * 1F801E00h..1F801E5Fh - Voice 0..23 Internal Registers
     * 1F801E60h..1F801E7Fh - Unknown?
     * 1F801E80h..1F801FFFh - Unused?
     */
    if (addr >= 0x1F801C00 && addr <= 0x1F801D7F) {
        // Voice 0..23 Registers (eight 16-bit regs per voice)
        
    } else if (addr >= 0x1F801D80 && addr <= 0x1F801D87) {
        // SPU Control (volume)
        return handleVolumeLoad(addr);
    } else if (addr >= 0x1F801D88 && addr <= 0x1F801D9F) {
        // Voice 0..23 Flags (six 1-bit flags per voice)
    } else if (addr >= 0x1F801DA2 && addr <= 0x1F801DBF) {
        // SPU Control (memory, control, etc.)
        return handleControlLoad(addr);
    } else if (addr >= 0x1F801DC0 && addr <= 0x1F801DFF) {
        // Reverb configuration area
    } else if (addr >= 0x1F801E00 && addr <= 0x1F801E5F) {
        // Voice 0..23 Internal Registers
    } else if (addr >= 0x1F801E60 && addr <= 0x1F801E7F) {
        // Unknown
    } else if (addr >= 0x1F801E80 && addr <= 0x1F801FFF) {
        // Unused
    }
    
    #ifdef LOG
        std::cerr << "Unhandled load from SPU register; " << std::hex << (addr) << "\n";
    #endif
    
    return 0;
}

void Emulator::SPU::store(uint32_t addr, uint32_t val) {
    /**
     * 1F801C00h..1F801D7Fh - Voice 0..23 Registers (eight 16bit regs per voice)
     * 1F801D80h..1F801D87h - SPU Control (volume)
     * 1F801D88h..1F801D9Fh - Voice 0..23 Flags (six 1bit flags per voice)
     * 1F801DA2h..1F801DBFh - SPU Control (memory, control, etc.)
     * 1F801DC0h..1F801DFFh - Reverb configuration area
     * 1F801E00h..1F801E5Fh - Voice 0..23 Internal Registers
     * 1F801E60h..1F801E7Fh - Unknown?
     * 1F801E80h..1F801FFFh - Unused?
     */
    if (addr >= 0x1F801C00 && addr <= 0x1F801D7F) {
        // Voice 0..23 Registers (eight 16-bit regs per voice)
        handleVoiceStore(addr, val);
        
        return;
    } else if (addr >= 0x1F801D80 && addr <= 0x1F801D87) {
        // SPU Control (volume)
        handleVolumeStore(addr, val);
        
        return;
    } else if (addr >= 0x1F801D88 && addr <= 0x1F801D9F) {
        // Voice 0..23 Flags (six 1-bit flags per voice)
        handleFlagsStore(addr, val);
        
        return;
    } else if (addr >= 0x1F801DA2 && addr <= 0x1F801DBF) {
        // SPU Control (memory, control, etc.)
        handleControlStore(addr, val);
        
        return;
    } else if (addr >= 0x1F801DC0 && addr <= 0x1F801DFF) {
        // Reverb configuration area
    } else if (addr >= 0x1F801E00 && addr <= 0x1F801E5F) {
        // Voice 0..23 Internal Registers
    } else if (addr >= 0x1F801E60 && addr <= 0x1F801E7F) {
        // Unknown
    } else if (addr >= 0x1F801E80 && addr <= 0x1F801FFF) {
        // Unused
    }
    
    #ifdef LOG
        std::cerr << "Unhandled store from SPU register; " << std::hex << (addr) << "\n";
    #endif
}

void Emulator::SPU::handleVoiceStore(uint32_t addr, uint32_t val) {
    switch(addr) {
        case 0x1F801C00: {
            // 1F801C00h+N*10h - Voice 0..23 Volume Left
            
            uint8_t index = (addr / 0x10);
            
            adsr[index].volLeft = (val * 0x10);
            
            break;
        }
        
        case 0x1F801C02: {
            // 1F801C02h+N*10h - Voice 0..23 Volume Right
            
            uint8_t index = (addr / 0x10);
            
            adsr[index].volRight = (val * 0x10);
            
            break;
        }
        
        case 0x1F801C04: {
            // 1F801C04h+N*10h - Voice 0..23 ADPCM Sample Rate (R/W) (VxPitch)
            //   0-15  Sample rate (0=stop, 4000h=fastest, 4001h..FFFFh=usually same as 4000h)
            
            uint8_t index = (addr / 0x10);
            
            adpcm[index].vxPitch = (val & 0xFFFF);
            
            break;
        }
        
        case 0x1F801C08: {
            // 1F801C08h+N*10h - Voice 0..23 Attack/Decay/Sustain/Release (ADSR) (32bit)
            
            // TODO; ?
            uint8_t index = (addr / 0x10);
            
            adsr[index].adsr = (val & 0xFFFF);
            
            break;
        }
        
        case 0x1F801C0A: {
            // ADSR - upper 16bit (at 1F801C0Ah+N*10h)
            
            uint8_t index = (addr / 0x10);
            
            adsr[index].adsr = (adsr[index].adsr & 0x0000FFFF) | ((val >> 16) & 0xFFFF) << 16;
            
            break;
        }
        
        case 0x1F801C06: {
            // 1F801C06h+N*10h - Voice 0..23 ADPCM Start Address (R/W)
            
            uint8_t index = (addr / 0x10);
            
            adpcm[index].startAddress = (val & 0xFFFF);
            
            break;
        }
        
        // Don't seem to exist in the Docs?
        case 0x1f801c10:
        case 0x1f801c12: {
            break;
        }
        
        default: {
            #ifdef LOG
                std::cerr << "Unhandled voice store from SPU register; " << std::hex << (addr) << "\n";
            #endif
            
            break;
        }
    }
}

uint32_t Emulator::SPU::handleVolumeLoad(uint32_t addr) {
    switch(addr) {
        case 0x1F801D80: {
            // 1F801D80h - Mainvolume left
            return mainValLeft;
        }
        
        case 0x1F801D82: {
            // 1F801D82h - Mainvolume right
            return mainValRight;
        }
        
        case 0x1f801d84: {
            // 1F801D84h spu   vLOUT   volume  Reverb Output Volume Left
            return vLOUT;
        }
        
        case 0x1f801d86: {
            //   1F801D86h spu   vROUT   volume  Reverb Output Volume Right
            return vROUT;
        }
        
        default: {
            #ifdef LOG
                std::cerr << "Unhandled volume read from SPU register; " << std::hex << (addr) << "\n";
            #endif
            
            break;
        }
    }
}

void Emulator::SPU::handleVolumeStore(uint32_t addr, uint32_t val) {
    switch(addr) {
        case 0x1f801D80: {
            // 1F801D80h - Mainvolume left
            mainValLeft = (val & 0xFFFF);
            
            break;
        }
        
        case 0x1f801D82: {
            // 1F801D82h - Mainvolume right
            mainValRight = (val & 0xFFFF);
            
            break;
        }
        
        case 0x1f801D84: {
            // 1F801D84h spu   vLOUT   volume  Reverb Output Volume Left
            vLOUT = (val & 0xFFFF);
            
            break;
        }
        
        case 0x1f801D86: {
            //   1F801D86h spu   vROUT   volume  Reverb Output Volume Right
            vROUT = (val & 0xFFFF);
            
            break;
        }
        
        default: {
            #ifdef LOG
                std::cerr << "Unhandled volume store from SPU register; " << std::hex << (addr) << "\n";
            #endif
            
            break;
        }
    }
}

void Emulator::SPU::handleFlagsStore(uint32_t addr, uint32_t val) {
    switch(addr) {
        case 0x1F801D8C: {
            // 1F801D8Ch - Voice 0..23 Key OFF (Start Release) (KOFF) (W)
            
            uint8_t index = (addr / 0x10);
            
            flags[index].keyOff = (val & 0x007FFFFF);
            
            break;
        }
        
        case 0x1F801D90: {
            // 1F801D90h - Voice 0..23 Pitch Modulation Enable Flags (PMON)
            
            /**
             *   0     Unknown... Unused?
             * 1-23  Flags for Voice 1..23 (0=Normal, 1=Modulate by Voice 0..22)
             */
            
            int index = (addr / 0x10);
            
            adpcm[index].PMON = ((val >> 1) & 0x007FFFFF);
            
            break;
        }
        
        case 0x1F801D94: {
            // 1F801D94h - Voice 0..23 Noise mode enable (NON)
            
            //   0-23  Voice 0..23 Noise (0=ADPCM, 1=Noise)
            NON = (val & 0x007FFFFF);
            
            break;
        }
        
        case 0x1f801d98: {
            // 1F801D98h - Voice 0..23 Reverb mode aka Echo On (EON) (R/W)
            
            //   0-23  Voice 0..23 Destination (0=To Mixer, 1=To Mixer and to Reverb)
            EON = (val & 0x007FFFFF);
            
            break;
        }
        
        // TODO; Those seem to not exist? Idk if I have an issue somewhere
        case 0x1f801d8e:
        case 0x1f801d92:
        case 0x1f801d96:
        case 0x1f801d9a: {
            break;
        }
        
        default: {
            #ifdef LOG
                std::cerr << "Unhandled flags store from SPU register; " << std::hex << (addr) << "\n";
            #endif
            
            break;
        }
    }
}

uint32_t Emulator::SPU::handleControlLoad(uint32_t addr) {
    switch(addr) {
        case 0x1F801DA6: {
            // 1F801DA6h - Sound RAM Data Transfer Address
            // TODO; ????
            return buffer[currentAddress--];
        }
        
        case 0x1F801DAA: {
            // 1F801DAAh - SPU Control Register (SPUCNT)
            return spunct._reg;
        }
        
        case 0x1f801DAE: {
            // 1F801DAEh - SPU Status Register (SPUSTAT) (R)
            return status._reg;
        }
        
        default: {
            #ifdef LOG
                std::cerr << "Unhandled control store from SPU register; " << std::hex << (addr) << "\n";
            #endif
            
            break;
        }
    }
}

void Emulator::SPU::handleControlStore(uint32_t addr, uint32_t val) {
    switch(addr) {
        case 0x1F801DA6: {
            // 1F801DA6h - Sound RAM Data Transfer Address
            currentAddress = (val & 0xFFFF) * 8;
            
            break;
        }
        
        case 0x1F801DA8: {
            buffer[currentAddress++] = (val & 0xFFFF);
            
            break;
        }
        
        case 0x1F801DAA: {
            // 1F801DAAh - SPU Control Register (SPUCNT)
            spunct._reg = val;
            
            break;
        }
        
        case 0x1f801DAE: {
            // 1F801DAEh - SPU Status Register (SPUSTAT) (R)
            // Read-only?
            //status._reg = val;
            
            throw std::runtime_error("[ERROR] [SPU] Attempt to write to a read-only address!");
            
            break;
        }
        
        case 0x1f801db0: {
            // 1F801DB0h - CD Audio Input Volume (for normal CD-DA, and compressed XA-ADPCM)
            cdInputVol = val;
            
            break;
        }
        
        case 0x1F801DB4: {
            // 1F801DB0h - CD Audio Input Volume (for normal CD-DA, and compressed XA-ADPCM)
            exVolLeft  = (val & 0x0000FFFF);
            exVolRight = (val >> 16) & 0xFFFF;
            
            break;
        }
        
        case 0x1F801DAC: {
            // 1F801DACh - Sound RAM Data Transfer Control (should be 0004h)
            
            assert(val == 4);
            
            // TODO; ???????????
            
            /**
             * 15-4   Unknown/no effect?                       (should be zero)
             * 3-1    Sound RAM Data Transfer Type (see below) (should be 2)
             * 0      Unknown/no effect?                       (should be zero)
             * The Transfer Type selects how data is forwarded from Fifo to SPU RAM:
             *   __Transfer Type___Halfwords in Fifo________Halfwords written to SPU RAM__
             *   0,1,6,7  Fill     A,B,C,D,E,F,G,H,...,X    X,X,X,X,X,X,X,X,...
             *   2        Normal   A,B,C,D,E,F,G,H,...,X    A,B,C,D,E,F,G,H,...
             *   3        Rep2     A,B,C,D,E,F,G,H,...,X    A,A,C,C,E,E,G,G,...
             *   4        Rep4     A,B,C,D,E,F,G,H,...,X    A,A,A,A,E,E,E,E,...
             *   5        Rep8     A,B,C,D,E,F,G,H,...,X    H,H,H,H,H,H,H,H,...
             * Rep2 skips the 2nd halfword, Rep4 skips 2nd..4th, Rep8 skips 1st..7th.
             * Fill uses only the LAST halfword in Fifo, that might be useful for memfill purposes, although, the length is probably determined by the number of writes to the Fifo (?) so one must still issue writes for ALL halfwords...?
             * Note:
             * The above rather bizarre results apply to WRITE mode. In READ mode, the register causes the same halfword to be read 2/4/8 times (for rep2/4/8).
             */
        }
        
        // TODO; It's calling those but there isn't anything about them in the docs?
        case 0x1f801db2:
        case 0x1f801db6: {
            break;
        }
        
        default: {
            #ifdef LOG
                std::cerr << "Unhandled control store from SPU register; " << std::hex << (addr) << "\n";
            #endif
            
            break;
        }
    }
}