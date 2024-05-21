﻿#pragma once
#include <cstdint>

/**
 * This class is used to allow the BIOS to communicate with the CPU! :D
 */

class Bios;

class Interconnect {
public:
    Interconnect(Bios* bios) : bios(bios) {  }
    
    // TODO; Use templates and only use 1 function each
    
    // Load word
    uint32_t load32(uint32_t addr);

    // Load half word
    uint32_t load16(uint32_t addr);

    // Load byte
    uint32_t load8(uint32_t addr);

    // Store word
    void store32(uint32_t addr, uint32_t val);

    // Store half word
    void store16(uint32_t addr, uint32_t val);

    // Store byte
    void store8(uint32_t addr, uint32_t val);
    
private:
    Bios* bios;
};
