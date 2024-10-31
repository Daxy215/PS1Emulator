#pragma once

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "Bios.h"
#include "DMA/Dma.h"
#include "GPU/Gpu.h"
#include "Ram.h"
#include "Range.h"
#include "Memory/IRQ.h"
#include "Memory/CDROM/CDROM.h"
#include "Memory/IO/Joypad.h"
#include "Memory/ScratchPad/ScratchPad.h"
#include "SPU/SPU.h"

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
        : ram(ram), _scratchPad(), bios(bios), dma(dma), gpu(gpu), spu(spu) {  }
    
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
                    return gpu.read(offset.value());
                case 4: {
                    return gpu.status();
                }
                default:
                    return 0;
            }
        }
        
        if (auto offset = map::TIMERS.contains(abs_addr)) {
            //printf("TIMERS control read %s\n", to_hex(abs_addr).c_str());
            if(offset == 32)
                return 5809;
            
            return 0;
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
            return _joypad.load(abs_addr);
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
        
        if (auto _ = map::MEMCONTROL.contains(abs_addr)) {
            if (sizeof(T) != 4) {
                throw std::runtime_error("Unhandled MEM_CONTROL access (" + std::to_string(sizeof(T)) + ")");
            }
            
            throw std::runtime_error("Unhandled MEM_CONTROL load at address 0x" + to_hex(addr));
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
        
        //return 0;
        throw std::runtime_error("Unhandled load at address 0x" + to_hex(addr));
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
            if(offset == 32)
                target = val;
            
            return;
            //throw std::runtime_error("Unhandled TIMERS control: 0x" + to_hex(offset.value()) + " <- 0x" + to_hex(val));
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
            _joypad.store(abs_addr, val);
            
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
            
            switch (offset.value()) {
                case 0:
                    if (val != 0x1f000000) {
                        throw std::runtime_error("Bad expansion 1 base address: 0x" + to_hex(val));
                    }
                    
                    break;
                case 4:
                    if (val != 0x1f802000) {
                        throw std::runtime_error("Bad expansion 2 base address: 0x" + to_hex(val));
                    }
                    
                    break;
                
                case 8: {
                    /*
                     * 1F801008h - Expansion 1 Delay/Size (usually 0013243Fh) (512Kbytes, 8bit bus)
                     */
                    
                    
                    
                    break;
                }
                
                case 12: {
                    /*
                     * 1F80100Ch - Expansion 3 Delay/Size (usually 00003022h) (1 byte)
                     */
                    
                    
                    
                    break;
                }
                
                case 16: {
                    // 1F801010h - BIOS ROM Delay/Size (usually 0013243Fh) (512Kbytes, 8bit bus)
                    
                    
                    
                    break;
                }
                
                case 20: {
                    // 1F801014h - SPU Delay/Size (200931E1h) (use 220931E1h for SPU-RAM reads)
                    
                    // Ignoring SPU for now
                    
                    break;
                }
                
                case 24: {
                    // 1F801018h - CDROM Delay/Size (00020843h or 00020943h)
                    
                    break;
                }
                
                case 28: {
                    /*
                     * 1F80101Ch - Expansion 2 Delay/Size (usually 00070777h) (128 bytes, 8bit bus)
                     */
                    
                    break;
                }
                
                case 32: {
                    /*
                     * 1F801020h - COM_DELAY / COMMON_DELAY (00031125h or 0000132Ch or 00001325h)
                     * 0-3   COM0 - Recovery period cycles
                     * 4-7   COM1 - Hold period cycles
                     * 8-11  COM2 - Floating release cycles
                     * 12-15 COM3 - Strobe active-going edge delay
                     * 16-31 Unknown/unused (read: always 0000h)
                     */
                    
                    
                    
                    break;
                }
                default:
                    throw std::runtime_error("Unhandled write to MEM_CONTROL register 0x" + to_hex(offset.value()) + ": 0x" + to_hex(val));
                    break;
            }
            
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
            switch (abs_addr) {
            case 0x1F802041:
                
                return;
            
            case 0x1F802023:
            case 0x1F802080:
                    
                return;
            
            default:
                std::cout << "[BUS] Write Unsupported to EXP2: " << std::hex << std::setw(8) << addr 
                          << " Value: " << std::hex << std::setw(8) << val << std::endl;
                    
                return;
            }
            
            throw std::runtime_error("Unhandled EXPANSION_2 store at address 0x" + to_hex(addr));
            return;
        }
        
        throw std::runtime_error("Unhandled store into address 0x" + to_hex(addr) + ": 0x" + to_hex(val));
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

    // Timers - TODO; Move to a class
    uint32_t target = 0;
    
public:
    Ram ram;
    CDROM _cdrom;
    ScratchPad _scratchPad;
    Joypad _joypad;
    
    IRQ _irq;
    
    CACHECONTROL _cacheControl = (0x001e988);
    
    Bios bios;
    Dma dma;
    Emulator::Gpu gpu;
    Emulator::SPU spu;
};
