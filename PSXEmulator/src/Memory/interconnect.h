#pragma once

#include <iostream>
#include <stdint.h>
#include <optional>
#include <sstream>
#include <string>

#include "Bios/Bios.h"
#include "../DMA/Dma.h"
#include "../GPU/Gpu.h"
#include "Memories/Ram.h"
#include "Range.h"
#include "../Memory/IRQ.h"
#include "../Memory/CDROM/CDROM.h"
#include "../Memory/IO/SIO.h"
#include "../Memory/Memories/ScratchPad.h"
#include "../Memory/Timers/Timers.h"
#include "../SPU/Stolen/spu.h"
#include "MDEC/MDEC.h"

//#include "CDROM/cdrom.h"
//#include "../SPU/SPU.h"

/**
 * This class is used to allow the BIOS to communicate with the CPU! :D
 */
/*class Ram;
class Dma;*/
class Bios;

struct CacheControl {
    uint32_t val = 0;
    
    CacheControl(uint32_t val) : val(val) {}
    
    bool isCacheEnabled() {
        return (val & 0x800) != 0;
    }
    
    bool isInTagMode() {
        return (val & 0x4) != 0;
    }
};

struct ICache {
    uint32_t tag;
    uint32_t data;
    
    ICache() : tag(0), data(0) {}
    ICache(uint32_t tag, uint32_t data) : tag(tag), data(data) {}
};

class Interconnect {
public:
    Interconnect()  : memControl{}, _gpu(nullptr), _spu(nullptr) {
        
    }
    
    Interconnect(Emulator::Gpu* gpu/*, Emulator::SPU spu*/)
        : memControl{}, _gpu(gpu), _spu(new spu::SPU())
    /*, spu(spu)*/ {
        _ram = Ram();
        
        // TODO;
        _bios = Bios("../BIOS/ps-22a.bin");
        //_bios = Bios("../BIOS/openbios.bin");
        //_bios = Bios("../BIOS/openbios2.bin");
        //_bios = Bios("../BIOS/openbios-fastboot.bin");
        //_bios = Bios("../BIOS/openbios-unirom.bin");
        
        _dma = Dma();
    }
    
    bool step(uint32_t cycles);
    
    uint32_t loadInstruction(uint32_t addr);
    
