#pragma once
#include <vector>

class Ram {
public:
    Ram();
    
    uint32_t load32(uint32_t offset);
    void store32(uint32_t offset, uint32_t val);
    
private:
    std::vector<uint8_t> data;
};
