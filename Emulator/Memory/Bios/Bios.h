#pragma once

#include <fstream>
#include <vector>

class Bios {
public:
    Bios(const std::string& path);
    
    template<typename T>
    T load(size_t offset) {
       /*uint32_t r = 0;
        
        for(uint32_t i = 0; i < sizeof(T); i++) {
            r |= (static_cast<uint32_t>(data[offset + i])) << (8 * i);
        }
        
        return r;*/
        
        uint32_t value = 0;
        
        for(uint32_t i = 0; i < sizeof(T); i++) {
            value |= static_cast<uint32_t>(data[offset + i]) << (i * 8);
        }
        
        return value;
        //return static_cast<T>(value);
    }
    
    uint32_t getLittleEndian(std::ifstream& file);
    
    /*uint32_t load32(size_t offset);
    uint8_t load8(size_t offset);*/
    
public:
    // The SCPH-1001 bios is exactly 512 kbs. 
    static const uint64_t BIOS_SIZE = 512 * 1024; // 512 kbs
    
public:
    std::vector<uint8_t> data;
};
