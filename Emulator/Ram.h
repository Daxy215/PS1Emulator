#pragma once

#include <vector>

class Ram {
public:
    Ram();
    
    template<typename T>
    uint32_t load(uint32_t offset) const {
        // The two MSB are ignored, the 2MB RAM is mirrored four times
        // over the first 8MB of address space
        offset &= 0x1fffff;
        
        uint32_t v = 0;
        for (size_t i = 0; i < sizeof(T); i++) {
            v |= static_cast<uint32_t>(data[offset + i]) << (i * 8);
        }
        
        return v;
    }
    
    bool test = false;
    
    template<typename T>
    void store(uint32_t offset, uint32_t val) {
        // The two MSB are ignored, the 2MB RAM is mirrored four times
        // over the first 8MB of address space
        offset &= 0x1fffff;
        
        if(offset == 964820) {
            test = true;
        }
        
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

private:
    std::vector<uint8_t> data;
};