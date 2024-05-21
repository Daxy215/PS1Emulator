#pragma once
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
    static const Range BIOS = {0xbfc00000, 512 * 1024};
    
    static const Range MEMCONTROL = { 0x1f801000, 36};
    
    static const Range RAM_SIZE = {0x1f801060, 4};
    static const Range RAM      = {0xa0000000, 2 * 1024 * 1024};
    
    static const Range CACHECONTROL = {0xfffe0130, 4};
}
