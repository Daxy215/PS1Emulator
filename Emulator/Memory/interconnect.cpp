#include "interconnect.h"

#include <bitset>
#include <string>

#include "Memories/Ram.h"

#include "../CPU/CPU.h"
#include "../GPU/VRAM.h"

void Interconnect::step(uint32_t cycles) {
    gpu.step(cycles);
    _cdrom.step(cycles);
    _sio.step(cycles);
    
    _timers.sync(gpu.isInHBlank, gpu.isInVBlank, gpu.dot);
    _timers.step(cycles);
}

uint32_t Interconnect::loadInstruction(uint32_t addr) {
    uint32_t abs_addr = map::maskRegion(addr);
    
    Emulator::Timers::Scheduler::tick(1);
    
    if (auto offset = map::RAM.contains(abs_addr)) {
        return ram.load<uint32_t>(offset.value());
    }
    
    if (auto offset = map::BIOS.contains(abs_addr)) {
        return bios.load<uint32_t>(offset.value());
    }
    
    throw std::runtime_error("Unhandled PC load!");
}

uint32_t Interconnect::dmaReg(uint32_t offset) {
    uint32_t major = (offset & 0x70) >> 4;
    uint32_t minor = offset & 0xF;
    
    switch (major) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 6: {
        // Per-channel registers 0...6
        Channel& channel = dma.getChannel(PortC::fromIndex(major));
        
        switch (minor) {
        case 8:
            return channel.control();
        default:
            //throw std::runtime_error("Unhandled DMA minor read at " + std::to_string(minor));
            printf("");
            break;
        }
        
        break;
    }
    // Common DMA registers
    case 7: {
        switch (minor) {
            case 0:
                return dma.control;
            case 4:
                return dma.interrupt();
            default:
                throw std::runtime_error("Unhandled DMA read at " + std::to_string(offset));
        }
    }
    default:
        throw std::runtime_error("Unhandled DMA read at " + std::to_string(offset));
    }
    
    return 0;
}

void Interconnect::doDma(Port port) {
    // DMA transfer has been started, for now let's
    // process everything in one pass (i.e. no
    // chopping or priority handling)
    
    if(dma.getChannel(port).sync == LinkedList) {
        dmaLinkedList(port);
    } else {
        dmaBlock(port);
    }
}

// Im too lazy
bool reset = false;

void Interconnect::dmaBlock(Port port) {
    Channel& channel = dma.getChannel(port);
    
    uint32_t increment = (channel.step == Increment) ? 4 : static_cast<uint32_t>(-4);
    uint32_t addr = channel.base;
    
    // Transfer size in words
    std::optional<uint32_t> remsz = channel.transferSize();
    
    // TODO; Testing.. im too lazy
    if((channel.direction != ToRam || port != Gpu)) {
        reset = true;
    }
    
    while(remsz.value() > 0) {
        // Not sure what happens if the address is bogus,
        // Mednafen just makes addr this way, maybe that's,
        // how the hardware behaves (i.e. the RAM address,
        // wraps and the two LSB are ignored, seems reasonable enough.
        uint32_t curAddr = addr & 0x1FFFFE;
        
        switch (channel.direction) {
        case FromRam: {
                uint32_t srcWord = ram.load<uint32_t>(curAddr);
                
                switch (port) {
                case Gpu:
                    //printf("GPU data %08x", srcWord);
                    gpu.gp0(srcWord);
                    break;
                default:
                    throw std::runtime_error("Unhandled DMA destination port " + std::to_string(static_cast<uint8_t>(port)));
                    break;
                }
                break;
            }
        case ToRam: {
                uint32_t srcWord = 0;
                
                switch(port) {
                case Otc:
                    // Clear ordering table
                    if(remsz == 1) {
                        // Last entry contains the end of the table marker
                        srcWord = 0xFFFFFF;
                    } else {
                        // Pointer to the previous entry
                        srcWord = CPU::wrappingSub(addr, 4) & 0x1FFFFF;
                    }
                    
                    break;
                case Port::Gpu:
                    // This gets called before the,
                    // menu pops up after the PS logo
                    //TODO; Implement
                    
                    if(reset) {
                        reset = false;
                        
                        gpu.curX = gpu.curY = gpu.startX = gpu.startY = 0;
                        gpu.endX = gpu.vram->MAX_WIDTH;
                        gpu.endY = gpu.vram->MAX_HEIGHT;
                    }
                    
                    srcWord = gpu.read();
                    
                    break;
                default:
                    throw std::runtime_error("Unhandled DMA source port" + std::to_string(static_cast<uint8_t>(port)));    
                    break;
                }
                
                ram.store<uint32_t>(curAddr, srcWord);    
                
                break;
            }
        }
        
        addr = CPU::wrappingAdd(addr, increment);
        remsz.value() -= 1;
    }
    
    channel.done(dma, port);
}

