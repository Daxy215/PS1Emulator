#include "SPU.h"

#include <iostream>
#include <sstream>

// TODO; Remove
std::string to_hex(uint32_t value) {
    std::stringstream ss;
    ss << std::hex << value;
    return ss.str();
}

void Emulator::SPU::load(uint32_t addr, uint32_t val) {
    switch(addr) {
    case 0x1F801DAE:
        // SPU STATU
        stat(val);
        
        break;
    case 0x1F801DAA:
        // SPU Control Register (SPUCNT)
        // https://psx-spx.consoledev.net/soundprocessingunitspu/#1f801daah-spu-control-register-spucnt
        //printf("SPUCNT\n");
        
        break;
    case 0x1F801D88:
        // Voice 0..23 Key ON (Start Attack/Decay/Sustain) (KON) (W)
        // https://psx-spx.consoledev.net/soundprocessingunitspu/#1f801d88h-voice-023-key-on-start-attackdecaysustain-kon-w
        //printf("Voice\n");
        
        break;
    case 0x1F801D8A:
        // https://psx-spx.consoledev.net/soundprocessingunitspu/#1f801d88h-voice-023-key-on-start-attackdecaysustain-kon-w
        // Keyon = (keyOn & 0xFFFF) | (value << 16);
        break;
    case 0x1F801D8C:
        // https://psx-spx.consoledev.net/soundprocessingunitspu/#1f801d8ch-voice-023-key-off-start-release-koff-w
        // keyOff = (keyOff & 0xFFFF0000) | value;
        break;
    default:
        //std::cerr << "Unhandled read from SPU register; " << to_hex(addr) << "\n";
        break;
    }
}

void Emulator::SPU::stat(uint32_t val) {
    /*
     *  15-12 Unknown/Unused (seems to be usually zero)
     *  11    Writing to First/Second half of Capture Buffers (0=First, 1=Second)
     *  10    Data Transfer Busy Flag          (0=Ready, 1=Busy)
     *  9     Data Transfer DMA Read Request   (0=No, 1=Yes)
     *  8     Data Transfer DMA Write Request  (0=No, 1=Yes)
     *  7     Data Transfer DMA Read/Write Request ;seems to be same as SPUCNT.Bit5
     *  6     IRQ9 Flag                        (0=No, 1=Interrupt Request)
     *  5-0   Current SPU Mode   (same as SPUCNT.Bit5-0, but, applied a bit delayed)
     */
    
    status.isWritingToFirstHalf = (val & (1 << 11)) != 0;
    status.dataTransferBusyFlag = (val & (1 << 10)) != 0;
    status.dataTransferDmaRead = (val & (1 << 9)) != 0;
    status.dataTransferDmaWrite = (val & (1 << 8)) != 0;
    status.dataDmaRequest = (val & (1 << 7)) != 0;
    status.irq9Flag = (val & (1 << 6)) != 0;
    
    // Mask to get the lower 5 bits representing the current SPU mode
    status.mode = (val & 0x1F) != 0;
}
