#include "interconnect.h"

#include <iomanip>
#include <bitset>
#include <iostream>
#include <sstream>
#include <string>

#include "Bios.h"
#include "Ram.h"
//#include "Range.h"

// TODO; Remove
//TODO Remove those
std::string getHex1(uint32_t value) {
    std::stringstream ss;
    ss << "0x" << std::setfill('0') << std::setw(8) << std::hex << value;
    return ss.str();
}

std::string getBinary1(uint32_t value) {
    std::string binary = std::bitset<32>(value).to_string();
    
    std::string shiftedBin = binary.substr(26);
    
    std::string results = std::bitset<6>(std::stoi(shiftedBin, nullptr, 2) & 0x3F).to_string();
    
    std::stringstream ss;
    // wrong hex values but who cares about those
    ss << "Binary; 0b" << results;
    
    return ss.str();
}

std::string getDetail(uint32_t value) {
    std::string hex = getHex1(value);
    std::string binary = getBinary1(value);

    return hex + " = " + binary;
}

/*  // NOLINT(clang-diagnostic-comment, clang-diagnostic-comment, clang-diagnostic-comment)
template <typename T>
T Interconnect::load(uint32_t addr) {
    addr = map::maskRegion(addr);
    
    if(auto offset = map::RAM.contains(addr)) {
        return ram->load<T>(offset.value());
    }
    
    if(auto offset = map::BIOS.contains(addr)) {
        return bios->load32(offset.value());
    }
    
    if(auto offset = map::IRQ_CONTROL.contains(addr)) {
        printf("IRAQ control read %x\n", offset.value());
        
        // GPUSTAT; set bit 28 to signal that GPU is read to receive DMA blocks
        switch (offset.value()) {
        case 4:
            return 0x10000000;
        default:
            return 0;
        }
        
        return 0;
    }
    
    if(auto offset = map::DMA.contains(addr)) {
        return dmaReg(offset.value());
    }
    
    if(auto offset = map::GPU.contains(addr)) {
        printf("GPU read at %x", offset.value());
        return 0;
    }
    
    if(auto offset = map::TIMERS.contains(addr)) {
        printf("TIMERS read at %x", offset.value());
        return 0;
    }
    
    if(auto offset = map::SPU.contains(addr)) {
        printf("SPU read at %x", offset.value());
        return 0;
    }
    
    if(auto offset = map::  EXPANSION1.contains(addr)) {
        printf("TIMERS read at %x", offset.value());
        return 0;
    }
    
    /*
    // To avoid "bus errors"
    if(addr % 4 != 0) {
        throw std::runtime_error("Unaligned fetch32 at address " + getDetail(addr));
    }
    
    throw std::runtime_error("Unhandled fetch32 at address " + getDetail(addr));
}

template <typename T>
void Interconnect::store(uint32_t addr, T val) {
    addr = map::maskRegion(addr);
    
    if(auto offset = map::RAM.contains(addr)) {
        return ram->store<T>(offset.value());
    }
    
    if(auto offset = map::IRQ_CONTROL.contains(addr)) {
        printf("IRQControl %x %08x", offset.value(), val);
        return;
    }

    if(auto offset = map::DMA.contains(addr)) {
        return setDmaReg(offset.value(), val);
    }

    if(auto offset = map::GPU.contains(addr)) {
        printf("GPU write at %x", offset.value());
        return;
    }

    if(auto offset = map::TIMERS.contains(addr)) {
        printf("GPU write at %x", offset.value());
        return;
    }

    if(auto offset = map::SPU.contains(addr)) {
        printf("SPU write at %x", offset.value());
        return;
    }

    if(auto offset = map::CACHECONTROL.contains(addr)) {
        printf("Cache Control write at %x", offset.value());
        return;
    }
    
    if(auto offset = map::GPU.contains(addr)) {
        printf("GPU write at %x", offset.value());
        return;
    }
    
    if(auto offset = map::MEMCONTROL.contains(addr)) {
        switch (offset.value()) {
        case 0:
            // Expansion 1 base address
                if (val != 0x1f000000) {
                    // Bad expansion 1 base address
                    throw std::runtime_error("Bad expansion 1 base address: 0x" + getDetail(val));
                }
            
            break;
        case 4:
            // Expansion 2 base address
                if (val != 0x1f802000) {
                    // Bad expansion 2 base address
                    throw std::runtime_error("Bad expansion 2 base address: 0x" + getDetail(val));
                }
            
            break;
        default:
            std::cerr << "Unhandled store to MEMCONROL register " << getDetail(addr) << " - " << offset.value() << '\n';
            break;
        }
    }

    if(auto offset = map::RAM_SIZE.contains(addr)) {
        printf("RAMSIZE Write ignore write at %x", offset.value());
        return;
    }

    if(auto offset = map::EXPANSION2.contains(addr)) {
        printf("EXMAPSINO2 Write ignore write at %x", offset.value());
        return;
    }
    
    throw std::runtime_error("Unhandled store into address %08x %08x", addr, static_cast<uint32_t>(val));
}
*/

