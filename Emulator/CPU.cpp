#include "CPU.h"

#include <bitset>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "Instruction.h"

/**
 * Byte - 8 bits
 * HalfWord - 16 bits or 2 bytes
 * Word - 32 bits or 4 bytes.
 */

void CPU::executeNextInstruction() {
    //regs[32] = { 0xdeadbeef};
    
    // R0 is "hardwired" to 0
    regs[0] = 0;
    
    /**
     * First amazing error, here I got "0x1300083c" which means..
     * I was reading the data in big-edian format amazingly..
     * To fix it, I just reversed how I was reading the BIOS.bin file.
     */
    
    Instruction* instruction = nextInstruction;
    
    nextInstruction = new Instruction(load32(pc));
    
    // increment the PC
    pc = wrappingAdd(pc, 4);
    
    // Executes the instruction
    decodeAndExecute(*(instruction));
}


std::string uint32ToHex(uint32_t value) {
    std::string binary = std::bitset<32>(value).to_string();
    
    std::string shiftedBin = binary.substr(26);
    
    std::string results = std::bitset<6>(std::stoi(shiftedBin, nullptr, 2) & 0x3F).to_string();
    
    std::stringstream ss;
    ss << "0x" << std::setfill('0') << std::setw(8) << std::hex << value << " Binary; 0b" << results;
    
    /*
    std::string hex = ss.str();
    uint32_t hexVal;
    ss >> std::hex >> hexVal;
    
    uint32_t binary = (hexVal >> 21) & 0x3f;
    
    ss.str("");
    ss.clear();
    
    ss << "Original Value: " << hex << ", Modified Value: 0b" << std::setw(8) << std::setfill('0') << binary;
    */

    return ss.str();
}

// page 92
//https://github.com/deadcore/playstation-emulator/blob/master/src/cpu/mod.rs
void CPU::decodeAndExecute(Instruction& instruction) {
    // Gotta decode the instructions using the;
    // Playstation R3000 processor
    // https://en.wikipedia.org/wiki/R3000
    
    std::cout << "Processing; " + uint32ToHex(instruction.func()) << '\n';
    
    switch (instruction.func()) {
    case 0b000000:
        decodeAndExecuteSubFunctions(instruction);
        
        break;
    case 0b001111:
        oplui(instruction);
        break;
    case 0b001101:
        opori(instruction);
        break;
    case 0b101011:
        opsw(instruction);
        break;
    case 0b000010:
        op_j(instruction);
        break;
    case 0b001001:
        addiu(instruction);
        break;
    case 0b010000:
        opmtc0(instruction);
        break;
    default:
        std::cout << "Unhandled instruction: " << uint32ToHex(instruction.func()) << '\n';
        throw std::runtime_error("Unhandled instruction: " + uint32ToHex(instruction.op));
    }
}

void CPU::decodeAndExecuteSubFunctions(Instruction& instruction) {
    switch (instruction.subfunction()) {
    case 0b000000:
        opsll(instruction);
        break;
    case 0b100001:
        //addu(instruction);
        break;
    case 0b100101:
        opor(instruction);
        break;
    default:
        throw std::runtime_error("Unhandled instruction: " + uint32ToHex(instruction.op));
        
        break;
    }
}

void CPU::opsll(Instruction instruction) {
    uint32_t i = instruction.shift();
    uint32_t t = instruction.t();
    uint32_t d = instruction.d();
    
    uint32_t v = reg(t).reg << i;
    
    set_reg(d, v);
}

// Load Upper Immediate(LUI)
void CPU::oplui(Instruction& instruction) {
    uint32_t i = instruction.imm();
    uint32_t t = instruction.t();
    
    uint32_t v = i << 16;
    
    set_reg(t, v);
    
    //throw std::runtime_error("what now?");
}

void CPU::opori(Instruction& instruction) {
    uint32_t i = instruction.imm();
    uint32_t t = instruction.t();
    uint32_t s = instruction.s();
    
    uint32_t v = reg(s).reg | i;
    
    set_reg(t, v);
}

void CPU::opor(Instruction& instruction) {
    uint32_t d = instruction.d();
    uint32_t s = instruction.s();
    uint32_t t = instruction.t();
    
    RegisterIndex v = reg(s) | reg(t);
    
    set_reg(d, v);
}

void CPU::opsw(Instruction& instruction) {
    uint32_t i = instruction.imm_se();
    uint32_t t = instruction.t();
    uint32_t s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s).reg, i);
    uint32_t v    = reg(t).reg;
    
    store32(addr, v);
}

// addiu $8, $zero, 0xb88
void CPU::addiu(Instruction& instruction) {
    uint32_t i = instruction.imm_se();
    uint32_t t = instruction.t();
    uint32_t s = instruction.s();
    
    uint32_t v = wrappingAdd(reg(s).reg, i);
    
    set_reg(t, v);
}

void CPU::opcop0(Instruction& instruction) {
    // Formation goes as:
    // 0b0100nn where nn is the coprocessor number.

    switch (instruction.copOpcode()) {
    case 0b00100: // COP0
        opmtc0(instruction);
        break;
    default:
        std::cout << "Unhandled instruction: " + uint32ToHex(instruction.copOpcode()) << "\n";
        throw std::runtime_error("Unhandled instruction: " + uint32ToHex(instruction.copOpcode()));
        
        break;        
    }
}

void CPU::opmtc0(Instruction& instruction) {
    // This basically makes it so that all the data,
    // goes to the cache instead of the main memory.
    uint32_t cpu_r = instruction.t();
    uint32_t cop_r = instruction.d();
}

void CPU::op_j(Instruction& instruction) {
    uint32_t i = instruction.imm_jump();
    
    pc = (pc & 0xf0000000) | (i << 2);
}

uint32_t CPU::load32(uint32_t addr) {
    return interconnect->load32(addr);
}

void CPU::store32(uint32_t addr, uint32_t val) {
    interconnect->store32(addr, val);
}

uint32_t CPU::wrappingAdd(uint32_t a, uint32_t b) {
    // Perform wrapping addition (wrap around upon overflow)
    return a + b < a ? UINT32_MAX : a + b;
}