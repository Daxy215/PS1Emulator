#pragma once
#include <array>
#include <cstdint>
#include <optional>

struct Range {
    std::optional<uint32_t> contains(uint32_t addr) const {
        if (addr >= start && addr < start + length) {
            return addr - start;
        }
        
        return std::nullopt;
    }
    
    uint32_t start;
    uint32_t length;
};

namespace map {
    static const Range RAM      = {0x00000000, 2 * 1024 * 1024}; // 0xa0000000
    static const Range RAM_SIZE = {0x1f801060, 4}; // 0x1f801060
    
    static const Range BIOS = { 0x1fc00000, 512 * 1024}; // 0xbfc00000
    
    static const Range MEMCONTROL = { 0x1f801000, 36}; // 0x1f801000
    //static const Range MEMCONTROL = {0x1f801000, 36};
    static const Range CACHECONTROL = {0xfffe0130, 4}; // 0xfffe0130
    
    // SPU Registers
    static const Range SPU = {0x1f801c00, 640};
    
    // Expansion regions
    static const Range EXPANSION1 = {0x1f000000, 512 * 1024};
    static const Range EXPANSION2 = {0x1f802000, 66};
    
    // Interrupt Control registers (status and mask)
    static const Range IRQ_CONTROL = {0x1f801070, 8};
    
    // Region mask
    static const std::array<uint32_t, 8> REGION_MASK = {
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  // KUSEG: 2048MB
        0x7fffffff,                                      // KSEG0: 512MB
        0x1fffffff,                                      // KSEG1: 512MB
        0xffffffff, 0xffffffff                           // KSEG2: 1024MB
    };
    
    uint32_t maskRegion(uint32_t addr) {
        // Index address space in 512 chunks
        size_t index = addr >> 29;
        
        if(index >= REGION_MASK.size()) {
            throw std::out_of_range("Address index out of range");
        }
        
        return addr & REGION_MASK[index];
    }
}
