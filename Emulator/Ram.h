#pragma once

#include <vector>

class Ram {
public:
    Ram();
    
    template<typename T>
    T load(uint32_t offset) const {
        // The two MSB are ignored, the 2MB RAM is mirrored four times
        // over the first 8MB of address space
        offset &= 0x1fffff;
        
        T v = 0;
        for (size_t i = 0; i < sizeof(T); i++) {
            v |= static_cast<T>(data[offset + i]) << (i * 8);
        }
        
        return v;
    }
    
    template<typename T>
    void store(uint32_t offset, uint32_t val) {
        // The two MSB are ignored, the 2MB RAM is mirrored four times
        // over the first 8MB of address space
        offset &= 0x1fffff;
        
        for (size_t i = 0; i < sizeof(T); i++) {
            data[offset + i] = static_cast<uint8_t>((val >> (i * 8)));
        }
    }
    
    // Load using little endian at offset
    /*uint32_t load32(uint32_t offset);
    uint16_t load16(uint16_t offset);
    uint8_t load8(uint8_t offset);
    
    // Store val at offset
    void store32(uint32_t offset, uint32_t val);
    void store16(uint32_t offset, uint16_t val);
    void store8(uint32_t offset, uint8_t val);*/
    
public:
    std::vector<uint8_t> data;
};