#pragma once
#include <cstdint>

class Dma {
public:
    Dma() {}
    
    // Helper functions
    // Retrieve the value of the interrupt register
    uint32_t interrupt();

    // Set the value of the interrupt register
    void setInterrupt(uint32_t val);
    
    // Returns the status of the DMA interrupt
    bool irq();
    
    void setControl(uint32_t val);
    
public:
    // master IRQ enable
    bool irqEn;
    
    // IRQ enable for individual channels
    uint8_t channelIrqEn;

    // IRQ flags for individual channels
    uint8_t channelIraqFlags;
    
    // When set the interrupt is active unconditionally (even if 'irq_en' is false)
    bool forceIrq;
    
    // Bits [0:5] of the interrupt registers are RW but I don't know,
    // what they're supposed to do so I just store them and send them,
    // back untouched on reads
    uint8_t irqDummy;
    
    // Rest value taken from the Nocash PSX spec
    uint32_t control = 0x07654321;
};
