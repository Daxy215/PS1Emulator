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
        return *reinterpret_cast<const T*>(data.data() + offset);
        
        /*// Literally what above is but,
        // faster, I think.. I really hope so
        const uint8_t* ptr = data.data() + offset;
        T v;
        std::memcpy(&v, ptr, sizeof(T));
        
        return v;*/
    }
    
    template<typename T>
    void store(uint32_t offset, T val) {
        // The two MSB are ignored, the 2MB RAM is mirrored four times
        // over the first 8MB of address space
        offset &= 0x1fffff;

        *reinterpret_cast<T*>(data.data() + offset) = val;
        
        //uint8_t* ptr = data.data() + offset;
        //std::memcpy(ptr, &val, sizeof(T));
    }
    
    void reset() {
        std::fill(data.begin(), data.end(), 0);
    }
    
public:
    std::vector<uint8_t> data;
    
};