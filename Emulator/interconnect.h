#pragma once
#include <cstdint>

#include "Dma.h"

/**
 * This class is used to allow the BIOS to communicate with the CPU! :D
 */

class Ram;
class Bios;
class Dma;

class Interconnect {
public:
    Interconnect(Ram* ram, Bios* bios, Dma* dma) : ram(ram), bios(bios), dma(dma) {  }
    
    // TODO; Use templates and only use 1 function each
    
    // Load word
    uint32_t load32(uint32_t addr);

    // Load half word
    uint16_t load16(uint32_t addr);

    // Load byte
    uint8_t load8(uint32_t addr);
    
    // Store word
    void store32(uint32_t addr, uint32_t val);
    
    // Store half word
    void store16(uint32_t addr, uint16_t val);
    
    // Store byte
    void store8(uint32_t addr, uint8_t val);
    
    // DMA
    // DMA register read
    uint32_t dmaReg(uint32_t offset);

    void setDmaReg(uint32_t offset, uint32_t val);
    
private:
    Ram* ram;
    Bios* bios;
    Dma* dma;
};
