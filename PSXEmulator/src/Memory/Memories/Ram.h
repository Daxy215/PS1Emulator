#pragma once

#include <vector>
#include <cstdint>
#include <cstring>

class Ram {
public:
    Ram();
    
    template<typename T>
    T load(uint32_t offset) const {
        // The two MSB are ignored, the 2MB RAM is mirrored four times
        // over the first 8MB of address space
        offset &= 0x1fffff;
        
        // Literally what above is but,
        // faster, I think.. I really hope so
        const uint8_t* ptr = data.data() + offset;
        T v;
        std::memcpy(&v, ptr, sizeof(T));
        
        return v;
    }
    
    template<typename T>
    void store(uint32_t offset, uint32_t val) {
        // The two MSB are ignored, the 2MB RAM is mirrored four times
        // over the first 8MB of address space
        offset &= 0x1fffff;
        
        uint8_t* ptr = data.data() + offset;
        std::memcpy(ptr, &val, sizeof(T));
    }
    
    void reset() {
        std::fill(data.begin(), data.end(), 0);
    }
    
public:
    std::vector<uint8_t> data = { 0 };
    
};