#pragma once

#include <vector>
#include <cstdint>
#include <cstring>

class ScratchPad {
public:
    ScratchPad();
    
    template<typename T>
    T load(uint32_t offset) const {
        // The two MSB are ignored, the 2MB RAM is mirrored four times
        // over the first 8MB of address space
        //offset &= 0x1fffff;
        
        // TODO; Use memcpy

        T v = 0;
        for (size_t i = 0; i < sizeof(T); i++) {
            v |= static_cast<uint32_t>(data[offset + i]) << (i * 8);
        }
        
        return v;
    }
    
    template<typename T>
    void store(uint32_t offset, uint32_t val) {
        // The two MSB are ignored, the 2MB RAM is mirrored four times
        // over the first 8MB of address space
        //offset &= 0x1fffff;
        
        for (size_t i = 0; i < sizeof(T); i++) {
            data[offset + i] = static_cast<uint8_t>((val >> (i * 8)));
        }
    }
    
private:
    std::vector<uint8_t> data;
};