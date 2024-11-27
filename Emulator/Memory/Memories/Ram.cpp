#include "Ram.h"

#include <stdexcept>

// Default data contents are garbage
Ram::Ram() : data((8 * 1024 * 1024), /*0xCA*/0) {
    
}

/*
template<typename T>
T Ram::load(uint32_t offset) {
    uint32_t value = 0;
    
    for(uint32_t i = 0; i < sizeof(T); ++i) {
        value |= static_cast<uint32_t>(data[offset + i]) << (i * 8);
    }
    
    return reinterpret_cast<T>(value);
}

template <typename T>
void Ram::store(uint32_t offset, T val) {
    uint32_t valAsUint32 = static_cast<uint32_t>(val);
    
    for(uint32_t i = 0; i < sizeof(T); ++i) {
        data[offset + i] = static_cast<uint8_t>((valAsUint32 >> (i * 8)) & 0xFF);
    }
}*/

/*
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

uint16_t Ram::load16(uint16_t offset) {
    size_t of = static_cast<size_t>(offset);
    
    uint16_t b0 = data[of + 0];
    uint16_t b1 = data[of + 1];
    
    return b0 | static_cast<uint16_t>(b1 << 8);
}

uint8_t Ram::load8(uint8_t offset) {
    return data[offset];
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

/**
 * TODO;
 * It might be more efficient to add the test in the branch and jump instructions capable of
 * setting an invalid PC but I don’t really care about performance at that point and that would
 * make the code more complicated
 #1#
void Ram::store16(uint32_t offset, uint16_t val) {
    uint8_t b0 = static_cast<uint8_t>(val);
    uint8_t b1 = static_cast<uint8_t>((val >> 8));
    
    data[offset + 0] = b0;
    data[offset + 1] = b1;
}

void Ram::store8(uint32_t offset, uint8_t val) {
    data[offset] = val;
}
*/
