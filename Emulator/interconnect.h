#pragma once

#include <cstdint>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "Bios.h"
#include "Dma.h"
#include "GPU/Gpu.h"
#include "Ram.h"
#include "Range.h"
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
        : ram(ram), bios(bios), dma(dma), gpu(gpu), spu(spu) {  }
    
    template<typename T>
    T load(uint32_t addr) {
        uint32_t abs_addr = map::maskRegion(addr);
        
        if (auto offset = map::RAM.contains(abs_addr)) {
            return ram->load<T>(offset.value());
        }
        
        if (auto _ = map::SCRATCHPAD.contains(abs_addr)) {
            if (addr > 0xa0000000) {
                throw std::runtime_error("ScratchPad access through uncached memory");
            }
            
            throw std::runtime_error("Unhandled SCRATCH_PAD load at address 0x" + to_hex(addr));
        }
        
        if (auto offset = map::BIOS.contains(abs_addr)) {
            //std::cerr << "Offset; " + std::to_string(offset.value());
            return bios->load<T>(offset.value());
        }
        
        if (auto offset = map::IRQ_CONTROL.contains(abs_addr)) {
            //printf("IRQ control read %s\n", to_hex(abs_addr).c_str());
            return 0;
        }
        
        if (auto offset = map::DMA.contains(abs_addr)) {
            if(abs_addr == 0x01000200) {
                printf("Yay\n");
            }
            
            return dmaReg(offset.value());
        }
        
        if (auto offset = map::GPU.contains(abs_addr)) {
            //printf("GPU read %s\n", std::to_string(offset.value()).c_str());
            switch (offset.value()) {
                case 0:
                    return gpu->read();
                case 4: {
                    uint32_t r = gpu->status();
                    
                    if(r != 0x1c000000) {
                        // TODO;
                        //std::cerr << "y is it not THE SAME!; " << std::to_string(r) << "\n";
                    }
                    
                    //return gpu->status();
                    return 0x1c000000;
                    //return gpu->status();
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
            //return 0;
            throw std::runtime_error("Unhandled PAD_MEMCARD load at address 0x" + to_hex(addr));
        }
        
        if (auto _ = map::EXPANSION1.contains(abs_addr)) {
            return 0xff;
        }
        
        if (auto _ = map::RAM_SIZE.contains(abs_addr)) {
            //return 0;
            throw std::runtime_error("Unhandled RAM_SIZE load at address 0x" + to_hex(addr));
        }
        
        if (auto _ = map::MEMCONTROL.contains(abs_addr)) {
            if (sizeof(T) != 4) {
                throw std::runtime_error("Unhandled MEM_CONTROL access (" + std::to_string(sizeof(T)) + ")");
            }
            
            return 0;
            //throw std::runtime_error("Unhandled MEM_CONTROL load at address 0x" + to_hex(addr));
        }
        
        if (auto _ = map::CACHECONTROL.contains(abs_addr)) {
            if (sizeof(T) != 4) {
                throw std::runtime_error("Unhandled cache control access (" + std::to_string(sizeof(T)) + ")");
            }
            
            //return 0;
            throw std::runtime_error("Unhandled CACHE_CONTROL load at address 0x" + to_hex(addr));
        }
        
        if (auto _ = map::EXPANSION2.contains(abs_addr)) {
            //return 0;
            throw std::runtime_error("Unhandled EXPANSION_2 load at address 0x" + to_hex(addr));
        }
        
        //std::cerr << "Wee; 0x" << to_hex(addr) << "\n";
        //throw std::runtime_error("Unhandled load at address 0x" + to_hex(addr));
        return 0;
    }
    
    template<typename T>
    void store(uint32_t addr, T val) {
        uint32_t abs_addr = map::maskRegion(addr);
        
        if (auto offset = map::RAM.contains(abs_addr)) {
            ram->store<T>(offset.value(), val);
            //ram->store32(offset.value(), val);
            
            return;
        }
        
        if (auto offset = map::SCRATCHPAD.contains(abs_addr)) {
            if (addr > 0xa0000000) {
                throw std::runtime_error("ScratchPad access through uncached memory");
            }
            
            throw std::runtime_error(" to SCRATCH_PAD 0x" + to_hex(offset.value()));
        }
        
        if (auto offset = map::IRQ_CONTROL.contains(abs_addr)) {
            //throw std::runtime_error("Unhandled IRQ control: 0x" + to_hex(offset.value()) + " <- 0x" + to_hex(val));
            //printf("IRQ control unhandled..\n");
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
            //throw std::runtime_error("Unhandled TIMERS control: 0x" + to_hex(offset.value()) + " <- 0x" + to_hex(val));
            return;
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
            //printf("hex error spu\n");
            return;
            //throw std::runtime_error(": 0x" + to_hex(offset.value()) + " <- 0x" + to_hex(val));
        }
        
        if (auto offset = map::PADMEMCARD.contains(abs_addr)) {
            throw std::runtime_error("Unhandled write to PAD_MEMCARD 0x" + to_hex(offset.value()));
        }
        
        if (auto _ = map::CACHECONTROL.contains(abs_addr)) {
            if (sizeof(T) != 4) {
                throw std::runtime_error("Unhandled cache control access");
            }
            
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
                default:
                    //printf("Unhandled write to MEM_CONTROL register %s: %0x8\n", to_hex(offset.value()).c_str(), to_hex(val));
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
            return;
        }
        
        throw std::runtime_error("Unhandled store into address 0x" + to_hex(addr) + ": 0x" + to_hex(val));
    }
    
    /*// Load word
    uint32_t load32(uint32_t addr);
    
    // Load half word
    uint16_t load16(uint32_t addr);
    
    // Load byte
    uint8_t load8(uint32_t addr);
    
    // Store word
    void store32(uint32_t addr, uint32_t val);
    
    // Store half word
    void store16(uint32_t addr, uint16_t val);
    
    // Store byte
    void store8(uint32_t addr, uint8_t val);*/
    
    // DMA
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
    Bios* bios;
    Dma* dma;
    Emulator::Gpu* gpu;
    Emulator::SPU* spu;
};
