#pragma once
#include <cstdint>

/**
 * This class is used to allow the BIOS to communicate with the CPU! :D
 */

class Bios;

class Interconnect {
public:
    Interconnect(Bios* bios) : bios(bios) {  }
    
    uint32_t load32(uint32_t addr);
    void store32(uint32_t addr, uint32_t val);

private:
    Bios* bios;
};
