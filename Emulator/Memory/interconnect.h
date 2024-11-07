#pragma once

#include <cstdint>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "Bios.h"
#include "../DMA/Dma.h"
#include "../GPU/Gpu.h"
#include "Ram.h"
#include "Range.h"
#include "../Memory/IRQ.h"
#include "../Memory/CDROM/CDROM.h"
#include "../Memory/IO/SIO.h"
#include "../Memory/ScratchPad/ScratchPad.h"
#include "../Memory/Timers/Timers.h"
#include "../SPU/SPU.h"

/**
 * This class is used to allow the BIOS to communicate with the CPU! :D
 */
/*class Ram;
class Dma;*/
class Bios;

union CACHECONTROL {
    struct {
        uint32_t config : 16;
        uint32_t scratchpadEnabled1 : 1;
        uint32_t scratchpadEnabled2 : 1;
        uint32_t crash : 1;
        uint32_t codeCacheEnable : 1;
        
        uint32_t _ : 13; // Remaining 13 bits
    };
    
    uint32_t val = 0;
    
    CACHECONTROL(uint32_t val) : val(val) {}
};

class Interconnect {
public:
    Interconnect(Ram ram, Bios bios, Dma dma, Emulator::Gpu gpu, Emulator::SPU spu)
        : ram(ram), _scratchPad(), _timers(), bios(bios), dma(dma), gpu(gpu), spu(spu) {
        
    }

    void step(uint32_t cycles);
    
    template<typename T>
    T load(uint32_t addr) {
        uint32_t abs_addr = map::maskRegion(addr);
        
        if (auto offset = map::RAM.contains(abs_addr)) {
            return ram.load<T>(offset.value());
        }
        
        if (auto offset = map::SCRATCHPAD.contains(abs_addr)) {
            if (addr > 0xa0000000) {
                throw std::runtime_error("ScratchPad access through uncached memory");
            }
            
            return _scratchPad.load<T>(offset.value());
        }
        
        if (auto offset = map::BIOS.contains(abs_addr)) {
            return bios.load<T>(offset.value());
        }
        
        if (auto offset = map::IRQ_CONTROL.contains(abs_addr)) {
            switch (offset.value()) {
                case 0: {
                    return _irq.getStatus();
                }
                
                case 4: {
                    return _irq.getMask();
                }
                
                default: {
                    throw std::runtime_error("Error; Unsupported IRQ offset");
                }
            }
        }
        
        if (auto offset = map::DMA.contains(abs_addr)) {
            return dmaReg(offset.value());
        }
        
        if (auto offset = map::GPU.contains(abs_addr)) {
            switch (offset.value()) {
                case 0:
                    return gpu.read();
                case 4: {
                    return gpu.status();
                }
                default:
                    return 0;
            }
        }
        
        if (auto offset = map::TIMERS.contains(abs_addr)) {
            return _timers.load(offset.value());
        }
        
        if (auto offset = map::CDROM.contains(abs_addr)) {
            return _cdrom.load<uint32_t>(addr);
        }
        
        if (auto _ = map::MDEC.contains(abs_addr)) {
            throw std::runtime_error("Unhandled MDEC load at address 0x" + to_hex(addr));
        }
        
        if (auto offset = map::SPU.contains(abs_addr)) {
            // https://github.com/psx-spx/psx-spx.github.io/blob/master/docs/soundprocessingunitspu.md
            //std::cerr << "SPU load register; " << to_hex(addr) << "\n";
            //spu.load(addr, offset.value());
            
            return 0;
        }
        
        if (auto _ = map::PADMEMCARD.contains(abs_addr)) {
            return _sio.load(abs_addr);
        }
        
        if (auto _ = map::EXPANSION1.contains(abs_addr)) {
            // Gets called right before it prints the boot-id.
            // I guess this is used as a checker? Or something?
            /*if(_ == 132) {
                //   1F000084h 2Ch Pre-Boot ID  ("Licensed by Sony Computer Entertainment Inc.")
                return 0;
            }*/
            
            T r = 0;
            
            for(size_t i = 0; i < sizeof(T); i++) {
                // Simulate that nothing is connected
                uint8_t data = ~0;
                
                r |= (static_cast<uint32_t>(data) << (8 * i));
            }
            
            return r;
        }
        
        if (auto _ = map::RAM_SIZE.contains(abs_addr)) {
            return _ramSize;
        }
        
        if (auto offset = map::MEMCONTROL.contains(abs_addr)) {
            if (sizeof(T) != 4) {
                throw std::runtime_error("Unhandled MEM_CONTROL access (" + std::to_string(sizeof(T)) + ")");
            }
            
            uint32_t index = (offset.value() >> 2);
            return memControl[index];
        }
        
        if (auto _ = map::CACHECONTROL.contains(abs_addr)) {
            if (sizeof(T) != 4) {
                throw std::runtime_error("Unhandled cache control access (" + std::to_string(sizeof(T)) + ")");
            }
            
            throw std::runtime_error("Unhandled CACHE_CONTROL load at address 0x" + to_hex(addr));
        }
        
        if (auto _ = map::EXPANSION2.contains(abs_addr)) {
            throw std::runtime_error("Unhandled EXPANSION_2 load at address 0x" + to_hex(addr));
        }
        
        return 0;
        //throw std::runtime_error("Unhandled load at address 0x" + to_hex(addr));
    }
    