    template<typename T>
    T load(uint32_t addr) {
        uint32_t abs_addr = map::maskRegion(addr);
        uint32_t offset = 0;
        
        if (map::RAM.contains(abs_addr, offset)) {
            return _ram.load<T>(offset);
        }
        
        if (map::SCRATCHPAD.contains(abs_addr, offset)) {
            if (addr > 0xa0000000) {
                //throw std::runtime_error("ScratchPad access through uncached memory");
            }
            
            return _scratchPad.load<T>(offset);
        }
        
        if (map::BIOS.contains(abs_addr, offset)) {
            return _bios.load<T>(offset);
        }
        
        if (map::IRQ_CONTROL.contains(abs_addr, offset)) {
            switch (offset) {
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
        
        if (map::DMA.contains(abs_addr, offset)) {
            return dmaReg(offset);
        }
        
        if (map::PCSX_REDUX_EXPANSION.contains(abs_addr, offset)) {
            /*switch (offset) {
                case 0:
                        // Read "PCSX" expansion ID
                        return 0;
                case 1: 
                        // Handle console putchar (write character to console)
                        return 0;
                case 2: 
                        // Handle debug breakpoint (pause emulator)
                        return 0;
                case 3: 
                        // Handle exit code (set exit code)
                        return 0;
                case 4: 
                        // Handle notification message pointer (show pop-up)
                        return 0;
                default:
                    throw std::runtime_error("Unhandled PCSX-Redux Expansion access");
            }*/
            
            return 0;
        }
        
        // 0x1f801810
        if (map::GPU.contains(abs_addr, offset)) {
            switch (offset) {
                case 0:
                    return _gpu->read();
                case 4: {
                    return _gpu->status();
                }
                default:
                    throw std::runtime_error("Unhandled GPU load at address 0x");
            }
            
            printf("broo");
        }
        
        if (map::TIMERS.contains(abs_addr, offset)) {
            return _timers.load(offset);
        }
        
        if (map::CDROM.contains(abs_addr, offset)) {
            return _cdrom.load(offset);
            //return psx_cdrom_read8(_cdrom, offset);
        }
        
        if (map::MDEC.contains(abs_addr, offset)) {
            //throw std::runtime_error("Unhandled MDEC load at address 0x" + to_hex(addr));
            return mdec.load(abs_addr);
        }
        
        if (map::SPU.contains(abs_addr, offset)) {
            // https://github.com/psx-spx/psx-spx.github.io/blob/master/docs/soundprocessingunitspu.md
            //std::cerr << "SPU load register; " << to_hex(addr) << "\n";
            uint16_t v = _spu->read(offset) | _spu->read(offset + 1) << 8;
            
            return v;
        }
        
        if (auto _ = map::PADMEMCARD.contains(abs_addr, offset)) {
            return _sio.load(abs_addr);
        }
        
        if (auto _ = map::EXPANSION1.contains(abs_addr, offset)) {
            T r = 0;
            
            /*for(size_t i = 0; i < sizeof(T); i++) {
                // Simulate that nothing is connected
                uint8_t data = 0;
                
                r |= (static_cast<uint32_t>(data) << (8 * i));
            }*/
            
            return r;
        }
        
        if (auto _ = map::RAM_SIZE.contains(abs_addr, offset)) {
            return _ramSize;
        }
        
        if (map::MEMCONTROL.contains(abs_addr, offset)) {
            if (sizeof(T) != 4) {
                throw std::runtime_error("Unhandled MEM_CONTROL access (" + std::to_string(sizeof(T)) + ")");
            }
            
            uint32_t index = (offset >> 2);
            return memControl[index];
        }
        
        if (auto _ = map::CACHECONTROL.contains(abs_addr, offset)) {
            if (sizeof(T) != 4) {
                throw std::runtime_error("Unhandled cache control access (" + std::to_string(sizeof(T)) + ")");
            }
            
            return _cacheControl.val;
        }
        
        if (map::EXPANSION2.contains(abs_addr, offset)) {
            // https://psx-spx.consoledev.net/expansionportpio/#1f802021hread-sra-duart-status-register-a-r
            if(offset == 0x21) {
                // UART status register A.
                // Just indicating that,
                // the bit for "Tx ready" is set.
                return 1 << 2; // 4
            }
            
            throw std::runtime_error("Unhandled EXPANSION_2 load at address 0x");
        }
        
        //throw std::runtime_error("Unhandled load at address 0x");
        return 0;
    }
    
    template<typename T>
    void store(uint32_t addr, T val) {
        uint32_t abs_addr = map::maskRegion(addr);
        uint32_t offset = 0;
        
        if (map::RAM.contains(abs_addr, offset)) {
            _ram.store<T>(offset, val);
            
            return;
        }
        
        if (map::SCRATCHPAD.contains(abs_addr, offset)) {
            if (addr > 0xa0000000) {
                //throw std::runtime_error("ScratchPad access through uncached memory");
            }
            
            _scratchPad.store<T>(offset, val);
            
            return;
        }
        
        if (map::IRQ_CONTROL.contains(abs_addr, offset)) {
            switch (offset) {
                case 0: {
                    _irq.acknowledge(static_cast<uint16_t>(val));
                    break;
                }
                
                case 4: {
                    _irq.setMask(static_cast<uint16_t>(val));
                    break;
                }
                
                default: {
                    throw std::runtime_error("Unhandled IRQ control: 0x");
                }
            }
            
            return;
        }
        
        if (map::DMA.contains(abs_addr, offset)) {
            setDmaReg(offset, val);
            
            return;
        }
        
        if (map::GPU.contains(abs_addr, offset)) {
            switch (offset) {
                case 0:
                    _gpu->gp0(val);
                    break;
                case 4:
                    _gpu->gp1(val);
                    break;
                default:
                    throw std::runtime_error("GPU write " + std::to_string(offset) + ": 0x");
            }
            
            return;
        }
        
        if (map::TIMERS.contains(abs_addr, offset)) {
            _timers.store(offset, val);
            
            return;
        }
        
        if (map::CDROM.contains(abs_addr, offset)) {
            _cdrom.store(offset, val);
            //psx_cdrom_write8(_cdrom, offset, val);
            
            return;
        }
        
        if (map::MDEC.contains(abs_addr, offset)) {
            mdec.store(addr, val);
            
            return;
        }
        
        if (map::SPU.contains(abs_addr, offset)) {
            //throw std::runtime_error(": 0x" + to_hex(offset) + " <- 0x" + to_hex(val));
            //spu.store(abs_addr, val);
            _spu->write(offset, (val) & 0xFF);
            _spu->write(offset + 1, (val >> 8) & 0xFF);
            
            return;
        }
        
        // Peripheral I/O Ports
        if (map::PADMEMCARD.contains(abs_addr, offset)) {
            _sio.store(abs_addr, val);
            
            return;
        }
        
        if (auto _ = map::CACHECONTROL.contains(abs_addr, offset)) {
            if (sizeof(T) != 4) {
                throw std::runtime_error("Unhandled cache control access");
            }
            
            _cacheControl.val = val;
            
            return;
        }
        
        if (map::MEMCONTROL.contains(abs_addr, offset)) {
            if (sizeof(T) != 4) {
                throw std::runtime_error("Unbalanced MEM_CONTROL access (" + std::to_string(sizeof(T)) + ")");
            }
            
            uint32_t index = (offset >> 2);
            memControl[index] = val;
            
            return;
        }
        
        if (auto _ = map::EXPANSION1.contains(abs_addr, offset)) {
            if(_ == 132) {
                //   1F000084h 2Ch Pre-Boot ID  ("Licensed by Sony Computer Entertainment Inc.")
                return;
            }
            
            throw std::runtime_error("Unhandled EXPANSION1 load at address 0x");
        }
        
        if (auto _ = map::RAM_SIZE.contains(abs_addr, offset)) {
            if (sizeof(T) != 4) {
                throw std::runtime_error("Unhandled RAM_SIZE access");
            }
            
            _ramSize = val;
            
            return;
        }
        
        if (map::EXPANSION2.contains(abs_addr, offset)) {
            // TTY
            if(offset == 0x23) {
                std::cerr << static_cast<char>(val) << "";
            }
            
            return;
            throw std::runtime_error("Unhandled EXPANSION_2 store at address 0x");
        }
        
        //throw std::runtime_error("Unhandled store into address 0x");
        return;
    }
    
    // DMA register read
    uint32_t dmaReg(uint32_t offset);
    
    // Execute DMA transfer for a port
    void doDma(Port port);
    void dmaBlock(Port port);
    void dmaLinkedList(Port port);
    void setDmaReg(uint32_t offset, uint32_t val);
    
    void reset() {
        expansion1Base = 0;
        expansion2Base = 0;
        
        _ramSize = 0;
        
        for(unsigned int& i : memControl)
            i = 0;
        
        _ram.reset();
        _cdrom.reset();
        _scratchPad.reset();
        _irq.reset();
        _timers.reset();
        mdec.reset();
        
        _cacheControl = (0);
        for(auto& i : icache)
            i = {};
        
        // TODO;
        _bios.reset("../BIOS/ps-22a.bin");
        _dma.reset();
        _gpu->reset();
        //spu->reset();
    }
    
private:
    uint32_t expansion1Base = 0;
    uint32_t expansion2Base = 0;
    
    uint32_t _ramSize = 0;
    
    uint32_t memControl[9];
    
public:
    Ram _ram;
    //CDROM _cdrom;
    CDROM _cdrom;
    //cdrom _cdrom;
    //psx_cdrom_t* _cdrom;
    ScratchPad _scratchPad;
    //Joypad _joypad;
    Emulator::IO::SIO _sio;
    
    IRQ _irq;
    
    Emulator::IO::Timers _timers;
    
    CacheControl _cacheControl = (0);
    
    // The i-Cache can hold 4096 bytes, or 1024 instructions.
    ICache icache[1024];
    
    Bios _bios;
    Dma _dma;
    Emulator::Gpu* _gpu;
    MDEC mdec;
    spu::SPU* _spu;
    //Emulator::SPU spu;
};
