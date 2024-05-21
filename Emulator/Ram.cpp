#include "Ram.h"

#include <stdexcept>

// Default data contents are garbage
Ram::Ram() : data(2 * 1024 * 1024, 0xCA) {
    
}

uint32_t Ram::load32(uint32_t offset) {
    if (offset + 3 >= data.size()) {
        throw std::out_of_range("Offset out of range");
    }
    
    uint32_t b0 = data[offset];
    uint32_t b1 = data[offset + 1];
    uint32_t b2 = data[offset + 2];
    uint32_t b3 = data[offset + 3];
    
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

void Ram::store32(uint32_t offset, uint32_t val) {
    if (offset + 3 >= data.size()) {
        throw std::out_of_range("Offset out of range");
    }
    
    data[offset] = static_cast<uint8_t>(val);
    data[offset + 1] = static_cast<uint8_t>(val >> 8);
    data[offset + 2] = static_cast<uint8_t>(val >> 16);
    data[offset + 3] = static_cast<uint8_t>(val >> 24);
}
