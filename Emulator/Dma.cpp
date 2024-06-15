#include "Dma.h"

#include <functional>

uint32_t Dma::interrupt() {
    uint32_t r = 0;
    
    r |= static_cast<uint32_t>(irqDummy);
    r |= (static_cast<uint32_t>(forceIrq)) << 15;
    r |= (static_cast<uint32_t>(channelIrqEn)) << 16;
    r |= (static_cast<uint32_t>(irqEn)) << 23;
    r |= (static_cast<uint32_t>(channelIraqFlags)) << 24;
    r |= (static_cast<uint32_t>(irq())) << 31;
    
    return r;
}

void Dma::setInterrupt(uint32_t val) {
    // TODO; Research
    // Unknown what bts [5:0] do
    irqDummy = static_cast<uint8_t>(val & 0x3F);
    
    forceIrq = ((val >> 15) & 1) != 0;
    
    channelIrqEn = static_cast<uint8_t>(((val >> 16) & 0x7F));
    
    irqEn = ((val >> 23) & 1) != 0;
    
    // Writing 1 to a flag rests it
    uint8_t ack = ((val >> 24) & 0x3F);
    channelIraqFlags &= ~ack;
    
}

bool Dma::irq() {
    auto channelIrq = this->channelIraqFlags & this->channelIrqEn;
    
    return forceIrq || (this->irqEn && channelIrq != 0); 
}

void Dma::setControl(uint32_t val) {
    control = val;
}
