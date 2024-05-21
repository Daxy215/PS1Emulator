#include "interconnect.h"

#include <iomanip>
#include <bitset>
#include <iostream>
#include <sstream>
#include <string>

#include "Bios.h"
#include "Ram.h"
#include "Range.h"

// TODO; Remove
//TODO Remove those
std::string getHex1(uint32_t value) {
    std::stringstream ss;
    ss << "0x" << std::setfill('0') << std::setw(8) << std::hex << value;
    return ss.str();
}

std::string getBinary1(uint32_t value) {
    std::string binary = std::bitset<32>(value).to_string();
    
    std::string shiftedBin = binary.substr(26);
    
    std::string results = std::bitset<6>(std::stoi(shiftedBin, nullptr, 2) & 0x3F).to_string();
    
    std::stringstream ss;
    // wrong hex values but who cares about those
    ss << "Binary; 0b" << results;
    
    return ss.str();
}

std::string getDetail(uint32_t value) {
    std::string hex = getHex1(value);
    std::string binary = getBinary1(value);

    return hex + " = " + binary;
}

// Loads a 32bit word at 'addr'
uint32_t Interconnect::load32(uint32_t addr) {
    addr = map::maskRegion(addr);
    
    // To avoid "bus errors"
    if(addr % 4 != 0) {
        throw std::runtime_error("Unaligned fetch32 at address " + getDetail(addr));
    }
    
    if(auto offset = map::RAM.contains(addr)) {
        return ram->load32(offset.value());
    }
    
    if(map::DMA.contains(addr)) {
        throw std::runtime_error("Unhandled DMA read32 at address " + getDetail(addr));
    }
    
    if(auto offset = map::IRQ_CONTROL.contains(addr)) {
        printf("IRAQ control read %x\n", offset.value());
        return 0;
    }
    
    if(auto offset = map::BIOS.contains(addr)) {
        return bios->load32(offset.value());
    }
    
    throw std::runtime_error("Unhandled fetch32 at address " + getDetail(addr));
}

uint16_t Interconnect::load16(uint32_t addr) {
    throw std::runtime_error("Unhandled fetch16 at address " + getDetail(addr));
    
    return 0;
}

uint8_t Interconnect::load8(uint32_t addr) {
    addr = map::maskRegion(addr);
    
    // To avoid "bus errors"
    if(addr % 4 != 0) {
        throw std::runtime_error("Unaligned fetch8 at address " + getDetail(addr));
    }
    
    if(auto offset = map::RAM.contains(addr)) {
        return ram->load8(offset.value());
    }
    
    if(auto offset = map::BIOS.contains(addr)) {
        return bios->load8(offset.value());
    }
    
    if(map::EXPANSION2.contains(addr)) {
        // No expansion implemented
        return 0xFF;
    }
    
    throw std::runtime_error("Unhandled fetch8 at address " + getDetail(addr));
}

// Stores 32bit word 'val' into 'addr'
void Interconnect::store32(uint32_t addr, uint32_t val) {
    addr = map::maskRegion(addr);
    
    // To avoid "bus errors"
    if(addr % 4 != 0) {
        throw std::runtime_error("Unaligned fetch32 at address " + getDetail(addr));
    }

    if(auto offset = map::RAM.contains(addr)) {
        ram->store32(offset.value(), val);
    }
    
    if(auto offset = map::IRQ_CONTROL.contains(addr)) {
        printf("IRAQ control %x 0x%08x\n", offset.value(), val);
        return;
    }
    
    auto offset = map::MEMCONTROL.contains(addr);
    
    if(!offset) {
        std::cout << "Unhandled store32 at address " + getDetail(addr) << '\n';
        return;
    }
    
    switch (offset.value()) {
    case 0:
        // Expansion 1 base address
        if (val != 0x1f000000) {
            // Bad expansion 1 base address
            throw std::runtime_error("Bad expansion 1 base address: 0x" + getDetail(val));
        }
        
        break;
    case 4:
        // Expansion 2 base address
        if (val != 0x1f802000) {
            // Bad expansion 2 base address
            throw std::runtime_error("Bad expansion 2 base address: 0x" + getDetail(val));
        }
        
        break;
    default:
        std::cerr << "Unhandled store to MEMCONROL register " << getDetail(addr) << " - " << offset.value() << '\n';
        break;
    }
    
    //std::cout << "Unhandled store32 at address " + uint32xToHex(addr) << '\n';
    //throw std::runtime_error("Unhandled store32 into address " + std::to_string(addr));
}

void Interconnect::store16(uint32_t addr, uint16_t val) {
    // To avoid "bus errors"
    if(addr % 2 != 0) {
        throw std::runtime_error("Unaligned store16 at address " + getDetail(addr));
    }
    
    addr = map::maskRegion(addr);

    if(auto offset = map::RAM.contains(addr)) {
        return ram->store16(offset.value(), val);
    }
    
    if(auto offset = map::TIMERS.contains(addr)) {
        printf("Unhandled write to timer register %x 0x%08x\n", offset.value(), val);
        return;
    }
    
    if(auto offset = map::SPU.contains(addr)) {
        throw std::runtime_error("Unaligned store16 write to SPU register " + getDetail(addr));
    }
    
    throw std::runtime_error("Unhandled store16 at address " + getDetail(addr));
}

void Interconnect::store8(uint32_t addr, uint8_t val) {
    // To avoid "bus errors"
    if(addr % 2 != 0) {
        throw std::runtime_error("Unaligned store8 at address " + getDetail(addr));
    }
    
    addr = map::maskRegion(addr);
    
    if(auto offset = map::RAM.contains(addr)) {
        return ram->store8(addr, offset.value());
    }
    
    if(auto offset = map::EXPANSION2.contains(addr)) {
        throw std::runtime_error("Unaligned store8 write to SPU register " + getDetail(addr));
    }
    
    throw std::runtime_error("Unhandled store8 at address " + getDetail(addr));
}