/*
// Loads a 32bit word at 'addr'
uint32_t Interconnect::load32(uint32_t addr) {
    addr = map::maskRegion(addr);
    
    if(auto offset = map::RAM.contains(addr)) {
        return ram->load32(offset.value());
    }
    
    if(auto offset = map::BIOS.contains(addr)) {
        return bios->load32(offset.value());
    }
    
    if(auto offset = map::IRQ_CONTROL.contains(addr)) {
        printf("IRAQ control read %x\n", offset.value());
        
        // GPUSTAT; set bit 28 to signal that GPU is read to receive DMA blocks
        switch (offset.value()) {
        case 4:
            return 0x10000000;
        default:
            return 0;
        }
    }
    
    if(auto offset = map::DMA.contains(addr)) {
        return dmaReg(offset.value());
    }
    
    if(auto offset = map::GPU.contains(addr)) {
        switch (offset.value()) {
            // GPUSTAT: set bit 26, 27 and 28 to signal that,
            // the GPU is ready for DMA and CPU access.
            // This way the BIOS won't deadlock waiting,
            // for an event that'll never come
        case 4:
            return 0x1C000000;
        default:
            return 0;
            break;
        }
        return 0;
    }
    
    if(auto offset = map::TIMERS.contains(addr)) {
        printf("TIMERS read at %x", offset.value());
        return 0;
    }
    
    if(auto offset = map::SPU.contains(addr)) {
        printf("SPU read at %x", offset.value());
        return 0;
    }
    
    if(auto offset = map::  EXPANSION1.contains(addr)) {
        printf("TIMERS read at %x", offset.value());
        return 0;
    }
    
    /*
    // To avoid "bus errors"
    if(addr % 4 != 0) {
    throw std::runtime_error("Unaligned fetch32 at address " + getDetail(addr));
    }
    
    throw std::runtime_error("Unhandled fetch32 at address " + getDetail(addr));#1#
    return 0;
}

uint16_t Interconnect::load16(uint32_t addr) {
    addr = map::maskRegion(addr);
    
    if(std::optional<uint16_t> offset = map::RAM.contains(addr)) {
        return ram->load16(offset.value());
    }
    
    if(auto offset = map::BIOS.contains(addr)) {
        //return bios->load16(offset.value());
        
        return 0;
    }
    
    if(auto offset = map::IRQ_CONTROL.contains(addr)) {
        printf("IRAQ control read %x\n", offset.value());
        
        // GPUSTAT; set bit 28 to signal that GPU is read to receive DMA blocks
        switch (offset.value()) {
        case 4:
            return 0x10000000;
        default:
            return 0;
        }
        
        return 0;
    }
    
    if(auto offset = map::DMA.contains(addr)) {
        return dmaReg(offset.value());
    }
    
    if(auto offset = map::GPU.contains(addr)) {
        printf("GPU read at %x", offset.value());
        return 0;
    }
    
    if(auto offset = map::TIMERS.contains(addr)) {
        printf("TIMERS read at %x", offset.value());
        return 0;
    }
    
    if(auto offset = map::SPU.contains(addr)) {
        printf("SPU read at %x", offset.value());
        return 0;
    }
    
    if(auto offset = map::  EXPANSION1.contains(addr)) {
        printf("TIMERS read at %x", offset.value());
        return 0;
    }
    
    /*
    // To avoid "bus errors"
    if(addr % 4 != 0) {
    throw std::runtime_error("Unaligned fetch32 at address " + getDetail(addr));
    }
    
    throw std::runtime_error("Unhandled fetch32 at address " + getDetail(addr));#1#
    return 0;
}

uint8_t Interconnect::load8(uint32_t addr) {
    addr = map::maskRegion(addr);
    
    if(std::optional<uint8_t> offset = map::RAM.contains(addr)) {
        return ram->load8(offset.value());
    }
    
    if(std::optional<uint8_t> offset = map::BIOS.contains(addr)) {
        return bios->load8(offset.value());
    }
    
    if(auto offset = map::IRQ_CONTROL.contains(addr)) {
        printf("IRAQ control read %x\n", offset.value());
        
        // GPUSTAT; set bit 28 to signal that GPU is read to receive DMA blocks
        switch (offset.value()) {
        case 4:
            return 0x10000000;
        default:
            return 0;
        }
        
        return 0;
    }
    
    if(auto offset = map::DMA.contains(addr)) {
        return dmaReg(offset.value());
    }
    
    if(auto offset = map::GPU.contains(addr)) {
        printf("GPU read at %x", offset.value());
        return 0;
    }
    
    if(auto offset = map::TIMERS.contains(addr)) {
        printf("TIMERS read at %x", offset.value());
        return 0;
    }
    
    if(auto offset = map::SPU.contains(addr)) {
        printf("SPU read at %x", offset.value());
        return 0;
    }
    
    if(auto offset = map::  EXPANSION1.contains(addr)) {
        printf("TIMERS read at %x", offset.value());
        return 0;
    }
    
    /*
    // To avoid "bus errors"
    if(addr % 4 != 0) {
    throw std::runtime_error("Unaligned fetch32 at address " + getDetail(addr));
    }
    
    throw std::runtime_error("Unhandled fetch32 at address " + getDetail(addr));#1#
}

// Stores 32bit word 'val' into 'addr'
void Interconnect::store32(uint32_t addr, uint32_t val) {
    addr = map::maskRegion(addr);
    
    // To avoid "bus errors"
    if(addr % 4 != 0) {
        throw std::runtime_error("Unaligned fetch32 at address " + getDetail(addr));
    }
    
    if(auto offset = map::RAM.contains(addr)) {
        ram->store32(offset.value(), val);
    }
    
    if(auto offset = map::SCRATCHPAD.contains(addr)) {
        if(addr > 0xa0000000) {
            throw std::runtime_error("ScratchPad access through uncached memory");
        }
        
        throw std::runtime_error("Unhandled write to SCRATCH_PAD " + std::to_string(offset.value()));
    }

    if(auto offset = map::IRQ_CONTROL.contains(addr)) {
        printf("IRAQ control %x 0x%08x\n", offset.value(), val);
        return;
    }
    
    if(auto offset = map::DMA.contains(addr)) {
        setDmaReg(offset.value(), val);
        return;
    }
        
    if(auto offset = map::GPU.contains(addr)) {
        printf("GPU read at %x = %08x", offset.value(), val);
        return;
    }
    
    if(auto offset = map::TIMERS.contains(addr)) {
        printf("Unhandled store32 to timer register %x 0x%08x\n", offset.value(), val);
        return;
    }

    if(auto offset = map::SPU.contains(addr)) {
        printf("Unhandled store32 to SPU register %x 0x%08x\n", offset.value(), val);
        return;
    }
    
    if(auto offset = map::CACHECONTROL.contains(addr)) {
        printf("Unhandled store32 to cache control register %x 0x%08x\n", offset.value(), val);
        return;
    }
    
    if(map::DMA.contains(addr)) {
        throw std::runtime_error("Unhandled DMA store32 at address " + getDetail(addr));
    }
    
    auto offset = map::MEMCONTROL.contains(addr);
    
    if(!offset) {
        std::cout << "Unhandled store32 at address " + getDetail(addr) << '\n';
        return;
    }
    
    switch (offset.value()) {
    case 0:
        // Expansion 1 base address
        if (val != 0x1f000000) {
            // Bad expansion 1 base address
            throw std::runtime_error("Bad expansion 1 base address: 0x" + getDetail(val));
        }
        
        break;
    case 4:
        // Expansion 2 base address
        if (val != 0x1f802000) {
            // Bad expansion 2 base address
            throw std::runtime_error("Bad expansion 2 base address: 0x" + getDetail(val));
        }
        
        break;
    default:
        std::cerr << "Unhandled store to MEMCONROL register " << getDetail(addr) << " - " << offset.value() << '\n';
        break;
    }

    if(auto offset = map::RAM_SIZE.contains(addr)) {
        printf("Unhandled store32 to RAMSIZE register %x 0x%08x\n", offset.value(), val);
        return;
    }

    if(auto offset = map::EXPANSION2.contains(addr)) {
        printf("Unhandled store32 to EXPASION2 register %x 0x%08x\n", offset.value(), val);
        return;
    }
    
    //std::cout << "Unhandled store32 at address " + uint32xToHex(addr) << '\n';
    //throw std::runtime_error("Unhandled store32 into address " + std::to_string(addr));
}

void Interconnect::store16(uint32_t addr, uint16_t val) {
    // To avoid "bus errors"
    if(addr % 2 != 0) {
        throw std::runtime_error("Unaligned store16 at address " + getDetail(addr));
    }
    
    addr = map::maskRegion(addr);

    if(auto offset = map::RAM.contains(addr)) {
        return ram->store16(offset.value(), val);
    }
    
    if(auto offset = map::TIMERS.contains(addr)) {
        printf("Unhandled write to timer register %x 0x%08x\n", offset.value(), val);
        return;
    }

    if(auto offset = map::IRQ_CONTROL.contains(addr)) {
        printf("IRAQ control write %x at %04x\n", offset.value(), val);
        return;
    }
    
    if(auto offset = map::SPU.contains(addr)) {
        printf("Unaligned store16 write to SPU register %s\n", getDetail(addr).c_str());
        //throw std::runtime_error("Unaligned store16 write to SPU register " + getDetail(addr));
        return;
    }
    
    throw std::runtime_error("Unhandled store16 at address " + getDetail(addr));
}

void Interconnect::store8(uint32_t addr, uint8_t val) {
    // To avoid "bus errors"
    //if(addr % 2 != 0) {
        //throw std::runtime_error("Unaligned store8 at address " + getDetail(addr));
    //}
    
    addr = map::maskRegion(addr);
    
    if(auto offset = map::RAM.contains(addr)) {
        ram->store8(offset.value(), val);
        return;
    }
    
    if(auto offset = map::EXPANSION2.contains(addr)) {
        printf("Unhandled store8 write to SPU register %s\n", getDetail(addr).c_str());
        //throw std::runtime_error("Unhandled store8 write to SPU register " + getDetail(addr));
        return;
    }
    
    throw std::runtime_error("Unhandled store8 at address " + getDetail(addr));
}
*/