    template<typename T>
    void store(uint32_t addr, T val) {
        uint32_t abs_addr = map::maskRegion(addr);
        
        if (auto offset = map::RAM.contains(abs_addr)) {
            ram.store<T>(offset.value(), val);
            
            return;
        }
        
        if (auto offset = map::SCRATCHPAD.contains(abs_addr)) {
            if (addr > 0xa0000000) {
                throw std::runtime_error("ScratchPad access through uncached memory");
            }
            
            _scratchPad.store<T>(offset.value(), val);
            
            return;
        }
        
        if (auto offset = map::IRQ_CONTROL.contains(abs_addr)) {
            switch (offset.value()) {
                case 0: {
                    _irq.acknowledge(static_cast<uint16_t>(val));
                    break;
                }
                
                case 4: {
                    _irq.setMask(static_cast<uint16_t>(val));
                    break;
                }
                
                default: {
                    throw std::runtime_error("Unhandled IRQ control: 0x" + to_hex(offset.value()) + " <- 0x" + to_hex(val));
                }
            }
            
            return;
        }
        
        if (auto offset = map::DMA.contains(abs_addr)) {
            setDmaReg(offset.value(), val);
            return;
        }
        
        if (auto offset = map::GPU.contains(abs_addr)) {
            switch (offset.value()) {
                case 0:
                    gpu.gp0(val);
                    break;
                case 4:
                    gpu.gp1(val);
                    break;
                default:
                    throw std::runtime_error("GPU write " + std::to_string(offset.value()) + ": 0x" + to_hex(val));
            }
            
            return;
        }
        
        if (auto offset = map::TIMERS.contains(abs_addr)) {
            _timers.store(offset.value(), val);
            
            return;
        }
        
        if (auto offset = map::CDROM.contains(abs_addr)) {
           _cdrom.store<uint32_t>(addr, val);
            
            return;
        }
        
        if (auto offset = map::MDEC.contains(abs_addr)) {
            throw std::runtime_error("Unhandled write to MDEC 0x" + to_hex(offset.value()));
        }
        
        if (auto offset = map::SPU.contains(abs_addr)) {
            //throw std::runtime_error(": 0x" + to_hex(offset.value()) + " <- 0x" + to_hex(val));
            return;
        }
        
        // Peripheral I/O Ports
        if (auto offset = map::PADMEMCARD.contains(abs_addr)) {
            _sio.store(abs_addr, val);
            
            return;
        }
        
        if (auto _ = map::CACHECONTROL.contains(abs_addr)) {
            if (sizeof(T) != 4) {
                throw std::runtime_error("Unhandled cache control access");
            }
            
            _cacheControl.val = val;
            
            return;
        }
        
        if (auto offset = map::MEMCONTROL.contains(abs_addr)) {
            if (sizeof(T) != 4) {
                throw std::runtime_error("Unbalanced MEM_CONTROL access (" + std::to_string(sizeof(T)) + ")");
            }
            
            uint32_t index = (offset.value() >> 2);
            memControl[index] = val;
            
            return;
        }
        
        if (auto _ = map::EXPANSION1.contains(abs_addr)) {
            if(_ == 132) {
                //   1F000084h 2Ch Pre-Boot ID  ("Licensed by Sony Computer Entertainment Inc.")
                return;
            }
            
            throw std::runtime_error("Unhandled EXPANSION1 load at address 0x" + to_hex(addr));
        }
        
        if (auto _ = map::RAM_SIZE.contains(abs_addr)) {
            if (sizeof(T) != 4) {
                throw std::runtime_error("Unhandled RAM_SIZE access");
            }
            
            _ramSize = val;
            
            return;
        }
        
        if (auto _ = map::EXPANSION2.contains(abs_addr)) {
            return;
            throw std::runtime_error("Unhandled EXPANSION_2 store at address 0x" + to_hex(addr));
        }
        
        return;
        //throw std::runtime_error("Unhandled store into address 0x" + to_hex(addr) + ": 0x" + to_hex(val));
    }
    
    // DMA register read
    uint32_t dmaReg(uint32_t offset);
    
    // Execute DMA transfer for a port
    void doDma(Port port);
    void dmaBlock(Port port);
    void dmaLinkedList(Port port);
    void setDmaReg(uint32_t offset, uint32_t val);
    
    std::string to_hex(uint32_t value) {
        std::stringstream ss;
        ss << std::hex << value;
        return ss.str();
    }
    
private:
    uint32_t expansion1Base = 0;
    uint32_t expansion2Base = 0;
    
    uint32_t _ramSize = 0;
    
    uint32_t memControl[9];
    
public:
    Ram ram;
    CDROM _cdrom;
    ScratchPad _scratchPad;
    //Joypad _joypad;
    Emulator::IO::SIO _sio;
    
    IRQ _irq;
    
    Emulator::IO::Timers _timers;
    
    CACHECONTROL _cacheControl = (0x001e988);
    
    Bios bios;
    Dma dma;
    Emulator::Gpu gpu;
    Emulator::SPU spu;
};
