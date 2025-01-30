#pragma once

#include <cstring>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <cstdint>

class Bios {
public:
    Bios(const std::string& path);
    
    template<typename T>
    T load(size_t offset) {
        auto it = cache.find(offset);
        if (it != cache.end()) return it->second;
        
        const uint8_t* data = ptr + offset;
        T v;
        std::memcpy(&v, data, sizeof(T));
        
        cache[offset] = v;
        return v;
    }
    
public:
    // The SCPH-1001 bios is exactly 512 kbs. 
    static constexpr uint64_t BIOS_SIZE = 512 * 1024; // 512 kbs
    
private:
    uint8_t* ptr;
    
    std::vector<uint8_t> data;
    std::unordered_map<size_t, uint32_t> cache;
    
};