uint32_t Interconnect::dmaReg(uint32_t offset) {
    uint32_t major = (offset & 0x70) >> 4;
    uint32_t minor = offset & 0xF;

    switch (major) {
        // Per-channel registers 0...6
    case 0: case 1: case 2: case 3: case 4: case 5: case 6: {
            Channel* channel = dma->getChannel(PortC::fromIndex(major));
        
            switch (minor) {
            case 8:
                return channel->control();
            default:
                throw std::runtime_error("Unhandled DMA read at " + std::to_string(offset));
            }
            break;
        }
    // Common DMA registers            
    case 7: {
        switch (minor) {
            case 0:
                return dma->control;
            case 4:
                return dma->interrupt();
            default:
                throw std::runtime_error("Unhandled DMA read at " + std::to_string(offset));
        }
    }
    default:
        throw std::runtime_error("Unhandled DMA read at " + std::to_string(offset));
    }
    
    /*switch (offset) {  // NOLINT(hicpp-multiway-paths-covered)
    case 0x70:
        return dma->control;
    default:
        printf("Unhandled DMA read access %x", offset);
        break;
    }*/
    
    return 0;
}

void Interconnect::doDma(Port port) {
    // DMA transfer has been started, for now let's
    // process everything in one pass (i.e. no
    // chopping or priority handling)

    switch(dma->getChannel(port)->sync) {
    case LinkedList:
        dmaLinkedList(port);
        break;
    default:
        dmaBlock(port);    
        break;
    }
}

