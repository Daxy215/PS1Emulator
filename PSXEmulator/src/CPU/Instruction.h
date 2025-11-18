#pragma once
#include <stdint.h>

/*struct uint32_t {
    uint32_t() : reg(0) {}
    uint32_t(uint32_t reg) : reg(reg) {}
    
    uint32_t operator|(const uint32_t& other) const {
        return {reg | other.reg};
    }
    
    operator uint32_t() const {
        return reg;
    }
    
    uint32_t reg;
    uint32_t lastWrite = 0;
};*/

// https://github.com/deadcore/playstation-emulator/blob/master/src/instruction/mod.rs#L40
class Instruction {
public:
    Instruction(uint32_t op) : op(op) {}
    
    // Returns bits [15:11] of the instruction
    uint32_t d() {
        return (op >> 11) & 0x1F;
    }
    
    // Returns register index in bits [20:16]
    uint32_t t() {
        return (op >> 16) & 0x1F;
    }
    
    // Returns register index in bits [25:21]
    uint32_t s() {
        return (op >> 21) & 0x1F;
    }
    
    // Returns immediate value in bits [16:0]
    uint32_t imm() {
        return op & 0xFFFF;
    }
    
    // Returns the immediate value in bts [16:0] as sign-extended 32 bits.
    int16_t imm_se() {
        //return static_cast<uint32_t>(static_cast<int16_t>(op & 0xFFFF));
        return (static_cast<int16_t>(op & 0xFFFF));
    }
    
    // Jumps to target stored in bits [25:0]
    uint32_t imm_jump() {
        return (op & 0x3FFFFFF);
    }
    
    // Returns bits [31:26] of the instruction
    uint32_t func() {
        return (op >> 26) & 0x3F;
    }
    
    // Returns the immediate values that are stored in bits [5:0]
    uint32_t subfunction() {
        return op & 0x3F;
    }
    
    // Same as the 's' function
    // Returns register index in bits [25:21]
    uint32_t copOpcode() {
        return (op >> 21) & 0x1F;
    }
    
    // Shift immediate values that are stored in bits [10:6]
    uint32_t shift() {
        return (op >> 6) & 0x1F;
    }
    
public:
    uint32_t op;
};
