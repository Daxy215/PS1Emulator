#pragma once

#include <fstream>
#include <vector>

class Bios {
public:
    Bios(const std::string& path);
    
    uint32_t load32(size_t offset);
    uint8_t load8(size_t offset);

private:
    // The SCPH-1001 bios is exactly 512 kbs. 
    static const uint64_t BIOS_SIZE = 512 * 1024; // 512 kbs
    
    std::vector<uint8_t> data;
    Bios() = default;
};
