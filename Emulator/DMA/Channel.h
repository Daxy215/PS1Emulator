#pragma once

//#include <stdint.h>
#include <optional>

#include "Attributes.h"

class Dma;

class Channel {
public:
    uint32_t control();
    
    void setControl(uint32_t val);
    
    // Retrieve value of the Block Control register
    uint32_t blockControl();
    
    // Set value of the Block Control register
    void setBlockControl(uint32_t val) {
        blockSize = static_cast<uint16_t>(val);
        blockCount = static_cast<uint16_t>((val >> 16));
    }
    
    void setBase(uint32_t val);
    
    // Return true if the channel has been started
    bool active();
    
    // Return the DMA transfer size in bytes or None for linked list mode
    std::optional<uint32_t> transferSize();
    
    // Set the channel status to "completed" state
    void done(Dma dma, Port port);

    bool enable = false;
    
    // Used to start the DMA transfer whe 'sync' is 'Manual'
    bool trigger = false;
    
    // If the DMA 'chops' the transfer and lets the CPU run in the gaps
    bool chop = false;
    
    // Chopping DMA window size (log2 number of words)
    uint8_t chopDmaSz = 0;
    
    // Chopping CPU window size (lgo2 number of cycles)
    uint8_t chopCpuSz = 0;
    
    // Unknown 2 RW bits in configuration register
    uint8_t dummy = 0;

    // Size of a block in words
    uint16_t blockSize = 0;

    // Block count, Only used when 'sync' is 'Request'
    uint16_t blockCount = 0;
    
    // DMA start address
    uint32_t base = 0;
    
    Direction direction = ToRam;
    Step step = Increment;
    Sync sync = Manual;
};