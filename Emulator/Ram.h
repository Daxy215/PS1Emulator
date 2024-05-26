#pragma once

#include <vector>

class Ram {
public:
    Ram();
    
    // Fetch using little endian from a given 'offset'
    template<typename T>
    T load(uint32_t offset) {
        uint32_t value = 0;
        
        for(uint32_t i = 0; i < sizeof(T); ++i) {
            value |= static_cast<uint32_t>(data[offset + i]) << (i * 8);
        }
        
        return static_cast<T>(value);
    }
    
    // Store using little endian into a given 'offset'
    template<typename T>
    void store(uint32_t offset, T val) {
        uint32_t valAsUint32 = static_cast<uint32_t>(val);
        
        for(uint32_t i = 0; i < sizeof(T); ++i) {
            data[offset + i] = static_cast<uint8_t>((valAsUint32 >> (i * 8)) & 0xFF);
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

private:
    std::vector<uint8_t> data;
};