void Interconnect::dmaLinkedList(Port port) {
    Channel& channel = dma.getChannel(port);
    
    uint32_t addr = channel.base & 0x1FFFFC;
    
    if(channel.direction == ToRam) {
        throw std::runtime_error("Invalid DMA direction for linked list mode");
    }
    
    // I don't know if the DMA even supports,
    // linked list mode for anything besides the GPU
    if(port != Gpu) {
        throw std::runtime_error("Attempted linked list DMA on port " + std::to_string(static_cast<uint8_t>(port)));
    }
    
    while(true) {
        // In linked list mode, each entry,
        // starts with a "header" word.
        // The high byte contains the number
        // of words in the "packet"
        // (not counting the header word)
        uint32_t header = ram.load<uint32_t>(addr);
        uint32_t remsz = header >> 24;
        
        while(remsz > 0) {
            addr = (addr + 4) & 0x1FFFFC;
            
            uint32_t command = ram.load<uint32_t>(addr);
            
            gpu.gp0(command);
            
            remsz -= 1;
        }
        
        // The end-of-table makrer is usually 0xFFFFFF but,
        // mednafen only checks for the MSB so maybe that's what
        // the harder does? Since this bit is not part of any,
        //valid address it makes some sense. I'll have to test
        // that at some point..
        if((header & 0x800000) != 0) {
            break;
        }
        
        addr = header & 0x1FFFFC;
    }
    
    channel.done(dma, port);
}

void Interconnect::setDmaReg(uint32_t offset, uint32_t val) {
    uint32_t major = (offset & 0x70) >> 4;
    uint32_t minor = offset & 0xf;
    std::optional<Port> activePort = std::nullopt;
    
    switch (major) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 6: {
            Port port = PortC::fromIndex(major);
            Channel& channel = dma.getChannel(port);
            
            switch (minor) {
            case 0:
                channel.setBase(val);
                break;
            case 4:
                channel.setBlockControl(val);
                break;
            case 8:
                channel.setControl(val);
                break;
            default:
                throw std::runtime_error("Unhandled DMA write " + std::to_string(offset) + " : " + std::to_string(val));
            }
            
            if(channel.active()) {
                activePort = port;
            }
            
            break;
    }
    case 7: {
            switch (minor) {
            case 0:
                dma.setControl(val);
                break;
            case 4:
                dma.setInterrupt(val);
                break;
            default:
                throw std::runtime_error("Unhandled DMA write " + std::to_string(offset) + " : " + std::to_string(val));
            }
            break;
    }
    default:
        throw std::runtime_error("Unhandled DMA write " + std::to_string(offset) + " : " + std::to_string(val));
    }

    if(activePort.has_value()) {
        doDma(activePort.value());
    }
    
    /*switch (offset) {  // NOLINT(hicpp-multiway-paths-covered)
    case 0x70:
        dma.setControl(val);
        break;
    default:
        printf("Unhandled DMA write access %x", offset);
        break;
    }*/
}
