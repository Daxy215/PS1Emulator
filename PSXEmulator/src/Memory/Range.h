#pragma once

#include <array>
#include <stdint.h>

#ifndef MAP_H
#define MAP_H
namespace map {
    struct Range {
        bool contains(uint32_t addr) const {
            if (addr >= start && addr < start + length) {
                return true;
            }
            
            return false;
        }
        
        bool contains(uint32_t addr, uint32_t& offset) const {
            if (addr >= start && addr < start + length) {
                offset = addr - start;
                return true;
            }
            
            return false;
        }
        
        uint32_t start;
        uint32_t length;
    };
     
    // Main RAM: 2MB mirrored four times over the first 8MB (probably
    // in case they decided to use a bigger RAM later on?)
    static const Range RAM      = {0x00000000, 8 * 1024 * 1024}; // 0xa0000000
    static const Range RAM_SIZE = {0x1f801060, 4}; // 0x1f801060
    
    static const Range BIOS = { 0x1fc00000, 512 * 1024}; // 0xbfc00000
    static const Range GPU  = {0x1f801810, 8};
    
    static const Range MEMCONTROL = { 0x1f801000, 36}; // 0x1f801000
    //static const Range SYSCONTROL = {0x1f801000, 36};
    static const Range CACHECONTROL = {0xfffe0130, 4}; // 0xfffe0130
    
    // Gamepad and memory card controller
    static const Range PADMEMCARD = {0x1f801040, 32};
    
    // SPU Registers
    // Basically sound thingies
    static const Range SPU = {0x1f801c00, 640};
    
    // Direct Memory Access registers
    // Used to move data back and forth between RAM & (GPU, CDROM, SPU.. etc..)
    static const Range DMA = {0x1f801080, 0x80};
    
    static const Range PCSX_REDUX_EXPANSION = {0x1F802080, 8};
    
    // Expansion regions
    static const Range EXPANSION1 = {0x1f000000, 512 * 1024};
    static const Range EXPANSION2 = {0x1f802000, 66};
    
    // CDROM controller
    static const Range CDROM = {0x1f801800, 0x4};
    
    static const Range MDEC = {0x1f801820, 8};
    
    // Interrupt Control registers (status and mask)
    static const Range IRQ_CONTROL = {0x1f801070, 8};
    
    static const Range TIMERS = {0x1f801100, 0x30};
    
    // Data cache used as a fast 1kb RAM
    static const Range SCRATCHPAD = {0x1f800000, 1024};
    
    // Region mask
    static const std::array<uint32_t, 8> REGION_MASK = {
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  // KUSEG: 2048MB
        0x7fffffff,                                      // KSEG0:  512MB
        0x1fffffff,                                      // KSEG1:  512MB
        0xffffffff, 0xffffffff,                          // KSEG2: 1024MB
    };
    
    inline uint32_t maskRegion(uint32_t addr) {
        // Index address space in 512 chunks
        size_t index = addr >> 29;
        
        if(index >= REGION_MASK.size()) {
            throw std::out_of_range("Address index out of range");
        }
        
        return addr & REGION_MASK[index];
    }
    inline bool isExecutableAddress(uint32_t addr) {
        uint32_t masked = map::maskRegion(addr);
        return map::BIOS.contains(masked) || map::RAM.contains(masked);
    }
}

#endif
