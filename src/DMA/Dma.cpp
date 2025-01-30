#include "Dma.h"

#include "../Memory/IRQ.h"

void Dma::step() {
    for(int i = 0; i < 7; i++) {
        Channel& channel = channels[i];
        
        if(channel.interruptPending) {
            channel.interruptPending = false;
            
            auto prvIrq = irq();
	        
            auto en = channelIrqEn & (1 << (static_cast<size_t>(i)));
	        
            /**
            * Was updating the wrong flag ;-;
            */
            channelIrqFlags |= en;
	        
            if(!prvIrq && irq()) {
                interruptPending = true;
            }
        }
    }
    
    if(interruptPending) {
        interruptPending = false;
        IRQ::trigger(IRQ::Interrupt::Dma);
    }
}

uint32_t Dma::interrupt() {
    uint32_t r = 0;
    
    r |= (static_cast<uint32_t>(irqDummy));
    r |= (static_cast<uint32_t>(forceIrq)) << 15;
    r |= (static_cast<uint32_t>(channelIrqEn)) << 16;
    r |= (static_cast<uint32_t>(irqEn)) << 23;
    r |= (static_cast<uint32_t>(channelIrqFlags)) << 24;
    r |= (static_cast<uint32_t>(irqFlag)) << 31;
    
    return r;
}

void Dma::setInterrupt(uint32_t val) {
    auto prevIrq = irq();
    
    // Unknown what bts [14:0] do
    irqDummy = static_cast<uint8_t>(val & 0x7FFF);
    
    forceIrq = ((val >> 15) & 1) != 0;
    
    channelIrqEn = static_cast<uint8_t>(((val >> 16) & 0x7F));
    
    irqEn = ((val >> 23) & 1) != 0;
    
    // Writing 1 to a flag rests it
    uint8_t ack = ((val >> 24) & 0x3F);
    channelIrqFlags &= ~ack;
    
    irqFlag = irq();
    
    if(!prevIrq && irq()) {
        IRQ::trigger(IRQ::Interrupt::Dma);
    }
}

bool Dma::irq() {
    auto channelIrq = this->channelIrqFlags & this->channelIrqEn;
    
    return forceIrq || (this->irqEn && (channelIrq != 0));
}

void Dma::setControl(uint32_t val) {
    control = val;
}
