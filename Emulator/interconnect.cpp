#include "interconnect.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "Bios.h"
#include "Range.h"

// TODO; Remove
std::string uint32xToHex(uint32_t value) {
    std::stringstream ss;
    ss << "0x" << std::setfill('0') << std::setw(8) << std::hex << value;
    return ss.str();
}

// Loads a 32bit word at 'addr'
uint32_t Interconnect::load32(uint32_t addr) {
    // To avoid "bus errors"
    //if(addr % 4 != 0) {
    //    throw std::runtime_error("Unaligned fetch32 at address " + uint32xToHex(addr));
    //}
    
    if(auto offset = map::RAM.contains(addr)) {
        throw std::runtime_error("Unhandled ram fetch32 at address " + uint32xToHex(addr));
    }
    
    if(auto offset = map::BIOS.contains(addr)) {
        return bios->load32(offset.value());
    }
    
    throw std::runtime_error("Unhandled fetch32 at address " + uint32xToHex(addr));
}

// Stores 32bit word 'val' into 'addr'
void Interconnect::store32(uint32_t addr, uint32_t val) {
    // To avoid "bus errors"
    if(addr % 4 != 0) {
        throw std::runtime_error("Unaligned fetch32 at address " + uint32xToHex(addr));
    }
    
    auto offset = map::MEMCONTROL.contains(addr);
    
    if(!offset) {
        std::cout << "Unhandled store32 at address " + uint32xToHex(addr) << '\n';
        return;
    }
    
    switch (offset.value()) {
    case 0:
        // Expansion 1 base address
        if (val != 0x1f000000) {
            // Bad expansion 1 base address
            throw std::runtime_error("Bad expansion 1 base address: 0x" + uint32xToHex(val));
        }
        
        break;
    case 4:
        // Expansion 2 base address
        if (val != 0x1f802000) {
            // Bad expansion 2 base address
            throw std::runtime_error("Bad expansion 2 base address: 0x" + uint32xToHex(val));
        }
        
        break;
    default:
        std::cout << "Unhandled write to MEMCONTROL register " << uint32xToHex(addr) << '\n';
        break;
    }
    
    //std::cout << "Unhandled store32 at address " + uint32xToHex(addr) << '\n';
    //throw std::runtime_error("Unhandled store32 into address " + std::to_string(addr));
}
