#pragma once

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "Bios.h"
#include "Dma.h"
#include "GPU/Gpu.h"
#include "Ram.h"
#include "Range.h"
#include "Memory/IO/Joypad.h"
#include "Memory/ScratchPad/ScratchPad.h"
#include "SPU/SPU.h"

/**
 * This class is used to allow the BIOS to communicate with the CPU! :D
 */
/*class Ram;
class Dma;*/
class Bios;

class Interconnect {
public:
    Interconnect(Ram* ram, Bios* bios, Dma* dma, Emulator::Gpu* gpu, Emulator::SPU* spu)
        : ram(ram), _scratchPad(), bios(bios), dma(dma), gpu(gpu), spu(spu) {  }
    
    template<typename T>
    T load(uint32_t addr) {
        uint32_t abs_addr = map::maskRegion(addr);
        
        if (auto offset = map::RAM.contains(abs_addr)) {
            return ram->load<T>(offset.value());
        }
        
        if (auto offset = map::SCRATCHPAD.contains(abs_addr)) {
            if (addr > 0xa0000000) {
                throw std::runtime_error("ScratchPad access through uncached memory");
            }
            
            return _scratchPad.load<T>(offset.value());
            //throw std::runtime_error("Unhandled SCRATCH_PAD load at address 0x" + to_hex(addr));
        }
        
        if (auto offset = map::BIOS.contains(abs_addr)) {
            return bios->load<T>(offset.value());
        }
        
        if (auto offset = map::IRQ_CONTROL.contains(abs_addr)) {
            //printf("IRQ control read %s\n", to_hex(abs_addr).c_str());
            return 0;
        }
        
        if (auto offset = map::DMA.contains(abs_addr)) {
            return dmaReg(offset.value());
        }
        
        if (auto offset = map::GPU.contains(abs_addr)) {
            switch (offset.value()) {
                case 0:
                    return gpu->read(offset.value());
                case 4: {
                    return gpu->status();
                }
                default:
                    return 0;
            }
        }
        
        if (auto offset = map::TIMERS.contains(abs_addr)) {
            //printf("TIMERS control read %s\n", to_hex(abs_addr).c_str());
            return 0;
        }
        
        if (auto offset = map::CDROM.contains(abs_addr)) {
            printf("CDROM load %x\n", abs_addr);
            std::cerr << "";
            
            return 0b11111111;
            //throw std::runtime_error("Unhandled CDROM load at address 0x" + to_hex(addr));
        }
        
        if (auto _ = map::MDEC.contains(abs_addr)) {
            //return 0;
            throw std::runtime_error("Unhandled MDEC load at address 0x" + to_hex(addr));
        }
        
        if (auto offset = map::SPU.contains(abs_addr)) {
            // https://github.com/psx-spx/psx-spx.github.io/blob/master/docs/soundprocessingunitspu.md
            //std::cerr << "SPU load register; " << to_hex(addr) << "\n";
            spu->load(addr, offset.value());
            
            return 0;
        }
        
        if (auto _ = map::PADMEMCARD.contains(abs_addr)) {
            return _joypad.load(abs_addr);
        }
        
        if (auto _ = map::EXPANSION1.contains(abs_addr)) {
            
            //throw std::runtime_error("Unhandled EXPANSION1 load at address 0x" + to_hex(addr));
        }
        
        if (auto _ = map::RAM_SIZE.contains(abs_addr)) {
            throw std::runtime_error("Unhandled RAM_SIZE load at address 0x" + to_hex(addr));
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
            return 0xFF;
            //throw std::runtime_error("Unhandled EXPANSION_2 load at address 0x" + to_hex(addr));
        }
        
        return 0;
        //throw std::runtime_error("Unhandled load at address 0x" + to_hex(addr));
    }
    
    template<typename T>
    void store(uint32_t addr, T val) {
        uint32_t abs_addr = map::maskRegion(addr);
        
        if (auto offset = map::RAM.contains(abs_addr)) {
            ram->store<T>(offset.value(), val);
            
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
            //throw std::runtime_error("Unhandled IRQ control: 0x" + to_hex(offset.value()) + " <- 0x" + to_hex(val));
            return;
        }
        
        if (auto offset = map::DMA.contains(abs_addr)) {
            setDmaReg(offset.value(), val);
            return;
        }
        
        if (auto offset = map::GPU.contains(abs_addr)) {
            switch (offset.value()) {
                case 0:
                    gpu->gp0(val);
                    break;
                case 4:
                    gpu->gp1(val);
                    break;
                default:
                    throw std::runtime_error("GPU write " + std::to_string(offset.value()) + ": 0x" + to_hex(val));
            }
            
            return;
        }
        
        if (auto offset = map::TIMERS.contains(abs_addr)) {
            return;
            //throw std::runtime_error("Unhandled TIMERS control: 0x" + to_hex(offset.value()) + " <- 0x" + to_hex(val));
        }
        
        if (auto offset = map::CDROM.contains(abs_addr)) {
            int8_t index = static_cast<int8_t>(val & 0x3);
            
            printf("CDROM %x - %x\n", abs_addr, offset.value());
            std::cerr << "";
            
            if(abs_addr == 0x1f801800) {
                // https://psx-spx.consoledev.net/cdromdrive/#1f801800h-indexstatus-register-bit0-1-rw-bit2-7-read-only
                // READ-ONLY
                return;
            }
            
            return;
            //throw std::runtime_error("Unhandled write to CDROM 0x" + to_hex(abs_addr));
        }
        
        if (auto offset = map::MDEC.contains(abs_addr)) {
            throw std::runtime_error("Unhandled write to MDEC 0x" + to_hex(offset.value()));
        }
        
        if (auto offset = map::SPU.contains(abs_addr)) {
            return;
            //throw std::runtime_error(": 0x" + to_hex(offset.value()) + " <- 0x" + to_hex(val));
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
            
            return;
            throw std::runtime_error("Unhandled cache control access");
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
                default:
                    //throw std::runtime_error("Unhandled write to MEM_CONTROL register 0x" + to_hex(offset.value()) + ": 0x" + to_hex(val));
                    break;
            }
            
            return;
        }
        
        if (auto _ = map::RAM_SIZE.contains(abs_addr)) {
            if (sizeof(T) != 4) {
                throw std::runtime_error("Unhandled RAM_SIZE access");
            }
            
            return;
        }
        
        if (auto _ = map::EXPANSION2.contains(abs_addr)) {
            switch (abs_addr) {
            case 0x1F802041:
                // Cyan
                /*std::cout << "\033[36m";
                std::cout << "[EXP2] PSX: POST [" << std::hex << std::setw(1) << static_cast<char>(val) << "]" << std::endl;
                std::cout << "\033[0m";*/
                
                break;
            
            case 0x1F802023:
            case 0x1F802080:
                // Cyan
                /*std::cout << "\033[36m";
                std::cout << static_cast<char>(val);
                std::cout << "\033[0m";*/
                
                break;
            
            default:
                std::cout << "[BUS] Write Unsupported to EXP2: " << std::hex << std::setw(8) << addr 
                          << " Value: " << std::hex << std::setw(8) << val << std::endl;
                
                break;
            }
            
            return;
            //throw std::runtime_error("Unhandled EXPANSION_2 store at address 0x" + to_hex(addr));
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
    
public:
    Ram* ram;
    ScratchPad _scratchPad;
    Joypad _joypad;
    
    Bios* bios;
    Dma* dma;
    Emulator::Gpu* gpu;
    Emulator::SPU* spu;
};