void Interconnect::dmaBlock(Port port) {
    Channel& channel = *dma->getChannel(port);
    
    uint32_t increment = (channel.step == Increment) ? 4 : -4;
    uint32_t addr = channel.base;
    
    // Transfer size in words
    std::optional<uint32_t> remsz = channel.transferSize();

    while(remsz.value() > 0) {
        // Not sure what happens if the address is bogus,
        // Mednafen just makes addr this way, maybe that's,
        // how the hardware behaves (i.e. the RAM address,
        // wraps and the two LSB are ignored, seems reasonable enough.
        uint32_t curAddr = addr & 0x1FFFFE;

        switch (channel.direction) {
        case FromRam: {
                uint32_t srcWord = ram->load<uint32_t>(curAddr);
                
                switch (port) {
                case Gpu:
                    printf("GPU data %08x", srcWord);
                    break;
                default:
                    throw std::runtime_error("Unhandled DMA destination port " + std::to_string(static_cast<uint8_t>(port)));
                    break;
                }
                break;
            }
        case ToRam: {
                uint32_t srcWord;
            
                switch(port) {
                    // Clear ordering table
                case Otc:
                    if(remsz == 1) {
                        // Last entry contains the end of the table marker
                        srcWord = 0xFFFFFF;
                    } else {
                        // Pointer to the previous entry
                        // TODO; Wrapped sub
                        srcWord = (addr - 4) & 0x1FFFFF;
                    }
                
                    break;
                default:
                    throw std::runtime_error("Unhandled DMA source port" + std::to_string(static_cast<uint8_t>(port)));    
                    break;
                }
            
                ram->store<uint32_t>(curAddr, srcWord);    
            
                break;
            }
        }
        
        // TODO; Wrapping add
        addr += increment;
        remsz.value() -= 1;
    }
    
    channel.done();
}

void Interconnect::dmaLinkedList(Port port) {
    Channel& channel = *dma->getChannel(port);
    
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
        uint32_t header = ram->load<uint32_t>(addr);
        uint32_t remsz = header >> 24;
        
        while(remsz > 0) {
            addr = (addr + 4) & 0x1FFFFC;
            
            uint32_t command = ram->load<uint32_t>(addr);
            
            printf("GPU command %08x", command);
            
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
    
    channel.done();
}

void Interconnect::setDmaReg(uint32_t offset, uint32_t val) {
    uint32_t major = (offset & 0x70) >> 4;
    uint32_t minor = offset & 0xf;
    std::optional<Port> activePort = std::nullopt;

    switch (major) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 6: {
            Port port = PortC::fromIndex(major);
            Channel& channel = *dma->getChannel(port);
            
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
                dma->setControl(val);
                break;
            case 4:
                dma->setInterrupt(val);
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
        dma->setControl(val);
        break;
    default:
        printf("Unhandled DMA write access %x", offset);
        break;
    }*/
}
