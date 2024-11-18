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
        
        /*if(offset + sizeof(T) >= data.size()) {
            printf("");
        }*/
        
        T v = 0;
        for (size_t i = 0; i < sizeof(T); i++) {
            v |= static_cast<T>(data[offset + i]) << (i * 8);
        }
        
        /*
        const uint8_t* ptr = data.data() + offset;
        T v;
        std::memcpy(&v, ptr, sizeof(T));
        */
        
        return v;
    }
    
    template<typename T>
    void store(uint32_t offset, uint32_t val) {
        // The two MSB are ignored, the 2MB RAM is mirrored four times
        // over the first 8MB of address space
        offset &= 0x1fffff;
        
        if(offset + sizeof(T) >= data.size()) {
            printf("");
        }
        
        for (size_t i = 0; i < sizeof(T); i++) {
            data[offset + i] = static_cast<uint8_t>((val >> (i * 8)));
        }
    }
    
public:
    std::vector<uint8_t> data;
    
};